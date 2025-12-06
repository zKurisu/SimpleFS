#include <gtest/gtest.h>
#include <stdio.h>
#include <stdint.h>
#include "disk.h"
#include "fs.h"
#include "path.h"

#define BLOCK_SIZE 4096
#define DISK_ID 0

class FSFixture : public ::testing::Test {
protected:
    disk *dd;
    filesystem *fs;

    void SetUp() override {
        uint32_t size;
        size = sizeof(disk);
        dd = (disk*)malloc(size);
        if (!dd) {
            fprintf(stderr, "SetUp error: failed to alloc memory for disk\n");
            exit(-1);
        }

        if (dattach(dd, BLOCK_SIZE, DISK_ID) != OK) {
            fprintf(stderr, "SetUp error: failed to attach to disk\n");
            free(dd);
            exit(-1);
        }

        size = sizeof(filesystem);
        fs = (filesystem*)malloc(size);
        if (!fs) {
            fprintf(stderr, "SetUp error: failed to alloc memory for filesystem\n");
            free(dd);
            exit(-1);
        }

        if (fs_mount(dd, fs) != OK) {
            fprintf(stderr, "SetUp error: failed to mount filesystem to disk\n");
            free(dd);
            free(fs);
            exit(-1);
        }
    }

    void TearDown() override {
        if (fs_unmount(fs) != OK) {
            fprintf(stderr, "SetUp error: failed to unmount filesystem\n");
            free(dd);
            free(fs);
            exit(-1);
        }

        if (ddetach(dd) != OK) {
            fprintf(stderr, "SetUp error: failed to detach disk\n");
            free(dd);
            free(fs);
            exit(-1);
        }
        free(dd);
        free(fs);
    }
};

TEST_F(FSFixture, test_abs_path) {
    path p;
    char path_str[] = "/home/jie/Documents/classes";
    char buf[MAX_PATH_LEN];
    path_parse(path_str, &p);
    path_to_string(&p, buf, MAX_PATH_LEN);

    ASSERT_EQ(4, p.count);
    ASSERT_TRUE(p.is_absolute);
    ASSERT_STREQ(path_str, buf);
}

TEST_F(FSFixture, test_rel_path) {
    path p;
    char path_str[] = "jie/Documents/classes";
    char buf[MAX_PATH_LEN];
    path_parse(path_str, &p);
    path_to_string(&p, buf, MAX_PATH_LEN);

    ASSERT_EQ(3, p.count);
    ASSERT_FALSE(p.is_absolute);
    ASSERT_STREQ(path_str, buf);
}

TEST_F(FSFixture, test_bitmap) {
    bitmap *bm = bm_create(100);
    bm_setbit(bm, 10);
    bm_setbit(bm, 20);
    bm_setbit(bm, 30);

    ASSERT_EQ(1, bm_getbit(bm, 10));
    ASSERT_EQ(1, bm_getbit(bm, 20));
    ASSERT_EQ(1, bm_getbit(bm, 30));

    bm_unsetbit(bm, 10);
    ASSERT_EQ(0, bm_getbit(bm, 10));

    bm_clearmap(bm);
    ASSERT_EQ(0, bm_getbit(bm, 20));
    ASSERT_EQ(0, bm_getbit(bm, 30));
    
    bm_destroy(bm);
}
