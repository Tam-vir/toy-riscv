/* Save as simple_test.c */
void main() {
    asm volatile (
        // Print character 'A'
        "li a0, 'A'\n"
        "li a7, 11\n"
        "ecall\n"

        // Print space (' ')
        "li a0, ' '\n"
        "li a7, 11\n"
        "ecall\n"

        // Print integer 123
        "li a0, 123\n"
        "li a7, 1\n"
        "ecall\n"

        // Print space
        "li a0, ' '\n"
        "li a7, 11\n"
        "ecall\n"

        // Print newline
        "li a0, '\\n'\n"
        "li a7, 11\n"
        "ecall\n"

        // exit
        "li a7, 10\n"
        "ecall\n"
    );
}