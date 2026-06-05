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
#include "topology.h"
#include "energy.h"

#define ITERS_PER_THREAD 10000000

static void *syscall_worker(void *arg)
{
    int thread_id = *(int *)arg;
    int i;
    
    topology_bind_thread(thread_id);

    for (i = 0; i < ITERS_PER_THREAD; i++) {
        syscall(SYS_gettid);
    }
    
    return NULL;
}

int run_syscall_benchmark(void)
{
    pthread_t *threads;
    int *tids;
    uint64_t start, end;
    int i;

    pr_info("Running Syscall Latency Benchmark (SYS_gettid) across %d thread(s)...\n", num_threads);

    threads = malloc(sizeof(pthread_t) * num_threads);
    tids = malloc(sizeof(int) * num_threads);
    if (!threads || !tids) return -1;

    energy_start();
    start = get_time_ns();

    for (i = 0; i < num_threads; i++) {
        tids[i] = i;
        pthread_create(&threads[i], NULL, syscall_worker, &tids[i]);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    end = get_time_ns();
    energy_stop();

    uint64_t total_ns = end - start;
    uint64_t total_iters = (uint64_t)ITERS_PER_THREAD * num_threads;
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
    free(tids);

    return 0;
}
