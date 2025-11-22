/*
 * directory.h
 * Functions for inode with type FTypeDirectory
 * Copyright (C) Jie
 * 2025-11-19
 *
 */

#ifndef MY_DIRECTORY_H_
#define MY_DIRECTORY_H_

#include "fs.h"
#include "inode.h"
#include "error.h"

/*
 * Find the inode number for specifile filename through a directory inode
 */
uint32_t dir_lookup(filesystem *fs, inode *dir_ino, const uint8_t *name);

/*
 * Add a new entry to a directory inode
 */
RC dir_add(filesystem *fs, inode *dir_ino, const uint8_t *name, uint32_t inode_num);

/*
 * Remove an entry from a directory inode
 */
RC dir_remove(filesystem *fs, inode *dir_ino, const uint8_t *name);

/*
 * List all entry from a directory inode
 */
RC dir_list(filesystem *fs, inode *dir_ino);

/*
 * Check a directory inode, whether it has entries
 */
RC dir_is_empty(filesystem *fs, inode *dir_ino);

// dir_create
//
// dir_delete
//
//
int dir_valid_name(const uint8_t *name);

// 显示目录内容
void dir_show(filesystem *fs, inode *dir_ino);

#endif
