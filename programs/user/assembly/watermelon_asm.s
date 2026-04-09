.section .text
.global _start

_start:
    # print "Enter weight: "
    la a0, inpmsg
    li a7, 4
    ecall

    # read weight
    li a7, 5
    ecall
    mv t0, a0

    # if (w % 2 != 0) → NO
    andi t2, t0, 1
    bne t2, zero, _no

    # if (w <= 2) → NO
    li t1, 2
    ble t0, t1, _no

    j _yes

_no:
    la a0, no
    li a7, 4
    ecall
    j _exit

_yes:
    la a0, yes
    li a7, 4
    ecall
    j _exit

_exit:
    li a7, 10
    ecall

.section .rodata
inpmsg: .string "Enter weight: "
yes:    .string "YES\n"
no:     .string "NO\n"