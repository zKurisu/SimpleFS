#include "disk.h"
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    disk *dd;
    uint32_t size = sizeof(disk);
    RC ret;

    dd = (disk*)malloc(size);

    // Notice: You should run: dd if=/dev/zero of=/tmp/disk0.img count=100 bs=4096
    ret = dattach(dd, 4096, 0);
    if (ret != OK) {
        fprintf(stderr, "Failed to attach to disk");
        free(dd);
        exit(-1);
    }
    dshow(dd);
    ddetach(dd);

    return 0;
}
