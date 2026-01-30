# Simple RISC-V program that prints "Hello, World!"
.text
.global _start
_start:
    # Load string address
    lui a0, %hi(msg)
    addi a0, a0, %lo(msg)
    
    # Set syscall: print string
    li a7, 4
    ecall
    
    # Exit
    li a7, 10
    ecall

.data
msg:
    .string "Hello, World!\n"