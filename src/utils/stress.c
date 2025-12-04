/*
 * stress.c - Simplified multi-threaded stress test for file read/write operations
 *
 * This program focuses on testing concurrent file read/write operations
 * to verify the correctness of file handle locking and data consistency.
 *
 * Usage:
 *   ./build/my_stress <disk_id> <block_size> <thread_count> <iterations>
 *
 * Example:
 *   ./build/my_stress 1 4096 4 100
 *
 * Copyright (C) Jie
 * 2025-12-03
 */

#include "disk.h"
#include "fs.h"
#include "fs_api.h"
#include "file.h"
#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdarg.h>

// ============================================
// Configuration
// ============================================
#define MAX_FILE_SIZE 8192
#define MAX_FILES 10
#define MAX_ERROR_LOGS 20

// ============================================
// Statistics
// ============================================
typedef struct {
    char message[256];
    int thread_id;
} error_log_entry;

typedef struct {
    atomic_uint total_ops;
    atomic_uint successful_ops;
    atomic_uint failed_ops;

    atomic_uint writes;
    atomic_uint reads;
    atomic_uint write_bytes;
    atomic_uint read_bytes;

    atomic_uint failed_touch;
    atomic_uint failed_open;
    atomic_uint failed_write;
    atomic_uint failed_read;
    atomic_uint failed_verify;

    error_log_entry error_logs[MAX_ERROR_LOGS];
    atomic_uint error_log_index;
} stress_stats;

typedef struct {
    filesystem *fs;
    int thread_id;
    int iterations;
    stress_stats *stats;
    unsigned int seed;
} thread_context;

stress_stats g_stats;
pthread_mutex_t g_print_lock = PTHREAD_MUTEX_INITIALIZER;

// ============================================
// Utility Functions
// ============================================

void ts_printf(const char *format, ...) {
    pthread_mutex_lock(&g_print_lock);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
    pthread_mutex_unlock(&g_print_lock);
}

void log_error(stress_stats *stats, int thread_id, const char *format, ...) {
    char message[256];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    ts_printf("[Thread %d] ERROR: %s\n", thread_id, message);

    uint32_t index = atomic_fetch_add(&stats->error_log_index, 1);
    index = index % MAX_ERROR_LOGS;

    pthread_mutex_lock(&g_print_lock);
    strncpy(stats->error_logs[index].message, message, sizeof(stats->error_logs[index].message) - 1);
    stats->error_logs[index].message[sizeof(stats->error_logs[index].message) - 1] = '\0';
    stats->error_logs[index].thread_id = thread_id;
    pthread_mutex_unlock(&g_print_lock);
}

// ============================================
// Test Operations
// ============================================

// Test 1: Write to a new file
int test_write_new_file(thread_context *ctx) {
    char path[256];
    snprintf(path, sizeof(path), "/test_file_%d_%d.txt",
             ctx->thread_id, rand_r(&ctx->seed) % 1000);

    // Create file
    if (fs_touch(ctx->fs, path) != OK) {
        log_error(ctx->stats, ctx->thread_id, "fs_touch failed: %s", path);
        atomic_fetch_add(&ctx->stats->failed_touch, 1);
        atomic_fetch_add(&ctx->stats->failed_ops, 1);
        return -1;
    }

    // Open for writing
    file_handle *fh = file_open(ctx->fs, path, MY_O_WRONLY);
    if (!fh) {
        log_error(ctx->stats, ctx->thread_id, "file_open(WRONLY) failed: %s", path);
        atomic_fetch_add(&ctx->stats->failed_open, 1);
        atomic_fetch_add(&ctx->stats->failed_ops, 1);
        fs_unlink(ctx->fs, path);
        return -1;
    }

    // Generate random data
    uint32_t size = (rand_r(&ctx->seed) % MAX_FILE_SIZE) + 1;
    uint8_t *data = malloc(size);
    if (!data) {
        log_error(ctx->stats, ctx->thread_id, "malloc failed for write buffer");
        file_close(fh);
        fs_unlink(ctx->fs, path);
        atomic_fetch_add(&ctx->stats->failed_ops, 1);
        return -1;
    }

    for (uint32_t i = 0; i < size; i++) {
        data[i] = (uint8_t)(rand_r(&ctx->seed) % 256);
    }

    // Write data
    uint32_t written = file_write(fh, data, size);
    if (written != size) {
        log_error(ctx->stats, ctx->thread_id,
                 "Write size mismatch: expected=%u got=%u file=%s",
                 size, written, path);
        atomic_fetch_add(&ctx->stats->failed_write, 1);
        atomic_fetch_add(&ctx->stats->failed_ops, 1);
        free(data);
        file_close(fh);
        fs_unlink(ctx->fs, path);
        return -1;
    }

    file_close(fh);

    // Verify by reading back
    fh = file_open(ctx->fs, path, MY_O_RDONLY);
    if (!fh) {
        log_error(ctx->stats, ctx->thread_id, "file_open(RDONLY) failed for verify: %s", path);
        atomic_fetch_add(&ctx->stats->failed_open, 1);
        atomic_fetch_add(&ctx->stats->failed_ops, 1);
        free(data);
        fs_unlink(ctx->fs, path);
        return -1;
    }

    uint8_t *read_buf = malloc(size);
    if (!read_buf) {
        log_error(ctx->stats, ctx->thread_id, "malloc failed for read buffer");
        free(data);
        file_close(fh);
        fs_unlink(ctx->fs, path);
        atomic_fetch_add(&ctx->stats->failed_ops, 1);
        return -1;
    }

    uint32_t read_size = file_read(fh, read_buf, size);
    if (read_size != size) {
        log_error(ctx->stats, ctx->thread_id,
                 "Read size mismatch: expected=%u got=%u file=%s",
                 size, read_size, path);
        atomic_fetch_add(&ctx->stats->failed_read, 1);
        atomic_fetch_add(&ctx->stats->failed_ops, 1);
        free(data);
        free(read_buf);
        file_close(fh);
        fs_unlink(ctx->fs, path);
        return -1;
    }

    // Verify content
    if (memcmp(data, read_buf, size) != 0) {
        // Find first mismatch
        uint32_t mismatch_pos = 0;
        for (uint32_t i = 0; i < size; i++) {
            if (data[i] != read_buf[i]) {
                mismatch_pos = i;
                break;
            }
        }
        log_error(ctx->stats, ctx->thread_id,
                 "Data verify failed: %s size=%u mismatch_at=%u (wrote:0x%02x read:0x%02x)",
                 path, size, mismatch_pos, data[mismatch_pos], read_buf[mismatch_pos]);
        atomic_fetch_add(&ctx->stats->failed_verify, 1);
        atomic_fetch_add(&ctx->stats->failed_ops, 1);
        free(data);
        free(read_buf);
        file_close(fh);
        fs_unlink(ctx->fs, path);
        return -1;
    }

    free(data);
    free(read_buf);
    file_close(fh);
    fs_unlink(ctx->fs, path);

    atomic_fetch_add(&ctx->stats->writes, 1);
    atomic_fetch_add(&ctx->stats->reads, 1);
    atomic_fetch_add(&ctx->stats->write_bytes, size);
    atomic_fetch_add(&ctx->stats->read_bytes, size);
    atomic_fetch_add(&ctx->stats->successful_ops, 1);
    return 0;
}

// Test 2: Write to shared file (concurrent writes with append)
int test_write_shared_file(thread_context *ctx) {
    char path[256];
    snprintf(path, sizeof(path), "/shared_file_%d.txt",
             rand_r(&ctx->seed) % MAX_FILES);

    // Try to create (ignore if exists)
    RC rc = fs_touch(ctx->fs, path);
    if (rc != OK && rc != ErrDirentExists) {
        log_error(ctx->stats, ctx->thread_id, "fs_touch failed for shared: %s", path);
        atomic_fetch_add(&ctx->stats->failed_touch, 1);
        atomic_fetch_add(&ctx->stats->failed_ops, 1);
        return -1;
    }

    // Open with append flag
    file_handle *fh = file_open(ctx->fs, path, MY_O_WRONLY | MY_O_APPEND);
    if (!fh) {
        log_error(ctx->stats, ctx->thread_id, "file_open(APPEND) failed: %s", path);
        atomic_fetch_add(&ctx->stats->failed_open, 1);
        atomic_fetch_add(&ctx->stats->failed_ops, 1);
        return -1;
    }

    // Write thread ID and timestamp
    char data[128];
    uint32_t size = snprintf(data, sizeof(data),
                            "[Thread %d] timestamp=%ld iteration=%d\n",
                            ctx->thread_id, time(NULL), rand_r(&ctx->seed));

    uint32_t written = file_write(fh, (uint8_t*)data, size);
    if (written != size) {
        log_error(ctx->stats, ctx->thread_id,
                 "Append write failed: expected=%u got=%u", size, written);
        atomic_fetch_add(&ctx->stats->failed_write, 1);
        atomic_fetch_add(&ctx->stats->failed_ops, 1);
        file_close(fh);
        return -1;
    }

    file_close(fh);

    atomic_fetch_add(&ctx->stats->writes, 1);
    atomic_fetch_add(&ctx->stats->write_bytes, size);
    atomic_fetch_add(&ctx->stats->successful_ops, 1);
    return 0;
}

// Test 3: Read from shared file (concurrent reads)
int test_read_shared_file(thread_context *ctx) {
    char path[256];
    snprintf(path, sizeof(path), "/shared_file_%d.txt",
             rand_r(&ctx->seed) % MAX_FILES);

    // Check if file exists
    if (fs_exists(ctx->fs, path) != OK) {
        // File doesn't exist yet, not an error
        atomic_fetch_add(&ctx->stats->successful_ops, 1);
        return 0;
    }

    // Open for reading
    file_handle *fh = file_open(ctx->fs, path, MY_O_RDONLY);
    if (!fh) {
        log_error(ctx->stats, ctx->thread_id, "file_open(RDONLY) failed: %s", path);
        atomic_fetch_add(&ctx->stats->failed_open, 1);
        atomic_fetch_add(&ctx->stats->failed_ops, 1);
        return -1;
    }

    // Read some data
    uint8_t buffer[256];
    uint32_t read_size = file_read(fh, buffer, sizeof(buffer));

    file_close(fh);

    atomic_fetch_add(&ctx->stats->reads, 1);
    atomic_fetch_add(&ctx->stats->read_bytes, read_size);
    atomic_fetch_add(&ctx->stats->successful_ops, 1);
    return 0;
}

// ============================================
// Thread Worker
// ============================================

void *stress_worker(void *arg) {
    thread_context *ctx = (thread_context *)arg;

    ts_printf("[Thread %d] Starting with %d iterations\n",
             ctx->thread_id, ctx->iterations);

    for (int i = 0; i < ctx->iterations; i++) {
        int test_type = rand_r(&ctx->seed) % 100;

        atomic_fetch_add(&ctx->stats->total_ops, 1);

        if (test_type < 50) {
            // 50% - Write to new file and verify
            test_write_new_file(ctx);
        } else if (test_type < 75) {
            // 25% - Write to shared file (append)
            test_write_shared_file(ctx);
        } else {
            // 25% - Read from shared file
            test_read_shared_file(ctx);
        }

        // Small delay to reduce contention
        if (i % 10 == 0) {
            usleep(100);
        }
    }

    ts_printf("[Thread %d] Completed\n", ctx->thread_id);
    return NULL;
}

// ============================================
// Statistics
// ============================================

void print_statistics(stress_stats *stats, double elapsed_time) {
    printf("\n");
    printf("========================================\n");
    printf("Stress Test Results (Read/Write Focus)\n");
    printf("========================================\n");
    printf("Total Operations:     %u\n", atomic_load(&stats->total_ops));
    printf("Successful:           %u\n", atomic_load(&stats->successful_ops));
    printf("Failed:               %u\n", atomic_load(&stats->failed_ops));
    printf("\n");
    printf("Operation Breakdown:\n");
    printf("  Write Operations:   %u\n", atomic_load(&stats->writes));
    printf("  Read Operations:    %u\n", atomic_load(&stats->reads));
    printf("  Write Bytes:        %u\n", atomic_load(&stats->write_bytes));
    printf("  Read Bytes:         %u\n", atomic_load(&stats->read_bytes));
    printf("\n");
    printf("Performance:\n");
    printf("  Elapsed Time:       %.2f seconds\n", elapsed_time);
    printf("  Operations/sec:     %.2f\n",
           atomic_load(&stats->total_ops) / elapsed_time);
    printf("  Write Throughput:   %.2f KB/s\n",
           (atomic_load(&stats->write_bytes) / 1024.0) / elapsed_time);
    printf("  Read Throughput:    %.2f KB/s\n",
           (atomic_load(&stats->read_bytes) / 1024.0) / elapsed_time);
    printf("========================================\n");

    if (atomic_load(&stats->failed_ops) > 0) {
        printf("\nWARNING: %u operations failed!\n",
               atomic_load(&stats->failed_ops));

        printf("\nSpecific API Failures:\n");
        uint32_t failed_touch = atomic_load(&stats->failed_touch);
        uint32_t failed_open = atomic_load(&stats->failed_open);
        uint32_t failed_write = atomic_load(&stats->failed_write);
        uint32_t failed_read = atomic_load(&stats->failed_read);
        uint32_t failed_verify = atomic_load(&stats->failed_verify);

        if (failed_touch > 0) printf("  fs_touch:           %u\n", failed_touch);
        if (failed_open > 0) printf("  file_open:          %u\n", failed_open);
        if (failed_write > 0) printf("  file_write:         %u\n", failed_write);
        if (failed_read > 0) printf("  file_read:          %u\n", failed_read);
        if (failed_verify > 0) printf("  Data Verify:        %u\n", failed_verify);

        // Show recent error messages
        uint32_t total_errors = atomic_load(&stats->error_log_index);
        uint32_t errors_to_show = (total_errors < MAX_ERROR_LOGS) ? total_errors : MAX_ERROR_LOGS;

        if (errors_to_show > 0) {
            printf("\nRecent Error Messages (last %u):\n", errors_to_show);
            for (uint32_t i = 0; i < errors_to_show; i++) {
                uint32_t idx = (total_errors - errors_to_show + i) % MAX_ERROR_LOGS;
                if (stats->error_logs[idx].message[0] != '\0') {
                    printf("  [Thread %d] %s\n",
                           stats->error_logs[idx].thread_id,
                           stats->error_logs[idx].message);
                }
            }
        }
    } else {
        printf("\nAll operations completed successfully!\n");
    }
}

void cleanup_shared_files(filesystem *fs) {
    ts_printf("\nCleaning up shared files...\n");
    int cleaned = 0;

    for (int i = 0; i < MAX_FILES; i++) {
        char path[256];
        snprintf(path, sizeof(path), "/shared_file_%d.txt", i);

        if (fs_exists(fs, path) == OK) {
            if (fs_unlink(fs, path) == OK) {
                cleaned++;
            }
        }
    }

    ts_printf("Cleaned up %d shared files\n", cleaned);
}

// ============================================
// Main
// ============================================

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <disk_id> <block_size> <thread_count> <iterations>\n", argv[0]);
        fprintf(stderr, "\nExample:\n");
        fprintf(stderr, "  %s 1 4096 4 100\n", argv[0]);
        return 1;
    }

    int32_t disk_id = atoi(argv[1]);
    int32_t block_size = atoi(argv[2]);
    int thread_count = atoi(argv[3]);
    int iterations = atoi(argv[4]);

    printf("========================================\n");
    printf("Filesystem Stress Test (Read/Write)\n");
    printf("========================================\n");
    printf("Disk ID:     %d\n", disk_id);
    printf("Block Size:  %d bytes\n", block_size);
    printf("Threads:     %d\n", thread_count);
    printf("Iterations:  %d per thread\n", iterations);
    printf("Total Ops:   ~%d\n", thread_count * iterations);
    printf("========================================\n\n");

    // Initialize disk and filesystem
    disk *dd = (disk*)malloc(sizeof(disk));
    filesystem *fs = (filesystem*)malloc(sizeof(filesystem));

    if (!dd || !fs) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }

    memset(dd, 0, sizeof(disk));
    memset(fs, 0, sizeof(filesystem));

    if (dattach(dd, block_size, disk_id) != OK) {
        fprintf(stderr, "Error: Failed to attach to disk %d\n", disk_id);
        free(dd);
        free(fs);
        return 1;
    }

    if (fs_mount(dd, fs) != OK) {
        fprintf(stderr, "Error: Failed to mount filesystem\n");
        ddetach(dd);
        free(dd);
        free(fs);
        return 1;
    }

    printf("Filesystem mounted successfully\n");
    fs_show(fs);
    printf("\n");

    // Initialize global file table
    file_table_init();

    // Initialize statistics
    memset(&g_stats, 0, sizeof(g_stats));

    // Create threads
    pthread_t *threads = malloc(sizeof(pthread_t) * thread_count);
    thread_context *contexts = malloc(sizeof(thread_context) * thread_count);

    if (!threads || !contexts) {
        fprintf(stderr, "Error: Failed to allocate thread memory\n");
        fs_unmount(fs);
        ddetach(dd);
        free(dd);
        free(fs);
        return 1;
    }

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Launch threads
    for (int i = 0; i < thread_count; i++) {
        contexts[i].fs = fs;
        contexts[i].thread_id = i;
        contexts[i].iterations = iterations;
        contexts[i].stats = &g_stats;
        contexts[i].seed = (unsigned int)(time(NULL) + i);

        if (pthread_create(&threads[i], NULL, stress_worker, &contexts[i]) != 0) {
            fprintf(stderr, "Error: Failed to create thread %d\n", i);
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            free(contexts);
            fs_unmount(fs);
            ddetach(dd);
            free(dd);
            free(fs);
            return 1;
        }
    }

    // Wait for threads
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed = (end_time.tv_sec - start_time.tv_sec) +
                    (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    // Print results
    print_statistics(&g_stats, elapsed);

    // Show file table status
    printf("\nGlobal File Table:\n");
    file_table_show();

    // Cleanup
    cleanup_shared_files(fs);

    free(threads);
    free(contexts);

    if (fs_unmount(fs) != OK) {
        fprintf(stderr, "Warning: Failed to unmount filesystem\n");
    }

    if (ddetach(dd) != OK) {
        fprintf(stderr, "Warning: Failed to detach disk\n");
    }

    free(dd);
    free(fs);

    printf("\nAll cleanup completed\n");

    return (atomic_load(&g_stats.failed_ops) > 0) ? 1 : 0;
}
