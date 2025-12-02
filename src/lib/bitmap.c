/*
 * bitmap.c
 * Bitmap implementation
 * Copyright (C) Jie
 * 2025-11-14
 *
 */

#include "bitmap.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bitmap *bm_create(uint32_t size) {
    bitmap *bm;

    if (size <= 0) {
        fprintf(stderr, "You should provide a size bigger than 0\n");
        return (bitmap *)0;
    }

    bm = (bitmap *)malloc(sizeof(struct s_bitmap));
    if (!bm) {
        fprintf(stderr, "No enough to allocate for a bitmap\n");
        return (bitmap *)0;
    }

    bm->byte_len = size;
    bm->len = size*8;

    bm->bytes = (uint8_t *)malloc(size);
    memset(bm->bytes, 0, size);

    return bm;
}

uint8_t bm_getbit(bitmap *bm, uint32_t idx) {
    uint8_t ret;
    uint32_t byte_pos, bit_pos;

    if (!bm) {
        fprintf(stderr, "You should provide a non null bitmap pointer\n");
        return -1;
    }

    if (idx < 0 || idx >= bm->len) {
        fprintf(stderr, "Wrong index [%d], it should range from 0 to %d\n",
                (int)idx, (int)(bm->len-1));
        return -1;
    }

    //             byte 1   byte 0
    //             76543210 76543210
    // two bytes = 00000000 00000000
    //                  ^          
    // idx = 10
    // byte_pos = 1
    // bit_pos  = 2
    byte_pos = idx / 8;
    bit_pos  = idx % 8;

    ret = (bm->bytes[byte_pos] >> bit_pos) & 1;
    
    return ret;
}

uint8_t bm_setbit(bitmap *bm, uint32_t idx) {
    uint32_t byte_pos, bit_pos;

    if (!bm) {
        fprintf(stderr, "You should provide a non null bitmap pointer\n");
        return -1;
    }

    if (idx < 0 || idx >= bm->len) {
        fprintf(stderr, "Wrong index [%d], it should range from 0 to %d\n",
                (int)idx, (int)(bm->len-1));
        return -1;
    }

    //             byte 1   byte 0
    //             76543210 76543210
    // two bytes = 00000000 00000000
    //                  ^          
    // mask      = 00000100 00000000
    // result    = 00000100 00000000
    // idx = 10
    // byte_pos = 1
    // bit_pos  = 2
    byte_pos = idx / 8;
    bit_pos  = idx % 8;

    bm->bytes[byte_pos] = bm->bytes[byte_pos] | (1 << bit_pos);
    
    return 0;
}

uint8_t bm_unsetbit(bitmap *bm, uint32_t idx) {
    uint32_t byte_pos, bit_pos;

    if (!bm) {
        fprintf(stderr, "You should provide a non null bitmap pointer\n");
        return -1;
    }

    if (idx < 0 || idx >= bm->len) {
        fprintf(stderr, "Wrong index [%d], it should range from 0 to %d\n",
                (int)idx, (int)(bm->len-1));
        return -1;
    }

    //             byte 1   byte 0
    //             76543210 76543210
    // two bytes = 00000100 00000000
    //                  ^          
    // mask      = 11111011 11111111
    // result    = 00000100 00000000
    // idx = 10
    // byte_pos = 1
    // bit_pos  = 2
    byte_pos = idx / 8;
    bit_pos  = idx % 8;

    bm->bytes[byte_pos] = bm->bytes[byte_pos] & ~(1 << bit_pos);
    
    return 0;
}

uint8_t bm_clearmap(bitmap *bm) {
    if (!bm) {
        fprintf(stderr, "You should provide a non null bitmap pointer\n");
        return -1;
    }

    if (!bm->bytes) {
        fprintf(stderr, "Error: bitmap->bytes is NULL\n");
        return -1;
    }

    memset(bm->bytes, 0, bm->byte_len);
    return 0;
}

void bm_show(bitmap *bm) {
    int32_t n, bit;
    if (!bm) {
        fprintf(stderr, "You should provide a non null bitmap pointer\n");
        return;
    }

    printf("bitmap: 0 ~ %d\n", (int)(bm->len-1));
    for (n=0; n<bm->len; n++) {
        bit = bm_getbit(bm, n);
        printf("%d", bit);
        if (!((n+1)%4))
            printf(" ");
        if (!((n+1)%64))
            printf("\n");
    }
    printf("\n============\n");

    return;
}
