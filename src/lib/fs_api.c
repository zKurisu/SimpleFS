/*
 * fs_api.c
 * 
 * Copyright (C) Jie
 * 2025-11-30
 *
 */

#include "fs_api.h"
#include "path.h"
#include "cwd.h"
#include "directory.h"
#include "block.h"

#include <stdio.h>
#include <string.h>

RC fs_touch(filesystem *fs, const char *path_str) {
    if (!fs || !path_str) {
        fprintf(stderr, "fs_touch error: wrong args...\n");
        return ErrArg;
    }

    
    if (path_str[strlen(path_str)-1] == '/') { // touch only allowed to create file
        fprintf(stderr, "fs_touch error: path_str should be a file path, not directory\n");
        return ErrArg;
    }

    path p;
    uint32_t size = sizeof(struct s_path);
    memset(&p, 0, size);

    if (path_parse(path_str, &p) != OK) {
        fprintf(stderr, "fs_touch error: failed to parse path_str [%s] to path structure...\n",
                path_str);
        return ErrName;
    }

    inode ino;
    uint32_t inode_num;
    if (p.is_absolute) {
        inode_num = 1; // Hard encode, root dir inode num
    } else { // relative path
        pthread_rwlock_rdlock(&g_cwd.rwlock);
        inode_num = g_cwd.cwd_inode_num;
        pthread_rwlock_unlock(&g_cwd.rwlock);
    }

    if (ino_read(fs, inode_num, &ino) != OK) {
        fprintf(stderr, "fs_touch error: failed to read base inode [%d]\n",
                inode_num);
    }
    for (uint32_t i=0; i<p.count; i++) {
        if (i != (p.count - 1)) {// path directory check
            if ((inode_num = dir_lookup(fs, &ino, (uint8_t*)p.components[i])) == 0) { 
                fprintf(stderr, "fs_touch error: directory not exists [%s] at level [%d]\n",
                        p.components[i], i);
                return ErrPath;
            }

            if (ino_read(fs, inode_num, &ino) != OK) {
                fprintf(stderr, "fs_touch error: failed to read dir inode [%d]\n",
                        inode_num);
            }
        } else { // last component, file name
            // Create new inode
            inode new_ino;
            ino_init(&new_ino);
            new_ino.file_type = FTypeFile;
            if ((new_ino.inode_number = ino_alloc(fs)) == 0) {
                fprintf(stderr, "fs_touch error: failed to alloc new inode number\n");
                return ErrInternal;
            }
            if ((new_ino.single_indirect = bl_alloc(fs)) == 0) {
                fprintf(stderr, "fs_touch error: failed to alloc new block number\n");
                return ErrInternal;
            }
            ino_write(fs, new_ino.inode_number, &new_ino);
            
            if (ino_read(fs, inode_num, &ino) != OK) {
                fprintf(stderr, "fs_touch error: failed to read parent inode [%d]\n",
                        inode_num);
                return ErrInode;
            }
            dir_add(fs, &ino, (uint8_t*)p.components[i], new_ino.inode_number);
            ino_write(fs, inode_num, &ino);
        }
    }

    return OK;
}
