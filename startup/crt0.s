.section .text
.global _start

_start:
    li sp, 0x4000000

    mv fp, zero
    
    call main

    li a7, 10
    ecall