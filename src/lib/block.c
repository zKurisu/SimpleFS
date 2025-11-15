/*
 * block.c
 * 
 * Copyright (C) Jie
 * 2025-11-15
 *
 */

#include "block.h"

#include <stdio.h>
#include <stdlib.h>

uint32_t get_inode_per_block(disk *dd) {
    uint32_t ret, size;
    if (!dd) {
        fprintf(stderr, "Non null disk pointer needed to get inode per block...");
        return -1;
    }

    size = sizeof(struct s_inode);
    if (dd->block_size % size) {
        fprintf(stderr, "inode size [%d] is not align with block_size [%d], please re-design...",
                (int)size, (int)dd->block_size);
        return -1;
    }
    ret = dd->block_size / size;
    return ret;
}

block *bl_create_super(disk* dd) {
    block *bl;
    uint32_t size;
    if (!dd) {
        fprintf(stderr, "Non null disk pointer needed to create super block...");
        return (block*)0;
    }

    size = sizeof(struct s_block);
    bl = (block *)malloc(size);
    if (!bl) {
        fprintf(stderr, "No enough memory for allocating a super block, usage [%lld]...",
                (long long)size);
        return (block*)0;
    }

    bl->type = BlockTypeSuper;
    bl->block_size = dd->block_size;
    size = sizeof(struct s_superblock_data);
    bl->data = (superblock_data *)malloc(size);
    if (!(bl->data)) {
        fprintf(stderr, "No enough memory for allocating a super block, usage [%lld]...",
                (long long)size);
        free(bl);
        return (block*)0;
    }
    return bl;
}

block *bl_create_inode(disk *dd) {
    block *bl;
    uint32_t inode_per_block, size;
    if (!dd) {
        fprintf(stderr, "Non null disk pointer needed to create inode block...");
        return (block*)0;
    }

    size = sizeof(struct s_block);
    bl = (block *)malloc(size);
    if (!bl) {
        fprintf(stderr, "No enough memory for allocating a inode block, usage [%lld]...",
                (long long)size);
        return (block*)0;
    }

    bl->type = BlockTypeInode;
    bl->block_size = dd->block_size;

    inode_per_block = get_inode_per_block(dd);
    size = inode_per_block*sizeof(struct s_inode);
    bl->data = (inode *)malloc(size);
    if (!(bl->data)) {
        fprintf(stderr, "No enough memory for allocating a inode block, usage [%lld]...",
                (long long)size);
        free(bl);
        return (block*)0;
    }

    return bl;
}

block *bl_create_data(disk *dd) {
    block *bl;
    uint32_t size;
    if (!dd) {
        fprintf(stderr, "Non null disk pointer needed to create data block...");
        return (block*)0;
    }

    size = sizeof(struct s_block);
    bl = (block *)malloc(size);
    if (!bl) {
        fprintf(stderr, "No enough memory for allocating a data block, usage [%lld]...",
                (long long)size);
        return (block*)0;
    }

    bl->type = BlockTypeData;
    bl->block_size = dd->block_size;

    size = dd->block_size;
    bl->data = (uint8_t *)malloc(size);
    if (!(bl->data)) {
        fprintf(stderr, "No enough memory for allocating a data block, usage [%lld]...",
                (long long)size);
        free(bl);
        return (block*)0;
    }

    return bl;
}
