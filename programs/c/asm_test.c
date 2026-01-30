/* Save as simple_test.c */
void main() {
    // Test basic syscalls
    asm volatile (
        // Print character 'A'
        "li a0, 'A'\n"
        "li a7, 0\n"
        "ecall\n"
        
        // Print space
        "li a7, 7\n"
        "ecall\n"
        
        // Print integer 123
        "li a0, 123\n"
        "li a7, 1\n"
        "ecall\n"
        
        // Print space
        "li a7, 7\n"
        "ecall\n"
        
        // Print hex 0xABC
        "li a0, 0xABC\n"
        "li a7, 2\n"
        "ecall\n"
        
        // Print newline
        "li a7, 6\n"
        "ecall\n"
    );
}