// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "bench_zram.h"
#include "cbench.h"
#include "utils.h"
#include "topology.h"
#include "energy.h"
#include "report.h"

#define PAGE_SIZE 4096
// Allocate 512MB per thread to force heavy memory pressure and potential swapping
#define ALLOC_SIZE (512 * 1024 * 1024) 

struct zram_args {
    int thread_id;
    int duration;
    uint64_t pages_touched;
};

static void *zram_worker(void *arg) {
    struct zram_args *args = arg;
    topology_bind_thread(args->thread_id);
    
    char *mem = malloc(ALLOC_SIZE);
    if (!mem) return NULL;
    
    uint64_t end_time = get_time_ns() + ((uint64_t)args->duration * 1000000000ULL);
    int num_pages = ALLOC_SIZE / PAGE_SIZE;
    
    // Seed for uncompressible random data
    unsigned int seed = 12345 + args->thread_id;
    
    while (get_time_ns() < end_time) {
        // Forward pass: Write alternating compressible (zeros) and uncompressible data
        for (int i = 0; i < num_pages; i += 2) {
            // Compressible page
            memset(mem + (i * PAGE_SIZE), 0, PAGE_SIZE);
            // Uncompressible page
            if (i + 1 < num_pages) {
                int *p = (int *)(mem + ((i + 1) * PAGE_SIZE));
                for (int j = 0; j < PAGE_SIZE / sizeof(int); j++) {
                    p[j] = rand_r(&seed);
                }
            }
            args->pages_touched += 2;
        }
        
        // Reverse pass: read to force page-ins if swapped out
        volatile char sink = 0;
        for (int i = num_pages - 1; i >= 0; i--) {
            sink ^= mem[i * PAGE_SIZE];
        }
        (void)sink;
    }
    
    free(mem);
    return NULL;
}

void run_zram_benchmark(int num_threads, int duration) {
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    struct zram_args *args = malloc(sizeof(struct zram_args) * num_threads);
    if (!threads || !args) {
        if (threads) free(threads);
        if (args) free(args);
        return;
    }
    
    pr_info("Running ZRAM Compression Stress Benchmark across %d thread(s) for %d seconds...\n", num_threads, duration);
    pr_info("Note: Allocating %d MB per thread (%d MB total).\n", ALLOC_SIZE / 1024 / 1024, (ALLOC_SIZE / 1024 / 1024) * num_threads);
    
    uint64_t start = get_time_ns();
    for (int i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].duration = duration;
        args[i].pages_touched = 0;
        pthread_create(&threads[i], NULL, zram_worker, &args[i]);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    uint64_t stop = get_time_ns();
    
    uint64_t total_pages = 0;
    for (int i = 0; i < num_threads; i++) total_pages += args[i].pages_touched;
    
    double time_sec = (double)(stop - start) / 1000000000.0;
    double throughput = (double)(total_pages * PAGE_SIZE) / (1024.0 * 1024.0 * time_sec);
    
    pr_info("Total Pages Processed: %llu\n", (unsigned long long)total_pages);
    pr_info("Throughput: %.2f MB/sec\n", throughput);
    
    report_add_metric("zram", "compression_throughput", throughput, "MB/s");
    
    double j = energy_get_joules();
    if (j > 0.0) {
        pr_info("Energy consumed: %.4f Joules\n", j);
        pr_info("Efficiency: %.2f MB/Joule\n", throughput / j);
        report_add_metric("zram", "energy_joules", j, "J");
        report_add_metric("zram", "efficiency_mb_per_j", throughput / j, "MB/J");
    }
    
    free(threads);
    free(args);
}
