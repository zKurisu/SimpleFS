/*
 * fs_api.c
 * 
 * Copyright (C) Jie
 * 2025-11-30
 *
 */

#include "fs_api.h"
#include "path.h"
#include "cwd.h"
#include "directory.h"
#include "block.h"
#include "file.h"

#include <stdio.h>
#include <string.h>

RC fs_touch(filesystem *fs, const char *path_str) {
    if (!fs || !path_str) {
        fprintf(stderr, "fs_touch error: wrong args...\n");
        return ErrArg;
    }
    
    if (path_str[strlen(path_str)-1] == '/') { // touch only allowed to create file
        fprintf(stderr, "fs_touch error: path_str should be a file path, not directory\n");
        return ErrArg;
    }

    path p;
    uint32_t size = sizeof(struct s_path);
    memset(&p, 0, size);

    if (path_parse(path_str, &p) != OK) {
        fprintf(stderr, "fs_touch error: failed to parse path_str [%s] to path structure...\n",
                path_str);
        return ErrName;
    }

    inode ino;
    uint32_t inode_num;
    if (p.is_absolute) {
        inode_num = 1; // Hard encode, root dir inode num
    } else { // relative path
        pthread_rwlock_rdlock(&g_cwd.rwlock);
        inode_num = g_cwd.cwd_inode_num;
        pthread_rwlock_unlock(&g_cwd.rwlock);
    }

    if (ino_read(fs, inode_num, &ino) != OK) {
        fprintf(stderr, "fs_touch error: failed to read base inode [%d]\n",
                inode_num);
        return ErrInode;
    }

    // Check whether file exists
    if ((inode_num = path_lookup(fs, ino, &p)) != 0) {
        fprintf(stderr, "fs_touch error: file already exists [%s]\n",
                path_str);
        return ErrDirentExists;
    }

    inode_num = 1;
    for (uint32_t i=0; i<p.count; i++) {
        if (i != (p.count - 1)) {// path directory check
            if ((inode_num = dir_lookup(fs, &ino, (uint8_t*)p.components[i])) == 0) { 
                fprintf(stderr, "fs_touch error: directory not exists [%s] at level [%d]\n",
                        p.components[i], i);
                return ErrPath;
            }

            if (ino_read(fs, inode_num, &ino) != OK) {
                fprintf(stderr, "fs_touch error: failed to read dir inode [%d]\n",
                        inode_num);
            }
        } else { // last component, file name
            // Create new inode
            inode new_ino;
            ino_init(&new_ino);
            new_ino.file_type = FTypeFile;
            if ((new_ino.inode_number = ino_alloc(fs)) == 0) {
                fprintf(stderr, "fs_touch error: failed to alloc new inode number\n");
                return ErrInternal;
            }
            if ((new_ino.single_indirect = bl_alloc(fs)) == 0) {
                fprintf(stderr, "fs_touch error: failed to alloc new block number\n");
                ino_free(fs, inode_num);
                return ErrInternal;
            }
            if (bl_clean(fs, new_ino.single_indirect) != OK) {
                fprintf(stderr, "fs_touch error: failed to init a single_indirect block\n");
                bl_free(fs, new_ino.single_indirect);
                ino_free(fs, inode_num);

                return ErrInternal;

            }
            ino_write(fs, new_ino.inode_number, &new_ino);
            
            dir_add(fs, &ino, (uint8_t*)p.components[i], new_ino.inode_number);
            ino_write(fs, inode_num, &ino);
        }
    }

    return OK;
}

// Just delete file, no link count info in inode now
RC fs_unlink(filesystem *fs, const char *path_str) { 
    if (!fs || !path_str) {
        fprintf(stderr, "fs_unlink error: wrong args...\n");
        return ErrArg;
    }

    if (path_str[strlen(path_str)-1] == '/') { // touch only allowed to create file
        fprintf(stderr, "fs_unlink error: path_str should be a file path, not directory\n");
        return ErrArg;
    }

    path p;
    uint32_t size = sizeof(struct s_path);
    memset(&p, 0, size);

    if (path_parse(path_str, &p) != OK) {
        fprintf(stderr, "fs_unlink error: failed to parse path_str [%s] to path structure...\n",
                path_str);
        return ErrName;
    }

    inode ino, target_ino;
    uint32_t inode_num;
    if (p.is_absolute) {
        inode_num = 1; // Hard encode, root dir inode num
    } else { // relative path
        pthread_rwlock_rdlock(&g_cwd.rwlock);
        inode_num = g_cwd.cwd_inode_num;
        pthread_rwlock_unlock(&g_cwd.rwlock);
    }

    // Read base inode
    if (ino_read(fs, inode_num, &ino) != OK) {
        fprintf(stderr, "fs_unlink error: failed to read base inode [%d]\n",
                inode_num);
        return ErrInode;
    }

    // Check whether file exists
    if ((inode_num = path_lookup(fs, ino, &p)) == 0) {
        fprintf(stderr, "fs_unlink error: no file exists [%s]\n",
                path_str);
        return ErrPath;
    }

    if (ino_read(fs, inode_num, &target_ino) != OK) {
        fprintf(stderr, "fs_unlink error: failed to read target inode [%d]\n",
                inode_num);
        return ErrInode;
    }
    // Free filesystem resource, inode number and block number
    ino_free_all_blocks(fs, &target_ino);
    ino_free(fs, inode_num);

    // Free directory entry
    inode_num = 1;
    for (uint32_t i=0; i<p.count; i++) {
        if (i != (p.count - 1)) { // path directory check
            inode_num = dir_lookup(fs, &ino, (uint8_t*)p.components[i]);

            if (ino_read(fs, inode_num, &ino) != OK) {
                fprintf(stderr, "fs_touch error: failed to read dir inode [%d]\n",
                        inode_num);
            }
        } else { // last component, file name
            // remove file from directory
            dir_remove(fs, &ino, (uint8_t*)p.components[i]);
            ino_write(fs, inode_num, &ino);
        }
    }

    return OK;
}

RC fs_exists(filesystem *fs, const char *path_str) {
    if (!fs || !path_str) {
        fprintf(stderr, "fs_exists error: wrong args...\n");
        return ErrArg;
    }
    
    path p;
    uint32_t size = sizeof(struct s_path);
    memset(&p, 0, size);

    if (path_parse(path_str, &p) != OK) {
        fprintf(stderr, "fs_exists error: failed to parse path_str [%s] to path structure...\n",
                path_str);
        return ErrName;
    }

    inode ino;
    uint32_t inode_num;
    if (p.is_absolute) {
        inode_num = 1; // Hard encode, root dir inode num
    } else { // relative path
        pthread_rwlock_rdlock(&g_cwd.rwlock);
        inode_num = g_cwd.cwd_inode_num;
        pthread_rwlock_unlock(&g_cwd.rwlock);
    }

    if (ino_read(fs, inode_num, &ino) != OK) {
        fprintf(stderr, "fs_exists error: failed to read base inode [%d]\n",
                inode_num);
        return ErrInode;
    }

    // Check whether file exists
    if ((inode_num = path_lookup(fs, ino, &p)) != 0) {
        return OK;
    }

    return ErrNotFound;
}

RC fs_mkdir(filesystem *fs, const char *path_str) {
    printf("input path is: %s\n", path_str);
    if (!fs || !path_str) {
        fprintf(stderr, "fs_mkdir error: wrong args...\n");
        return ErrArg;
    }
    
    path p;
    uint32_t size = sizeof(struct s_path);
    memset(&p, 0, size);

    if (path_parse(path_str, &p) != OK) {
        fprintf(stderr, "fs_mkdir error: failed to parse path_str [%s] to path structure...\n",
                path_str);
        return ErrName;
    }

    inode ino;
    uint32_t inode_num;
    if (p.is_absolute) {
        inode_num = 1; // Hard encode, root dir inode num
    } else { // relative path
        pthread_rwlock_rdlock(&g_cwd.rwlock);
        inode_num = g_cwd.cwd_inode_num;
        pthread_rwlock_unlock(&g_cwd.rwlock);
    }

    if (ino_read(fs, inode_num, &ino) != OK) {
        fprintf(stderr, "fs_mkdir error: failed to read base inode [%d]\n",
                inode_num);
        return ErrInode;
    }

    // Check whether dir exists
    if ((inode_num = path_lookup(fs, ino, &p)) != 0) {
        fprintf(stderr, "fs_mkdir error: dir already exists [%s]\n",
                path_str);
        return ErrDirentExists;
    }

    inode_num = 1;
    char checked_path[MAX_PATH_LEN];
    memset(checked_path, 0, MAX_PATH_LEN);
    checked_path[0] = '/';
    for (uint32_t i=0; i<p.count; i++) {
        if (i != (p.count - 1)) {// path directory check
            if ((inode_num = dir_lookup(fs, &ino, (uint8_t*)p.components[i])) == 0) { 
                strcat(checked_path, p.components[i]);
                strcat(checked_path, "/");

                if (fs_mkdir(fs, checked_path) != OK) {
                    fprintf(stderr, "fs_mkdir error: failed to recursively make directory [%s]\n",
                            checked_path);
                    return ErrInternal;
                }
                inode_num = dir_lookup(fs, &ino, (uint8_t*)p.components[i]);
            }

            if (ino_read(fs, inode_num, &ino) != OK) {
                fprintf(stderr, "fs_mkdir error: failed to read dir inode [%d]\n",
                        inode_num);
            }
        } else { // last component, dir name
            // Create new inode
            uint32_t new_inode_num = dir_create(fs, inode_num);
            
            dir_add(fs, &ino, (uint8_t*)p.components[i], new_inode_num);
            ino_write(fs, inode_num, &ino);
        }
    }

    return OK;
}

RC fs_rmdir(filesystem *fs, const char *path_str) {
    if (!fs || !path_str) {
        fprintf(stderr, "fs_rmdir error: wrong args...\n");
        return ErrArg;
    }

    path p;
    uint32_t size = sizeof(struct s_path);
    memset(&p, 0, size);

    if (path_parse(path_str, &p) != OK) {
        fprintf(stderr, "fs_rmdir error: failed to parse path_str [%s] to path structure...\n",
                path_str);
        return ErrName;
    }

    inode ino, target_ino;
    uint32_t inode_num;
    if (p.is_absolute) {
        inode_num = 1; // Hard encode, root dir inode num
    } else { // relative path
        pthread_rwlock_rdlock(&g_cwd.rwlock);
        inode_num = g_cwd.cwd_inode_num;
        pthread_rwlock_unlock(&g_cwd.rwlock);
    }

    // Read base inode
    if (ino_read(fs, inode_num, &ino) != OK) {
        fprintf(stderr, "fs_rmdir error: failed to read base inode [%d]\n",
                inode_num);
        return ErrInode;
    }

    // Check whether file exists
    if ((inode_num = path_lookup(fs, ino, &p)) == 0) {
        fprintf(stderr, "fs_rmdir error: no file exists [%s]\n",
                path_str);
        return ErrPath;
    }

    if (ino_read(fs, inode_num, &target_ino) != OK) {
        fprintf(stderr, "fs_rmdir error: failed to read target inode [%d]\n",
                inode_num);
        return ErrInode;
    }

    // Free directory entries
    uint32_t max_block_offset = ino_get_max_block_offset(fs);
    uint32_t dirent_per_block = get_dirent_per_block(fs);
    uint32_t block_size = fs->dd->block_size;
    uint8_t block_buf[block_size];
    uint32_t block_number;
    inode inner_ino;
    for (uint32_t i=0; i<max_block_offset; i++) {
        if ((block_number = ino_get_block_at(fs, &target_ino, i)) == 0)
            continue;

        if (dread(fs->dd, block_buf, block_number) != OK) {
            fprintf(stderr, "fs_rmdir error: failed to read block [%d]\n",
                    block_number);
            return ErrDread;
        }
        dirent *dirent_list = (dirent *)block_buf;

        for (uint32_t j=0; j<dirent_per_block; j++) {
            if (dirent_list[j].inode_num != 0 &&
                strcmp((char*)dirent_list[j].name, ".")  != 0 &&
                strcmp((char*)dirent_list[j].name, "..") != 0) {
                // Check inode type
                if (ino_read(fs, dirent_list[j].inode_num, &inner_ino) != OK) {
                    fprintf(stderr, "fs_rmdir error: failed to read inode [%d]",
                        dirent_list[j].inode_num);
                    return ErrInode;
                }
                if (inner_ino.file_type == FTypeFile) {
                    // Just free inode and block resources
                    ino_free_all_blocks(fs, &inner_ino);
                    ino_free(fs, dirent_list[j].inode_num);
                } else if (inner_ino.file_type == FTypeDirectory) {
                    char new_path[MAX_PATH_LEN] = {0};
                    strcpy(new_path, path_str);
                    if (path_str[strlen(path_str)-1] != '/') {
                        new_path[strlen(path_str)] = '/'; // put / at next position
                    }
                    strcat(new_path, (char*)dirent_list[j].name);

                    if (fs_rmdir(fs, new_path) != OK) {
                        fprintf(stderr, "fs_rmdir error: recursive rm [%s] error\n",
                                new_path);
                        return ErrInternal;
                    }
                }
            }
        }
    }

    // Free filesystem resource, inode number and block number
    dir_delete_empty(fs, &target_ino);

    // Free parent directory entry
    inode_num = 1;
    for (uint32_t i=0; i<p.count; i++) {
        if (i != (p.count - 1)) { // path directory check
            inode_num = dir_lookup(fs, &ino, (uint8_t*)p.components[i]);

            if (ino_read(fs, inode_num, &ino) != OK) {
                fprintf(stderr, "fs_touch error: failed to read dir inode [%d]\n",
                        inode_num);
            }
        } else { // last component, file name
            // remove file from directory
            dir_remove(fs, &ino, (uint8_t*)p.components[i]);
            ino_write(fs, inode_num, &ino);
        }
    }

    return OK;
}

RC fs_ls(filesystem *fs, const char *path_str) {
    if (!fs || !path_str) {
        fprintf(stderr, "fs_ls error: wrong args...\n");
        return ErrArg;
    }

    path p;
    uint32_t size = sizeof(struct s_path);
    memset(&p, 0, size);

    if (path_parse(path_str, &p) != OK) {
        fprintf(stderr, "fs_ls error: failed to parse path_str [%s] to path structure...\n",
                path_str);
        return ErrName;
    }

    inode ino, target_ino;
    uint32_t inode_num;
    if (p.is_absolute) {
        inode_num = 1; // Hard encode, root dir inode num
    } else { // relative path
        pthread_rwlock_rdlock(&g_cwd.rwlock);
        inode_num = g_cwd.cwd_inode_num;
        pthread_rwlock_unlock(&g_cwd.rwlock);
    }

    // Read base inode
    if (ino_read(fs, inode_num, &ino) != OK) {
        fprintf(stderr, "fs_ls error: failed to read base inode [%d]\n",
                inode_num);
        return ErrInode;
    }

    // Check whether file exists
    if ((inode_num = path_lookup(fs, ino, &p)) == 0) {
        fprintf(stderr, "fs_ls error: no file exists [%s]\n",
                path_str);
        return ErrPath;
    }

    if (ino_read(fs, inode_num, &target_ino) != OK) {
        fprintf(stderr, "fs_ls error: failed to read target inode [%d]\n",
                inode_num);
        return ErrInode;
    }

    uint32_t max_block_offset = ino_get_max_block_offset(fs);
    uint32_t dirent_per_block = get_dirent_per_block(fs);
    uint32_t block_size = fs->dd->block_size;
    uint8_t block_buf[block_size];
    uint32_t block_number;
    inode inner_ino;
    for (uint32_t i=0; i<max_block_offset; i++) {
        if ((block_number = ino_get_block_at(fs, &target_ino, i)) == 0)
            continue;

        if (dread(fs->dd, block_buf, block_number) != OK) {
            fprintf(stderr, "fs_ls error: failed to read block [%d]\n",
                    block_number);
            return ErrDread;
        }
        dirent *dirent_list = (dirent *)block_buf;

        for (uint32_t j=0; j<dirent_per_block; j++) {
            if (dirent_list[j].inode_num != 0) {
                // Check inode type
                if (ino_read(fs, dirent_list[j].inode_num, &inner_ino) != OK) {
                    fprintf(stderr, "fs_ls error: failed to read inode [%d]",
                        dirent_list[j].inode_num);
                    return ErrInode;
                }
                if (inner_ino.file_type == FTypeFile) {
                    printf("f: %s\n", dirent_list[j].name);
                } else if (inner_ino.file_type == FTypeDirectory) {
                    printf("d: %s\n", dirent_list[j].name);
                }
            }
        }
    }

    return OK;
}

RC fs_cat(filesystem *fs, const char *path_str) {
    file_handle *fh = file_open(fs, path_str, MY_O_RDONLY);
    if (!fh) {
        fprintf(stderr, "fs_cat error: failed to open file [%s]",
            path_str);
        return ErrInternal;
    }
    uint32_t file_size = fh->cached_inode.file_size;
    uint8_t buf[file_size+1];
    uint32_t bytes_read = file_read(fh, buf, file_size);
    if (bytes_read != file_size) {
        fprintf(stderr, "fs_cat error: failed to read file, bytes read [%d]\n",
            bytes_read);
        return ErrInternal;
    }
    buf[file_size] = '\0';
    printf("%s\n", buf);

    return OK;
}

RC fs_stat(filesystem *fs, const char *path_str, f_stat *st) {
    if (!fs || !path_str || !st) {
        fprintf(stderr, "fs_stat error: wrong args...\n");
        return ErrArg;
    }

    // Parse the path
    path p;
    uint32_t size = sizeof(struct s_path);
    memset(&p, 0, size);

    if (path_parse(path_str, &p) != OK) {
        fprintf(stderr, "fs_stat error: failed to parse path_str [%s] to path structure...\n",
                path_str);
        return ErrName;
    }

    // Get base inode (root or cwd)
    inode ino;
    uint32_t inode_num;
    if (p.is_absolute) {
        inode_num = 1; // Hard encoded, root dir inode num
    } else { // relative path
        pthread_rwlock_rdlock(&g_cwd.rwlock);
        inode_num = g_cwd.cwd_inode_num;
        pthread_rwlock_unlock(&g_cwd.rwlock);
    }

    // Read base inode
    if (ino_read(fs, inode_num, &ino) != OK) {
        fprintf(stderr, "fs_stat error: failed to read base inode [%d]\n",
                inode_num);
        return ErrInode;
    }

    // Lookup target inode
    if ((inode_num = path_lookup(fs, ino, &p)) == 0) {
        fprintf(stderr, "fs_stat error: no file exists [%s]\n",
                path_str);
        return ErrNotFound;
    }

    // Read target inode
    inode target_ino;
    if (ino_read(fs, inode_num, &target_ino) != OK) {
        fprintf(stderr, "fs_stat error: failed to read target inode [%d]\n",
                inode_num);
        return ErrInode;
    }

    // Populate f_stat structure
    st->inode_num = inode_num;
    st->type = target_ino.file_type;
    st->size = target_ino.file_size;
    st->blocks = ino_get_block_count(fs, &target_ino);

    return OK;
}
