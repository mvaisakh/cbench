// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include "cbench.h"
#include "utils.h"
#include "bench_mem.h"
#include "report.h"
#include "telemetry.h"
#include "topology.h"
#include "energy.h"

#define MEM_SIZE_PER_THREAD (64 * 1024 * 1024) /* 64 MB per thread for cache thrashing */
#define PAGE_SIZE 4096

struct mem_worker_args {
    int thread_id;
    double write_bw;
    double copy_bw;
};

static void *mem_worker(void *arg)
{
    struct mem_worker_args *args = (struct mem_worker_args *)arg;
    void *mem1, *mem2;
    size_t i;
    uint64_t end_time;

    topology_bind_thread(args->thread_id);

    mem1 = mmap(NULL, MEM_SIZE_PER_THREAD, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    mem2 = mmap(NULL, MEM_SIZE_PER_THREAD, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem1 == MAP_FAILED || mem2 == MAP_FAILED) return NULL;

    /* Fault pages */
    volatile char *ptr = (volatile char *)mem1;
    for (i = 0; i < MEM_SIZE_PER_THREAD; i += PAGE_SIZE) ptr[i] = 1;
    ptr = (volatile char *)mem2;
    for (i = 0; i < MEM_SIZE_PER_THREAD; i += PAGE_SIZE) ptr[i] = 1;

    /* 1. Sequential Write */
    uint64_t write_bytes = 0;
    uint64_t start = get_time_ns();
    end_time = start + ((uint64_t)benchmark_duration_sec * 1000000000ULL);
    while (get_time_ns() < end_time) {
        memset(mem1, 0xAA, MEM_SIZE_PER_THREAD);
        write_bytes += MEM_SIZE_PER_THREAD;
    }
    double total_ns = (double)(get_time_ns() - start);
    args->write_bw = (write_bytes / (1024.0 * 1024.0)) / (total_ns / 1000000000.0);

    /* 2. Sequential Copy */
    uint64_t copy_bytes = 0;
    start = get_time_ns();
    end_time = start + ((uint64_t)benchmark_duration_sec * 1000000000ULL);
    while (get_time_ns() < end_time) {
        memcpy(mem2, mem1, MEM_SIZE_PER_THREAD);
        copy_bytes += MEM_SIZE_PER_THREAD;
    }
    total_ns = (double)(get_time_ns() - start);
    args->copy_bw = (copy_bytes / (1024.0 * 1024.0)) / (total_ns / 1000000000.0);

    munmap(mem1, MEM_SIZE_PER_THREAD);
    munmap(mem2, MEM_SIZE_PER_THREAD);
    return NULL;
}

int run_mem_benchmark(void)
{
    pthread_t *threads;
    struct mem_worker_args *args;
    int i;
    double total_write_bw = 0.0, total_copy_bw = 0.0;

    pr_info("Running Memory Subsystem Benchmarks across %d thread(s) for %d seconds...\n", num_threads, benchmark_duration_sec);

    threads = malloc(sizeof(pthread_t) * num_threads);
    args = malloc(sizeof(struct mem_worker_args) * num_threads);
    if (!threads || !args) return -1;


    for (i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].write_bw = 0;
        args[i].copy_bw = 0;
        pthread_create(&threads[i], NULL, mem_worker, &args[i]);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        total_write_bw += args[i].write_bw;
        total_copy_bw += args[i].copy_bw;
    }

    double joules = energy_get_joules();

    pr_info("Total Aggregated Write Bandwidth: %.2f MB/s\n", total_write_bw);
    pr_info("Total Aggregated Copy Bandwidth: %.2f MB/s\n", total_copy_bw);
    
    report_add_metric("mem", "agg_seq_write_bw", total_write_bw, "MB/s");
    report_add_metric("mem", "agg_memcpy_bw", total_copy_bw, "MB/s");

    if (joules > 0.0) {
        /* Estimate MB copied based on BW and duration */
        double total_mb_copied = total_copy_bw * benchmark_duration_sec;
        pr_info("Energy consumed: %.4f Joules\n", joules);
        pr_info("Efficiency: %.2f MB_copied/Joule\n", total_mb_copied / joules);
        report_add_metric("mem", "energy_joules", joules, "J");
        report_add_metric("mem", "efficiency_mb_per_j", total_mb_copied / joules, "MB/J");
    }

    free(threads);
    free(args);

    return 0;
}
