#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "cbench.h"
#include "utils.h"
#include "bench_io.h"

#define IO_FILE_SIZE (128 * 1024 * 1024) /* 128 MB */
#define IO_BLOCK_SIZE (64 * 1024)        /* 64 KB */

int run_io_benchmark(void)
{
    int fd;
    char *buf;
    uint64_t start, end;
    double total_ns, bw_mb_s;
    size_t i;
    const char *test_file = "cbench_io_test.tmp";

    pr_info("Running I/O Subsystem Benchmarks...\n");

    buf = aligned_alloc(4096, IO_BLOCK_SIZE);
    if (!buf) {
        pr_err("Failed to allocate I/O buffer\n");
        return -1;
    }
    memset(buf, 0xBB, IO_BLOCK_SIZE);

    /* 1. Sequential Write (Buffered) */
    fd = open(test_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        pr_err("Failed to open test file for writing\n");
        free(buf);
        return -1;
    }

    start = get_time_ns();
    for (i = 0; i < IO_FILE_SIZE; i += IO_BLOCK_SIZE) {
        if (write(fd, buf, IO_BLOCK_SIZE) != IO_BLOCK_SIZE) {
            pr_err("Write failed\n");
            break;
        }
    }
    fsync(fd); /* Ensure it hits the disk */
    end = get_time_ns();
    close(fd);

    total_ns = (double)(end - start);
    bw_mb_s = (IO_FILE_SIZE / (1024.0 * 1024.0)) / (total_ns / 1000000000.0);
    pr_info("Sequential Write (Buffered + fsync): %.2f MB/s\n", bw_mb_s);

    /* 2. Sequential Read (Cached usually) */
    fd = open(test_file, O_RDONLY);
    if (fd < 0) {
        pr_err("Failed to open test file for reading\n");
        free(buf);
        return -1;
    }

    start = get_time_ns();
    for (i = 0; i < IO_FILE_SIZE; i += IO_BLOCK_SIZE) {
        if (read(fd, buf, IO_BLOCK_SIZE) != IO_BLOCK_SIZE) {
            pr_err("Read failed\n");
            break;
        }
    }
    end = get_time_ns();
    close(fd);

    total_ns = (double)(end - start);
    bw_mb_s = (IO_FILE_SIZE / (1024.0 * 1024.0)) / (total_ns / 1000000000.0);
    pr_info("Sequential Read (Cached): %.2f MB/s\n", bw_mb_s);

    unlink(test_file);
    free(buf);

    return 0;
}
