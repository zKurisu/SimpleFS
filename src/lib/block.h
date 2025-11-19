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
#include "fs.h"
#include "error.h"

#include <stdint.h>

typedef enum {
    BlockTypeSuper,
    BlockTypeInode,
    BlockTypeData
} blocktype;

// 12 byte?
struct s_block {
    blocktype type;
    uint32_t block_size;
    uint8_t *data;
};
typedef struct s_block block;

// 16 bytes now
struct s_superblock_data {
    uint16_t magic1; // 2 bytes
    uint16_t magic2; // 2 bytes
    uint32_t blocks; // 4 bytes
    uint32_t inodeblocks; // 4 bytes
    uint32_t inodes; // 4 bytes
    
    uint32_t inode_bitmap_start;  // inode bitmap 起始块号
    uint32_t inode_bitmap_bl_count;
    
    uint32_t block_bitmap_start;  // data block bitmap 起始块号
    uint32_t block_bitmap_bl_count;
    
    uint32_t inode_table_start;   // inode table 起始块号
    
    uint32_t datablock_start;     // 数据块起始位置
    uint32_t datablock_bl_count;
    
    uint32_t free_blocks;         // 空闲块数（用于快速检查）
    uint32_t free_inodes;         // 空闲 inode 数
};
typedef struct s_superblock_data superblock_data;

uint32_t get_inode_per_block(disk *dd);

// Create a block
block *bl_create(disk *dd, blocktype type);

// Write to a data block
RC bl_set_data(block* data_block, uint8_t *data);

// Maybe only one read function is enough??
void *bl_get_data(block* bl);

// Allocate a free block, return block number
// This will edit block bitmap
uint32_t bl_alloc(filesystem *fs);

// Free a block
// This will edit block bitmap
RC bl_free(filesystem *fs, uint32_t block_number);
#endif
