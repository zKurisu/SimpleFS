/*
 * dirent.h
 * Directory entry, using to map file name to a inode number
 * Copyright (C) Jie
 * 2025-11-19
 *
 */

#ifndef MY_DIRENT_H_
#define MY_DIRENT_H_

#include "error.h"
#include "fs.h"

#include <stdint.h>

#define MAX_FILENAME_LEN 252 // ori is 28, 255 
#define VALID_NAME_SPECIAL_CHARS (uint8_t*)"._-" // Special chars

// 32 bytes now
struct s_dirent {
    uint32_t inode_num;              // 4 bytes
    uint8_t name[MAX_FILENAME_LEN];  // 28 bytes
};
typedef struct s_dirent dirent;

RC dirent_check_valid_name(const uint8_t *name);
uint32_t get_dirent_per_block(filesystem *fs); // Block size should be integer multiple of directory entry size

#endif
