/*
 * inode.c
 * 
 * Copyright (C) Jie
 * 2025-11-17
 *
 */
#include "inode.h"
#include "error.h"

#include <stdio.h>
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
