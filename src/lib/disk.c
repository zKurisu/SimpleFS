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
