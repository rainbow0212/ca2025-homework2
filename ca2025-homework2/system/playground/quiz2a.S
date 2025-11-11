.data
obdata:     .byte   0x3c, 0x3b, 0x3a
str1:       .asciz  "Move Disk "
str2:       .asciz  " from "
str3:       .asciz  " to "
newline:    .asciz  "\n"

.text
    .globl  hanoi_tower


# x10 = string pointer
print_str:
    mv      x5, x10         
    mv      x6, x10         
    
    # calculate string length
strlen_loop:
    lbu     x7, 0(x6)
    beq     x7, x0, strlen_done
    addi    x6, x6, 1
    j       strlen_loop
    
strlen_done:
    # calcaulate length: t1 - t0
    sub     x12, x6, x5     
    
    # write system call
    li      x17, 0x40       
    li      x10, 1          
    mv      x11, x5         
    ecall
    
    ret

# print char
# x10 = char's ASCII code
print_char:
    addi    x2, x2, -4
    sb      x10, 0(x2)      
    
    # write system call
    li      x17, 0x40       
    li      x10, 1          
    mv      x11, x2         
    li      x12, 1          
    ecall
    
    addi    x2, x2, 4
    ret


hanoi_tower:
    addi    x2, x2, -32
    sw      x8, 0(x2)
    sw      x9, 4(x2)
    sw      x18, 8(x2)
    sw      x19, 12(x2)
    sw      x20, 16(x2)
    li      x5, 0x15
    sw      x5, 20(x2)
    sw      x5, 24(x2)
    sw      x5, 28(x2)
    
    # Fix disk positions (BLANK 1-3: neutralize x5 effect)
    # BLANK 1: Fix position at x2+20
    sw      x0, 20(x2)
    # BLANK 2: Fix position at x2+24
    sw      x0, 24(x2)
    # BLANK 3: Fix position at x2+28
    sw      x0, 28(x2)
    
    addi    x8, x0, 1

game_loop:
    # BLANK 4: Check loop termination (2^3 moves)
    addi    x5, x0, 8
    beq     x8, x5, finish_game
    
    # Gray code formula: gray(n) = n XOR (n >> k)
    # BLANK 5: What is k for Gray code?
    srli    x5, x8, 1
    # BLANK 6: Complete Gray(n) calculation
    xor     x6, x8, x5
    
    # BLANK 7-8: Calculate previous value and its shift
    addi    x7, x8, -1
    srli    x28, x7, 1
    # BLANK 9: Generate Gray(n-1)
    xor     x7, x7, x28
    
    # BLANK 10: Which bits changed?
    xor     x5, x6, x7
    
    # Initialize disk number
    addi    x9, x0, 0
    # BLANK 11: Mask for testing LSB
    andi    x6, x5, 1
    # BLANK 12: Branch if disk 0 moves
    bne     x6, x0, disk_found
    
    # BLANK 13: Set disk 1
    addi    x9, x0, 1
    # BLANK 14: Test second bit with proper mask
    andi    x6, x5, 2
    bne     x6, x0, disk_found
    
    # BLANK 15: Last disk number
    addi    x9, x0, 2

disk_found:
    # BLANK 16: Check impossible pattern (multiple bits)
    andi    x30, x5, 5
    addi    x31, x0, 5
    beq     x30, x31, pattern_match
    jal     x0, continue_move

pattern_match:

continue_move:
    # BLANK 17: Word-align disk index (multiply by what?)
    slli    x5, x9, 2
    # BLANK 18: Base offset for disk array
    addi    x5, x5, 20
    add     x5, x2, x5
    lw      x18, 0(x5)
    bne     x9, x0, handle_large
    
    # BLANK 19: Small disk moves by how many positions?
    addi    x19, x18, 2
    # BLANK 20: Number of pegs
    addi    x6, x0, 3
    blt     x19, x6, display_move
    sub     x19, x19, x6
    jal     x0, display_move

handle_large:
    # BLANK 21: Load reference disk position
    lw      x6, 20(x2)
    # BLANK 22: Sum of all peg indices (0+1+2)
    addi    x19, x0, 3
    sub     x19, x19, x18
    sub     x19, x19, x6

display_move:
    la      x20, obdata
    add     x5, x20, x18
    # BLANK 23: Load byte from obfuscated data
    lbu     x11, 0(x5)
    # BLANK 24: Decode constant (0x6F)
    li      x6, 0x6F
    xor     x11, x11, x6
    # BLANK 25: Final offset adjustment
    addi    x11, x11, -0x12
    
    add     x7, x20, x19
    lbu     x12, 0(x7)
    xor     x12, x12, x6
    addi    x12, x12, -0x12
    
    # push
    addi    x2, x2, -32
    sw      x8, 0(x2)
    sw      x9, 4(x2)
    sw      x11, 8(x2)
    sw      x12, 12(x2)
    sw      x18, 16(x2)
    sw      x19, 20(x2)
    sw      x20, 24(x2)
    sw      x1, 28(x2)
    
    # print "Move Disk "
    la      x10, str1
    jal     x1, print_str
    
    # print disk number (x9+1)
    lw      x5, 4(x2)       
    addi    x10, x5, 1
    addi    x10, x10, 0x30  
    jal     x1, print_char
    
    # print " from "
    la      x10, str2
    jal     x1, print_str
    
    # print src peg (x11)
    lw      x10, 8(x2)      
    jal     x1, print_char
    
    # print " to "
    la      x10, str3
    jal     x1, print_str
    
    # print target peg (x12)
    lw      x10, 12(x2)     
    jal     x1, print_char
    
    # newline
    la      x10, newline
    jal     x1, print_str
    
    # pop
    lw      x8, 0(x2)
    lw      x9, 4(x2)
    lw      x11, 8(x2)
    lw      x12, 12(x2)
    lw      x18, 16(x2)
    lw      x19, 20(x2)
    lw      x20, 24(x2)
    lw      x1, 28(x2)
    addi    x2, x2, 32
    
    # BLANK 26: Calculate storage offset
    slli    x5, x9, 2
    addi    x5, x5, 20
    add     x5, x2, x5
    # BLANK 27: Update disk position
    sw      x19, 0(x5)
    
    # BLANK 28-29: Increment counter and loop
    addi    x8, x8, 1
    jal     x0, game_loop

finish_game:
    lw      x8, 0(x2)
    lw      x9, 4(x2)
    lw      x18, 8(x2)
    lw      x19, 12(x2)
    lw      x20, 16(x2)
    addi    x2, x2, 32
    ret