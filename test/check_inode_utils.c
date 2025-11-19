/*
 * Test inode utility functions
 */

#include "inode.h"
#include "fs.h"
#include "disk.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    disk *dd;
    filesystem *fs;
    RC ret;

    // Allocate structures
    dd = (disk*)malloc(sizeof(struct s_disk));
    fs = (filesystem*)malloc(sizeof(struct s_filesystem));

    // Attach and format
    ret = dattach(dd, 4096, 0);
    if (ret != OK) {
        fprintf(stderr, "Failed to attach disk\n");
        return -1;
    }

    printf("=== Formatting filesystem ===\n\n");
    ret = fs_format(dd);
    if (ret != OK) {
        fprintf(stderr, "Failed to format\n");
        return -1;
    }

    printf("=== Mounting filesystem ===\n\n");
    ret = fs_mount(dd, fs);
    if (ret != OK) {
        fprintf(stderr, "Failed to mount\n");
        return -1;
    }

    // Test 1: Create and show an empty inode
    printf("=== Test 1: Empty inode ===\n\n");
    inode ino1;
    ino_init(&ino1);
    printf("Is valid? %s\n", ino_is_valid(&ino1) ? "Yes" : "No");
    printf("Block count: %u\n\n", ino_get_block_count(fs, &ino1));

    // Test 2: Allocate inode and set properties
    printf("=== Test 2: Allocate inode ===\n\n");
    uint32_t inode_num = ino_alloc(fs);
    printf("Allocated inode number: %u\n\n", inode_num);

    inode ino2;
    ino_init(&ino2);
    ino2.inode_number = inode_num;
    ino2.file_type = FTypeFile;
    ino2.file_size = 5000;

    printf("Is valid? %s\n", ino_is_valid(&ino2) ? "Yes" : "No");
    printf("Block count (logical): %u\n\n", ino_get_block_count(fs, &ino2));

    ino_show(&ino2);

    // Test 3: Allocate some blocks
    printf("\n=== Test 3: Allocate blocks ===\n\n");
    uint32_t block1 = ino_alloc_block_at(fs, &ino2, 0);
    uint32_t block2 = ino_alloc_block_at(fs, &ino2, 1);
    uint32_t block3 = ino_alloc_block_at(fs, &ino2, 5);

    printf("Allocated blocks: %u, %u, %u\n\n", block1, block2, block3);
    printf("Block count (actual): %u\n\n", ino_get_block_count(fs, &ino2));

    ino_show(&ino2);

    // Test 4: Write and read back
    printf("\n=== Test 4: Write and read inode ===\n\n");
    ret = ino_write(fs, inode_num, &ino2);
    if (ret != OK) {
        fprintf(stderr, "Failed to write inode\n");
        return -1;
    }
    printf("Inode written to disk\n");

    inode ino;
    ret = ino_read(fs, inode_num, &ino);
    if (ret != OK) {
        fprintf(stderr, "Failed to read inode\n");
        return -1;
    }
    printf("Inode read from disk\n\n");

    ino_show(&ino);

    // Test 5: Invalid inode
    printf("\n=== Test 5: Invalid inode ===\n\n");
    inode invalid;
    ino_init(&invalid);
    invalid.inode_number = 999;
    invalid.file_type = FTypeNotValid;
    printf("Is valid? %s\n\n", ino_is_valid(&invalid) ? "Yes" : "No");

    // Cleanup
    printf("=== Unmounting ===\n");
    fs_unmount(fs);
    ddetach(dd);
    free(fs);

    return 0;
}
