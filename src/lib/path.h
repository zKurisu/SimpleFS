/*
 * path.h
 * Path resolv
 * Copyright (C) Jie
 * 2025-11-29
 *
 */

#ifndef MY_PATH_H_
#define MY_PATH_H_

#include "dirent.h"
#include "error.h"

#include <stdint.h>

#define MAX_PATH_DEPTH 32

struct s_path {
    char components[MAX_PATH_DEPTH][MAX_FILENAME_LEN];
    uint32_t count;         // Number of components
    uint8_t is_absolute;    // 1 if starts with '/', 0 otherwise
};
typedef struct s_path path;
 
/*
 * Parse a path string into components
 * Handles "." (current dir) and ".." (parent dir)
 *
 * Examples:
 *   "/home/user" -> absolute, ["home", "user"]
 *   "dir/file"   -> relative, ["dir", "file"]
 *   "/a/./b/../c" -> absolute, ["a", "c"]
 */
RC path_parse(const char *path_str, path *p);

/*
 * Convert path structure back to string
 *
 * Parameters:
 *   p: path structure
 *   buf: output buffer
 *   size: buffer size
 *
 * Returns:
 *   OK if successful, error code otherwise
 */
RC path_to_string(const path *p, char *buf, uint32_t size);

/*
 * Display path information (for debugging)
 */
void path_show(const path *p);

/*
 * Check if path is valid
 */
uint8_t path_is_valid(const path *p);

#endif
