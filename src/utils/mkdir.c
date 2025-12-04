/*
 * mkdir.c
 *
 * Usage:
 *  ./build/my_mkdir [disk_id] [block_size] [dir_path]
 *
 * Example:
 *  ./build/my_mkdir 0 4096 "/"
 *
 * Copyright (C) Jie
 * 2025-12-03
 *
 */
#include "disk.h"
#include "fs.h"
#include "fs_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_HELP_MSG 4096

int main(int argn, char *argv[]) {
    char buf[MAX_HELP_MSG];
    sprintf(buf, "mkdir: wrong args count [%d]\n\n"
        "Usage:\n"
        "  ./build/my_mkdir [disk_id] [block_size] [dir_path]\n\n"
        "Example:\n"
        "  ./build/my_mkdir 0 4096 \"/home/jie\"\n"
        "  This will create directory recursively\n", argn);
    
    int32_t block_size;
    int32_t disk_id   = atoi(argv[1]);
    if (argn != 4 || // file name, block number, block size, dir path
        disk_id > 9 || disk_id < 0 ||
        (block_size = atoi(argv[2])) == 0) { // Error
        fprintf(stderr, "%s", buf);
        exit(0);
    }

    char *dir_path = argv[3];
    disk *dd;
    filesystem *fs;
    uint32_t size;

    size = sizeof(disk);
    dd = (disk*)malloc(size);
    if (dd == NULL) {
        fprintf(stderr, "mkdir: [error] no enough memory for disk allocation\n");
        exit(0);
    }
    memset(dd, 0, size);

    size = sizeof(filesystem);
    fs = (filesystem*)malloc(size);
    if (fs == NULL) {
        fprintf(stderr, "mkdir: [error] no enough memory for filesystem allocation\n");
        free(dd);
        exit(0);
    }
    memset(fs, 0, size);

    if (dattach(dd, block_size, disk_id) != OK) {
        fprintf(stderr, "mkdir: [error] failed to attach dd to disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }

    if (fs_mount(dd, fs) != OK) {
        fprintf(stderr, "mkdir: [error] failed to mount fs to disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }

    if (fs_mkdir(fs, dir_path) != OK) {
        fprintf(stderr, "mkdir: [error] failed to create dir [%s]\n", dir_path);
        free(dd);
        free(fs);
        exit(0);
    }
    printf("mkdir: success to run [fs_mkdir]\n");
    printf("mkdir: success to create dir [%s]\n", dir_path);

    if (fs_unmount(fs) != OK) {
        fprintf(stderr, "mkdir: [error] failed to unmount fs\n");
        free(dd);
        free(fs);
        exit(0);
    }

    if (ddetach(dd) != OK) {
        fprintf(stderr, "mkdir: [error] failed to ddetach dd from disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }

    return 0;
}
