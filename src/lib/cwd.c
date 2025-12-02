/*
 * cwd.c
 * 
 * Copyright (C) Jie
 * 2025-12-01
 *
 */
#include "cwd.h"
#include "path.h"
#include "directory.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

cwd_context_t g_cwd = {0};

RC cwd_init(filesystem *fs, uint32_t root_inode_num) {
    if (!fs) {
        fprintf(stderr, "cwd_init error: wrong args...\n");
        return ErrArg;
    }

    g_cwd.fs = fs;
    g_cwd.cwd_inode_num = root_inode_num;
    if (ino_read(fs, root_inode_num, &g_cwd.cached_cwd_inode) != OK) {
        fprintf(stderr, "cwd_init error: failed to read root inode: %d\n",
                root_inode_num);
    }
    g_cwd.cache_valid = 1;

    if (path_parse("/", &g_cwd.p) != OK) {
        fprintf(stderr, "cwd_init error: failed to parse [/] to path structure\n");
        return ErrInternal;
    }

    if (pthread_rwlock_init(&g_cwd.rwlock, NULL) != 0) {
        fprintf(stderr, "cwd_init error: failed to init rwlock\n");
        return ErrInternal;
    }

    return OK;
}

uint32_t cwd_get_inode() {
    pthread_rwlock_rdlock(&g_cwd.rwlock);
    uint32_t inode_num = g_cwd.cwd_inode_num;
    pthread_rwlock_unlock(&g_cwd.rwlock);
    return inode_num;
}

void cwd_get_path(path *p) {
    if (!p) {
        fprintf(stderr, "cwd_get_path error: wrong args\n");
    }

    pthread_rwlock_rdlock(&g_cwd.rwlock);
    uint32_t size = sizeof(struct s_path);
    memcpy(p, &g_cwd.p, size);
    pthread_rwlock_unlock(&g_cwd.rwlock);
}

RC cwd_chdir_inode(uint32_t new_inode_num) {
    pthread_rwlock_rdlock(&g_cwd.rwlock);
    filesystem fs = *g_cwd.fs;
    pthread_rwlock_unlock(&g_cwd.rwlock);

    inode ino;
    uint32_t cur_ino_num = new_inode_num;
    uint8_t *parent_ino_name = (uint8_t*)"..";
    uint32_t parent_ino_num = 0; // Just init, not valid inode number
    uint8_t buf[MAX_FILENAME_LEN];
    // path is      : /home/jie/Documents/homework
    // store here is: ["homework", "Documents", "jie", "home"]
    char reverse_components[MAX_PATH_DEPTH][MAX_FILENAME_LEN];
    uint32_t depth = 0;

    while (depth < MAX_PATH_DEPTH) { // parent_ino_num == cur_ino_num mean root directory, end
        if (ino_read(&fs, cur_ino_num, &ino) != OK) {
            fprintf(stderr, "cwd_chdir_inode error: failed to read inode [%d]\n",
                    cur_ino_num);
            return ErrInode;
        }

        if (ino.file_type != FTypeDirectory) {
            fprintf(stderr, "cwd_chdir_inode error: inode [%d] is not a directory\n",
                    new_inode_num);
            return ErrInode;
        }

        parent_ino_num  = dir_lookup(&fs, &ino, parent_ino_name);
        if (parent_ino_num == cur_ino_num) {
            break;
        }

        if (ino_read(&fs, parent_ino_num, &ino) != OK) {
            fprintf(stderr, "cwd_chdir_inode error: failed to read inode [%d]\n",
                    parent_ino_num);
            return ErrInode;
        }

        if (dir_lookup_by_id(&fs, &ino, buf, cur_ino_num) != OK) {
            fprintf(stderr, "cwd_chdir_inode error: can not find inode [%d] under dir [%s]",
                    cur_ino_num, parent_ino_name);
            return ErrInode;
        }
        // Get current name, buf how to store, no depth info now
        memcpy(reverse_components[depth++], buf, MAX_FILENAME_LEN);
        cur_ino_num = parent_ino_num;
        parent_ino_num = 0;
    }

    path new_p;
    memset(&new_p, 0, sizeof(struct s_path));
    new_p.count = depth;
    new_p.is_absolute = 1;
    for (uint32_t i=0; i<depth; i++)
        memcpy(new_p.components[depth-i-1], reverse_components[i], MAX_FILENAME_LEN);

    pthread_rwlock_wrlock(&g_cwd.rwlock);
    g_cwd.cwd_inode_num = new_inode_num;
    g_cwd.cache_valid = 0;

    memcpy(&g_cwd.p, &new_p, sizeof(struct s_path));
    pthread_rwlock_unlock(&g_cwd.rwlock);

    return OK;
}

RC cwd_chdir_path(const char *path_str) {
    if (!path_str) {
        fprintf(stderr, "cwd_chdir_path error: wrong args\n");
        return ErrArg;
    }

    path p;
    if (path_parse(path_str, &p) != OK) {
        fprintf(stderr, "cwd_chdir_path error: failed to parse path_str [%s] to path structure\n",
                path_str);
    }

    pthread_rwlock_wrlock(&g_cwd.rwlock);
    if (g_cwd.cache_valid == 0) {
        if (ino_read(g_cwd.fs, g_cwd.cwd_inode_num, &g_cwd.cached_cwd_inode) != OK) {
            fprintf(stderr, "cwd_chdir_path error: failed to cache g_cwd inode [%d]\n",
                    g_cwd.cwd_inode_num);
            return ErrInode;
        }
        g_cwd.cache_valid = 1;
    }
    pthread_rwlock_unlock(&g_cwd.rwlock);

    pthread_rwlock_rdlock(&g_cwd.rwlock);
    inode ino;
    memcpy(&ino, &g_cwd.cached_cwd_inode, sizeof(struct s_inode));

    filesystem fs = *g_cwd.fs;
    pthread_rwlock_unlock(&g_cwd.rwlock);

    uint32_t inode_num;
    if (p.is_absolute) { // Find fron root
        if (ino_read(&fs, 1, &ino) != OK) { // Hard encode!! pay attention
            fprintf(stderr, "cwd_chdir_path error: failed to read root inode [%d]\n",1);
            return ErrInode;
        }
    }
    // else p is relative path
    if ((inode_num = path_lookup(&fs, ino, &p)) == 0) {
        fprintf(stderr, "cwd_chdir_path error: failed find inode number for path\n");
        return ErrPath;
    }

    pthread_rwlock_wrlock(&g_cwd.rwlock);
    if (p.is_absolute) {
        memcpy(&g_cwd.p, &p, sizeof(struct s_path));
    } else {
        if (path_merge(&g_cwd.p, &p) != OK) {
            fprintf(stderr, "cwd_chdir_path error: failed to merge path\n");
            return ErrInternal;
        }
    }
    g_cwd.cwd_inode_num = inode_num;
    memcpy(&g_cwd.cached_cwd_inode, &ino, sizeof(struct s_inode));
    g_cwd.cache_valid = 1;
    pthread_rwlock_unlock(&g_cwd.rwlock);

    return OK;
}

void cwd_show() {
    char buf[MAX_PATH_LEN];
    pthread_rwlock_rdlock(&g_cwd.rwlock);
    path_to_string(&g_cwd.p, buf, MAX_PATH_LEN);
    pthread_rwlock_unlock(&g_cwd.rwlock);
    printf("%s\n", buf);
}
