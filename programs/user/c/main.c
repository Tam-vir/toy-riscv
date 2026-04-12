#include "../../../include/stndio.h"

int main()
{
    prts("Hello from RISC-V C program!\n");
    prtnum(12345);
    prts("\n");
    prts("Let's run a loop! :p\n");

    for(int i = 0; i < 5; i++) {
        prtnum(i);
        prts(" ");
    }
    prts("\n");
}