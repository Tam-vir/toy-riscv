#include "stndio.h"
#include "stdarg.h"

// Print Character
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

// Print String
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

// Print Integer
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

// String → Integer
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

// Read Integer
void scanint(int *x) {
    asm volatile(
        "li a7, 5\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(*x)
        :
        : "a0", "a7"
    );
}

// Read Character
void scanchar(char *c) {
    asm volatile(
        "li a7, 12\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r"(*c)
        :
        : "a0", "a7"
    );
}

// Read String
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
static void prthex(unsigned int x) {
    char buf[16];
    int i = 0;

    if (x == 0) {
        prtc('0');
        return;
    }

    while (x) {
        int d = x % 16;
        buf[i++] = (d < 10) ? ('0' + d) : ('a' + d - 10);
        x /= 16;
    }

    while (i--) {
        prtc(buf[i]);
    }
}

int printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            prtc(*fmt++);
            continue;
        }

        fmt++; // skip %

        switch (*fmt) {
        case 'd': {
            int val = va_arg(args, int);
            prtnum(val);
            break;
        }

        case 'c': {
            char c = (char)va_arg(args, int);
            prtc(c);
            break;
        }

        case 's': {
            char *s = va_arg(args, char *);
            prts(s);
            break;
        }

        case 'x': {
            unsigned int val = va_arg(args, unsigned int);
            prthex(val);
            break;
        }

        case '%':
            prtc('%');
            break;

        default:
            prtc('?'); // unknown format
        }

        fmt++;
    }

    va_end(args);
    return 0;
}