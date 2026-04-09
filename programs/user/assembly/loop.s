.section .text
.global _start

_start:
    la a0, header
    li a7, 4
    ecall
    
    li a0, 1
    
loop:
    li a7, 1
    ecall
    mv t2, a0

    li a0, ' '
    li a7, 11
    ecall
    
    mv a0, t2
    addi a0, a0, 1

    li t0, 6
    blt a0, t0, loop
    
    j exit

exit:
    li a7, 10
    ecall

.section .data
header: .string "Numbers 1 to 5: "