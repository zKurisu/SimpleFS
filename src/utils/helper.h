/*
 * helper.h
 * Helper function signatures
 * Copyright (C) Jie
 * 2025-11-14
 *
 */

#ifndef MY_HELPER_H_
#define MY_HELPER_H_
#include <string.h>
#include <stdint.h>

#define zero(str,size) memset(str,0,size)

uint32_t cal_needed_bitmap_blocks(uint32_t bits, uint32_t block_size);

#endif
