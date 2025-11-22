/*
 * inode.h
 * 
 * Copyright (C) Jie
 * 2025-11-15
 *
 */
#ifndef MY_INODE_H_
#define MY_INODE_H_

#include "error.h"
#include "fs.h"

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

// Set a inode to init state
RC ino_init(inode *ino);

// Get a available inode by inode number
// it will edit inode_bitmap
uint32_t ino_alloc(filesystem *fs);

// Set a inode to invalid and clear the content
// it will edit inode_bitmap
RC ino_free(filesystem *fs, uint32_t inode_number);

// Read inode from disk
RC ino_read(filesystem *fs, uint32_t inode_number, inode *ino);

// Write inode to disk
RC ino_write(filesystem *fs, uint32_t inode_number, inode *ino); // Should check inode number

// Get a available block by block number
// offset is used for creating sparse file easily
// wrap bl_alloc inside
uint32_t ino_alloc_block_at(filesystem *fs, inode *ino, uint32_t offset);

// Get block number
uint32_t ino_get_block_at(filesystem *fs, inode *ino, uint32_t offset);

// Clear a block
// wrap bl_free inside
RC ino_free_block_at(filesystem *fs, inode *ino, uint32_t offset);

// Clear all blocks
// wrap bl_free inside
RC ino_free_all_blocks(filesystem *fs, inode *ino);

void ino_show(inode *ino);
int ino_is_valid(inode *ino);
uint32_t ino_get_block_count(filesystem *fs, inode *ino);
uint32_t ino_get_max_block_offset(filesystem *fs);

#endif
