/*
 * file.h
 * Global file table and file handle structure
 * Copyright (C) Jie
 * 2025-11-29
 *
 */

#ifndef MY_FILE_H_
#define MY_FILE_H_

#include "inode.h"
#include "fs.h"

#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MY_ACCMODE  0x03 // 0000 0011

#define MY_O_RDONLY 0x00 // 0000 0000
#define MY_O_WRONLY 0x01 // 0000 0001
#define MY_O_RDWR   0x02 // 0000 0010
#define MY_O_CREATE 0x04 // 0000 0100
#define MY_O_APPEND 0x08 // 0000 1000
#define MY_O_TRUNC  0x010// 0001 0000
#define MY_ALL_FLAGS (\
        MY_O_RDONLY | \
        MY_O_WRONLY | \
        MY_O_RDWR   | \
        MY_O_CREATE | \
        MY_O_APPEND | \
        MY_O_TRUNC \
)

#define MY_SEEK_SET 0 // begin
#define MY_SEEK_CUR 1 // current offset
#define MY_SEEK_END 2 // end

#define MAX_OPEN_FILES 1024


struct s_file_handle {
    filesystem *fs;

    uint32_t inode_number;
    inode cached_inode;
    uint8_t cache_valid;

    uint32_t refcount;
    uint32_t offset;
    uint32_t flags;

    pthread_rwlock_t rwlock;
};
typedef struct s_file_handle file_handle;

struct s_global_file_table {
    file_handle *handles[MAX_OPEN_FILES];
    pthread_mutex_t lock;
    uint32_t count;
};
typedef struct s_global_file_table global_file_table;

extern global_file_table g_file_table;

/*
 * Init global file table
 * */
void file_table_init();

/*
 * Display global file table content
 * */
void file_table_show();

/*
 * Return current opened file count
 * */
uint32_t file_table_count();

/*
 * Wrap inode to file handle
 * */

file_handle *file_open(filesystem *fs, const char *path_str, uint32_t flags);

/*
 * Close a file handle, free resources
 * */
RC file_close(file_handle *fh);

/*
 * Read file content
 * */
uint32_t file_read(file_handle *fh, uint8_t *buf, uint32_t size);

/*
 * Write content to disk
 * */
uint32_t file_write(file_handle *fh, uint8_t *buf, uint32_t size);

/*
 * Set file offset
 * */
RC file_seek(file_handle *fh, uint32_t offset, uint8_t whence);

/*
 * Get current offset
 * */
uint32_t file_tell(file_handle *fh);

/*
 * Return file size
 * */
uint32_t file_size(file_handle *fh);

/*
 * Display file content
 * */
void file_show(file_handle *fh);

RC file_check_flags(uint32_t flags);

RC file_check_whence(uint8_t whence);

#ifdef __cplusplus
}
#endif

#endif
