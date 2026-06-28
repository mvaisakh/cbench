// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include "cbench.h"
#include "utils.h"
#include "bench_rcu.h"
#include "report.h"
#include "topology.h"

struct rcu_worker_args {
    int thread_id;
    uint64_t iters;
};

static void *rcu_worker(void *arg)
{
    struct rcu_worker_args *args = (struct rcu_worker_args *)arg;
    args->iters = 0;
    topology_bind_thread(args->thread_id);

    struct stat st;
    uint64_t end_time = get_time_ns() + ((uint64_t)benchmark_duration_sec * 1000000000ULL);
    
    while (get_time_ns() < end_time) {
        for (int i = 0; i < 100; i++) {
            stat("/", &st);
        }
        args->iters += 100;
    }
    return NULL;
}

int run_rcu_benchmark(int threads, int duration_sec)
{
    pthread_t *pids = malloc(sizeof(pthread_t) * threads);
    struct rcu_worker_args *args = malloc(sizeof(struct rcu_worker_args) * threads);
    if (!pids || !args) {
        if (pids) free(pids);
        if (args) free(args);
        return -1;
    }
    
    pr_info("Running Kernel VFS RCU Stress Benchmark across %d thread(s) for %d seconds...\n", threads, duration_sec);
    uint64_t start = get_time_ns();
    for (int i = 0; i < threads; i++) {
        args[i].thread_id = i;
        pthread_create(&pids[i], NULL, rcu_worker, &args[i]);
    }
    uint64_t total_iters = 0;
    for (int i = 0; i < threads; i++) {
        pthread_join(pids[i], NULL);
        total_iters += args[i].iters;
    }
    uint64_t end = get_time_ns();
    double throughput = (double)total_iters / ((end - start) / 1000000000.0);
    
    pr_info("VFS RCU Pathwalk: %llu total iterations\n", (unsigned long long)total_iters);
    pr_info("Throughput: %.2f ops/sec\n", throughput);
    report_add_metric("rcu", "vfs_rcu_throughput", throughput, "ops/sec");
    
    free(pids);
    free(args);
    return 0;
}
