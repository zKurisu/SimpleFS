/*
 * bitmap.h
 * Structure for manage block and inode allocate
 * Copyright (C) Jie
 * 2025-11-14
 *
 */

#ifndef MY_BITMAP_H_
#define MY_BITMAP_H_
#include <stdint.h>

struct s_bitmap {
    uint8_t *bytes;
    uint32_t byte_len;
    uint32_t len; // max bit number
};
typedef struct s_bitmap bitmap;

// bm is short for bitmap

// Return -1 for error condition, not RC this time
bitmap *bm_create(uint32_t size); // create a bitmap
uint8_t bm_getbit(bitmap *bm, uint32_t idx);
uint8_t bm_setbit(bitmap *bm, uint32_t idx);
uint8_t bm_unsetbit(bitmap *bm, uint32_t idx);
uint8_t bm_clearmap(bitmap *bm);
void bm_show(bitmap *bm);

#endif
