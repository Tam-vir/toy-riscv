#include "../../include/stdo.h"

int main(){
    for(int w = 0; w < 20; w++){
        if (w % 2 == 0 && (w > 2)) {
            prts("Yes\n");
        } else {
            prts("No\n");
        }
    }

    return 0;
}