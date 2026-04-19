.section .text
.global main

main:
    la a0, header
    li a7, 4
    ecall
    
    li t2, 0      # First Fibonacci number
    li t3, 1      # Second Fibonacci number  
    li t4, 10     # Counter
    
loop:
    mv a0, t2     # Print the current Fibonacci number
    li a7, 1
    ecall

    add t5, t2, t3 # Calculate the next Fibonacci number
    mv t2, t3      # Update the first number to the second
    mv t3, t5      # Update the second number to the next
    addi t4, t4, -1 # Decrement the counter

    #print space
    li a0, ' '
    li a7, 11
    ecall

    bnez t4, loop   # Loop until we have printed 10 numbers

    j exit

exit:
    #print newline
    li a0, '\n'
    li a7, 11
    ecall
    
    li a7, 10
    ecall
.section .rodata
header: .string "Fibonacci: "