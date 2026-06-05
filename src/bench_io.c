// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include "cbench.h"
#include "utils.h"
#include "bench_io.h"
#include "report.h"
#include "topology.h"
#include "energy.h"

#define IO_FILE_SIZE_PER_THREAD (64 * 1024 * 1024) /* 64 MB per thread */
#define IO_BLOCK_SIZE (64 * 1024)                  /* 64 KB */

struct io_worker_args {
    int thread_id;
    double write_bw;
    double read_bw;
};

static void *io_worker(void *arg)
{
    struct io_worker_args *args = (struct io_worker_args *)arg;
    int fd;
    char *buf;
    uint64_t start, end;
    double total_ns;
    size_t i;
    char test_file[64];

    topology_bind_thread(args->thread_id);

    snprintf(test_file, sizeof(test_file), "cbench_io_test_%d.tmp", args->thread_id);

    buf = aligned_alloc(4096, IO_BLOCK_SIZE);
    if (!buf) return NULL;
    memset(buf, 0xBB, IO_BLOCK_SIZE);

    /* 1. Sequential Write (Buffered) */
    fd = open(test_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        start = get_time_ns();
        for (i = 0; i < IO_FILE_SIZE_PER_THREAD; i += IO_BLOCK_SIZE) {
            if (write(fd, buf, IO_BLOCK_SIZE) != IO_BLOCK_SIZE) break;
        }
        fsync(fd);
        end = get_time_ns();
        close(fd);

        total_ns = (double)(end - start);
        args->write_bw = (IO_FILE_SIZE_PER_THREAD / (1024.0 * 1024.0)) / (total_ns / 1000000000.0);
    } else {
        args->write_bw = 0;
    }

    /* 2. Sequential Read (Cached) */
    fd = open(test_file, O_RDONLY);
    if (fd >= 0) {
        start = get_time_ns();
        for (i = 0; i < IO_FILE_SIZE_PER_THREAD; i += IO_BLOCK_SIZE) {
            if (read(fd, buf, IO_BLOCK_SIZE) != IO_BLOCK_SIZE) break;
        }
        end = get_time_ns();
        close(fd);

        total_ns = (double)(end - start);
        args->read_bw = (IO_FILE_SIZE_PER_THREAD / (1024.0 * 1024.0)) / (total_ns / 1000000000.0);
    } else {
        args->read_bw = 0;
    }

    unlink(test_file);
    free(buf);

    return NULL;
}

int run_io_benchmark(void)
{
    pthread_t *threads;
    struct io_worker_args *args;
    int i;
    double total_write_bw = 0.0, total_read_bw = 0.0;

    pr_info("Running I/O Subsystem Benchmarks across %d thread(s)...\n", num_threads);

    threads = malloc(sizeof(pthread_t) * num_threads);
    args = malloc(sizeof(struct io_worker_args) * num_threads);
    if (!threads || !args) return -1;

    energy_start();

    for (i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].write_bw = 0;
        args[i].read_bw = 0;
        pthread_create(&threads[i], NULL, io_worker, &args[i]);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        total_write_bw += args[i].write_bw;
        total_read_bw += args[i].read_bw;
    }

    energy_stop();
    double joules = energy_get_joules();

    pr_info("Total Aggregated Write Bandwidth (Buffered + fsync): %.2f MB/s\n", total_write_bw);
    pr_info("Total Aggregated Read Bandwidth (Cached): %.2f MB/s\n", total_read_bw);

    report_add_metric("io", "agg_seq_write_buffered_fsync_bw", total_write_bw, "MB/s");
    report_add_metric("io", "agg_seq_read_cached_bw", total_read_bw, "MB/s");

    if (joules > 0.0) {
        double total_mb_io = (IO_FILE_SIZE_PER_THREAD * 2.0 * num_threads) / (1024.0 * 1024.0);
        pr_info("Energy consumed: %.4f Joules\n", joules);
        pr_info("Efficiency: %.2f MB_io/Joule\n", total_mb_io / joules);
        report_add_metric("io", "energy_joules", joules, "J");
        report_add_metric("io", "efficiency_mb_per_j", total_mb_io / joules, "MB/J");
    }

    free(threads);
    free(args);

    return 0;
}
