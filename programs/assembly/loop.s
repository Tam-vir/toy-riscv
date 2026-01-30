.section .text
.global _start

_start:
    la a0, header
    li a7, 4
    ecall
    
    # Loop from 1 to 5
    li t0, 1           # i = 1
    li t1, 5           # max = 5
    
loop:
    # Print number
    mv a0, t0
    li a7, 1
    ecall
    
    # Print space
    li a0, ' '
    li a7, 0
    ecall
    
    # Increment and loop
    addi t0, t0, 1
    ble t0, t1, loop
    
    # Newline and exit
    li a0, '\n'
    li a7, 0
    ecall
    
    li a7, 10
    ecall

.section .rodata
header: .string "Numbers 1 to 5: "