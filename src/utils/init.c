/*
 * init.c
 *
 * Usage:
 *  ./build/my_init [disk_id] [block_count] [block_size]
 *
 * Example:
 *  ./build/my_init 0 100 4096
 *
 * Copyright (C) Jie
 * 2025-12-03
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_CMD_LEN 4096

int main(int argn, char *argv[]) {
    int32_t disk_id   = atoi(argv[1]);
    if (argn != 4 ||
        disk_id > 9 || disk_id < 0) { // file name, block number, block count, and block size
        fprintf(stderr, "init: wrong args count [%d]\n\n"
                "Usage:\n"
                "  ./build/my_init [disk_id] [block_count] [block_size]\n\n"
                "Example:\n"
                "  ./build/my_init 0 100 4096\n"
                "  This will create /tmp/disk0.img file, contain 100 blocks, block size is 4096\n", argn);
        exit(0);
    }

    char cmd_buf[MAX_CMD_LEN];
    sprintf(cmd_buf, "dd if=/dev/zero of=/tmp/disk%s.img count=%s bs=%s", argv[1], argv[2], argv[3]);
    if (system(cmd_buf) != 0) {
        fprintf(stderr, "init: error to run [%s]\n", cmd_buf);
    }

    printf("init: success to run [%s]\n", cmd_buf);
    printf("init: created disk%s.img with %s blocks of %s bytes\n",
        argv[1], argv[2], argv[3]);

    return 0;
}
