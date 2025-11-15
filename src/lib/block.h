/*
 * block.h
 * Block strcuture
 * Copyright (C) Jie
 * 2025-11-14
 *
 */

#ifndef MY_BLOCK_H_
#define MY_BLOCK_H_

#include "disk.h"
#include "inode.h"

#include <stdint.h>

typedef enum {
    BlockTypeSuper,
    BlockTypeInode,
    BlockTypeData
} blocktype;

struct s_block {
    blocktype type;
    uint32_t block_size;
    void *data;
};
typedef struct s_block block;

// 16 bytes now
struct s_superblock_data {
    uint32_t blocks; // 4 bytes
    uint32_t inodeblocks; // 4 bytes
    uint32_t inodes; // 4 bytes
    uint16_t magic1; // 2 bytes
    uint16_t magic2; // 2 bytes
};
typedef struct s_superblock_data superblock_data;

uint32_t get_inode_per_block(disk *dd);

// Create a superblock
block *bl_create_super(disk *dd);

// Create a inode block
block *bl_create_inode(disk *dd);

// Create a data block
block *bl_create_data(disk *dd);

#endif
