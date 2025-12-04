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
#include "block.h"

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

    // printf("dir_lookup: Cound not find one...\n");
    return 0; // Could not find one
}

RC dir_lookup_by_id(filesystem *fs, inode *dir_ino, uint8_t *buf, uint32_t inode_num) {
    if (!fs || !dir_ino || !buf) {
        fprintf(stderr, "dir_lookup_by_id error: wrong args");
        return ErrArg;
    }

    uint32_t max_ino_block_offset = ino_get_max_block_offset(fs);
    uint32_t block_size = fs->dd->block_size;
    uint32_t dirent_per_block = get_dirent_per_block(fs);
    uint8_t block_buf[block_size];
    memset(block_buf, 0, block_size);
    uint32_t block_number;
    for (uint32_t i=0; i<max_ino_block_offset; i++) {
        if ((block_number = ino_get_block_at(fs, dir_ino, i)) == 0) {
            continue;
        }

        // Find a block
        if (dread(fs->dd, block_buf, block_number) != OK) {
            fprintf(stderr, "dir_lookup_by_id error: failed to read block [%d]\n",
                    block_number);
            return ErrDread;
        }
        dirent *dirent_list = (dirent *)block_buf;
        for (uint32_t j=0; j<dirent_per_block; j++) {
            if (dirent_list[j].inode_num == inode_num) {
                memset(buf, 0, MAX_FILENAME_LEN);
                memcpy(buf, dirent_list[j].name, MAX_FILENAME_LEN);
                return OK;
            }
        }
    }

    return ErrNotFound;
}

RC dir_add(filesystem *fs, inode *dir_ino, const uint8_t *name, uint32_t inode_num) {
    if (!fs || !dir_ino || !name || dirent_check_valid_name(name) != OK
            || inode_num <= 0 || inode_num > fs->inodes) {
        fprintf(stderr, "dir_add error: wrong args...\n");
        return ErrArg;
    }

    // Lock directory operations to prevent race conditions
    pthread_mutex_lock(&fs->dir_lock);

    if (dir_lookup(fs, dir_ino, name) != 0) {// Check whether this name already added
        fprintf(stderr, "dir_add error: entry '%s' already exists\n", name);
        pthread_mutex_unlock(&fs->dir_lock);
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
                if (bl_clean(fs, block_number) != OK) {
                    fprintf(stderr, "dir_add error: failed to clean block\n");
                    pthread_mutex_unlock(&fs->dir_lock);
                    return ErrDwrite;
                }
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
                    pthread_mutex_unlock(&fs->dir_lock);
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
        pthread_mutex_unlock(&fs->dir_lock);
        return ErrDread;
    }
    dirent new_dirent = {.inode_num = inode_num};
    memcpy(new_dirent.name, name, strlen((char*)name));

    memcpy(block_buf+dir_store_pos, &new_dirent, dirent_size);
    if (dwrite(fs->dd, block_buf, block_number) != OK) {
        fprintf(stderr, "dir_add error, failed to dwrite from block_number: %d\n",
                (int)block_number);
        pthread_mutex_unlock(&fs->dir_lock);
        return ErrDwrite;
    }
    dir_ino->file_size += dirent_size;

    pthread_mutex_unlock(&fs->dir_lock);
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

    // Lock directory operations to prevent race conditions
    pthread_mutex_lock(&fs->dir_lock);

    uint32_t inode_num;
    // 查找条目
    if ((inode_num = dir_lookup(fs, dir_ino, name)) == 0) {
        fprintf(stderr, "dir_remove error: entry '%s' not found\n", name);
        pthread_mutex_unlock(&fs->dir_lock);
        return ErrNotFound;
    }

    // 定位并删除目录项
    uint32_t block_size = fs->dd->block_size;
    uint8_t block_buf[block_size];
    uint32_t dirent_per_block = get_dirent_per_block(fs);
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
            pthread_mutex_unlock(&fs->dir_lock);
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
                    pthread_mutex_unlock(&fs->dir_lock);
                    return ErrDwrite;
                }

                // 释放对应的 inode（如果需要的话）
                // 注意：这里只从目录中删除条目，不释放 inode
                // inode 的释放应该由更高层的函数（如 fs_unlink）决定
                // 因为可能有硬链接等情况

                // 可选：减少 file_size（如果采用"删除即压缩"策略）
                // 这里我们保持 file_size 不变，允许空洞
                // dir_ino->file_size -= dirent_size;

                pthread_mutex_unlock(&fs->dir_lock);
                return OK;
            }
        }
    }

    // 理论上不应该到这里（dir_lookup 已经确认存在）
    fprintf(stderr, "dir_remove error: inconsistent state\n");
    pthread_mutex_unlock(&fs->dir_lock);
    return ErrInternal;
}

RC dir_list(filesystem *fs, inode *dir_ino) {
    if (!fs || !dir_ino) {
        fprintf(stderr, "dir_list error: wrong args...\n");
        return ErrArg;
    }

    if (dir_ino->file_type != FTypeDirectory) {
        fprintf(stderr, "dir_list error: not a directory\n");
        return ErrArg;
    }

    uint32_t block_size = fs->dd->block_size;
    uint8_t block_buf[block_size];
    uint32_t dirent_per_block = get_dirent_per_block(fs);
    uint32_t max_offset = ino_get_max_block_offset(fs);
    uint32_t dirent_count = 0;
    printf("========================================\n");
    printf("Directory list\n");
    printf("========================================\n");

    for (uint32_t offset = 0; offset < max_offset; offset++) {
        uint32_t block_number = ino_get_block_at(fs, dir_ino, offset);

        // Ignore unused offset
        if (block_number == 0) {
            continue;
        }

        // If find used offset, read the block
        if (dread(fs->dd, block_buf, block_number) != OK) {
            fprintf(stderr, "dir_list error: failed to read block %u\n", block_number);
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
    printf("Directory list END\n");
    printf("========================================\n");
    return OK;
}

uint8_t dir_is_empty(filesystem *fs, inode *dir_ino) {
    if (!fs || !dir_ino) {
        fprintf(stderr, "dir_is_empty error: wrong args...\n");
        return ErrArg;
    }

    if (dir_ino->file_type != FTypeDirectory) {
        fprintf(stderr, "dir_is_empty error: not a directory\n");
        return ErrArg;
    }

    uint8_t block_buf[fs->dd->block_size];
    uint32_t dirent_per_block = get_dirent_per_block(fs);
    uint32_t max_offset = ino_get_max_block_offset(fs);

    for (uint32_t offset = 0; offset < max_offset; offset++) {
        uint32_t block_number = ino_get_block_at(fs, dir_ino, offset);

        // Ignore unused offset
        if (block_number == 0) {
            continue;
        }

        // If find used offset, read the block
        if (dread(fs->dd, block_buf, block_number) != OK) {
            fprintf(stderr, "dir_is_empty error: failed to read block %u\n", block_number);
            return ErrDread;
        }

        dirent *dirent_list = (dirent *)block_buf;
        for (uint32_t j = 0; j < dirent_per_block; j++) {
            if (dirent_list[j].inode_num != 0 &&
                (strcmp((char*)dirent_list[j].name, "..") != 0) && // Not parent
                (strcmp((char*)dirent_list[j].name, ".")  != 0)    // Not self
            )
                return 0;
        }
    }
    return 1;
}

uint32_t dir_create_root(filesystem *fs) {
    if (!fs) {
        fprintf(stderr, "dir_create_root error: fs pointer is null...\n");
        return 0;
    }

    uint32_t inode_num, indirect_blk_num;
    inode dir_ino;
    const uint8_t *cur_dir_name = (uint8_t*)".";
    const uint8_t *parent_dir_name = (uint8_t*)"..";
    RC ret;

    ret = ino_init(&dir_ino);
    if (ret != OK) {
        fprintf(stderr, "dir_create_root error: failed to init an inode...\n");
        return 0;
    }

    inode_num = ino_alloc(fs);
    if (inode_num == 0) {
        fprintf(stderr, "dir_create_root error: failed to alloc inode number...\n");
        return 0;
    }

    indirect_blk_num = bl_alloc(fs);
    if (indirect_blk_num == 0) {
        fprintf(stderr, "dir_create_root error: failed to alloc single indirect block...\n");
        ino_free(fs, inode_num);
        return 0;
    }

    uint8_t zero_buf[fs->dd->block_size];
    memset(zero_buf, 0, fs->dd->block_size);
    if (dwrite(fs->dd, zero_buf, indirect_blk_num) != OK) {
        fprintf(stderr, "dir_create_root error: failed to initialize indirect block...\n");
        bl_free(fs, indirect_blk_num);
        ino_free(fs, inode_num);
        return 0;
    }

    dir_ino.inode_number = inode_num;
    dir_ino.file_type = FTypeDirectory;
    dir_ino.single_indirect = indirect_blk_num;

    // Add "." entry pointing to itself
    ret = dir_add(fs, &dir_ino, cur_dir_name, inode_num);
    if (ret != OK) {
        fprintf(stderr, "dir_create_root error: failed to add current dir entry...\n");
        bl_free(fs, indirect_blk_num);
        ino_free(fs, inode_num);
        return 0;
    }

    // Add ".." entry also pointing to itself (root's parent is itself)
    ret = dir_add(fs, &dir_ino, parent_dir_name, inode_num);
    if (ret != OK) {
        fprintf(stderr, "dir_create_root error: failed to add parent dir entry...\n");
        ino_free_all_blocks(fs, &dir_ino);
        bl_free(fs, indirect_blk_num);
        ino_free(fs, inode_num);
        return 0;
    }

    // 8. Write inode to disk
    ret = ino_write(fs, inode_num, &dir_ino);
    if (ret != OK) {
        fprintf(stderr, "dir_create_root error: failed to write dir_inode to disk...\n");
        ino_free_all_blocks(fs, &dir_ino);
        bl_free(fs, indirect_blk_num);
        ino_free(fs, inode_num);
        return 0;
    }

    return inode_num;
}

uint32_t dir_create(filesystem *fs, uint32_t parent_ino) {
    if (!fs) {
        fprintf(stderr, "dir_create error: fs pointer is null...\n");
        return 0; // 0 is a bad inode number
    }

    // fs is not null
    if (parent_ino < 1 || parent_ino > fs->inodes) {
        fprintf(stderr, "dir_create error: parent_ino [%d] is not valid...\n",
                parent_ino);
        return 0;
    }

    uint32_t inode_num, indirect_blk_num;
    inode dir_ino;
    const uint8_t *parent_dir_name = (uint8_t*)"..";
    const uint8_t *cur_dir_name = (uint8_t*)".";
    uint32_t block_size = fs->dd->block_size;
    uint8_t block_buf[block_size];
    RC ret;

    ret = ino_init(&dir_ino);
    if (ret != OK) {
        fprintf(stderr, "dir_create error: failed to init an inode...\n");
        return 0;
    }

    inode_num = ino_alloc(fs);
    if (inode_num == 0) {
        fprintf(stderr, "dir_create error: failed to alloc inode number...\n");
        return 0;
    }

    indirect_blk_num = bl_alloc(fs);
    if (indirect_blk_num == 0) {
        fprintf(stderr, "dir_create error: failed to alloc single indirect block...\n");
        ino_free(fs, inode_num);
        return 0;
    }
    memset(block_buf, 0, block_size);
    ret = dwrite(fs->dd, block_buf, indirect_blk_num);
    if (ret != OK) {
        fprintf(stderr, "dir_create error: failed to init an indirect block...\n");
        bl_free(fs, indirect_blk_num);
        ino_free(fs, inode_num);
        return 0;
    }

    dir_ino.inode_number = inode_num;
    dir_ino.file_type = FTypeDirectory;
    dir_ino.single_indirect = indirect_blk_num;

    // Add ".." => inum
    ret = dir_add(fs, &dir_ino, parent_dir_name, parent_ino);
    if (ret != OK) {
        fprintf(stderr, "dir_create error: failed to add parent dir entry...\n"
                "parent_dir_name: %s\n"
                "parent_ino_num: %d\n",
                parent_dir_name, parent_ino);
        ino_free_all_blocks(fs, &dir_ino);
        bl_free(fs, indirect_blk_num);
        ino_free(fs, inode_num);
        return 0;
    }
    // Add "." => inum
    ret = dir_add(fs, &dir_ino, cur_dir_name, inode_num);
    if (ret != OK) {
        fprintf(stderr, "dir_create error: failed to add current dir entry...\n"
                "cur_dir_name: %s\n"
                "cur_ino_num: %d\n",
                cur_dir_name, inode_num);
        ino_free_all_blocks(fs, &dir_ino);
        // bl_free(fs, indirect_blk_num);
        ino_free(fs, inode_num);
        return 0;
    }

    // Update inode to disk
    ret = ino_write(fs, inode_num, &dir_ino);
    if (ret != OK) {
        fprintf(stderr, "dir_create error: failed to write dir_inode to disk...\n"
                "inode_num: %d\n", inode_num);
        ino_free_all_blocks(fs, &dir_ino);
        // bl_free(fs, indirect_blk_num);
        ino_free(fs, inode_num);
        return 0;
    }

    return inode_num;
}

RC dir_delete_empty(filesystem *fs, inode *dir_ino) {
    if (!fs || !dir_ino) {
        fprintf(stderr, "dir_delete_empty error: wrong args...\n");
        return ErrArg;
    }

    if (dir_ino->file_type != FTypeDirectory) {
        fprintf(stderr, "dir_delete_empty error: not a directory\n");
        return ErrArg;
    }

    if (!dir_is_empty(fs, dir_ino)) {
        fprintf(stderr, "dir_delete_empty error: directory is not empty\n");
        return ErrInode;
    }

    RC ret;
    ret = ino_free_all_blocks(fs, dir_ino);
    if (ret != OK) {
        fprintf(stderr, "dir_delete error: failed to free inode blocks %d\n",
                dir_ino->inode_number);
        return ErrInode;
    }

    ret = ino_free(fs, dir_ino->inode_number);
    if (ret != OK) {
        fprintf(stderr, "dir_delete error: failed to free inode number %d\n",
                dir_ino->inode_number);
        return ErrInode;
    }

    return ret;
}

void dir_show(filesystem *fs, inode *dir_ino) {
    if (!fs || !dir_ino) {
        fprintf(stderr, "dir_show error: wrong args...\n");
        return;
    }

    if (dir_ino->file_type != FTypeDirectory) {
        fprintf(stderr, "dir_show error: not a directory\n");
        return;
    }

    printf("========================================\n");
    printf("Directory Information\n");
    printf("========================================\n");

    // Show basic inode information
    printf("Inode number:       %u\n", dir_ino->inode_number);
    printf("File type:          Directory\n");
    printf("Directory size:     %u bytes", dir_ino->file_size);

    // Show human-readable size
    if (dir_ino->file_size >= 1024*1024) {
        printf(" (%.2f MB)", (double)dir_ino->file_size / (1024*1024));
    } else if (dir_ino->file_size >= 1024) {
        printf(" (%.2f KB)", (double)dir_ino->file_size / 1024);
    }
    printf("\n");

    // Count entries
    uint32_t block_size = fs->dd->block_size;
    uint8_t block_buf[block_size];
    uint32_t dirent_per_block = get_dirent_per_block(fs);
    uint32_t max_offset = ino_get_max_block_offset(fs);
    uint32_t entry_count = 0;
    uint32_t block_count = 0;

    // Count allocated blocks and entries
    for (uint32_t offset = 0; offset < max_offset; offset++) {
        uint32_t block_number = ino_get_block_at(fs, dir_ino, offset);

        if (block_number == 0) {
            continue;
        }

        block_count++;

        if (dread(fs->dd, block_buf, block_number) != OK) {
            fprintf(stderr, "dir_show warning: failed to read block %u\n", block_number);
            continue;
        }

        dirent *dirent_list = (dirent *)block_buf;
        for (uint32_t j = 0; j < dirent_per_block; j++) {
            if (dirent_list[j].inode_num != 0) {
                entry_count++;
            }
        }
    }

    printf("Total entries:      %u\n", entry_count);
    printf("Blocks used:        %u\n", block_count);

    // Show storage efficiency
    uint32_t dirent_size = sizeof(struct s_dirent);
    uint32_t expected_size = entry_count * dirent_size;
    if (dir_ino->file_size > 0) {
        printf("Storage efficiency: %.1f%% (%u / %u bytes)\n",
               (double)expected_size / dir_ino->file_size * 100,
               expected_size, dir_ino->file_size);
    }

    printf("\n");
    printf("Directory Entries:\n");
    printf("----------------------------------------\n");
    printf("%-4s %-10s %-28s\n", "No.", "Inode", "Name");
    printf("----------------------------------------\n");

    uint32_t entry_idx = 0;

    for (uint32_t offset = 0; offset < max_offset; offset++) {
        uint32_t block_number = ino_get_block_at(fs, dir_ino, offset);

        if (block_number == 0) {
            continue;
        }

        if (dread(fs->dd, block_buf, block_number) != OK) {
            continue;
        }

        dirent *dirent_list = (dirent *)block_buf;
        for (uint32_t j = 0; j < dirent_per_block; j++) {
            if (dirent_list[j].inode_num != 0) {
                entry_idx++;
                printf("%-4u %-10u %-28s",
                       entry_idx,
                       dirent_list[j].inode_num,
                       (char*)dirent_list[j].name);

                // Mark special entries
                if (strcmp((char*)dirent_list[j].name, ".") == 0) {
                    printf(" (current dir)");
                } else if (strcmp((char*)dirent_list[j].name, "..") == 0) {
                    printf(" (parent dir)");
                }
                printf("\n");
            }
        }
    }

    if (entry_count == 0) {
        printf("(empty directory)\n");
    }

    printf("========================================\n");
}
