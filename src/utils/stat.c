/*
 * stat.c
 *
 * Usage:
 *  ./build/my_stat [disk_id] [block_size] [path]
 *
 * Example:
 *  ./build/my_stat 0 4096 "/"
 *
 * Copyright (C) Jie
 * 2025-12-03
 *
 */
#include "disk.h"
#include "fs.h"
#include "fs_api.h"
#include "inode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_HELP_MSG 4096
#define MAX_TYPE_STR 100

int main(int argn, char *argv[]) {
    char buf[MAX_HELP_MSG];
    sprintf(buf, "stat: wrong args count [%d]\n\n"
        "Usage:\n"
        "  ./build/my_stat [disk_id] [block_size] [path]\n\n"
        "Example:\n"
        "  ./build/my_stat 0 4096 \"/home/jie\"\n"
        "  This will show file/directory info\n", argn);
    
    int32_t block_size;
    int32_t disk_id   = atoi(argv[1]);
    if (argn != 4 || // file name, block number, block size, path
        disk_id > 9 || disk_id < 0 ||
        (block_size = atoi(argv[2])) == 0) { // Error
        fprintf(stderr, "%s", buf);
        exit(0);
    }

    char *path = argv[3];
    disk *dd;
    filesystem *fs;
    uint32_t size;

    size = sizeof(disk);
    dd = (disk*)malloc(size);
    if (dd == NULL) {
        fprintf(stderr, "stat: [error] no enough memory for disk allocation\n");
        exit(0);
    }
    memset(dd, 0, size);

    size = sizeof(filesystem);
    fs = (filesystem*)malloc(size);
    if (fs == NULL) {
        fprintf(stderr, "stat: [error] no enough memory for filesystem allocation\n");
        free(dd);
        exit(0);
    }
    memset(fs, 0, size);

    if (dattach(dd, block_size, disk_id) != OK) {
        fprintf(stderr, "stat: [error] failed to attach dd to disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }

    if (fs_mount(dd, fs) != OK) {
        fprintf(stderr, "stat: [error] failed to mount fs to disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }

    f_stat stat;
    memset(&stat, 0, sizeof(f_stat));

    if (fs_stat(fs, path, &stat) != OK) {
        fprintf(stderr, "stat: [error] failed to create dir [%s]\n", path);
        free(dd);
        free(fs);
        exit(0);
    }
    printf("stat: success to run [fs_stat]\n");

    char file_type[MAX_TYPE_STR];
    if (stat.type == FTypeFile) {
        strcpy(file_type, "file");
    } else if (stat.type == FTypeDirectory) {
        strcpy(file_type, "directory");
    }

    printf("stat info:\n");
    printf("  path   : %s\n"
           "  type   : %s\n"
           "  size   : %d\n"
           "  blocks : %d\n",
           path, file_type, stat.size, stat.blocks);

    if (fs_unmount(fs) != OK) {
        fprintf(stderr, "stat: [error] failed to unmount fs\n");
        free(dd);
        free(fs);
        exit(0);
    }

    if (ddetach(dd) != OK) {
        fprintf(stderr, "stat: [error] failed to ddetach dd from disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }
    free(dd);
    free(fs);

    return 0;
}
