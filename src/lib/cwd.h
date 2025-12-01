/*
 * cwd.h
 * Current working directory related
 * Copyright (C) Jie
 * 2025-12-01
 *
 */

#ifndef MY_CWD_H_
#define MY_CWD_H_

#include "fs.h"
#include "path.h"
#include "inode.h"

#include <stdint.h>
#include <pthread.h>

struct s_cwd_context {
    filesystem *fs;

    uint32_t cwd_inode_num;
    inode cached_cwd_inode;
    uint8_t cache_valid;

    path p;
    pthread_rwlock_t rwlock;
};
typedef struct s_cwd_context cwd_context_t;

extern cwd_context_t g_cwd;

/*
 * Init a global cwd structure
 * */
RC cwd_init(filesystem *fs, uint32_t root_inode_num);

/*
 * Get current cwd inode number
 * */
uint32_t cwd_get_inode();

/*
 * Get current cwd path
 * */
void cwd_get_path(path *p);

/*
 * Change directory throught new inode number
 * */
RC cwd_chdir_inode(uint32_t new_inode_num);

/*
 * Change directory throught path string
 * */
RC cwd_chdir_path(const char *path_str);

/*
 * Display current cwd
 * */
void cwd_show();

#endif
