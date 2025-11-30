/*
 * fs_api.h
 * function
 * Copyright (C) Jie
 * 2025-11-30
 *
 */

#ifndef MY_FS_API_H_
#define MY_FS_API_H_

#include "error.h"
#include "inode.h"
#include "fs.h"

#include <stdint.h>

typedef struct {
    uint32_t inode_num;
    filetype type;
    uint32_t size;
    uint32_t blocks;
} f_stat;

/*
 * Like shell command 'touch', create a file
 * */
RC fs_touch(filesystem *fs, const uint8_t *path);

/*
 * Like shell command 'rm', decrease the link count by 1, remove the file when link count is 0
 * */
RC fs_unlink(filesystem *fs, const uint8_t *path);

/*
 * Like shell command 'mkdir', create a directory, could recursively create
 * */
RC fs_mkdir(filesystem *fs, const uint8_t *path);

/*
 * Like shell command 'rm -r', remove a directory 
 * */
RC fs_rmdir(filesystem *fs, const uint8_t *path);

/*
 * Like shell command 'ls', list all entries under a directory
 * */
RC fs_ls(filesystem *fs, const uint8_t *path);

/*
 * Like shell command 'cat', dump all contents of a file
 * */
RC fs_cat(filesystem *fs, const uint8_t *path);

/*
 * Check whether a file is exists
 * */
RC fs_exists(filesystem *fs, const uint8_t *path);

/*
 * Like shell command 'stat', get file metadata
 * */
RC fs_stat(filesystem *fs, const uint8_t *path, f_stat *st);

#endif
