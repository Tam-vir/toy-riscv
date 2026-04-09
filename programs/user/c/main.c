#include "../../../include/stndio.h"

void main(void) {
    prts("Hello World!\n");
    prtnum(1234);
    prtc('\n');
    prtnum(stoi("1234"));
    prtc('\n');
    for(int i = 0; i < 5; i++) {
        prtnum(i);
        prtc(' ');
    }
    prtc('\n');
}
