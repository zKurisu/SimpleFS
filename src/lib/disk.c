/*
 * disk.c
 * Simple disk emulator
 * Copyright (C) Jie
 * 2025-11-14
 *
 */

#include "disk.h"
#include "error.h"
#include "helper.h"

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

RC dattach(disk *dd, uint32_t block_size, diskno disk_id) {
    uint16_t size;
    uint32_t tmp;
    struct stat statbuf;

    if (!dd || (disk_id >= MAX_DISKS) || block_size < MIN_BLOCK_SIZE)
        return ErrAttach;
    
    size = sizeof(struct s_disk);
    zero((uint8_t*)dd, size);

    dd->block_size = block_size;
    dd->id = disk_id;

    uint8_t *file = (uint8_t*)disk_paths[disk_id];
    tmp = open((const char*)file, O_RDWR);
    if ((int)tmp < 3) {
        fprintf(stderr, "Failed to open %s\n", file);
        return ErrAttach;
    }
    dd->fd = tmp; // Get low lever file descriptor

    tmp = fstat(dd->fd, &statbuf);
    if ((int)tmp < 0 || !statbuf.st_size) {
        close(dd->fd);
        fprintf(stderr, "Failed to get disk file stat\n");
        return ErrAttach;
    }

    // printf("statbuf.st_size: %lld\n", (long long)statbuf.st_size);
    dd->blocks = (uint32_t)(statbuf.st_size / block_size); // Calculate blocks

    return OK;    
}

RC ddetach(disk *dd) {
    if (!dd) {
        fprintf(stderr, "You should provide a [disk*]\n");
        return ErrDetach;
    }

    close(dd->fd);
    free(dd);

    return OK;
}

void dshow(disk *dd) {
    if (!dd) {
        fprintf(stderr, "You should provide a [disk*] for dshow\n");
        return;
    }

    printf(
        "Disk %d:\n"
        "  blocks %d\n"
        "  block size %d\n"
        "  total size %lld\n",
        dd->id, dd->blocks, dd->block_size, (long long)(dd->blocks*dd->block_size)
    );
    return;
}

RC dread(disk *dd, uint8_t *block, uint32_t blockno) {
    if (!dd || !block) {
        fprintf(stderr, "dread error, dd or block pointer is null...\n");
        return ErrArg;
    }

    // blockno start from 1
    if (blockno <= 0 || blockno > dd->blocks) { // PAY ATTENTION TO THIS
        fprintf(stderr, "Bad blockno provided [%d], should between 0 and %d\n", 
                (int)blockno, (int)dd->blocks);
        return ErrArg;
    } 

    // If read block 1, should range: 0 ~ block_size
    uint32_t offset = (blockno-1)*dd->block_size;

    if (pread(dd->fd, block, dd->block_size, offset) < 0) {
        fprintf(stderr, "dread error, could not read block [%d]...\n", (int)blockno);
        return ErrDread;
    }
    return OK;
}

RC dreads(disk *dd, uint8_t *block, uint32_t start, uint32_t end) {
    if (!dd || !block) {
        fprintf(stderr, "dreads error, dd or block pointer is null...\n");
        return ErrArg;
    }

    if (start < 1) {
        fprintf(stderr, "dreads error, block start from [%d], it should greater equal to 1...\n",
                (int)start);
        return ErrArg;
    }

    if (end < 1 || end > dd->blocks) {
        fprintf(stderr, "dreads error, block end to [%d], it should in range %d ~ %d...\n",
                (int)end, 1, (int)dd->blocks);
        return ErrArg;
    }

    if (start > end) {
         fprintf(stderr, "dreads error, start [%d] > end [%d]\n",
                 (int)start, (int)end);
         return ErrArg;
     }

    uint32_t n;
    RC ret = OK;
    for (n=start; n<=end; n++) {
        ret = dread(dd, block, n);
        if (ret != OK) {
            fprintf(stderr, "dreads failed at block %d\n", (int)n);
            break;
        }

        block += dd->block_size;
    }
    return ret;
}

RC dwrite(disk *dd, uint8_t *block, uint32_t blockno) {
    if (!dd || !block) {
        fprintf(stderr, "dwrite error, dd or block pointer is null...\n");
        return ErrArg;
    }

    // blockno start from 0
    if (blockno <= 0 || blockno > dd->blocks) { // PAY ATTENTION TO THIS
        fprintf(stderr, "Bad blockno provided [%d], should between 0 and %d\n", 
                (int)blockno, (int)dd->blocks);
        return ErrArg;
    } 

    // If read block 1, should range: 0 ~ block_size
    uint32_t offset = (blockno-1)*dd->block_size;

    if (pwrite(dd->fd, block, dd->block_size, offset) < 0) {
        fprintf(stderr, "dwrite error, could not write block [%d]...\n", (int)blockno);
        return ErrDwrite;
    }
    return OK;
}

RC dwrites(disk *dd, uint8_t *block, uint32_t start, uint32_t end) {
    if (!dd || !block) {
        fprintf(stderr, "dwrites error, dd or block pointer is null...\n");
        return ErrArg;
    }
    if (start < 1) {
        fprintf(stderr, "dreads error, block start from [%d], it should greater equal to 1...\n",
                (int)start);
        return ErrArg;
    }

    if (end < 1 || end > dd->blocks) {
        fprintf(stderr, "dreads error, block end to [%d], it should in range %d ~ %d...\n",
                (int)end, 1, (int)dd->blocks);
        return ErrArg;
    }

    if (start > end) {
         fprintf(stderr, "dreads error, start [%d] > end [%d]\n",
                 (int)start, (int)end);
         return ErrArg;
     }

    uint32_t n;
    RC ret = OK;
    for (n=start; n<=end; n++) {
        ret = dwrite(dd, block, n);
        if (ret != OK) {
            fprintf(stderr, "dwrites failed at block %d\n", (int)n);
            break;
        }

        block += dd->block_size;
    }
    return ret;
}
