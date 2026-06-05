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
#include "topology.h"
#include "energy.h"

#define MEM_SIZE_PER_THREAD (128 * 1024 * 1024) /* 128 MB per thread */
#define PAGE_SIZE 4096

struct mem_worker_args {
    int thread_id;
    double fault_ms;
    double write_bw;
    double copy_bw;
};

static void *mem_worker(void *arg)
{
    struct mem_worker_args *args = (struct mem_worker_args *)arg;
    void *mem1, *mem2;
    size_t i;
    uint64_t start, end;
    double total_ns;

    topology_bind_thread(args->thread_id);

    /* 1. Page Faults */
    start = get_time_ns();
    mem1 = mmap(NULL, MEM_SIZE_PER_THREAD, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem1 == MAP_FAILED) return NULL;
    
    volatile char *ptr = (volatile char *)mem1;
    for (i = 0; i < MEM_SIZE_PER_THREAD; i += PAGE_SIZE) {
        ptr[i] = 1;
    }
    end = get_time_ns();
    total_ns = (double)(end - start);
    args->fault_ms = total_ns / 1000000.0;

    /* 2. Sequential Write */
    start = get_time_ns();
    memset(mem1, 0xAA, MEM_SIZE_PER_THREAD);
    end = get_time_ns();
    total_ns = (double)(end - start);
    args->write_bw = (MEM_SIZE_PER_THREAD / (1024.0 * 1024.0)) / (total_ns / 1000000000.0);

    /* 3. Sequential Copy */
    mem2 = mmap(NULL, MEM_SIZE_PER_THREAD, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem2 != MAP_FAILED) {
        memset(mem2, 0, MEM_SIZE_PER_THREAD);
        
        start = get_time_ns();
        memcpy(mem2, mem1, MEM_SIZE_PER_THREAD);
        end = get_time_ns();

        total_ns = (double)(end - start);
        args->copy_bw = (MEM_SIZE_PER_THREAD / (1024.0 * 1024.0)) / (total_ns / 1000000000.0);
        munmap(mem2, MEM_SIZE_PER_THREAD);
    }

    munmap(mem1, MEM_SIZE_PER_THREAD);
    return NULL;
}

int run_mem_benchmark(void)
{
    pthread_t *threads;
    struct mem_worker_args *args;
    int i;
    double total_write_bw = 0.0, total_copy_bw = 0.0;

    pr_info("Running Memory Subsystem Benchmarks across %d thread(s)...\n", num_threads);

    threads = malloc(sizeof(pthread_t) * num_threads);
    args = malloc(sizeof(struct mem_worker_args) * num_threads);
    if (!threads || !args) return -1;

    energy_start();

    for (i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].fault_ms = 0;
        args[i].write_bw = 0;
        args[i].copy_bw = 0;
        pthread_create(&threads[i], NULL, mem_worker, &args[i]);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        total_write_bw += args[i].write_bw;
        total_copy_bw += args[i].copy_bw;
    }

    energy_stop();
    double joules = energy_get_joules();

    pr_info("Total Aggregated Write Bandwidth: %.2f MB/s\n", total_write_bw);
    pr_info("Total Aggregated Copy Bandwidth: %.2f MB/s\n", total_copy_bw);
    
    report_add_metric("mem", "agg_seq_write_bw", total_write_bw, "MB/s");
    report_add_metric("mem", "agg_memcpy_bw", total_copy_bw, "MB/s");

    if (joules > 0.0) {
        /* Calculate efficiency: Megabytes copied per Joule */
        double total_mb_copied = (MEM_SIZE_PER_THREAD * num_threads) / (1024.0 * 1024.0);
        pr_info("Energy consumed: %.4f Joules\n", joules);
        pr_info("Efficiency: %.2f MB_copied/Joule\n", total_mb_copied / joules);
        report_add_metric("mem", "energy_joules", joules, "J");
        report_add_metric("mem", "efficiency_mb_per_j", total_mb_copied / joules, "MB/J");
    }

    free(threads);
    free(args);

    return 0;
}
