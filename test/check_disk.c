#include "disk.h"
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    disk *dd;
    uint32_t size, n;
    uint32_t rw_size = 255;
    uint8_t *block;
    RC ret;

    size = sizeof(struct s_disk);
    dd = (disk*)malloc(size);

    // Notice: You should run: dd if=/dev/zero of=/tmp/disk0.img count=100 bs=4096
    ret = dattach(dd, 4096, 0);
    if (ret != OK) {
        fprintf(stderr, "Failed to attach to disk\n");
        free(dd);
        exit(-1);
    }
    printf("File attach to struct s_disk success...\n");
    dshow(dd);

    size = dd->block_size;
    block = (uint8_t *)malloc(size);
    for (n=1; n<rw_size; n++)
        block[n] = n;

    ret = dwrite(dd, block, 1); // Write a tmp super block
    if (ret != OK) {
        fprintf(stderr, "Failed to write a block to disk\n");
        ddetach(dd);
        free(dd);
        free(block);
        exit(-1);
    }
    printf("Disk write success...\n");

    ret = dread(dd, block, 1); // Read a tmp super block
    if (ret != OK) {
        fprintf(stderr, "Failed to read a block from disk\n");
        ddetach(dd);
        free(dd);
        free(block);
        exit(-1);
    }
    printf("Disk Read success...\n");
    printf("========\n");
    for (n=1; n<rw_size; n++) {
        printf("%.02hhx", (int)block[n]);
        if (!(n%2))
            printf(" ");
        if (!(n%16))
            printf("\n");
    }
    printf("\n");
    printf("========\n");

    ddetach(dd);
    printf("File detach from struct s_disk success...\n");

    return 0;
}
