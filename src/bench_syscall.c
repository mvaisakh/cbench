// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include "cbench.h"
#include "utils.h"
#include "bench_syscall.h"
#include "report.h"
#include "telemetry.h"
#include "topology.h"
#include "energy.h"

struct syscall_worker_args {
    int thread_id;
    uint64_t iters;
};

static void *syscall_worker(void *arg)
{
    struct syscall_worker_args *args = (struct syscall_worker_args *)arg;
    int i;
    
    args->iters = 0;
    topology_bind_thread(args->thread_id);

    uint64_t end_time = get_time_ns() + ((uint64_t)benchmark_duration_sec * 1000000000ULL);

    while (get_time_ns() < end_time) {
        for (i = 0; i < 10000; i++) {
            syscall(SYS_gettid);
        }
        args->iters += 10000;
    }
    
    return NULL;
}

int run_syscall_benchmark(void)
{
    pthread_t *threads;
    struct syscall_worker_args *args;
    uint64_t start, end;
    int i;

    pr_info("Running Syscall Latency Benchmark (SYS_gettid) across %d thread(s) for %d seconds...\n", num_threads, benchmark_duration_sec);

    threads = malloc(sizeof(pthread_t) * num_threads);
    args = malloc(sizeof(struct syscall_worker_args) * num_threads);
    if (!threads || !args) {
        if (threads) free(threads);
        if (args) free(args);
        return -1;
    }

    start = get_time_ns();

    for (i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        pthread_create(&threads[i], NULL, syscall_worker, &args[i]);
    }

    uint64_t total_iters = 0;
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        total_iters += args[i].iters;
    }

    end = get_time_ns();

    uint64_t total_ns = end - start;
    double throughput = (double)total_iters / (total_ns / 1000000000.0);
    double joules = energy_get_joules();

    pr_info("Syscall SYS_gettid: %llu total iterations\n", (unsigned long long)total_iters);
    pr_info("Total wall time: %llu ns\n", (unsigned long long)total_ns);
    pr_info("Throughput: %.2f ops/sec\n", throughput);
    
    report_add_metric("syscall", "gettid_throughput", throughput, "ops/sec");
    
    if (joules > 0.0) {
        pr_info("Energy consumed: %.4f Joules\n", joules);
        pr_info("Efficiency: %.2f ops/Joule\n", throughput / joules);
        report_add_metric("syscall", "energy_joules", joules, "J");
        report_add_metric("syscall", "efficiency_ops_per_j", throughput / joules, "ops/J");
    }

    free(threads);
    free(args);

    return 0;
}
