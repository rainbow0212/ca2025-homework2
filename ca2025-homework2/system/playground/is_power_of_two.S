.text
.globl is_power_of_two
.globl clz_branchless

is_power_of_two:
    
    # save reg
    addi sp, sp, -20
    sw ra, 16(sp)   
    sw s0, 12(sp)   
    sw s1, 8(sp)    
    sw s2, 4(sp)    
    sw s3, 0(sp)    

    mv s0, a0       

    blez s0, return_false

    # 2. check: n == (1 << (31 - clz(n)))
    mv a0, s0           
    jal clz_branchless 
    mv s1, a0           # s1 = clz_result

    # 3. calculate 1 << (31 - clz_result)
    li t0, 31
    sub s2, t0, s1      # s2 = msb_pos = 31 - clz_result
    
    li t0, 1
    sll s3, t0, s2      # s3 = 1 << msb_pos

    # 4. compare n == (1 << msb)
    beq s0, s3, return_true

 
return_false:
    li a0, 0            
    j restore_and_ret

return_true:
    li a0, 1            


restore_and_ret:

    # restore reg
    lw s3, 0(sp)
    lw s2, 4(sp)
    lw s1, 8(sp)
    lw s0, 12(sp)
    lw ra, 16(sp)
    addi sp, sp, 20     
    ret                 



clz_branchless:
    # Step 1: Fill all bits to the right of MSB with 1s
    # x |= x >> 1; x |= x >> 2; x |= x >> 4; x |= x >> 8; x |= x >> 16;
    srli t0, a0, 1
    or a0, a0, t0
    srli t0, a0, 2
    or a0, a0, t0
    srli t0, a0, 4
    or a0, a0, t0
    srli t0, a0, 8
    or a0, a0, t0
    srli t0, a0, 16
    or a0, a0, t0
    
    # Step 2: Count the number of 1s (Hamming weight)
    # x = x - ((x >> 1) & 0x55555555);
    srli t0, a0, 1
    li t1, 0x55555555
    and t0, t0, t1
    sub a0, a0, t0
    
    # x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    li t1, 0x33333333
    and t0, a0, t1
    srli a0, a0, 2
    and a0, a0, t1
    add a0, a0, t0
    
    # x = (x + (x >> 4)) & 0x0F0F0F0F;
    srli t0, a0, 4
    add a0, a0, t0
    li t1, 0x0F0F0F0F
    and a0, a0, t1
    
    # x = x + (x >> 8);
    srli t0, a0, 8
    add a0, a0, t0
    
    # x = x + (x >> 16);
    srli t0, a0, 16
    add a0, a0, t0
    
    # x = x & 0x3F;
    andi a0, a0, 0x3F
    
    # Step 3: Return 32 - popcount
    li t0, 32
    sub a0, t0, a0
    
    ret