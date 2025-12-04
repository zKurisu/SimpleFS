/*
 * write.c
 *
 * Usage:
 *  ./build/my_write [disk_id] [block_size] [file_path] [offset] [content]
 *
 * Example:
 *  ./build/my_write 0 4096 "/hello.txt" "hello world"
 *
 * Copyright (C) Jie
 * 2025-12-03
 *
 */
#include "disk.h"
#include "fs.h"
#include "fs_api.h"
#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_HELP_MSG  4096

int main(int argn, char *argv[]) {
    char buf[MAX_HELP_MSG];
    sprintf(buf, "write: wrong args count [%d]\n\n"
        "Usage:\n"
        "  ./build/my_write [disk_id] [block_size] [file_path] [offset] [content]\n\n"
        "Example:\n"
        "  ./build/my_write 0 4096 \"/hello.txt\" 0 \"hello world\"\n"
        "  This will write content to file, offset -1 means append\n", argn);
    
    int32_t block_size, offset;
    int32_t disk_id   = atoi(argv[1]);
    if (argn != 6 || // file name, block number, block size, file path, offset, content
        disk_id > 9 || disk_id < 0 ||
        (block_size = atoi(argv[2])) == 0||
        (offset = atoi(argv[4]))     == 0) { // Error
        fprintf(stderr, "%s", buf);
        exit(0);
    }

    char *file_path    = argv[3];
    char *file_content = argv[5];
    uint32_t bytes_written = 0;
    disk *dd;
    filesystem *fs;
    uint32_t size;

    size = sizeof(disk);
    dd = (disk*)malloc(size);
    if (dd == NULL) {
        fprintf(stderr, "write: [error] no enough memory for disk allowriteion\n");
        exit(0);
    }
    memset(dd, 0, size);

    size = sizeof(filesystem);
    fs = (filesystem*)malloc(size);
    if (fs == NULL) {
        fprintf(stderr, "write: [error] no enough memory for filesystem allowriteion\n");
        free(dd);
        exit(0);
    }
    memset(fs, 0, size);

    if (dattach(dd, block_size, disk_id) != OK) {
        fprintf(stderr, "write: [error] failed to attach dd to disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }

    if (fs_mount(dd, fs) != OK) {
        fprintf(stderr, "write: [error] failed to mount fs to disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }

    if (fs_exists(fs, file_path) != OK) {
        fprintf(stderr, "write: [error] file does not exists\n");
        free(dd);
        free(fs);
        exit(0);
    }

    file_handle *fh = file_open(fs, file_path, MY_O_WRONLY);
    if (!fh) {
        fprintf(stderr, "write: [error] can not get file handle\n");
        free(dd);
        free(fs);
        exit(0);
    }

    uint8_t seek_mode;
    switch (offset) {
        case -1:
            seek_mode = MY_SEEK_END;
            break;
        default:
            seek_mode = MY_SEEK_SET;
    }
    if (file_seek(fh, offset, seek_mode) != OK) {
        fprintf(stderr, "write: [error] failed to seek file [%s]\n", file_path);
        free(dd);
        free(fs);
        free(fh);
        exit(0);
    }

    if ((bytes_written = file_write(fh, (uint8_t*)file_content, strlen(file_content))) == 0) {
        fprintf(stderr, "write: [error] failed to write file [%s]\n", file_path);
        free(dd);
        free(fs);
        free(fh);
        exit(0);
    }
    printf("write: success to run [fs_write]\n");
    printf("write: success to write %d bytes to [%s]\n", bytes_written, file_path);

    if (file_close(fh) != OK) {
        fprintf(stderr, "write: [error] failed to close file\n");
        free(dd);
        free(fs);
        exit(0);
    }

    if (fs_unmount(fs) != OK) {
        fprintf(stderr, "write: [error] failed to unmount fs\n");
        free(dd);
        free(fs);
        exit(0);
    }

    if (ddetach(dd) != OK) {
        fprintf(stderr, "write: [error] failed to ddetach dd from disk with id [%d]\n", disk_id);
        free(dd);
        free(fs);
        exit(0);
    }

    return 0;
}
