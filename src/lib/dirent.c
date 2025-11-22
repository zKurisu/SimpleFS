/*
 * dirent.c
 * function
 * Copyright (C) Jie
 * 2025-11-22
 *
 */

#include "dirent.h"

#include <stdio.h>
#include <ctype.h>

RC dirent_check_valid_name(const uint8_t *name) {
    if (!name) {
        fprintf(stderr, "dirent_check_valid_name error: name pointer is null...\n");
        return ErrArg;
    }

    for (uint32_t n=0; name[n]; n++) {
        if (!isalnum(name[n])) { // Check is alphabet or digit
            uint8_t *special_char = VALID_NAME_SPECIAL_CHARS;
            uint8_t find_specific_flag = 0;
            for (uint32_t j=0; special_char[j]; j++) {
                if (name[n] == special_char[j]) {
                    find_specific_flag = 1;
                    break;
                }
                // printf("check [%c] with special char [%c], find_specific_flag: %s\n", name[n], *special_char, find_specific_flag == 0 ? "no" : "yes");
            }

            if (!find_specific_flag)
                return ErrName;
        }
    }
    return OK;
}

uint32_t get_dirent_per_block(filesystem *fs) {
    if (!fs) {
        fprintf(stderr, "dirent_per_block error: null filesystem pointer...");
        return 0;
    }

    uint32_t dirent_size = sizeof(struct s_dirent);
    uint32_t remainder   = fs->dd->block_size % dirent_size;
    if (remainder != 0) {
        fprintf(stderr, "dirent_per_block error: block size is not integer multiple of directory entry size...");
        return 0;
    }

    return fs->dd->block_size / dirent_size;
}
