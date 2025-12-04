/*
 * format.c
 *
 * Usage:
 *  ./build/my_format [disk_id] [block_size]
 *
 * Example:
 *  ./build/my_format 0 4096
 *
 * Copyright (C) Jie
 * 2025-12-03
 *
 */
#include "disk.h"
#include "fs.h"
#include "directory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_HELP_MSG 4096

int main(int argn, char *argv[]) {
    char buf[MAX_HELP_MSG];
    sprintf(buf, "format: wrong args count [%d]\n\n"
        "Usage:\n"
        "  ./build/my_format [disk_id] [block_size]\n\n"
        "Example:\n"
        "  ./build/my_format 0 4096\n"
        "  This will create superblock, bitmap block and inode table\n", argn);
    
    int32_t block_size;
    int32_t disk_id   = atoi(argv[1]);
    if (argn != 3 || // file name, disk id, block size
        disk_id > 9 || disk_id < 0 ||
        (block_size = atoi(argv[2])) == 0) { // Error
        fprintf(stderr, "%s", buf);
        exit(0);
    }

    disk *dd;
    filesystem *fs;
    uint32_t size;

    size = sizeof(disk);
    dd = (disk*)malloc(size);
    if (dd == NULL) {
        fprintf(stderr, "format: [error] no enough memory for disk allocation\n");
        exit(0);
    }
    memset(dd, 0, size);

    size = sizeof(filesystem);
    fs = (filesystem*)malloc(size);
    if (fs == NULL) {
        fprintf(stderr, "format: [error] no enough memory for filesystem allocation\n");
        free(dd);
        exit(0);
    }
    memset(fs, 0, size);

    if (dattach(dd, block_size, disk_id) != OK) {
        fprintf(stderr, "format: [error] failed to attach dd to disk with id [%d]\n", disk_id);
        free(dd);
        exit(0);
    }

    if (fs_format(dd) != OK) {
        fprintf(stderr, "format: [error] failed to format disk with id [%d]\n", disk_id);
        free(dd);
        exit(0);
    }

    printf("format: success to run fs_format\n");
    printf("format: format disk%s.img with %s blocks of %s bytes\n",
        argv[1], argv[2], argv[3]);

    if (fs_mount(dd, fs) != OK) {
        fprintf(stderr, "format: [error] failed to mount fs to disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }

    uint32_t inode_number;
    if ((inode_number = dir_create_root(fs)) == 0) {
        fprintf(stderr, "format: [error] failed to init root directory\n");
        free(dd);
        free(fs);
        exit(0);
    }
    printf("format: success to init root directory with inode number [%d]\n", inode_number);

    if (fs_unmount(fs) != OK) {
        fprintf(stderr, "format: [error] failed to unmount fs\n");
        free(dd);
        free(fs);
        exit(0);
    }

    if (ddetach(dd) != OK) {
        fprintf(stderr, "format: [error] failed to ddetach dd from disk with id [%d]\n", disk_id);
        free(dd);
        exit(0);
    }

    return 0;
}
