#include "fs.h"
#include "block.h"
#include "inode.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    disk *dd;
    uint32_t size, n;
    uint32_t rw_size = 255;
    RC ret;
    filesystem *fs;

    size = sizeof(struct s_disk);
    dd = (disk*)malloc(size);

    // Notice: You should run: dd if=/dev/zero of=/tmp/disk0.img count=100 bs=4096
    ret = dattach(dd, 4096, 0);
    if (ret != OK) {
        fprintf(stderr, "Failed to attach to disk: %d\n", ret);
        free(dd);
        exit(-1);
    }
    printf("File attach to struct s_disk success...\n");
    dshow(dd);

    size = sizeof(struct s_filesystem);
    fs = (filesystem *)malloc(size);
    ret = fs_mount(dd, fs);
    if (ret != OK) {
        fprintf(stderr, "Failed to mount filesystem: %d\n", ret);
        free(dd);
        free(fs);
        exit(-1);
    }
    printf("Success to mount disk to filesystem...\n");

    ///////////////////

    // inode ino;
    // ret = ino_init(&ino);
    // if (ret != OK) {
    //     fprintf(stderr, "Failed to init inode: %d\n", ret);
    //     free(dd);
    //     free(fs);
    //     exit(-1);
    // }
    // // Write
    // uint32_t idx = ino_alloc(fs);
    // if (!idx) {
    //     fprintf(stderr, "Failed to alloc a inode number: %d\n", ret);
    //     free(dd);
    //     free(fs);
    //     exit(-1);
    // }
    // printf("Success to allocate a inode number...\n");
    // ino.inode_number = idx;
    // ino.file_size = 255;
    // ino.file_type = FTypeFile;
    // ret = ino_write(fs, idx, &ino);
    // if (ret != OK) {
    //     fprintf(stderr, "Failed to write inode: %d\n", ret);
    //     free(dd);
    //     free(fs);
    //     exit(-1);
    // }
    // printf("Success to write inode to disk...\n");
    

    // Read
    // ret = ino_read(fs, 1, &ino);
    // if (ret != OK) {
    //     fprintf(stderr, "Failed to read inode: %d\n", ret);
    //     free(dd);
    //     free(fs);
    //     exit(-1);
    // }
    // printf(
    //         "ino.inode_number = %d\n"
    //         "ino.file_size    = %d\n"
    //         "ino.file_type    = %d\n",
    //         ino.inode_number,
    //         ino.file_size,
    //         ino.file_type
    // );
    
    // Free
    // ret = ino_write(fs, 1, &ino);
    // ret = ino_free(fs, 1);
    // if (ret != OK) {
    //     fprintf(stderr, "Failed to write inode: %d\n", ret);
    //     free(dd);
    //     free(fs);
    //     exit(-1);
    // }

    // uint32_t block_number;
    // block_number = bl_alloc(fs);
    // if (!block_number) {
    //      fprintf(stderr, "Failed to allocate block: %d\n", ret);
    //      free(dd);
    //      free(fs);
    //      exit(-1);
    // }
    // printf("Success to allocate a block at [%d]\n", (int)block_number);
    
    uint32_t block_number = 16;
    ret = bl_free(fs, block_number);
    if (ret != OK) {
         fprintf(stderr, "Failed to free block: %d\n", ret);
         free(dd);
         free(fs);
         exit(-1);
    }
    printf("Success to free a block at [%d]\n", (int)block_number);


    ///////////////////

    ret = fs_show(fs);
    if (ret != OK) {
        fprintf(stderr, "Failed to show filesystem: %d\n", ret);
        free(dd);
        free(fs);
        exit(-1);
    }
    printf("Success to show filesystem info...\n");

    ret = fs_unmount(fs);
    if (ret != OK) {
        fprintf(stderr, "Failed to unmount filesystem: %d\n", ret);
        free(dd);
        free(fs);
        exit(-1);
    }
    printf("Success to unmount disk to filesystem...\n");

    ddetach(dd);
    printf("File detach from struct s_disk success...\n");

    return 0;
}
