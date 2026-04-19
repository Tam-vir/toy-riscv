.section .text
.global main

main:
    # la sp, 0x4000000
    #print stack_top
    addi t0, t0, 10
    addi sp, sp, -4
    sw t0, 0(sp)
    lw t1, 0(sp)
    addi sp, sp, 4
    beq t1, t0, stack_passed
    j stack_failed
    
stack_passed:
    la a0, pmsg
    li a7, 4
    ecall
    j exit
    
stack_failed:
    la a0, fmsg
    li a7, 4
    ecall
    j exit

exit:
    li a7, 10
    ecall
    
.section .rodata
pmsg:
    .string "Stack passed\n"
fmsg:
    .string "Stack failed\n"
