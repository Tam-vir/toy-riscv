########################################
# BOOT / RESET ENTRY
########################################

.section .text
.globl _start

_start:
    # set stack pointer
    la sp, 0x800000

    # set mtvec (vectored mode)
    la t0, trap_vector
    ori t0, t0, 1          # vectored mode bit
    csrw mtvec, t0

    # jump to user program
    la t0, 0x2000
    jr t0


########################################
# TRAP VECTOR TABLE (vectored mode)
# pc = mtvec_base + 4 * mcause
########################################

.align 4
trap_vector:

    j trap_handler          # 0  (instruction address misaligned)
    j trap_handler          # 1  (instruction access fault)
    j trap_handler          # 2  (illegal instruction)
    j trap_handler          # 3
    j trap_handler          # 4
    j trap_handler          # 5
    j trap_handler          # 6
    j trap_handler          # 7
    j trap_handler          # 8  (ECALL U-mode)
    j trap_handler          # 9  (ECALL S-mode)
    j trap_handler          # 10
    j trap_handler          # 11 (ECALL M-mode)


########################################
# COMMON TRAP HANDLER
########################################

trap_handler:
    # read mcause
    csrr t0, mcause

    # everything else
    j handle_exception


########################################
# SYSCALLS
########################################

# print int (a0)
sys_print_int:
    csrw 0x7C0, a0
    mret
# print char (a0 low byte)
sys_print_char:
    csrw 0x7C0, a0
    mret
sys_print_string:
    csrw 0x7C0, a0
    mret
sys_read_char:
    csrw 0x7C0, a0
    mret
sys_read_int:
    csrw 0x7C0, a0
    mret
sys_read_string:
    csrw 0x7C0, a0
    mret
# exit program
sys_exit:
    csrw 0x7C0, a0


########################################
# DEFAULT EXCEPTION HANDLER
########################################

handle_exception:
    # infinite loop for now
    j handle_exception