/*
 * fs.c
 * 
 * Copyright (C) Jie
 * 2025-11-17
 *
 */

#include "fs.h"
#include "block.h"
#include "inode.h"
#include "error.h"
#include "disk.h"
#include "bitmap.h"

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
    super_data->datablock_bl_count = super_data->blocks - super_data->inodeblocks - 1 -
        super_data->inode_bitmap_bl_count - super_data->block_bitmap_bl_count;

    super_data->free_blocks = super_data->blocks - super_data->datablock_start + 1;
    super_data->free_inodes = super_data->inodes;

    // Write super block data to disk
    ret = dwrite(dd, (uint8_t*)super_data, 1);

    // Init inode bitmap block
    size = dd->block_size;
    uint8_t *bitmap_buf = (uint8_t *)malloc(size);
    memset(bitmap_buf, 0, size);
    for (uint32_t i = 0; i < super_data->inode_bitmap_bl_count; i++) {
        ret = dwrite(dd, bitmap_buf, super_data->inode_bitmap_start+i);
        printf("Write inode bitmap block at block number: %d\n", (int)(super_data->inode_bitmap_start+i));
        if (ret != OK) {
            free(super_data);
            fprintf(stderr, "fs_format error: falied to init inode bitmap block at block %d\n",i);
            return ret;
        }
    }
    free(bitmap_buf);

    // Init block bitmap block
    bitmap *block_bitmap;
    uint32_t start, end;
    size = super_data->datablock_bl_count * dd->block_size;
    block_bitmap = bm_create(size);
    if (!block_bitmap) {
        free(super_data);
        fprintf(stderr, "fs_format error: Failed to create a block_bitmap...\n");
        return ErrBmCreate;
    }
    // bitmap index is 0-based, block number is 1-based. should change it????
    for (uint32_t i=0; i<super_data->datablock_start-1; i++)
        bm_setbit(block_bitmap, i);
    start = super_data->block_bitmap_start;
    printf("Start block number for block bitmap is %d\n", start);
    end = start+super_data->block_bitmap_bl_count-1;
    ret = dwrites(dd, block_bitmap->bytes, start, end);
    if (ret != OK) {
        free(super_data);
        bm_destroy(block_bitmap);
        fprintf(stderr, "fs_format error: Failed to init block_bitmap...\n");
        return ret;
    }

    bm_destroy(block_bitmap);
    // Init inode table blocks
    uint8_t *block_buf = (uint8_t *)malloc(dd->block_size);
    if (!block_buf) {
        fprintf(stderr, "Failed to allocate block buffer\n");
        return ErrNoMem;
    }

    uint32_t inodes_per_block = get_inode_per_block(dd);

    // Initialize each inode block
    for (uint32_t i = 0; i < super_data->inodeblocks; i++) {
        memset(block_buf, 0, dd->block_size);

        inode *inodes = (inode *)block_buf;
        for (uint32_t j = 0; j < inodes_per_block; j++) {
            ret = ino_init(&inodes[j]);
            if (ret != OK) {
                fprintf(stderr, "fs_format error: falied to init inode block at block %d, slot %d\n",
                        i, j);
                return ret;
            }
        }

        // write back inode block to disk
        ret = dwrite(dd, block_buf, super_data->inode_table_start + i);
        if (ret != OK) {
            fprintf(stderr, "Failed to write inode block %d\n", i);
            free(block_buf);
            return ret;
        }
    }

    return ret;
}

RC fs_mount(disk *dd, filesystem *fs) {
    if (!dd || !fs)
        return ErrArg;

    RC ret = OK;
    uint32_t size, start, end;
    superblock_data *super_data;
    bitmap *inode_bitmap, *block_bitmap;

    size = dd->block_size;
    super_data = (superblock_data *)malloc(size);
    memset(super_data, 0, size);

    ret = dread(dd, (uint8_t *)super_data, 1);
    if (ret != OK) {
        free(super_data);
        return ret;
    }

    // Clear filesystem structure
    size = sizeof(struct s_filesystem);
    memset(fs, 0, size);

    fs->dd = dd;
    fs->blocks                = super_data->blocks;
    fs->inodeblocks           = super_data->inodeblocks;
    fs->inodes                = super_data->inodes;
    fs->inode_bitmap_start    = super_data->inode_bitmap_start;
    fs->inode_bitmap_bl_count = super_data->inode_bitmap_bl_count;
    fs->block_bitmap_start    = super_data->block_bitmap_start;
    fs->block_bitmap_bl_count = super_data->block_bitmap_bl_count;
    fs->inode_table_start     = super_data->inode_table_start;
    fs->datablock_start       = super_data->datablock_start;
    fs->datablock_bl_count    = super_data->datablock_bl_count;

    // setup bitmap
    size = super_data->inode_bitmap_bl_count * dd->block_size;
    inode_bitmap = bm_create(size);
    if (!inode_bitmap) {
        free(super_data);
        fprintf(stderr, "Failed to create a inode_bitmap...\n");
        return ErrBmCreate;
    }
    // if start: 3, count: 1, end should be 3 = start + count - 1
    start = super_data->inode_bitmap_start;
    end = start+super_data->inode_bitmap_bl_count-1;
    ret = dreads(dd, inode_bitmap->bytes, start, end);
    if (ret != OK) {
        free(super_data);
        bm_destroy(inode_bitmap);
        fprintf(stderr, "fs_mount error, failed to read inode bitmap info from disk...\n");
        return ret;
    }

    size = super_data->datablock_bl_count * dd->block_size;
    block_bitmap = bm_create(size);
    if (!block_bitmap) {
        free(super_data);
        fprintf(stderr, "Failed to create a block_bitmap...\n");
        return ErrBmCreate;
    }
    start = super_data->block_bitmap_start;
    end = start+super_data->block_bitmap_bl_count-1;
    ret = dreads(dd, block_bitmap->bytes, start, end);
    if (ret != OK) {
        free(super_data);
        bm_destroy(inode_bitmap);
        bm_destroy(block_bitmap);
        fprintf(stderr, "fs_mount error, failed to read inode bitmap info from disk...\n");
        return ret;
    }

    fs->inode_bitmap = inode_bitmap;
    fs->block_bitmap = block_bitmap;

    // Initialize directory lock
    if (pthread_mutex_init(&fs->dir_lock, NULL) != 0) {
        free(super_data);
        bm_destroy(inode_bitmap);
        bm_destroy(block_bitmap);
        fprintf(stderr, "fs_mount error: failed to initialize directory lock\n");
        return ErrInternal;
    }

    free(super_data);
    return ret;
}

RC fs_unmount(filesystem *fs) {
    if (!fs) {
        fprintf(stderr, "fs_unmount error, non null pointer needed...\n");
        return ErrArg;
    }

    RC ret = OK;
    uint32_t start, end;

    start = fs->inode_bitmap_start;
    end = start+fs->inode_bitmap_bl_count-1;
    ret = dwrites(fs->dd, fs->inode_bitmap->bytes, start, end);
    if (ret != OK) {
        fprintf(stderr, "fs_unmount error, failed to write inode bitmap back to disk...\n");
        return ret;
    }

    start = fs->block_bitmap_start;
    end = start+fs->block_bitmap_bl_count-1;
    ret = dwrites(fs->dd, fs->block_bitmap->bytes, start, end);
    if (ret != OK) {
        fprintf(stderr, "fs_unmount error, failed to write block bitmap back to disk...\n");
        return ret;
    }

    // Use bm_destroy() to properly free bitmaps
    if (fs->inode_bitmap) {
        bm_destroy(fs->inode_bitmap);
        fs->inode_bitmap = NULL;
    }

    if (fs->block_bitmap) {
        bm_destroy(fs->block_bitmap);
        fs->block_bitmap = NULL;
    }

    // Destroy directory lock
    pthread_mutex_destroy(&fs->dir_lock);

    return ret;
}

RC fs_show(filesystem *fs) {
    if (!fs) {
        fprintf(stderr, "fs_show error: null filesystem pointer\n");
        return ErrArg;
    }

    printf("========================================\n");
    printf("Filesystem Information\n");
    printf("========================================\n");

    // 1. Basic disk information
    if (fs->dd) {
        printf("Disk:\n");
        printf("  Disk ID:          %d\n", fs->dd->id);
        printf("  Block size:       %u bytes\n", fs->dd->block_size);
        printf("  Total blocks:     %u\n", fs->dd->blocks);
        printf("  Total size:       %lld bytes (%.2f KB)\n",
               (long long)(fs->dd->blocks * fs->dd->block_size),
               (double)(fs->dd->blocks * fs->dd->block_size) / 1024.0);
    } else {
        printf("Disk:               <not attached>\n");
    }

    printf("\n");

    // 2. Filesystem layout
    printf("Filesystem Layout:\n");
    printf("  Total blocks:     %u\n", fs->blocks);
    printf("  Inode blocks:     %u (%.1f%%)\n",
           fs->inodeblocks,
           (double)fs->inodeblocks / fs->blocks * 100.0);
    printf("  Total inodes:     %u\n", fs->inodes);
    printf("  Data blocks:      %u (%.1f%%)\n",
           fs->datablock_bl_count,
           (double)fs->datablock_bl_count / fs->blocks * 100.0);

    printf("\n");

    // 3. Block allocation map
    printf("Block Allocation Map:\n");
    printf("  [Block   1]:      Super Block\n");
    printf("  [Block %3u]:      Inode Bitmap (%u block%s)\n",
           fs->inode_bitmap_start,
           fs->inode_bitmap_bl_count,
           fs->inode_bitmap_bl_count > 1 ? "s" : "");
    printf("  [Block %3u]:      Block Bitmap (%u block%s)\n",
           fs->block_bitmap_start,
           fs->block_bitmap_bl_count,
           fs->block_bitmap_bl_count > 1 ? "s" : "");
    printf("  [Block %3u-%3u]:  Inode Table (%u blocks)\n",
           fs->inode_table_start,
           fs->inode_table_start + fs->inodeblocks - 1,
           fs->inodeblocks);
    printf("  [Block %3u-%3u]:  Data Blocks (%u blocks)\n",
           fs->datablock_start,
           fs->blocks,
           fs->datablock_bl_count);

    printf("\n");

    // 4. Bitmap statistics
    printf("Bitmap Statistics:\n");

    // Count free inodes
    uint32_t free_inodes = 0;
    if (fs->inode_bitmap) {
        for (uint32_t i = 0; i < fs->inodes; i++) {
            if (bm_getbit(fs->inode_bitmap, i) == 0) {
                free_inodes++;
            }
        }
        uint32_t used_inodes = fs->inodes - free_inodes;
        printf("  Inodes:\n");
        printf("    Total:          %u\n", fs->inodes);
        printf("    Used:           %u (%.1f%%)\n",
               used_inodes,
               (double)used_inodes / fs->inodes * 100.0);
        printf("    Free:           %u (%.1f%%)\n",
               free_inodes,
               (double)free_inodes / fs->inodes * 100.0);
    } else {
        printf("  Inode bitmap:     <not loaded>\n");
    }

    printf("\n");

    // Count free blocks
    uint32_t free_blocks = 0;
    if (fs->block_bitmap) {
        for (uint32_t i = 0; i < fs->blocks; i++) {
            if (bm_getbit(fs->block_bitmap, i) == 0) {
                free_blocks++;
            }
        }
        uint32_t used_blocks = fs->blocks - free_blocks;
        printf("  Blocks:\n");
        printf("    Total:          %u\n", fs->blocks);
        printf("    Used:           %u (%.1f%%)\n",
               used_blocks,
               (double)used_blocks / fs->blocks * 100.0);
        printf("    Free:           %u (%.1f%%)\n",
               free_blocks,
               (double)free_blocks / fs->blocks * 100.0);

        // Calculate usable data blocks
        uint32_t metadata_blocks = fs->datablock_start - 1;
        uint32_t free_data_blocks = free_blocks;
        if (free_blocks >= metadata_blocks) {
            printf("    Free (data):    %u blocks (%lld bytes, %.2f KB)\n",
                   free_data_blocks,
                   (long long)(free_data_blocks * fs->dd->block_size),
                   (double)(free_data_blocks * fs->dd->block_size) / 1024.0);
        }
    } else {
        printf("  Block bitmap:     <not loaded>\n");
    }

    printf("========================================\n");

    return OK;
}

uint32_t cal_needed_bitmap_blocks(uint32_t bits, uint32_t block_size) {
    uint32_t bits_per_block = block_size * 8;
    uint32_t ret = bits / bits_per_block;
    if (bits % bits_per_block)
        ret += 1;
    return ret;
}
