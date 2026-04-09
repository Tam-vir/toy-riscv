.section .text
.global _start

_start:
    la a0, header
    li a7, 4
    ecall
    
    li t0, 0
    li t1, 1
    li t2, 10
    
fib_loop:
    mv a0, t0
    li a7, 1
    ecall
    
    li a0, ' '
    li a7, 11
    ecall
    
    add t3, t0, t1
    mv t0, t1
    mv t1, t3
    
    addi t2, t2, -1
    bnez t2, fib_loop
    
    li a0, '\n'
    li a7, 11
    ecall
    
    li a7, 10
    ecall

.section .rodata
header: .string "Fibonacci (first 10): "