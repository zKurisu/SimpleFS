#include "fs.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    disk *dd;
    uint32_t size, n;
    uint32_t rw_size = 255;
    RC ret;

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

    ret = fs_format(dd);
    if (ret != OK) {
        fprintf(stderr, "Failed to format a disk: %d\n", ret);
        free(dd);
        exit(-1);
    }

    ddetach(dd);
    printf("File detach from struct s_disk success...\n");

    return 0;
}
