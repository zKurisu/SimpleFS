/*
 * directory.c
 * 
 * Copyright (C) Jie
 * 2025-11-22
 *
 */

#include "directory.h"
#include "dirent.h"
#include "inode.h"
#include "error.h"

#include <stdio.h>
#include <string.h>

uint32_t dir_lookup(filesystem *fs, inode *dir_ino, const uint8_t *name) {
    if (!fs || !dir_ino || !name) { // Is null
        return 0;
    }

    if (dir_ino->file_type != FTypeDirectory) { // Must be a directory inode
        return 0;
    }

    if (dirent_check_valid_name(name) == ErrName) { // Is a valid name
        return 0;
    }

    uint32_t block_size = fs->dd->block_size;
    uint8_t  block_buf[block_size];
    uint32_t dirent_per_block = get_dirent_per_block(fs);
    if (dirent_per_block == 0) {
        fprintf(stderr, "dir_lookup error: could not get dirent_per_block...\n");
        return 0;
    }
    dirent dirent_list[dirent_per_block];
    uint32_t block_number, ino_block_offset = 0, max_ino_block_offset = ino_get_max_block_offset(fs);

    // Check direntry in blocks
    for (uint32_t n=0; n<max_ino_block_offset; n++) {
        block_number = ino_get_block_at(fs, dir_ino, ino_block_offset++);
        if (block_number == 0)
            continue;
            
        if (dread(fs->dd, block_buf, block_number) != OK) {
            fprintf(stderr, "dir_lookup error: could not read block from block_number: %d\n",
                    block_number);
            return 0;
        }
        memcpy(dirent_list, block_buf, block_size);
        for (uint32_t j=0; j<dirent_per_block; j++) {
            if (dirent_list[j].inode_num == 0) { // Not allocated
                continue;
            }
            if (strcmp((char*)dirent_list[j].name, (char*)name) == 0) {
                return dirent_list[j].inode_num;
            }
        }
    }

    printf("dir_lookup: Cound not find one...\n");
    return 0; // Could not find one
}

RC dir_add(filesystem *fs, inode *dir_ino, const uint8_t *name, uint32_t inode_num) {
    if (!fs || !dir_ino || !name || dirent_check_valid_name(name) != OK
            || inode_num <= 0 || inode_num > fs->inodes) {
        fprintf(stderr, "dir_add error: wrong args...\n");
        return ErrArg;
    }

    if (dir_lookup(fs, dir_ino, name) != 0) {// Check whether this name already added
        fprintf(stderr, "dir_add error: entry '%s' already exists\n", name);
        return ErrDirentExists;
    }

    // Check free space
    uint32_t dirent_size = sizeof(struct s_dirent);
    uint32_t max_block_offset = ino_get_max_block_offset(fs);
    uint32_t block_number;
    uint32_t block_used_space = dir_ino->file_size % fs->dd->block_size; 
    uint32_t block_size = fs->dd->block_size;
    uint8_t  block_buf[block_size];
    uint32_t dir_store_pos;
    if (block_used_space == 0) { // No free space, allocate a new one
        for (uint32_t n=0; n<max_block_offset; n++) {
            if ((block_number = ino_alloc_block_at(fs, dir_ino, n)) != 0) { // Suc to allocate
                break;
            }
        }
        dir_store_pos = 0;

    } else { // Have space, find that block
        for (uint32_t n=0; n<max_block_offset; n++) {
            block_number = ino_get_block_at(fs, dir_ino, n);
            if (block_number == 0) {
                continue;
            } else {
                // Use hole first
                dirent *dirent_list;
                uint32_t dirent_per_block = get_dirent_per_block(fs);
                if (dread(fs->dd, block_buf, block_number) != OK) {
                    fprintf(stderr, "dir_add error, failed to dread from block_number: %d\n",
                            (int)block_number);
                    return ErrDread;
                }
                dirent_list = (dirent*)block_buf;
                for (uint32_t j=0; j<dirent_per_block; j++) {
                    if (dirent_list[j].inode_num == 0) {
                        dir_store_pos = dirent_size*j;
                        break;
                    }
                }
                break;
            }
        }
    }
    if (dread(fs->dd, block_buf, block_number) != OK) {
        fprintf(stderr, "dir_add error, failed to dread from block_number: %d\n",
                (int)block_number);
        return ErrDread;
    }
    dirent new_dirent = {.inode_num = inode_num};
    memcpy(new_dirent.name, name, strlen((char*)name));
    
    memcpy(block_buf+dir_store_pos, &new_dirent, dirent_size);
    if (dwrite(fs->dd, block_buf, block_number) != OK) {
        fprintf(stderr, "dir_add error, failed to dwrite from block_number: %d\n",
                (int)block_number);
        return ErrDwrite;
    }
    dir_ino->file_size += dirent_size;
    
    return OK;
}

RC dir_remove(filesystem *fs, inode *dir_ino, const uint8_t *name) {
    if (!fs || !dir_ino || !name || dirent_check_valid_name(name) != OK) {
        fprintf(stderr, "dir_remove error: wrong args...\n");
        return ErrArg;
    }

    if (dir_ino->file_type != FTypeDirectory) {
        fprintf(stderr, "dir_remove error: not a directory\n");
        return ErrArg;
    }

    uint32_t inode_num;
    // 查找条目
    if ((inode_num = dir_lookup(fs, dir_ino, name)) == 0) {
        fprintf(stderr, "dir_remove error: entry '%s' not found\n", name);
        return ErrNotFound;
    }

    // 定位并删除目录项
    uint32_t block_size = fs->dd->block_size;
    uint8_t block_buf[block_size];
    uint32_t dirent_per_block = get_dirent_per_block(fs);
    uint32_t dirent_size = sizeof(struct s_dirent);
    uint32_t max_offset = ino_get_max_block_offset(fs);

    // 遍历所有块，找到并删除该条目
    for (uint32_t offset = 0; offset < max_offset; offset++) {
        uint32_t block_number = ino_get_block_at(fs, dir_ino, offset);

        // 跳过未分配的块
        if (block_number == 0) {
            continue;
        }

        // 读取块
        if (dread(fs->dd, block_buf, block_number) != OK) {
            fprintf(stderr, "dir_remove error: failed to read block %u\n", block_number);
            return ErrDread;
        }

        // 解析目录项
        dirent *dirent_list = (dirent *)block_buf;
        for (uint32_t j = 0; j < dirent_per_block; j++) {
            // 跳过空条目
            if (dirent_list[j].inode_num == 0) {
                continue;
            }

            // 找到匹配的条目
            if (strcmp((char*)dirent_list[j].name, (char*)name) == 0) {
                // 标记为删除（设置 inode_num = 0）
                dirent_list[j].inode_num = 0;
                memset(dirent_list[j].name, 0, MAX_FILENAME_LEN);

                // 写回磁盘
                if (dwrite(fs->dd, block_buf, block_number) != OK) {
                    fprintf(stderr, "dir_remove error: failed to write block %u\n", block_number);
                    return ErrDwrite;
                }

                // 释放对应的 inode（如果需要的话）
                // 注意：这里只从目录中删除条目，不释放 inode
                // inode 的释放应该由更高层的函数（如 fs_unlink）决定
                // 因为可能有硬链接等情况

                // 可选：减少 file_size（如果采用"删除即压缩"策略）
                // 这里我们保持 file_size 不变，允许空洞
                // dir_ino->file_size -= dirent_size;

                return OK;
            }
        }
    }

    // 理论上不应该到这里（dir_lookup 已经确认存在）
    fprintf(stderr, "dir_remove error: inconsistent state\n");
    return ErrInternal;
}

RC dir_list(filesystem *fs, inode *dir_ino) {
    if (!fs || !dir_ino) {
        fprintf(stderr, "dir_remove error: wrong args...\n");
        return ErrArg;
    }

    if (dir_ino->file_type != FTypeDirectory) {
        fprintf(stderr, "dir_remove error: not a directory\n");
        return ErrArg;
    }

    uint32_t block_size = fs->dd->block_size;
    uint8_t block_buf[block_size];
    uint32_t dirent_per_block = get_dirent_per_block(fs);
    uint32_t dirent_size = sizeof(struct s_dirent);
    uint32_t max_offset = ino_get_max_block_offset(fs);
    uint32_t dirent_count = 0;
    printf("========================================\n");
    printf("Directory list\n");
    printf("========================================\n");

    for (uint32_t offset = 0; offset < max_offset; offset++) {
        uint32_t block_number = ino_get_block_at(fs, dir_ino, offset);

        // 跳过未分配的块
        if (block_number == 0) {
            continue;
        }

        // 读取块
        if (dread(fs->dd, block_buf, block_number) != OK) {
            fprintf(stderr, "dir_remove error: failed to read block %u\n", block_number);
            return ErrDread;
        }

        dirent *dirent_list = (dirent *)block_buf;
        for (uint32_t j = 0; j < dirent_per_block; j++) {
            if (dirent_list[j].inode_num != 0) {
                dirent_count++;
                printf("dirent %d:\n"
                       "  inode_num: %d\n"
                       "  name     : %s\n"
                        , dirent_count, dirent_list[j].inode_num,
                        (char*)dirent_list[j].name
                );
            }
        }
    }
    printf("dirent count: %d\n", dirent_count);
    printf("========================================\n");
    return OK;
}
