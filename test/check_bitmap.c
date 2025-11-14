#include "bitmap.h"
#include <stdio.h>

int main(void) {
    int32_t size = 100;
    int8_t ret, n;
    bitmap *bm = bm_create(size);
    
    printf("After create map:\n");
    bm_show(bm);

    for (n=0; n<100; n++)
        bm_setbit(bm, n);
    printf("After setbits:\n");
    bm_show(bm);

    printf("After unset index 10:\n");
    bm_unsetbit(bm, 10);
    bm_show(bm);

    bm_clearmap(bm);
    printf("After clearmap:\n");
    bm_show(bm);

    return 0;
}
