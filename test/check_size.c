#include "disk.h"
#include "inode.h"
#include "block.h"
#include <stdio.h>

// Not packed version
int main(void) {
    printf(
        "Check s_disk structure size: %lu, it should be %d\n",
        sizeof(struct s_disk), 16
    );
    printf(
        "Check s_block structure size: %lu, it should be %d\n",
        sizeof(struct s_block), 16
    );
    printf(
        "Check s_superblock_data structure size: %lu, it shoud be %d\n",
        sizeof(struct s_superblock_data), 16
    );
    printf(
        "Check s_inode structure size: %lu, it should be %d\n",
        sizeof(struct s_inode), 64
    );
}
