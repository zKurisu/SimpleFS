/*
 * inode.h
 * 
 * Copyright (C) Jie
 * 2025-11-15
 *
 */
#ifndef MY_INODE_H_
#define MY_INODE_H_

#include <stdint.h>

#define DIRECT_POINTERS 12

// 4 bytes?
typedef enum {
    FTypeNotValid,
    FTypeFile,
    FTypeDirectory
} filetype;

// 64 bytes
struct s_inode {
    uint32_t inode_number;
    filetype file_type;
    uint32_t file_size;

    uint32_t direct_blocks[DIRECT_POINTERS]; // 12 * 4 = 48 bytes

    uint32_t single_indirect;
};
typedef struct s_inode inode;

#endif
