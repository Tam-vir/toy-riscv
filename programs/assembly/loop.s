.section .text
.global _start

_start:
    la a0, header
    li a7, 4
    ecall
    
    li t0, 1
    li t1, 5
    
loop:
    mv a0, t0
    li a7, 1
    ecall
    
    li a0, ' '
    li a7, 11
    ecall
    
    addi t0, t0, 1
    ble t0, t1, loop
    
    li a0, '\n'
    li a7, 11
    ecall
    
    li a7, 10
    ecall

.section .data
header: .string "Numbers 1 to 5: "