.section .text
.global main

main:
    li t0, 100
    li t1, 23
    
    add t2, t0, t1      # 123
    sub t3, t0, t1      # 77
    
    li t4, 0
    li t5, 7
    li t6, 5
    
mul_loop:
    add t4, t4, t5
    addi t6, t6, -1
    bnez t6, mul_loop
    
    # Print
    la a0, add_msg
    li a7, 4
    ecall
    
    mv a0, t2
    li a7, 1
    ecall
    
    la a0, sub_msg
    li a7, 4
    ecall
    
    mv a0, t3
    li a7, 1
    ecall
    
    la a0, mul_msg
    li a7, 4
    ecall
    
    mv a0, t4
    li a7, 1
    ecall
    
    # newline
    li a0, '\n'
    li a7, 11
    ecall
    
    li a7, 10
    ecall

.section .rodata
add_msg:  .string "100 + 23 = "
sub_msg:  .string "\n100 - 23 = "
mul_msg:  .string "\n7 * 5 = "