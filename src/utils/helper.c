/*
 * helper.c
 * Helper functions
 * Copyright (C) Jie
 * 2025-11-14
 *
 */

#include "helper.h"

uint32_t cal_needed_bitmap_blocks(uint32_t bits, uint32_t block_size) {
    uint32_t bits_per_block = block_size * 8;
    uint32_t ret = bits / bits_per_block;
    if (bits % bits_per_block)
        ret += 1;
    return ret;
}

