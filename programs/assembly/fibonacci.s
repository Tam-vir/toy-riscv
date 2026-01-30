.section .text
.global _start

_start:
    la a0, header
    li a7, 4
    ecall
    
    # Fibonacci: a=0, b=1
    li t0, 0            # a = 0
    li t1, 1            # b = 1
    li t2, 10            # count = 5
    
fib_loop:
    # Print current
    mv a0, t0
    li a7, 1
    ecall
    
    # Print space
    li a0, ' '
    li a7, 0
    ecall
    
    # Calculate next: c = a + b
    add t3, t0, t1      # c = a + b
    mv t0, t1           # a = b
    mv t1, t3           # b = c
    
    # Decrement counter
    addi t2, t2, -1
    bnez t2, fib_loop
    
    # Newline and exit
    li a0, '\n'
    li a7, 0
    ecall
    
    li a7, 10
    ecall

.section .rodata
header: .string "Fibonacci (first 5): "