#include "../../../include/stndio.h"

int main(){
    int w;
    prts("Enter the weight: ");
    scanint(&w);

    if(w % 2 == 0 && w > 2){
        prts("YES\n");
    } else {
        prts("NO\n");
    }

    return 0;
}