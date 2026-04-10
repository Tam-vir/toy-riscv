########################################
# BOOTLOADER - Minimal code to jump to user program
# Runs at: 0x0000
########################################

.section .text
.globl _start

_start:
    # Set up minimal stack pointer
    la sp, 0x4000000
    
    # Jump to user program at 0x2000
    la t0, 0x2000
    jr t0

# End of bootloader