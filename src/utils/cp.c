/*
 * cp.c
 *
 * Usage:
 *  ./build/my_cp [disk_id] [block_size] [src_file_path] [dst_file_path]
 *
 * Example:
 *  ./build/my_cp 0 4096 "/hello.txt" "/new_hello.txt"
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
    sprintf(buf, "cp: wrong args count [%d]\n\n"
        "Usage:\n"
        "  ./build/my_cp [disk_id] [block_size] [src_file_path] [dst_file_path]\n\n"
        "Example:\n"
        "  ./build/my_cp 0 4096 \"/hello.txt\" \"/new_hello.txt\"\n"
        "  This will copy file to a new one\n", argn);
    
    int32_t block_size;
    int32_t disk_id   = atoi(argv[1]);
    if (argn != 5 || // file name, block number, block size, source file path, destination file path
        disk_id > 9 || disk_id < 0 ||
        (block_size = atoi(argv[2])) == 0) { // Error
        fprintf(stderr, "%s", buf);
        exit(0);
    }

    char *src_file_path = argv[3];
    char *dst_file_path = argv[4];
    disk *dd;
    filesystem *fs;
    uint32_t size;

    size = sizeof(disk);
    dd = (disk*)malloc(size);
    if (dd == NULL) {
        fprintf(stderr, "cp: [error] no enough memory for disk allocation\n");
        exit(0);
    }
    memset(dd, 0, size);

    size = sizeof(filesystem);
    fs = (filesystem*)malloc(size);
    if (fs == NULL) {
        fprintf(stderr, "cp: [error] no enough memory for filesystem allocation\n");
        free(dd);
        exit(0);
    }
    memset(fs, 0, size);

    if (dattach(dd, block_size, disk_id) != OK) {
        fprintf(stderr, "cp: [error] failed to attach dd to disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }

    if (fs_mount(dd, fs) != OK) {
        fprintf(stderr, "cp: [error] failed to mount fs to disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }

    if (fs_exists(fs, src_file_path) != OK) {
        fprintf(stderr, "cp: [error] source file does not exists\n");
        free(dd);
        free(fs);
        exit(0);
    }

    if (fs_cp(fs, src_file_path, dst_file_path) != OK) {
        fprintf(stderr, "cp: [error] failed to cp file [%s] to [%s]\n",
                src_file_path, dst_file_path);
        free(dd);
        free(fs);
        exit(0);
    }
    printf("cp: success to run [fs_cp]\n");
    printf("cp: success to cp file [%s] to [%s]\n",
            src_file_path, dst_file_path);

    if (fs_unmount(fs) != OK) {
        fprintf(stderr, "cp: [error] failed to unmount fs\n");
        free(dd);
        free(fs);
        exit(0);
    }

    if (ddetach(dd) != OK) {
        fprintf(stderr, "cp: [error] failed to ddetach dd from disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }

    return 0;
}
