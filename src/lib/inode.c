/*
 * inode.c
 * 
 * Copyright (C) Jie
 * 2025-11-17
 *
 */
#include "inode.h"
#include "block.h"
#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RC ino_init(inode *ino) {
    if (!ino) {
        fprintf(stderr, "ino_init err: non null pointer needed...");
        return ErrArg;
    }

    uint32_t size = sizeof(struct s_inode);
    memset(ino, 0, size);

    ino->inode_number = 0;  // 0 means not used
    ino->file_type = FTypeNotValid;
    ino->file_size = 0;

    return OK;
}

uint32_t ino_alloc(filesystem *fs) {
    if (!fs) {
        fprintf(stderr, "ino_alloc error: non null filesystem pointer is needed...\n");
        return 0;
    }

    // Iterating all possible inodes
    uint32_t idx;
    for (idx=0; idx<=fs->inodes; idx++) {
        if (!bm_getbit(fs->inode_bitmap, idx)) { // if bit == 0, which means it is available
            bm_setbit(fs->inode_bitmap, idx); // Allocate means it is used
            // bitmap is 0-based
            // inode number/index is 1-based
            // Therefore return idx + 1
            return idx+1;
        }
    }

    fprintf(stderr, "ino_alloc error: no free inode...\n");
    return idx;
}

RC ino_free(filesystem *fs, uint32_t inode_number) {
    if (!fs || inode_number < 1 || inode_number > fs->inodes) {
        fprintf(stderr, "ino_free error: wrong arguments\n");
        return ErrArg;
    }

    // Convert back to 0-based
    if (bm_unsetbit(fs->inode_bitmap, inode_number-1) < 0) {
        fprintf(stderr, "ino_free error: failed to unsetbit at [%d]\n",
                (int)(inode_number));
        return ErrBmOpe;
    }

    return OK;
}

RC ino_read(filesystem *fs, uint32_t inode_number, inode* ino) {
    if (!fs || inode_number < 1 || inode_number > fs->inodes
            || !ino) {
        fprintf(stderr, "ino_read error: wrong arguments\n");
        return ErrArg;
    }

    // Check: is inode_number is allocated before, if not, return ErrArg
    if (!bm_getbit(fs->inode_bitmap, inode_number-1)) {
        fprintf(stderr, "ino_read error: wrong arguments, inode [%d] is not even allocated...\n",
                inode_number);
        return ErrArg;
    }

    uint32_t ino_per_block, block_number, inode_pos;
    uint32_t size = fs->dd->block_size;
    uint8_t block[size];
    uint8_t *data_begin;
    RC ret = OK;

    ino_per_block = get_inode_per_block(fs->dd);
    // Example:
    //  inode_number = 50
    //  ino_per_block = 16
    //  block_number = 50 / 16 = 3, should += 1
    //  block 1: 1~16
    //  block 2: 17~32
    //  block 3: 33~48
    //  block 4: 49~64
    block_number = inode_number / ino_per_block + fs->inode_table_start;

    // Read inode_pos 0
    // Read inode_pos 15
    inode_pos = (inode_number-1) % ino_per_block;

    ret = dread(fs->dd, block, block_number);
    if (ret != OK) {
        fprintf(stderr, "ino_read error: failed to read from block [%d]...\n",
                (int)block_number);
        return ret;
    }

    size = sizeof(struct s_inode);
    data_begin = block+(inode_pos*size);
    memcpy(ino, data_begin, size);
    
    return ret;
}

RC ino_write(filesystem *fs, uint32_t inode_number, inode* ino) {
    if (!fs || inode_number < 1 || inode_number > fs->inodes
            || !ino) {
        fprintf(stderr, "ino_write error: wrong arguments\n");
        return ErrArg;
    }

    // Check: is inode_number is allocated before, if not, return ErrArg
    if (!bm_getbit(fs->inode_bitmap, inode_number-1)) {
        fprintf(stderr, "ino_write error: wrong arguments, inode [%d] is not even allocated...\n",
                inode_number);
        return ErrArg;
    }
    uint32_t ino_per_block, block_number, inode_pos;
    uint32_t size = fs->dd->block_size;
    uint8_t block[size];
    uint8_t *data_begin;
    RC ret = OK;

    ino_per_block = get_inode_per_block(fs->dd);
    block_number = inode_number / ino_per_block + fs->inode_table_start;

    inode_pos = (inode_number-1) % ino_per_block;

    ret = dread(fs->dd, block, block_number);
    if (ret != OK) {
        fprintf(stderr, "ino_write error: failed to read from block [%d]...\n",
                (int)block_number);
        return ret;
    }

    size = sizeof(struct s_inode);
    data_begin = block+(inode_pos*size);
    memcpy(data_begin, ino, size);

    ret = dwrite(fs->dd, block, block_number);
    if (ret != OK) {
        fprintf(stderr, "ino_write error: failed to write to block [%d]...\n",
                (int)block_number);
    }
    
    return ret;
}

uint32_t ino_get_block_at(filesystem *fs, inode *ino, uint32_t offset) {
    uint32_t block_number_size = (sizeof(uint32_t));
    uint32_t max_offset = DIRECT_POINTERS + fs->dd->block_size/block_number_size; // Block number is uint32_t type
    if (!fs || !ino || offset < 0 || offset > max_offset) {
        fprintf(stderr, "ino_alloc_block error: wrong arguments...\n");
        return 0;
    }


    if (offset < DIRECT_POINTERS) {
        return ino->direct_blocks[offset];
    } else if (!ino->single_indirect) {
        fprintf(stderr, "ino_get_block error: failed to get block at offset [%d], no single_indirect pointer...\n",
                (int)offset);
        return 0;
    } else {
        RC ret;
        uint32_t size = fs->dd->block_size;
        uint32_t block_number;
        uint8_t block_buf[size];
        memset(block_buf, 0, size);
        ret = dread(fs->dd, block_buf, ino->single_indirect);
        if (ret != OK) {
            fprintf(stderr, "ino_get_block error: failed to read from a block at [%d]...\n",
                    ino->single_indirect);
            return 0;
        }

        // Example: offset = 12, DIRECT_POINTERS = 12
        // Should write to slot 0, pos is 0*size=0
        uint32_t slot_pos = (offset - DIRECT_POINTERS)*block_number_size;
        memcpy(&block_number, block_buf+slot_pos, block_number_size);
        return block_number;
    }

    return 0; // 0 is bad block number
}


uint32_t ino_alloc_block_at(filesystem *fs, inode *ino, uint32_t offset) {
    uint32_t block_number_size = (sizeof(uint32_t));
    uint32_t max_offset = DIRECT_POINTERS + fs->dd->block_size/block_number_size; // Block number is uint32_t type
    if (!fs || !ino || offset < 0 || offset > max_offset) {
        fprintf(stderr, "ino_alloc_block error: wrong arguments...\n");
        return 0;
    }

    uint32_t block_number = ino_get_block_at(fs, ino, offset);
    if (block_number != 0) {
        fprintf(stderr, "ino_alloc_block error: offset at [%d] is already allocated...\n",
                (int)offset);
        return 0;
    }

    block_number = bl_alloc(fs);
    if (!block_number) { // block_number == 0 means failed
        fprintf(stderr, "ino_alloc_block error: failed to alloc a block number...\n");
        return 0;
    }

    // If offset point to direct blocks
    if (offset < DIRECT_POINTERS) {
        ino->direct_blocks[offset] = block_number;
    } else if (!ino->single_indirect) { // Handle indirect condition
        fprintf(stderr, "ino_alloc_block error: failed to alloc at offset [%d], no single_indirect pointer...\n",
                (int)offset);
        return 0;
    } else { // Has single indirect
        RC ret;
        uint32_t size = fs->dd->block_size;
        uint8_t block_buf[size];
        memset(block_buf, 0, size);
        ret = dread(fs->dd, block_buf, ino->single_indirect);
        if (ret != OK) {
            fprintf(stderr, "ino_alloc_block error: failed to read from a block at [%d]...\n",
                    ino->single_indirect);
            return 0;
        }

        // Example: offset = 12, DIRECT_POINTERS = 12
        // Should write to slot 0, pos is 0*size=0
        uint32_t slot_pos = (offset - DIRECT_POINTERS)*block_number_size;
        memcpy(block_buf+slot_pos, &block_number, block_number_size);

        ret = dwrite(fs->dd, block_buf, ino->single_indirect);
        if (ret != OK) {
            fprintf(stderr, "ino_alloc_block error: failed to write a block at [%d]...\n",
                    ino->single_indirect);
            return 0;
        }
    }

    return block_number; // 0 is bad block number
}

RC ino_free_block_at(filesystem *fs, inode *ino, uint32_t offset) {
    uint32_t block_number_size = (sizeof(uint32_t));
    uint32_t max_offset = DIRECT_POINTERS + fs->dd->block_size/block_number_size; // Block number is uint32_t type
    if (!fs || !ino || offset < 0 || offset > max_offset) {
        fprintf(stderr, "ino_free_block_at error: wrong arguments...\n");
        return ErrArg;
    }

    uint32_t block_number = ino_get_block_at(fs, ino, offset);
    RC ret = OK;
    if (block_number == 0) {
        return ret;
    }

    // unset bitmap
    ret = bl_free(fs, block_number);
    if (ret != OK) {
        fprintf(stderr, "ino_free_block_at error: failed to free a block at [%d]...\n",
                block_number);
        return ret;
    }

    // If offset point to direct blocks
    if (offset < DIRECT_POINTERS) {
        ino->direct_blocks[offset] = 0;
    } else if (!ino->single_indirect) { // Handle indirect condition
        fprintf(stderr, "ino_free_block_at error: failed to free at offset [%d], no single_indirect pointer...\n",
                (int)offset);
        return ErrInode;
    } else { // Has single indirect
        uint32_t size = fs->dd->block_size;
        uint8_t block_buf[size];
        memset(block_buf, 0, size);
        ret = dread(fs->dd, block_buf, ino->single_indirect);
        if (ret != OK) {
            fprintf(stderr, "ino_free_block_at error: failed to read from a block at [%d]...\n",
                    ino->single_indirect);
            return ret;
        }

        uint32_t slot_pos = (offset - DIRECT_POINTERS)*block_number_size;
        memset(block_buf+slot_pos, 0, block_number_size);

        ret = dwrite(fs->dd, block_buf, ino->single_indirect);
        if (ret != OK) {
            fprintf(stderr, "ino_free_block_at error: failed to write a block at [%d]...\n",
                    ino->single_indirect);
            return ret;
        }
    }

    return OK;
}

RC ino_free_all_blocks(filesystem *fs, inode *ino) {
    uint32_t block_number_size = (sizeof(uint32_t));
    uint32_t max_offset = DIRECT_POINTERS + fs->dd->block_size/block_number_size; // Block number is uint32_t type
    if (!fs || !ino) {
        fprintf(stderr, "ino_free_block_at error: wrong arguments...\n");
        return ErrArg;
    }

    RC ret = OK;

    uint32_t offset;
    for (offset=0; offset < DIRECT_POINTERS; offset++) {
        if (ino->direct_blocks[offset] == 0)
            continue;

        // unset bitmap
        ret = bl_free(fs, ino->direct_blocks[offset]);
        if (ret != OK) {
            fprintf(stderr, "ino_free_block_at error: failed to free a block at [%d]...\n",
                    ino->direct_blocks[offset]);
            return ret;
        }

        ino->direct_blocks[offset] = 0;
    }

    if (!ino->single_indirect) { // Handle indirect condition
        return ret; // It's OK
    } else { // Has single indirect
        uint32_t size = fs->dd->block_size;
        uint8_t block_buf[size];
        memset(block_buf, 0, size);
        ret = dread(fs->dd, block_buf, ino->single_indirect);
        if (ret != OK) {
            fprintf(stderr, "ino_free_block_at error: failed to read from a block at [%d]...\n",
                    ino->single_indirect);
            return ret;
        }
        // unset bitmap
        uint32_t block_number_per_block = size / block_number_size;
        uint32_t block_numbers[block_number_per_block];
        memcpy(block_numbers, block_buf, size);
        for (uint32_t n=0; n < block_number_per_block; n++) {
            if (block_numbers[n] == 0)
                continue;

            ret = bl_free(fs, block_numbers[n]);
            if (ret != OK) {
                fprintf(stderr, "ino_free_block_at error: failed to free a block at [%d]...\n",
                        block_numbers[n]);
                return ret;
            }
        }

        // unset bitmap for single_indirect block
        ret = bl_free(fs, ino->single_indirect);
        if (ret != OK) {
            fprintf(stderr, "ino_free_block_at error: failed to free a block at [%d]...\n",
                    ino->single_indirect);
            return ret;
        }
    }

    return OK;
}

void ino_show(inode *ino) {
    if (!ino) {
        fprintf(stderr, "ino_show: null inode pointer\n");
        return;
    }

    const char *type_str;
    switch (ino->file_type) {
        case FTypeNotValid:
            type_str = "Invalid";
            break;
        case FTypeFile:
            type_str = "File";
            break;
        case FTypeDirectory:
            type_str = "Directory";
            break;
        default:
            type_str = "Unknown";
            break;
    }

    printf("========================================\n");
    printf("Inode Information\n");
    printf("========================================\n");
    printf("Inode number:       %u\n", ino->inode_number);
    printf("File type:          %s (%d)\n", type_str, ino->file_type);
    printf("File size:          %u bytes", ino->file_size);

    // Show human-readable size
    if (ino->file_size >= 1024*1024) {
        printf(" (%.2f MB)", (double)ino->file_size / (1024*1024));
    } else if (ino->file_size >= 1024) {
        printf(" (%.2f KB)", (double)ino->file_size / 1024);
    }
    printf("\n");

    // Show direct blocks
    printf("\nDirect blocks:\n");
    int has_direct = 0;
    for (int i = 0; i < DIRECT_POINTERS; i++) {
        if (ino->direct_blocks[i] != 0) {
            printf("  [%2d] → Block %u\n", i, ino->direct_blocks[i]);
            has_direct = 1;
        }
    }
    if (!has_direct) {
        printf("  (none allocated)\n");
    }

    // Show indirect block
    printf("\nIndirect block:\n");
    if (ino->single_indirect != 0) {
        printf("  Pointer → Block %u\n", ino->single_indirect);
    } else {
        printf("  (not allocated)\n");
    }

    printf("========================================\n");
}

int ino_is_valid(inode *ino) {
    if (!ino) {
        return 0;  // NULL pointer is invalid
    }

    // Check if inode number is valid (not 0)
    if (ino->inode_number == 0) {
        return 0;
    }

    // Check if file type is valid
    if (ino->file_type == FTypeNotValid) {
        return 0;
    }

    // Valid inode
    return 1;
}

uint32_t ino_get_block_count(filesystem *fs, inode *ino) {
    if (!fs || !ino) {
        fprintf(stderr, "ino_get_block_count: invalid arguments\n");
        return 0;
    }

    uint32_t block_size = fs->dd->block_size;

    // Method 1: Calculate from file_size (logical blocks needed)
    if (ino->file_size == 0) {
        return 0;
    }

    uint32_t blocks_needed = (ino->file_size + block_size - 1) / block_size;

    // Method 2: Count actually allocated blocks (more accurate)
    uint32_t allocated_blocks = 0;

    // Count direct blocks
    for (int i = 0; i < DIRECT_POINTERS; i++) {
        if (ino->direct_blocks[i] != 0) {
            allocated_blocks++;
        }
    }

    // Count indirect blocks (if exists)
    if (ino->single_indirect != 0) {
        // Read indirect block and count non-zero entries
        uint32_t block_number_size = sizeof(uint32_t);
        uint32_t pointers_per_block = block_size / block_number_size;
        uint8_t block_buf[block_size];

        if (dread(fs->dd, block_buf, ino->single_indirect) == OK) {
            uint32_t *block_numbers = (uint32_t *)block_buf;
            for (uint32_t i = 0; i < pointers_per_block; i++) {
                if (block_numbers[i] != 0) {
                    allocated_blocks++;
                }
            }
        }
    }

    // Return the actual allocated count
    return allocated_blocks;
}

uint32_t ino_get_max_block_offset(filesystem *fs) {
    return DIRECT_POINTERS + fs->dd->block_size / sizeof(uint32_t);
}

uint32_t ino_get_max_filesize(filesystem *fs) {
    return fs->dd->block_size * ino_get_max_block_offset(fs);
}
