.section .text
.global _start

_start:
    # Test arithmetic operations
    li t0, 100
    li t1, 23
    
    # Addition
    add t2, t0, t1      # t2 = 123
    
    # Subtraction
    sub t3, t0, t1      # t3 = 77
    
    # Multiplication (using repeated addition for RV32I)
    li t4, 0           # result = 0
    li t5, 7           # multiplicand = 7
    li t6, 5           # counter = 5
    
mul_loop:
    add t4, t4, t5     # result += multiplicand
    addi t6, t6, -1    # counter--
    bnez t6, mul_loop  # loop if counter != 0
    
    # Print results
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
    
    # Newline and exit
    li a0, 10
    li a7, 0
    ecall
    
    li a7, 10
    ecall

.section .rodata
add_msg:  .string "100 + 23 = "
sub_msg:  .string "\n100 - 23 = "
mul_msg:  .string "\n7 * 5 = "
