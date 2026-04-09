#include "stndio.h"

// =====================
// Print Character
// =====================
void prtc(char c) {
    asm volatile(
        "mv a0, %0\n"
        "li a7, 11\n"
        "ecall\n"
        :
        : "r"(c)
        : "a0", "a7"
    );
}

// =====================
// Print String
// =====================
void prts(const char *str) {
    asm volatile(
        "mv a0, %0\n"
        "li a7, 4\n"
        "ecall\n"
        :
        : "r"(str)
        : "a0", "a7"
    );
}

// =====================
// Print Integer
// =====================
void prtnum(int i) {
    if (i < 0) {
        prtc('-');
        i = -i;
    }

    if (i >= 10) {
        prtnum(i / 10);
    }

    prtc((i % 10) + '0');
}

// =====================
// String → Integer
// =====================
int stoi(const char *str) {
    int result = 0;
    int i = 0;
    int sign = 1;

    if (str[0] == '-') {
        sign = -1;
        i++;
    }

    while (str[i]) {
        result = result * 10 + (str[i] - '0');
        i++;
    }

    return result * sign;
}

// =====================
// Read Integer
// =====================
void scanint(int *x) {
    asm volatile(
        "li a7, 5\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(*x)
        :
        : "a0", "a7"
    );
    return *x;
}

// =====================
// Read Character
// =====================
void scanchar(char *c) {
    asm volatile(
        "li a7, 12\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(*c)
        :
        : "a0", "a7"
    );
    return c;
}

// =====================
// Read String
// =====================
void scanstr(char *buf, int max_len) {
    asm volatile(
        "mv a0, %0\n"
        "mv a1, %1\n"
        "li a7, 8\n"
        "ecall\n"
        :
        : "r"(buf), "r"(max_len)
        : "a0", "a1", "a7"
    );
}