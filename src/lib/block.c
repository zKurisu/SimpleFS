/*
 * block.c
 * 
 * Copyright (C) Jie
 * 2025-11-15
 *
 */

#include "block.h"
#include "inode.h"
#include "helper.h"
#include "bitmap.h"
#include "error.h"

#include <stdio.h>
#include <stdlib.h>

uint32_t get_inode_per_block(disk *dd) {
    uint32_t ret, size;
    if (!dd) {
        fprintf(stderr, "Non null disk pointer needed to get inode per block...\n");
        return -1;
    }

    size = sizeof(struct s_inode);
    if (dd->block_size % size) {
        fprintf(stderr, "inode size [%d] is not align with block_size [%d], please re-design...\n",
                (int)size, (int)dd->block_size);
        return -1;
    }
    ret = dd->block_size / size;
    return ret;
}

block *bl_create(disk* dd, blocktype type) {
    block *bl;
    uint32_t size;
    if (!dd) {
        fprintf(stderr, "Non null disk pointer needed to create super block...\n");
        return (block*)0;
    }

    size = sizeof(struct s_block);
    bl = (block *)malloc(size);
    zero((uint8_t*)bl, size);
    if (!bl) {
        fprintf(stderr, "No enough memory for allocating a super block, usage [%lld]...\n",
                (long long)size);
        return (block*)0;
    }

    bl->type = type;
    bl->block_size = dd->block_size;
    size = dd->block_size;
    bl->data = (uint8_t *)malloc(size);
    zero((uint8_t*)(bl->data), size);
    if (!(bl->data)) {
        fprintf(stderr, "No enough memory for allocating a super block, usage [%lld]...\n",
                (long long)size);
        free(bl);
        return (block*)0;
    }
    return bl;
}

RC bl_set_data(block* super_block, uint8_t* data) {
    if (!super_block) {
        fprintf(stderr, "Set super error, you should not pass a null block pointer...\n");
        return ErrArg;
    }
    if (!data) {
        fprintf(stderr, "Warn: You pass a null data pointer...\n");
    }

    data = (uint8_t*)realloc(data, super_block->block_size);
    if (!data) {
        fprintf(stderr, "Failed to realloc, no enough memory [tar: %d]\n",
                super_block->block_size);
    }
    super_block->data = data;
    return OK;
}

void *bl_get_data(block* bl) {
    if (!bl) {
        fprintf(stderr, "Should not get data from a null block pointer...\n");
    }

    printf("Get data from type %hhx block\n", bl->type);

    return bl->data;
}

uint32_t bl_alloc(filesystem *fs) {
    if (!fs) {
        fprintf(stderr, "bl_alloc error: a non null pointer is needed...\n");
        return 0;
    }

    uint32_t block_number;
    for (block_number=0; block_number < fs->blocks; block_number++) {
        if (!bm_getbit(fs->block_bitmap, block_number)) {
            if (!bm_setbit(fs->block_bitmap, block_number)) // bm_setbit return 0 means success
                return block_number + 1; // convert to 1-based
            else
                return 0;
        }
    }

    fprintf(stderr, "bl_alloc error: could not find a available block...\n");

    return 0;
}

RC bl_free(filesystem *fs, uint32_t block_number) {
    if (!fs || block_number <= 0 || block_number > fs->blocks) {
        fprintf(stderr, "bl_free error: wrong args...\n");
        return 0;
    }

    // convert to 0-based
    if (bm_unsetbit(fs->block_bitmap, block_number-1) < 0) {
        fprintf(stderr, "bl_free error: could not unset bitmap index at [%d]",
                (int)block_number);
        return ErrBmOpe;
    }

    return OK;
}

RC bl_clean(filesystem *fs, uint32_t block_number) {
    if (!fs || block_number <= 0 || block_number > fs->blocks) {
        fprintf(stderr, "bl_clean error: wrong args...\n");
        return 0;
    }

    uint32_t block_size = fs->dd->block_size;
    uint8_t block_buf[block_size];
    memset(block_buf, 0, block_size);
    if (dwrite(fs->dd, block_buf, block_number) != OK) {
        fprintf(stderr, "bl_clean error: failed to write to block [%d]\n",
                block_number);
        return ErrDwrite;
    }

    return OK;
}
