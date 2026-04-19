.text
.global main

main:
    la a0, msg
    li a7, 4
    ecall
    
    li a7, 10
    ecall

.data
msg:
    .string "Hello, World!\n"