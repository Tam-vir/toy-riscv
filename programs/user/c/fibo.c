#include "../../../include/stndio.h"

int fibo(int n)
{
    if (n <= 1)
        return n;
    return fibo(n - 1) + fibo(n - 2);
}

void main(void)
{
    int n = 10;
    int curr = 0;

    for (int i = 0; i < n; i++)
    {
        curr = fibo(i);
        prtnum(curr);
        prtc(' ');
    }
    prtc('\n');
}
