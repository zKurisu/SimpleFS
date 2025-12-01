/*
 * file.c
 * 
 * Copyright (C) Jie
 * 2025-11-29
 *
 */

#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// How to handle with global file table?
file_handle *file_open(filesystem *fs, uint32_t inode_num, uint32_t flags) {
    if (!fs || inode_num < 1 || inode_num > fs->inodes || file_check_flags(flags) != OK) {
        fprintf(stderr, "file_open error: error args...\n");
        return (file_handle*)0;
    }

    file_handle *fh;
    uint32_t size;

    size = sizeof(struct s_file_handle);
    fh = (file_handle *)malloc(size);
    if (!fh) {
        fprintf(stderr, "file_open error: failed to allocate memory for file handle\n");
        return (file_handle*)0;
    }

    memset(fh, 0, size);
    fh->fs = fs;
    fh->inode_number = inode_num;
    fh->flags = flags;
    fh->refcount = 1;

    if (ino_read(fs, inode_num, &fh->cached_inode) != OK) {
        fprintf(stderr, "file_open error: could not read inode %d...\n",
                inode_num);
        free(fh);
        return (file_handle*)0;
    }
    fh->cache_valid = 1;

    if (flags & MY_O_APPEND) {
        fh->offset = fh->cached_inode.file_size;
    }

    if (pthread_rwlock_init(&fh->rwlock, NULL) != 0) {
        fprintf(stderr, "file_open error: could not init rwlock...\n");
        free(fh);
        return (file_handle*)0;
    }

    return fh;
}

// How to handle with global file table?
RC file_close(file_handle *fh) {
    fh->refcount--;
    if (fh->refcount == 0) {
        free(fh);
    }

    return OK;
}

uint32_t file_read(file_handle *fh, uint8_t *buf, uint32_t size) {
    if (!fh || !buf) {
        fprintf(stderr, "file_read error: wrong args\n");
        return 0;
    }

    // Check flags, whether can read this file
    uint32_t accmode = fh->flags & MY_ACCMODE;
    if (accmode != MY_O_RDONLY && accmode != MY_O_RDWR) {
        fprintf(stderr, "file_read error: file not opened for reading\n");
        return 0;
    }

    if (fh->offset >= fh->cached_inode.file_size) {
        return 0;  // EOF
    }

    uint32_t remaining = fh->cached_inode.file_size - fh->offset;
    if (size > remaining) {
        size = remaining;
    }

    if (fh->cache_valid == 0) {
        pthread_rwlock_wrlock(&fh->rwlock);
        if (ino_read(fh->fs, fh->inode_number, &fh->cached_inode) != OK) {
            fprintf(stderr, "file_read error: could not read inode %d...\n",
                    fh->inode_number);
            return 0;
        }
        fh->cache_valid = 1;
        pthread_rwlock_unlock(&fh->rwlock);
    }

    pthread_rwlock_rdlock(&fh->rwlock);
    uint32_t block_size = fh->fs->dd->block_size;

    uint32_t start_block_idx = fh->offset / block_size;
    uint32_t block_offset = fh->offset % block_size;
    uint32_t bytes_read = 0;

    uint8_t block_buf[block_size];
    uint32_t cur_block_idx = start_block_idx;
    while (bytes_read > size) {
        uint32_t physical_block = ino_get_block_at(fh->fs, &fh->cached_inode, cur_block_idx);

        uint32_t copy_size = block_size - block_offset;
        if (copy_size > size - bytes_read) {
            copy_size = size - bytes_read;
        }

        if (physical_block == 0) { // Hole
            memset(buf+bytes_read, 0, copy_size);
        } else {
            if (dread(fh->fs->dd, block_buf, physical_block) != OK) {
                fprintf(stderr, "file_read error: failed to read block [%d]\n",
                    physical_block);
                return bytes_read;
            }

            memcpy(buf+bytes_read, block_buf+block_offset, copy_size);
        }
        bytes_read += copy_size;
        block_offset = 0;
        cur_block_idx++;
    }
    pthread_rwlock_unlock(&fh->rwlock);

    pthread_rwlock_wrlock(&fh->rwlock);
    fh->offset += bytes_read;
    pthread_rwlock_unlock(&fh->rwlock);

    return bytes_read;
}

uint32_t file_write(file_handle *fh, uint8_t *buf, uint32_t size) {
    if (!fh || !buf) {
        fprintf(stderr, "file_write error: wrong args\n");
        return 0;
    }

    uint32_t accmode = fh->flags & MY_ACCMODE;
    if ((accmode != MY_O_WRONLY) || (accmode != MY_O_RDWR)) {
        fprintf(stderr, "file_write error: error file handle flags %x\n",
            fh->flags);
        return 0;
    }

    pthread_rwlock_wrlock(&fh->rwlock);

    if (fh->cache_valid == 0) {
        if (ino_read(fh->fs, fh->inode_number, &fh->cached_inode) != OK) {
            fprintf(stderr, "file_read error: could not read inode %d...\n",
                    fh->inode_number);
            return 0;
        }
        fh->cache_valid = 1;
    }

    if (fh->flags & MY_O_APPEND) {
        fh->offset = fh->cached_inode.file_size;
    }

    // Check whether over maximum file size
    uint32_t max_file_size = ino_get_max_filesize(fh->fs);
    if (fh->offset + size > max_file_size) {
        fprintf(stderr, "file_write error: offset [%d] + size [%d] over maximum file size [%d]\n",
                fh->offset, size, max_file_size);
        return 0;
    }

    uint32_t block_size = fh->fs->dd->block_size;
    uint32_t start_block_idx = fh->offset / block_size;
    uint32_t block_offset = fh->offset % block_size;
    uint8_t  block_buf[block_size];
    memset(block_buf, 0, block_size);

    uint32_t bytes_write = 0;
    uint32_t cur_block_idx = start_block_idx;
    while (bytes_write < size) {
        uint32_t physical_block = ino_get_block_at(fh->fs, &fh->cached_inode, cur_block_idx);

        if (physical_block == 0) {
            physical_block = ino_alloc_block_at(fh->fs, &fh->cached_inode, cur_block_idx);

            if (physical_block == 0) {
                fprintf(stderr, "file_write error: failed to allocate block\n");
                break;
            }
            memset(block_buf, 0, block_size);
        } else {
            if (block_offset != 0 || size - bytes_write < block_size) {
                if (dread(fh->fs->dd, block_buf, physical_block) != OK) {
                    fprintf(stderr, "file_write error: failed to read block [%d]\n",
                        physical_block);
                    break;
                }
            }
        }

        uint32_t write_size = block_size - block_offset;
        if (write_size > size - bytes_write) {
            write_size = size - bytes_write;
        }

        memcpy(block_buf+block_offset, buf+bytes_write, write_size);

        if (dwrite(fh->fs->dd, block_buf, physical_block) != OK) {
            fprintf(stderr, "file_write error: failed to write block\n");
            break;
        }

        bytes_write += write_size;
        block_offset = 0;
        cur_block_idx++;
    }

    fh->offset += bytes_write;
    if (fh->offset > fh->cached_inode.file_size) {
        fh->cached_inode.file_size = fh->offset;
        fh->cache_valid = 1;

        ino_write(fh->fs, fh->inode_number, &fh->cached_inode);
    }

    pthread_rwlock_unlock(&fh->rwlock);
    return bytes_write; // TMP RETURN
}

RC file_seek(file_handle *fh, uint32_t offset, uint8_t whence) {
    if (!fh ||
        offset > ino_get_max_filesize(fh->fs) ||
        file_check_whence(whence) != OK) {
        fprintf(stderr, "file_seek error: wrong args\n");
        return ErrArg;
    }

    pthread_rwlock_wrlock(&fh->rwlock);
    switch (whence) {
    case MY_SEEK_SET:
        fh->offset = offset;
        break;
    case MY_SEEK_CUR:
        fh->offset += offset;
        break;
    case MY_SEEK_END:
        if (fh->cache_valid == 0) {
            if (ino_read(fh->fs, fh->inode_number, &fh->cached_inode) != OK) {
                fprintf(stderr, "file_seek error: failed to read inode [%d]\n",
                    fh->inode_number);
                pthread_rwlock_unlock(&fh->rwlock);
                return ErrInode;
            }
            fh->cache_valid = 1;
        }
        fh->offset = fh->cached_inode.file_size + offset;
        break;
    }
    pthread_rwlock_unlock(&fh->rwlock);
    return OK;
}

uint32_t file_tell(file_handle *fh) {
    if (!fh) {
        fprintf(stderr, "file_tell error: wrong args\n");
        return 0;
    }

    pthread_rwlock_rdlock(&fh->rwlock);
    uint32_t offset = fh->offset;
    pthread_rwlock_unlock(&fh->rwlock);

    return offset;
}

uint32_t file_size(file_handle *fh) {
    if (!fh) {
        fprintf(stderr, "file_size error: wrong args\n");
        return 0;
    }

    pthread_rwlock_rdlock(&fh->rwlock);

    if (fh->cache_valid == 0) {
        pthread_rwlock_unlock(&fh->rwlock);
        pthread_rwlock_wrlock(&fh->rwlock);

        if (fh->cache_valid == 0) {
            if (ino_read(fh->fs, fh->inode_number, &fh->cached_inode) != OK) {
                fprintf(stderr, "file_size error: failed to read inode [%d]\n",
                    fh->inode_number);
                pthread_rwlock_unlock(&fh->rwlock);
                return 0;
            }
            fh->cache_valid = 1;
        }
        pthread_rwlock_unlock(&fh->rwlock);
        pthread_rwlock_rdlock(&fh->rwlock);
    }

    uint32_t size = fh->cached_inode.file_size;
    pthread_rwlock_unlock(&fh->rwlock);

    return size;
}

RC file_check_flags(uint32_t flags) {
    if (flags & (~MY_ALL_FLAGS)) {
        fprintf(stderr, "file_check_flags error: invalid flags %x\n", flags);
        return ErrFileFlags;
    }

    uint8_t access_mode = flags & MY_ACCMODE;
    if (access_mode != MY_O_RDONLY |
        access_mode != MY_O_WRONLY |
        access_mode != MY_O_RDWR) {
        fprintf(stderr, "file_check_flags error: invalid access mode flag %x\n", access_mode);
        return ErrFileFlags;
    }
    
    if (access_mode == MY_O_RDONLY &&
        (flags & (MY_O_APPEND | MY_O_TRUNC))) {
        fprintf(stderr, "file_check_flags error: invalid flag, RDONLY conflicts with TRUNC/APPEND\n");
        return ErrFileFlags;
    }

    if ((flags & MY_O_APPEND) && (flags & MY_O_TRUNC)) {
        fprintf(stderr, "file_check_flags error: invalid flags, APPEND and TRUNC are mutually exclusive\n");
        return ErrFileFlags;
    }

    return OK;
}

RC file_check_whence(uint8_t whence) {
    if (whence < MY_SEEK_SET || whence > MY_SEEK_END)
        return ErrWhence;
    return OK;
}
