#include "stdo.h"


void prtc(char c) {
    asm volatile("mv a0, %0\nli a7,0\necall\n" : : "r"(c) : "a0","a7");
}
void prts(const char *str) {
    while (*str) {
        prtc(*str++);
    }
}

void prtnum(int i) {
    while (i >= 10) {
        prtnum(i / 10);
        i %= 10;
    }
    prtc(i + '0');
}

//convert string to integer

int stoi(char *str) {
    int i = 0;
    int k;
    while (str[k]) {
        i = i * 10 + (str[k] - '0');
        k++;
    }
    return i;
}
