// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include "bench_futex.h"
#include "cbench.h"
#include "utils.h"
#include "topology.h"
#include "energy.h"
#include "profiler.h"
#include "report.h"
#include "cpuidle.h"

struct futex_args {
    int thread_id;
    int duration;
    uint64_t wakes;
};

static int shared_futex_val = 0;

static void *futex_worker(void *arg) {
    struct futex_args *args = arg;
    topology_bind_thread(args->thread_id);
    
    uint64_t end_time = get_time_ns() + ((uint64_t)args->duration * 1000000000ULL);
    
    while (get_time_ns() < end_time) {
        for(int i=0; i<1000; i++) {
            // Rapid wake on a single shared memory address to heavily contend the kernel futex hash bucket spinlocks
            syscall(SYS_futex, &shared_futex_val, FUTEX_WAKE, 1, NULL, NULL, 0);
        }
        args->wakes += 1000;
    }
    return NULL;
}

void run_futex_benchmark(int num_threads, int duration) {
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    struct futex_args *args = malloc(sizeof(struct futex_args) * num_threads);
    
    pr_info("Running Futex Contention Benchmark (SYS_futex) across %d thread(s) for %d seconds...\n", num_threads, duration);
    
    cpuidle_start();
    energy_start();
    profiler_start();
    
    uint64_t start = get_time_ns();
    for (int i=0; i<num_threads; i++) {
        args[i].thread_id = i;
        args[i].duration = duration;
        args[i].wakes = 0;
        pthread_create(&threads[i], NULL, futex_worker, &args[i]);
    }
    for (int i=0; i<num_threads; i++) pthread_join(threads[i], NULL);
    uint64_t stop = get_time_ns();
    
    profiler_stop("futex");
    energy_stop();
    cpuidle_stop("futex");
    
    uint64_t total_wakes = 0;
    for (int i=0; i<num_threads; i++) total_wakes += args[i].wakes;
    double time_sec = (double)(stop - start) / 1000000000.0;
    double thr = (double)total_wakes / time_sec;
    
    pr_info("Total FUTEX_WAKE operations: %llu\n", (unsigned long long)total_wakes);
    pr_info("Throughput: %.2f ops/sec\n", thr);
    
    double j = energy_get_joules();
    if (j > 0.0) {
        pr_info("Energy consumed: %.4f Joules\n", j);
        pr_info("Efficiency: %.2f ops/Joule\n", thr * time_sec / j);
        report_add_metric("futex", "energy_joules", j, "J");
        report_add_metric("futex", "efficiency_ops_per_j", thr * time_sec / j, "ops/J");
    }
    
    report_add_metric("futex", "futex_throughput", thr, "ops/sec");
    
    free(threads);
    free(args);
}
