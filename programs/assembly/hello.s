.section .text
.global _start

_start:

    la a0, msg
    li a7, 4
    ecall
    
    # Newline and exit
    li a0, 10
    li a7, 0
    ecall
    
    li a7, 10
    ecall

.section .rodata
msg:  .string "Hello World!"

