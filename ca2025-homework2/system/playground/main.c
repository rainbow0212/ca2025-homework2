#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#define printstr(ptr, length)                   \
    do {                                        \
        asm volatile(                           \
            "add a7, x0, 0x40;"                 \
            "add a0, x0, 0x1;" /* stdout */     \
            "add a1, x0, %0;"                   \
            "mv a2, %1;" /* length character */ \
            "ecall;"                            \
            :                                   \
            : "r"(ptr), "r"(length)             \
            : "a0", "a1", "a2", "a7");          \
    } while (0)

#define TEST_OUTPUT(msg, length) printstr(msg, length)

#define TEST_LOGGER(msg)                     \
    {                                        \
        char _msg[] = msg;                   \
        TEST_OUTPUT(_msg, sizeof(_msg) - 1); \
    }

extern uint64_t get_cycles(void);
extern uint64_t get_instret(void);

/* Bare metal memcpy implementation */
void *memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *) dest;
    const uint8_t *s = (const uint8_t *) src;
    while (n--)
        *d++ = *s++;
    return dest;
}

/* Software division for RV32I (no M extension) */
static unsigned long udiv(unsigned long dividend, unsigned long divisor)
{
    if (divisor == 0)
        return 0;

    unsigned long quotient = 0;
    unsigned long remainder = 0;

    for (int i = 31; i >= 0; i--) {
        remainder <<= 1;
        remainder |= (dividend >> i) & 1;

        if (remainder >= divisor) {
            remainder -= divisor;
            quotient |= (1UL << i);
        }
    }

    return quotient;
}

static unsigned long umod(unsigned long dividend, unsigned long divisor)
{
    if (divisor == 0)
        return 0;

    unsigned long remainder = 0;

    for (int i = 31; i >= 0; i--) {
        remainder <<= 1;
        remainder |= (dividend >> i) & 1;

        if (remainder >= divisor) {
            remainder -= divisor;
        }
    }

    return remainder;
}

/* Software multiplication for RV32I (no M extension) */
static uint32_t umul(uint32_t a, uint32_t b)
{
    uint32_t result = 0;
    while (b) {
        if (b & 1)
            result += a;
        a <<= 1;
        b >>= 1;
    }
    return result;
}

/* Provide __mulsi3 for GCC */
uint32_t __mulsi3(uint32_t a, uint32_t b)
{
    return umul(a, b);
}

/* Simple integer to hex string conversion */
static void print_hex(unsigned long val)
{
    char buf[20];
    char *p = buf + sizeof(buf) - 1;
    *p = '\n';
    p--;

    if (val == 0) {
        *p = '0';
        p--;
    } else {
        while (val > 0) {
            int digit = val & 0xf;
            *p = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
            p--;
            val >>= 4;
        }
    }

    p++;
    printstr(p, (buf + sizeof(buf) - p));
}

/* Simple integer to decimal string conversion */
static void print_dec(unsigned long val)
{
    char buf[20];
    char *p = buf + sizeof(buf) - 1;
    *p = '\n';
    p--;

    if (val == 0) {
        *p = '0';
        p--;
    } else {
        while (val > 0) {
            *p = '0' + umod(val, 10);
            p--;
            val = udiv(val, 10);
        }
    }

    p++;
    printstr(p, (buf + sizeof(buf) - p));
}


/*implement rsqrt with c code */
static int clz(uint32_t x) {
    if (!x) return 32;
    int n = 0;
    if (!(x & 0xFFFF0000)) { n += 16; x <<= 16; }
    if (!(x & 0xFF000000)) { n += 8; x <<= 8; }
    if (!(x & 0xF0000000)) { n += 4; x <<= 4; }
    if (!(x & 0xC0000000)) { n += 2; x <<= 2; }
    if (!(x & 0x80000000)) { n += 1; }
    return n;
}

static uint64_t mul32(uint32_t a, uint32_t b) {
    uint64_t r = 0;
    for (int i = 0; i < 32; i++) {
        if (b & (1U << i))
            r += (uint64_t)a << i;
    }
    return r;
}


static const uint16_t rsqrt_table[32] = {
    65536, 46341, 32768, 23170, 16384,
    11585, 8192, 5793, 4096, 2896,
    2048, 1448, 1024, 724, 512,
    362, 256, 181, 128, 90,
    64, 45, 32, 23, 16,
    11, 8, 6, 4, 3,
    2, 1
};

uint32_t fast_rsqrt(uint32_t x) {
    if (x == 0) return 0xFFFFFFFF;
    if (x == 1) return 65536;

    int exp = 31 - clz(x);

    uint32_t y = rsqrt_table[exp];

    if (x > (1u << exp)) {
        uint32_t y_next = (exp < 31) ? rsqrt_table[exp + 1] : 0;
        uint32_t delta = y - y_next;
        uint32_t frac = (uint32_t) ((((uint64_t)x - (1UL << exp)) << 16) >> exp);
        y -= (uint32_t) ((delta * frac) >> 16);
    }

    for (int iter = 0; iter < 2; iter++) {
        uint32_t y2 = (uint32_t)mul32(y, y);
        uint32_t xy2 = (uint32_t)(mul32(x, y2) >> 16);
        y = (uint32_t)(mul32(y, (3u << 16) - xy2) >> 17);
    }

    return y;
}

/* ============= Test Suite ============= */

static void test_rsqrt(void)
{
    TEST_LOGGER("\n=== Test: rsqrt ===\n");
    
    uint32_t x = 0xFFFFFFFF; /*input =  4294967295*/
    uint32_t result = fast_rsqrt(x); /*output = 1 */
    
    TEST_LOGGER("fast_rsqrt ( 0xFFFFFFFF ) = ");
    
    print_hex(result);


    if (result == 0x00000001){
        TEST_LOGGER("PASSED\n");
    }else {
        TEST_LOGGER("FAILED (expected 0x00000001)\n");
    }
    
    //TEST_LOGGER("\nrsqrt completed!\n");
}
extern void hanoi_tower_loop_unroll(void);
extern void hanoi_tower(void);
extern int is_power_of_two(int n);

typedef struct {
    int input;
    bool expected;
} power_of_two_test_case_t;


static void test_power_of_two(void)
{
    static const power_of_two_test_case_t power_of_two_tests[] = {
    {0x1,        true},   // 1
    {0x10,       true},   // 16
    {0x3,        false},  // 3
    {0xFFFFFFFF, false},  // -1
    {0x0,        false},  // 0
    {0x800,      true}    // 2048
    };

    bool all_passed = true;
    int num_tests = sizeof(power_of_two_tests) / sizeof(power_of_two_tests[0]);

    for (int i = 0; i < num_tests; i++) {
        
        power_of_two_test_case_t test = power_of_two_tests[i];
        bool actual_result = is_power_of_two(test.input);
        if (actual_result != test.expected) {
            all_passed = false;
            break;
        } 
       
    }
    
    TEST_LOGGER("is_power_of_two completed!\n");

}



int main(void)
{
    uint64_t start_cycles, end_cycles, cycles_elapsed;
    uint64_t start_instret, end_instret, instret_elapsed;

    

    /*hanoi of tower test*/
    

    TEST_LOGGER("\n=== Test: Towers of Hanoi (3 disks) === \n");
    start_cycles = get_cycles();
    start_instret = get_instret();

    hanoi_tower();

    end_cycles = get_cycles();
    end_instret = get_instret();
    cycles_elapsed = end_cycles - start_cycles;
    instret_elapsed = end_instret - start_instret;

    TEST_LOGGER("\nPerformance Statistics:\n");
    TEST_LOGGER("  Cycles: ");
    print_dec((unsigned long) cycles_elapsed);
    TEST_LOGGER("  Instructions: ");
    print_dec((unsigned long) instret_elapsed);



    /*tower of hanoi loop unrolling*/
    TEST_LOGGER("\n=== Test: Towers of Hanoi (3 disks)(loop unrolling) === \n");

    start_cycles = get_cycles();
    start_instret = get_instret();
    
    
    hanoi_tower_loop_unroll();
    //TEST_LOGGER("\nTowers of Hanoi completed!\n");
    end_cycles = get_cycles();
    end_instret = get_instret();
    cycles_elapsed = end_cycles - start_cycles;
    instret_elapsed = end_instret - start_instret;

    TEST_LOGGER("\nPerformance Statistics:\n");
    TEST_LOGGER("  Cycles: ");
    print_dec((unsigned long) cycles_elapsed);
    TEST_LOGGER("  Instructions: ");
    print_dec((unsigned long) instret_elapsed);


    /*rsqrt test*/
    
    start_cycles = get_cycles();
    start_instret = get_instret();

    test_rsqrt();

    end_cycles = get_cycles();
    end_instret = get_instret();
    cycles_elapsed = end_cycles - start_cycles;
    instret_elapsed = end_instret - start_instret;

    TEST_LOGGER("\nPerformance Statistics:\n");
    TEST_LOGGER("  Cycles: ");
    print_dec((unsigned long) cycles_elapsed);
    TEST_LOGGER("  Instructions: ");
    print_dec((unsigned long) instret_elapsed);


    /*is_power_of_two test*/
    TEST_LOGGER("\n== Test : is power of two \n");
    start_cycles = get_cycles();
    start_instret = get_instret();

    test_power_of_two();

    end_cycles = get_cycles();
    end_instret = get_instret();
    cycles_elapsed = end_cycles - start_cycles;
    instret_elapsed = end_instret - start_instret;

    TEST_LOGGER("\nPerformance Statistics:\n");
    TEST_LOGGER("  Cycles: ");
    print_dec((unsigned long) cycles_elapsed);
    TEST_LOGGER("  Instructions: ");
    print_dec((unsigned long) instret_elapsed);

    TEST_LOGGER("\n=== Test Completed ===\n");

    return 0;
}