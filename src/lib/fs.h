/*
 * fs.h
 * Filesystem structure
 * Copyright (C) Jie
 * 2025-11-15
 *
 */

#ifndef MY_FS_H_
#define MY_FS_H_

#include "disk.h"
#include "bitmap.h"

#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define Magic1 (0x04)
#define Magic2 (0x17)
#define InodeBlockPercentage (0.1) // How many blocks inode table takes in

struct s_filesystem {
    disk *dd;                     // Low level disk simulator

    // superblock metadata
    uint32_t blocks;
    uint32_t inodeblocks;
    uint32_t inodes;
    uint32_t inode_bitmap_start; // Dynamic calc
    uint32_t inode_bitmap_bl_count;
    uint32_t block_bitmap_start; // Dynamic calc
    uint32_t block_bitmap_bl_count;
    uint32_t inode_table_start; // Dynamic calc
    uint32_t datablock_start;    // Dynamic calc
    uint32_t datablock_bl_count;

    // Bitmap objs
    bitmap *inode_bitmap;          // inode alloc
    bitmap *block_bitmap;          // block alloc

    // Thread synchronization
    pthread_mutex_t dir_lock;      // Protects directory operations (dir_add, dir_remove, dir_lookup)
};
typedef struct s_filesystem filesystem;

RC fs_format(disk *dd);  // Format disk, init a superblock
RC fs_mount(disk *dd, filesystem *fs);   // Init a filesystem struct
RC fs_unmount(filesystem *fs);           // Destroy a filesystem struct
RC fs_show(filesystem *fs); // display filesystem infomation

// Helper function
uint32_t cal_needed_bitmap_blocks(uint32_t bits, uint32_t block_size);

#ifdef __cplusplus
}
#endif

#endif
