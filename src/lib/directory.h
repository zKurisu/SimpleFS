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
 * Find dirent name by inode number
 */
RC dir_lookup_by_id(filesystem *fs, inode*dir_ino, uint8_t *buf, uint32_t inode_num);

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
uint8_t dir_is_empty(filesystem *fs, inode *dir_ino);

/*
 * Create the root directory inode, return inode number.
 * Root directory's . and .. both point to itself
 *
 * This should be called during filesystem formatting
 */
uint32_t dir_create_root(filesystem *fs);

/*
 * Create a directory inode, return inode number.
 *   parent_ino is used to create .. entry
 *
 *   This function will auto create . and .. entry
 */
uint32_t dir_create(filesystem *fs, uint32_t parent_ino);

/*
 * Delete a directory inode recursively
 * free all inodes, blocks
 * init inode, block area
 */
RC dir_delete_empty(filesystem *fs, inode *dir_ino);

/*
 * Display directory infomation
 */
void dir_show(filesystem *fs, inode *dir_ino);

#endif
