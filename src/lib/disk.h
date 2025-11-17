/*
 * disk.h
 * Copyright (C) Jie
 * 2025-11-14
 * Disk Simulator
 */

#ifndef MY_DISK_H_
#define MY_DISK_H_

#include <stdint.h>
#include "error.h"

#define MAX_DISKS 10
#define MIN_BLOCK_SIZE 512
static char *disk_paths[MAX_DISKS] = {
    "/tmp/disk0.img",
    "/tmp/disk1.img",
    "/tmp/disk2.img",
    "/tmp/disk3.img",
    "/tmp/disk4.img",
    "/tmp/disk5.img",
    "/tmp/disk6.img",
    "/tmp/disk7.img",
    "/tmp/disk8.img",
    "/tmp/disk9.img",
};

typedef uint8_t diskno; // Using to find disk file

// s_disk means: disk structure
struct s_disk {
    uint32_t fd;         // File descriptor, 4 bytes
    uint32_t blocks;     // Total block number, 4 bytes
    uint32_t block_size; // Block size, 4 bytes
    diskno id;
};
typedef struct s_disk disk;

/*
 * Initialize a disk structure
*/
RC dattach(disk *dd, uint32_t block_size, diskno disk_id);

/*
 * Destroy a disk structure
*/
RC ddetach(disk *dd);

/*
 * Print metadata of a disk
*/
void dshow(disk *dd);

/*
 * Read from a disk block:
 *  dd: Disk structure, using low level file descriptor
 *  block: a byte array, using to store data
 *  blockno: using to calculate offset, find start of the block
*/
RC dread(disk *dd, uint8_t *block, uint32_t blockno);

// Read include number of END
RC dreads(disk *dd, uint8_t *block, uint32_t start, uint32_t end);

/*
 * Write to a disk block: Same with dread
*/
RC dwrite(disk *dd, uint8_t *block, uint32_t blockno);

// Write include number of END
RC dwrites(disk *dd, uint8_t *block, uint32_t start, uint32_t end);

#endif
