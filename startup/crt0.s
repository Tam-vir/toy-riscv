########################################
# USER PROGRAM STARTUP CODE (crt0.s)
# Runs at: 0x2000
########################################

.section .text
.globl _start

_start:
    # set stack pointer
    la sp, 0x4000000

    # set mtvec (vectored mode)
    la t0, trap_vector
    ori t0, t0, 1          # vectored mode bit
    csrw mtvec, t0

    mv fp, zero
    
    # Call main
    call main

    # If main returns, exit with syscall
    li a7, 10
    ecall

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
    j trap_handler          # 12 (instruction page fault)
    j trap_handler          # 13 (load page fault)
    j trap_handler          # 14 (reserved)
    j trap_handler          # 15 (store page fault)
    
    # External interrupts start at cause 16
    j gpio_pin0_handler     # 16 GPIO Pin 0
    j gpio_pin1_handler     # 17 GPIO Pin 1
    j gpio_pin2_handler     # 18 GPIO Pin 2
    j gpio_pin3_handler     # 19 GPIO Pin 3
    j gpio_pin4_handler     # 20 GPIO Pin 4
    j gpio_pin5_handler     # 21 GPIO Pin 5
    j gpio_pin6_handler     # 22 GPIO Pin 6
    j gpio_pin7_handler     # 23 GPIO Pin 7
    j gpio_pin8_handler     # 24 GPIO Pin 8
    j gpio_pin9_handler     # 25 GPIO Pin 9
    j gpio_pin10_handler    # 26 GPIO Pin 10
    j gpio_pin11_handler    # 27 GPIO Pin 11
    j gpio_pin12_handler    # 28 GPIO Pin 12
    j gpio_pin13_handler    # 29 GPIO Pin 13
    j gpio_pin14_handler    # 30 GPIO Pin 14
    j gpio_pin15_handler    # 31 GPIO Pin 15
    j gpio_pin16_handler    # 32 GPIO Pin 16
    j gpio_pin17_handler    # 33 GPIO Pin 17
    j gpio_pin18_handler    # 34 GPIO Pin 18
    j gpio_pin19_handler    # 35 GPIO Pin 19
    j gpio_pin20_handler    # 36 GPIO Pin 20
    j gpio_pin21_handler    # 37 GPIO Pin 21
    j gpio_pin22_handler    # 38 GPIO Pin 22
    j gpio_pin23_handler    # 39 GPIO Pin 23
    j gpio_pin24_handler    # 40 GPIO Pin 24
    j gpio_pin25_handler    # 41 GPIO Pin 25
    j gpio_pin26_handler    # 42 GPIO Pin 26
    j gpio_pin27_handler    # 43 GPIO Pin 27
    j gpio_pin28_handler    # 44 GPIO Pin 28
    j gpio_pin29_handler    # 45 GPIO Pin 29
    j gpio_pin30_handler    # 46 GPIO Pin 30
    j gpio_pin31_handler    # 47 GPIO Pin 31
    j gpio_port0_handler    # 48 GPIO Port 0
    j gpio_port1_handler    # 49 GPIO Port 1
    j gpio_port2_handler    # 50 GPIO Port 2
    j gpio_port3_handler    # 51 GPIO Port 3
    j uart_tx_handler       # 52 UART TX
    j uart_rx_handler       # 53 UART RX


########################################
# COMMON TRAP HANDLER
########################################

trap_handler:
    # read mcause
    csrr t0, mcause

    # everything else
    j handle_exception


########################################
# GPIO PIN INTERRUPT HANDLERS
########################################

# Macro for generating individual pin handlers
.macro gpio_pin_handler pin_num
    # Save context
    addi sp, sp, -32
    sw ra, 28(sp)
    sw t0, 24(sp)
    sw t1, 20(sp)
    sw t2, 16(sp)
    sw t3, 12(sp)
    sw t4, 8(sp)
    sw t5, 4(sp)
    sw t6, 0(sp)
    
    # Read interrupt status from bus
    li t0, 0x1100          # BUS_INTERRUPT_STATUS
    lw t1, 0(t0)
    
    # Check if this pin caused the interrupt
    li t2, 1<<\pin_num
    and t3, t1, t2
    beqz t3, gpio_pin\pin_num\()_done
    
    # Clear the interrupt
    li t0, 0x110C          # BUS_INTERRUPT_CLEAR
    sw t2, 0(t0)
    
    # Call user-defined pin interrupt handler
    la t0, gpio_pin\pin_num\()_user_handler
    beqz t0, gpio_pin\pin_num\()_done
    jalr ra, t0
    
gpio_pin\pin_num\()_done:
    # Restore context
    lw ra, 28(sp)
    lw t0, 24(sp)
    lw t1, 20(sp)
    lw t2, 16(sp)
    lw t3, 12(sp)
    lw t4, 8(sp)
    lw t5, 4(sp)
    lw t6, 0(sp)
    addi sp, sp, 32
    
    # Return from interrupt
    mret
.endm

# Generate all 32 individual pin handlers
gpio_pin0_handler:  gpio_pin_handler 0
gpio_pin1_handler:  gpio_pin_handler 1
gpio_pin2_handler:  gpio_pin_handler 2
gpio_pin3_handler:  gpio_pin_handler 3
gpio_pin4_handler:  gpio_pin_handler 4
gpio_pin5_handler:  gpio_pin_handler 5
gpio_pin6_handler:  gpio_pin_handler 6
gpio_pin7_handler:  gpio_pin_handler 7
gpio_pin8_handler:  gpio_pin_handler 8
gpio_pin9_handler:  gpio_pin_handler 9
gpio_pin10_handler: gpio_pin_handler 10
gpio_pin11_handler: gpio_pin_handler 11
gpio_pin12_handler: gpio_pin_handler 12
gpio_pin13_handler: gpio_pin_handler 13
gpio_pin14_handler: gpio_pin_handler 14
gpio_pin15_handler: gpio_pin_handler 15
gpio_pin16_handler: gpio_pin_handler 16
gpio_pin17_handler: gpio_pin_handler 17
gpio_pin18_handler: gpio_pin_handler 18
gpio_pin19_handler: gpio_pin_handler 19
gpio_pin20_handler: gpio_pin_handler 20
gpio_pin21_handler: gpio_pin_handler 21
gpio_pin22_handler: gpio_pin_handler 22
gpio_pin23_handler: gpio_pin_handler 23
gpio_pin24_handler: gpio_pin_handler 24
gpio_pin25_handler: gpio_pin_handler 25
gpio_pin26_handler: gpio_pin_handler 26
gpio_pin27_handler: gpio_pin_handler 27
gpio_pin28_handler: gpio_pin_handler 28
gpio_pin29_handler: gpio_pin_handler 29
gpio_pin30_handler: gpio_pin_handler 30
gpio_pin31_handler: gpio_pin_handler 31

########################################
# GPIO PORT INTERRUPT HANDLERS
########################################

# Macro for generating port handlers
.macro gpio_port_handler port_num
    # Save context
    addi sp, sp, -32
    sw ra, 28(sp)
    sw t0, 24(sp)
    sw t1, 20(sp)
    sw t2, 16(sp)
    sw t3, 12(sp)
    sw t4, 8(sp)
    sw t5, 4(sp)
    sw t6, 0(sp)
    
    # Read interrupt status from bus
    li t0, 0x1100          # BUS_INTERRUPT_STATUS
    lw t1, 0(t0)
    
    # Check which pins in this port caused interrupt
    li t2, 0xFF            # 8 pins per port
    li t3, \port_num
    slli t3, t3, 3         # port_num * 8
    sll t2, t2, t3         # shift to port position
    and t4, t1, t2
    beqz t4, gpio_port\port_num\()_done
    
    # Clear port interrupts
    li t0, 0x110C          # BUS_INTERRUPT_CLEAR
    sw t2, 0(t0)
    
    # Call user-defined port interrupt handler
    la t0, gpio_port\port_num\()_user_handler
    beqz t0, gpio_port\port_num\()_done
    jalr ra, t0
    
gpio_port\port_num\()_done:
    # Restore context
    lw ra, 28(sp)
    lw t0, 24(sp)
    lw t1, 20(sp)
    lw t2, 16(sp)
    lw t3, 12(sp)
    lw t4, 8(sp)
    lw t5, 4(sp)
    lw t6, 0(sp)
    addi sp, sp, 32
    
    # Return from interrupt
    mret
.endm

# Generate all 4 port handlers
gpio_port0_handler: gpio_port_handler 0
gpio_port1_handler: gpio_port_handler 1
gpio_port2_handler: gpio_port_handler 2
gpio_port3_handler: gpio_port_handler 3

########################################
# UART INTERRUPT HANDLERS
########################################

uart_tx_handler:
    # Save context
    addi sp, sp, -32
    sw ra, 28(sp)
    sw t0, 24(sp)
    sw t1, 20(sp)
    sw t2, 16(sp)
    sw t3, 12(sp)
    sw t4, 8(sp)
    sw t5, 4(sp)
    sw t6, 0(sp)
    
    # Read interrupt status from bus
    li t0, 0x1100          # BUS_INTERRUPT_STATUS
    lw t1, 0(t0)
    
    # Check if UART TX caused the interrupt
    li t2, 1<<16           # UART TX interrupt bit
    and t3, t1, t2
    beqz t3, uart_tx_done
    
    # Clear the interrupt
    li t0, 0x110C          # BUS_INTERRUPT_CLEAR
    sw t2, 0(t0)
    
    # Call user-defined UART TX handler
    la t0, uart_tx_user_handler
    beqz t0, uart_tx_done
    jalr ra, t0
    
uart_tx_done:
    # Restore context
    lw ra, 28(sp)
    lw t0, 24(sp)
    lw t1, 20(sp)
    lw t2, 16(sp)
    lw t3, 12(sp)
    lw t4, 8(sp)
    lw t5, 4(sp)
    lw t6, 0(sp)
    addi sp, sp, 32
    
    # Return from interrupt
    mret

uart_rx_handler:
    # Save context
    addi sp, sp, -32
    sw ra, 28(sp)
    sw t0, 24(sp)
    sw t1, 20(sp)
    sw t2, 16(sp)
    sw t3, 12(sp)
    sw t4, 8(sp)
    sw t5, 4(sp)
    sw t6, 0(sp)
    
    # Read interrupt status from bus
    li t0, 0x1100          # BUS_INTERRUPT_STATUS
    lw t1, 0(t0)
    
    # Check if UART RX caused the interrupt
    li t2, 1<<17           # UART RX interrupt bit
    and t3, t1, t2
    beqz t3, uart_rx_done
    
    # Clear the interrupt
    li t0, 0x110C          # BUS_INTERRUPT_CLEAR
    sw t2, 0(t0)
    
    # Call user-defined UART RX handler
    la t0, uart_rx_user_handler
    beqz t0, uart_rx_done
    jalr ra, t0
    
uart_rx_done:
    # Restore context
    lw ra, 28(sp)
    lw t0, 24(sp)
    lw t1, 20(sp)
    lw t2, 16(sp)
    lw t3, 12(sp)
    lw t4, 8(sp)
    lw t5, 4(sp)
    lw t6, 0(sp)
    addi sp, sp, 32
    
    # Return from interrupt
    mret

########################################
# INTERRUPT MANAGEMENT FUNCTIONS
########################################

# Make these functions globally accessible for C programs
.global enable_interrupt
.global disable_interrupt
.global enable_interrupts
.global disable_interrupts
.global configure_gpio_interrupt
.global disable_gpio_interrupt

# Enable specific interrupt
# a0 = interrupt number (0-31)
enable_interrupt:
    li t0, 0x1104          # BUS_INTERRUPT_ENABLE
    lw t1, 0(t0)
    li t2, 1
    sll t2, t2, a0         # 1 << interrupt_number
    or t1, t1, t2
    sw t1, 0(t0)
    ret


# Disable specific interrupt
# a0 = interrupt number (0-31)
disable_interrupt:
    li t0, 0x1104          # BUS_INTERRUPT_ENABLE
    lw t1, 0(t0)
    li t2, 1
    sll t2, t2, a0         # 1 << interrupt_number
    not t2, t2
    and t1, t1, t2
    sw t1, 0(t0)
    ret

# Enable global interrupts
enable_interrupts:
    li t0, 0x300           # MSTATUS register
    li t1, 0x800           # Enable external interrupts (set MIE bit)
    csrw mstatus, t1
    ret

# Disable global interrupts
disable_interrupts:
    li t0, 0x300           # MSTATUS register
    li t1, 0x0             # Clear MIE bit
    csrw mstatus, t1
    ret

########################################
# USER INTERRUPT HANDLER DECLARATIONS
########################################

# These are weak symbols that can be overridden by user programs
.weak gpio_pin0_user_handler
.weak gpio_pin1_user_handler
.weak gpio_pin2_user_handler
.weak gpio_pin3_user_handler
.weak gpio_pin4_user_handler
.weak gpio_pin5_user_handler
.weak gpio_pin6_user_handler
.weak gpio_pin7_user_handler
.weak gpio_pin8_user_handler
.weak gpio_pin9_user_handler
.weak gpio_pin10_user_handler
.weak gpio_pin11_user_handler
.weak gpio_pin12_user_handler
.weak gpio_pin13_user_handler
.weak gpio_pin14_user_handler
.weak gpio_pin15_user_handler
.weak gpio_pin16_user_handler
.weak gpio_pin17_user_handler
.weak gpio_pin18_user_handler
.weak gpio_pin19_user_handler
.weak gpio_pin20_user_handler
.weak gpio_pin21_user_handler
.weak gpio_pin22_user_handler
.weak gpio_pin23_user_handler
.weak gpio_pin24_user_handler
.weak gpio_pin25_user_handler
.weak gpio_pin26_user_handler
.weak gpio_pin27_user_handler
.weak gpio_pin28_user_handler
.weak gpio_pin29_user_handler
.weak gpio_pin30_user_handler
.weak gpio_pin31_user_handler
.weak gpio_port0_user_handler
.weak gpio_port1_user_handler
.weak gpio_port2_user_handler
.weak gpio_port3_user_handler
.weak uart_tx_user_handler
.weak uart_rx_user_handler

# Default weak implementations (do nothing)
gpio_pin0_user_handler:  ret
gpio_pin1_user_handler:  ret
gpio_pin2_user_handler:  ret
gpio_pin3_user_handler:  ret
gpio_pin4_user_handler:  ret
gpio_pin5_user_handler:  ret
gpio_pin6_user_handler:  ret
gpio_pin7_user_handler:  ret
gpio_pin8_user_handler:  ret
gpio_pin9_user_handler:  ret
gpio_pin10_user_handler: ret
gpio_pin11_user_handler: ret
gpio_pin12_user_handler: ret
gpio_pin13_user_handler: ret
gpio_pin14_user_handler: ret
gpio_pin15_user_handler: ret
gpio_pin16_user_handler: ret
gpio_pin17_user_handler: ret
gpio_pin18_user_handler: ret
gpio_pin19_user_handler: ret
gpio_pin20_user_handler: ret
gpio_pin21_user_handler: ret
gpio_pin22_user_handler: ret
gpio_pin23_user_handler: ret
gpio_pin24_user_handler: ret
gpio_pin25_user_handler: ret
gpio_pin26_user_handler: ret
gpio_pin27_user_handler: ret
gpio_pin28_user_handler: ret
gpio_pin29_user_handler: ret
gpio_pin30_user_handler: ret
gpio_pin31_user_handler: ret
gpio_port0_user_handler: ret
gpio_port1_user_handler: ret
gpio_port2_user_handler: ret
gpio_port3_user_handler: ret
uart_tx_user_handler:    ret
uart_rx_user_handler:    ret

########################################
# GPIO INTERRUPT CONFIGURATION FUNCTIONS
########################################

# Configure GPIO pin interrupt
# a0 = port (0-3)
# a1 = pin (0-7)  
# a2 = mode (0=disabled, 1=low, 2=high, 3=rising, 4=falling, 5=change)
# Returns: nothing
configure_gpio_interrupt:
    # Calculate pin index: port * 8 + pin
    li t0, 8
    mul t1, a0, t0
    add t1, t1, a1         # t1 = pin index (0-31)
    
    # Configure the GPIO interrupt mode
    # TODO: This would require writing to GPIO interrupt configuration registers
    # For now, just return without doing anything
    # The actual interrupt enabling should be done separately with enable_interrupt()
    ret

# Disable GPIO pin interrupt
# a0 = port (0-3)
# a1 = pin (0-7)
disable_gpio_interrupt:
    # Calculate pin index: port * 8 + pin
    li t0, 8
    mul t1, a0, t0
    add t1, t1, a1         # t1 = pin index (0-31)
    
    # Disable the interrupt in bus
    mv a0, t1
    jal disable_interrupt
    ret

########################################
# DEFAULT EXCEPTION HANDLER
########################################

handle_exception:
    # infinite loop for now
    j handle_exception

########################################
# EXAMPLE USAGE (can be called from user program)
########################################

# Example: Enable GPIO interrupts for pins 0, 1, 8, 9
# and configure them for rising edge detection
example_gpio_setup:
    # Enable global interrupts
    jal enable_interrupts
    
    # Configure pin 0 (port 0, pin 0) for rising edge
    li a0, 0               # port 0
    li a1, 0               # pin 0
    li a2, 3               # rising edge mode
    jal configure_gpio_interrupt
    
    # Configure pin 1 (port 0, pin 1) for falling edge
    li a0, 0               # port 0
    li a1, 1               # pin 1
    li a2, 4               # falling edge mode
    jal configure_gpio_interrupt
    
    # Configure pin 8 (port 1, pin 0) for any change
    li a0, 1               # port 1
    li a1, 0               # pin 0
    li a2, 5               # change mode
    jal configure_gpio_interrupt
    
    # Configure pin 9 (port 1, pin 1) for high level
    li a0, 1               # port 1
    li a1, 1               # pin 1
    li a2, 2               # high level mode
    jal configure_gpio_interrupt
    
    ret

# Example: Enable UART interrupts
example_uart_setup:
    # Enable UART TX interrupt (vector 16)
    li a0, 16
    jal enable_interrupt
    
    # Enable UART RX interrupt (vector 17)
    li a0, 17
    jal enable_interrupt
    
    ret