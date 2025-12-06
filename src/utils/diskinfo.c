/*
 * diskinfo.c
 *
 * Usage:
 *  ./build/my_diskinfo [disk_id] [block_size]
 *
 * Example:
 *  ./build/my_diskinfo 0 4096
 *
 * Copyright (C) Jie
 * 2025-12-03
 *
 */
#include "disk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_HELP_MSG 4096

int main(int argn, char *argv[]) {
    char buf[MAX_HELP_MSG];
    sprintf(buf, "diskinfo: wrong args count [%d]\n\n"
        "Usage:\n"
        "  ./build/my_diskinfo [disk_id] [block_size]\n\n"
        "Example:\n"
        "  ./build/my_diskinfo 0 4096\n"
        "  This will display disk infomation\n", argn);
    
    int32_t block_size;
    int32_t disk_id   = atoi(argv[1]);
    if (argn != 3 || // file name, disk id, block size
        disk_id > 9 || disk_id < 0 ||
        (block_size = atoi(argv[2])) == 0) { // Error
        fprintf(stderr, "%s", buf);
        exit(0);
    }

    disk *dd;
    uint32_t size;

    size = sizeof(disk);
    dd = (disk*)malloc(size);
    if (dd == NULL) {
        fprintf(stderr, "diskinfo: [error] no enough memory for disk allocation\n");
        exit(0);
    }
    memset(dd, 0, size);

    if (dattach(dd, block_size, disk_id) != OK) {
        fprintf(stderr, "diskinfo: [error] failed to attach dd to disk with id [%d]\n", disk_id);
        free(dd);
        exit(0);
    }


    printf("diskinfo: success to run fs_diskinfo\n");
    dshow(dd);

    if (ddetach(dd) != OK) {
        fprintf(stderr, "diskinfo: [error] failed to ddetach dd from disk with id [%d]\n", disk_id);
        free(dd);
        exit(0);
    }
    free(dd);

    return 0;
}
