/*
 * path.c
 * Path parsing and resolution
 * Copyright (C) Jie
 * 2025-11-29
 *
 */

#include "path.h"
#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RC path_parse(const char *path_str, path *p) {
    if (!path_str || !p) {
        fprintf(stderr, "path_parse error: wrong arguments...\n");
        return ErrArg;
    }

    if (path_str[0] == '\0') {
        fprintf(stderr, "path_parse error: empty path...\n");
        return ErrArg;
    }

    memset(p, 0, sizeof(struct s_path));

    // absolute or relative path
    if (path_str[0] == '/') {
        p->is_absolute = 1;
        path_str++;
    }

    if (p->is_absolute && path_str[0] == '\0') {
        // Path is just root
        return OK;
    }

    char *copy = strdup(path_str);
    if (copy == NULL) {
        fprintf(stderr, "path_parse error: no enough memory to copy path...\n");
        return ErrNoMem;
    }
    char *saveptr;
    char *token = strtok_r(copy, "/", &saveptr);

    while (token != NULL) {
        if (p->count >= MAX_PATH_DEPTH) {
            fprintf(stderr, "path_parse error: over max path depth...\n");
            free(copy);
            return ErrArg;
        }

        if (strcmp(token, ".") == 0) {
            // skip current dir
        } else if (strcmp(token, "..") == 0) {
            if (p->count > 0)
                p->count--;
        } else {
            if (strlen(token) >= MAX_FILENAME_LEN) {
                fprintf(stderr, "path_parse error: component over max filename len...\n");
                free(copy);
                return ErrArg;
            }
            strcpy(p->components[p->count++], token);
        }

        token = strtok_r(NULL, "/", &saveptr);
    }

    free(copy);

    return OK;
}

RC path_to_string(const path *p, char *buf, uint32_t size) {
    if (!p || !buf || size <= 0) {
        fprintf(stderr, "path_to_string error: wrong arguments...\n");
    }

    memset(buf, 0, size);
    if (p->count == 0) {
        if (p->is_absolute) {
            strncpy(buf, "/", size - 1);
        } else {
            strncpy(buf, ".", size - 1);
        }
    }

    uint32_t offset = 0;
    if (p->is_absolute) {
        buf[offset++] = '/';
    }

    for (uint32_t i=0; i < p->count; i++) {
        uint32_t component_len = strlen(p->components[i]);
        if (offset + component_len + 2 > size) {
            fprintf(stderr, "path_to_string error: buffer size too small...\n");
            return ErrArg;
        }

        strcpy(buf + offset, p->components[i]);
        offset += component_len;

        if (i < p->count - 1) {
            buf[offset++] = '/';
        }
    }

    buf[offset] = '\0';

    return OK;
}

void path_show(const path *p) {
    if (!p) {
        fprintf(stderr, "path_show error: null path\n");
        return;
    }

    printf("========================================\n");
    printf("Path Information\n");
    printf("========================================\n");
    printf("Type:       %s\n", p->is_absolute ? "Absolute" : "Relative");
    printf("Components: %u\n", p->count);

    if (p->count > 0) {
        printf("\nComponents:\n");
        for (uint32_t i = 0; i < p->count; i++) {
            printf("  [%u] %s\n", i, (char*)p->components[i]);
        }
    }

    // Show full path
    char buf[1024];
    if (path_to_string(p, buf, sizeof(buf)) == OK) {
        printf("\nFull path: %s\n", buf);
    }

    printf("========================================\n");
}

uint8_t path_is_valid(const path *p) {
    if (!p) {
        return 0;
    }

    if (p->count > MAX_PATH_DEPTH) {
        return 0;
    }

    // Check each component
    for (uint32_t i = 0; i < p->count; i++) {
        if (dirent_check_valid_name((uint8_t*)p->components[i]) != OK) {
            return 0;
        }
    }

    return 1;
}

