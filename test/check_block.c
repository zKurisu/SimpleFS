#include "block.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    disk *dd;
    uint32_t size, n;
    block *bl;
    superblock_data *super_data, *tmp;
    RC ret;

    size = sizeof(struct s_disk);
    dd = (disk*)malloc(size);

    // Notice: You should run: dd if=/dev/zero of=/tmp/disk0.img count=100 bs=4096
    ret = dattach(dd, 512, 0);
    if (ret != OK) {
        fprintf(stderr, "Failed to attach to disk\n");
        free(dd);
        exit(-1);
    }
    printf("File attach to struct s_disk success...\n");
    dshow(dd);

    printf("Test super block creation...\n");
    bl = bl_create(dd, BlockTypeSuper);
    if (!bl) {
        fprintf(stderr, "Failed to create super block\n");
        free(dd);
        exit(-1);
    }

    size = dd->block_size;
    super_data = (superblock_data*)malloc(size);
    tmp = (superblock_data*)malloc(size);
    super_data->blocks = 255;
    super_data->inodeblocks = 255;
    ret = bl_set_data(bl, (uint8_t*)super_data);
    if (ret != OK) {
        fprintf(stderr, "Failed to set super to block\n");
        free(dd);
        exit(-1);
    }
    printf("Success to set super block...\n");

    tmp = bl_get_data(bl);
    printf("Success to get data from block...\n"
            "  blocks: %d\n"
            "  inodeblocks: %d\n", tmp->blocks, tmp->inodeblocks);
    printf("===========\n");

    dwrite(dd, (uint8_t*)bl->data, 1);

    ddetach(dd);
    printf("File detach from struct s_disk success...\n");

    return 0;
}
