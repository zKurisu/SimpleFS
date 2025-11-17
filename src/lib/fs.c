/*
 * fs.c
 * 
 * Copyright (C) Jie
 * 2025-11-17
 *
 */

#include "fs.h"
#include "block.h"
#include "error.h"
#include "helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RC fs_format(disk *dd) {
    if (!dd) {
        fprintf(stderr, "Should pass a non null disk pointer for formatting\n");
        return ErrArg;
    }

    uint32_t size;
    superblock_data *super_data;
    RC ret = OK;

    // Init a superblock data structure
    size = dd->block_size;
    super_data = (superblock_data *)malloc(size);
    if (!super_data) {
        fprintf(stderr, "Failed to allocate memory for superblock_data structure...\n");
        return ErrNoMem;
    }
    memset(super_data, 0, size);

    super_data->magic1 = Magic1;    // Constant define in fs.h
    super_data->magic2 = Magic2;    // Constant define in fs.h
    super_data->blocks = dd->blocks;

    super_data->inodeblocks = dd->blocks
        * InodeBlockPercentage;     // Constant define in fs.h

    super_data->inodes = super_data->inodeblocks
        * get_inode_per_block(dd);
    super_data->inode_bitmap_start = 2; // 1 is superblock
    super_data->inode_bitmap_bl_count = cal_needed_bitmap_blocks(super_data->inodes, dd->block_size);

    super_data->block_bitmap_start = 2
        + super_data->inode_bitmap_bl_count;
    super_data->block_bitmap_bl_count = cal_needed_bitmap_blocks(dd->blocks, dd->block_size);

    super_data->inode_table_start  = super_data->block_bitmap_start 
        + super_data->block_bitmap_bl_count;

    super_data->datablock_start = super_data->inode_table_start 
        + super_data->inodeblocks;
    super_data->datablock_bl_count = super_data->blocks - super_data->inodeblocks;

    super_data->free_blocks = super_data->blocks - super_data->datablock_start + 1;
    super_data->free_inodes = super_data->inodes;

    ret = dwrite(dd, (uint8_t*)super_data, 1);

    return ret;
}
