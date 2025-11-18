/*
 * inode.c
 * 
 * Copyright (C) Jie
 * 2025-11-17
 *
 */
#include "inode.h"
#include "block.h"
#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RC ino_init(inode *ino) {
    if (!ino) {
        fprintf(stderr, "ino_init err: non null pointer needed...");
        return ErrArg;
    }

    uint32_t size = sizeof(struct s_inode);
    memset(ino, 0, size);

    ino->inode_number = 0;  // 0 means not used
    ino->file_type = FTypeNotValid;
    ino->file_size = 0;

    return OK;
}

uint32_t ino_alloc(filesystem *fs) {
    if (!fs) {
        fprintf(stderr, "ino_alloc error: non null filesystem pointer is needed...\n");
        return 0;
    }

    // Iterating all possible inodes
    uint32_t idx;
    for (idx=0; idx<=fs->inodes; idx++) {
        if (!bm_getbit(fs->inode_bitmap, idx)) { // if bit == 0, which means it is available
            bm_setbit(fs->inode_bitmap, idx); // Allocate means it is used
            // bitmap is 0-based
            // inode number/index is 1-based
            // Therefore return idx + 1
            return idx+1;
        }
    }

    fprintf(stderr, "ino_alloc error: no free inode...\n");
    return idx;
}

RC ino_free(filesystem *fs, uint32_t inode_number) {
    if (!fs || inode_number < 1 || inode_number > fs->inodes) {
        fprintf(stderr, "ino_free error: wrong arguments\n");
        return ErrArg;
    }

    // Convert back to 0-based
    if (bm_unsetbit(fs->inode_bitmap, inode_number-1) < 0) {
        fprintf(stderr, "ino_free error: failed to unsetbit at [%d]\n",
                (int)(inode_number));
        return ErrBmOpe;
    }

    return OK;
}

RC ino_read(filesystem *fs, uint32_t inode_number, inode* ino) {
    if (!fs || inode_number < 1 || inode_number > fs->inodes
            || !ino) {
        fprintf(stderr, "ino_read error: wrong arguments\n");
        return ErrArg;
    }

    // Check: is inode_number is allocated before, if not, return ErrArg
    if (!bm_getbit(fs->inode_bitmap, inode_number-1)) {
        fprintf(stderr, "ino_read error: wrong arguments, inode [%d] is not even allocated...\n",
                inode_number);
        return ErrArg;
    }

    uint32_t ino_per_block, block_number, inode_pos;
    uint32_t size = fs->dd->block_size;
    uint8_t block[size];
    uint8_t *data_begin;
    RC ret = OK;

    ino_per_block = get_inode_per_block(fs->dd);
    // Example:
    //  inode_number = 50
    //  ino_per_block = 16
    //  block_number = 50 / 16 = 3, should += 1
    //  block 1: 1~16
    //  block 2: 17~32
    //  block 3: 33~48
    //  block 4: 49~64
    block_number = inode_number / ino_per_block + fs->inode_table_start;

    // Read inode_pos 0
    // Read inode_pos 15
    inode_pos = (inode_number-1) % ino_per_block;

    ret = dread(fs->dd, block, block_number);
    if (ret != OK) {
        fprintf(stderr, "ino_read error: failed to read from block [%d]...\n",
                (int)block_number);
        return ret;
    }

    size = sizeof(struct s_inode);
    data_begin = block+(inode_pos*size);
    memcpy(ino, data_begin, size);
    
    return ret;
}

RC ino_write(filesystem *fs, uint32_t inode_number, inode* ino) {
    if (!fs || inode_number < 1 || inode_number > fs->inodes
            || !ino) {
        fprintf(stderr, "ino_write error: wrong arguments\n");
        return ErrArg;
    }

    // Check: is inode_number is allocated before, if not, return ErrArg
    if (!bm_getbit(fs->inode_bitmap, inode_number-1)) {
        fprintf(stderr, "ino_write error: wrong arguments, inode [%d] is not even allocated...\n",
                inode_number);
        return ErrArg;
    }
    uint32_t ino_per_block, block_number, inode_pos;
    uint32_t size = fs->dd->block_size;
    uint8_t block[size];
    uint8_t *data_begin;
    RC ret = OK;

    ino_per_block = get_inode_per_block(fs->dd);
    block_number = inode_number / ino_per_block + fs->inode_table_start;

    inode_pos = (inode_number-1) % ino_per_block;

    ret = dread(fs->dd, block, block_number);
    if (ret != OK) {
        fprintf(stderr, "ino_write error: failed to read from block [%d]...\n",
                (int)block_number);
        return ret;
    }

    size = sizeof(struct s_inode);
    data_begin = block+(inode_pos*size);
    memcpy(data_begin, ino, size);

    ret = dwrite(fs->dd, block, block_number);
    if (ret != OK) {
        fprintf(stderr, "ino_write error: failed to write to block [%d]...\n",
                (int)block_number);
    }
    
    return ret;
}
