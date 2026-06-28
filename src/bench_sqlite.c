// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include "bench_sqlite.h"
#include "cbench.h"
#include "utils.h"
#include "topology.h"
#include "energy.h"
#include "report.h"

struct sqlite_args {
    int thread_id;
    int duration;
    uint64_t ops;
};

static void *sqlite_worker(void *arg) {
    struct sqlite_args *args = arg;
    topology_bind_thread(args->thread_id);
    
    char path[128];
    snprintf(path, sizeof(path), "/tmp/cbench_sqlite_emulation_%d.db", args->thread_id);
    
    int fd = open(path, O_CREAT | O_RDWR | O_SYNC, 0600);
    if (fd < 0) {
        return NULL;
    }
    
    // SQLite WAL style: 4KB block sizes written and immediately fsynced
    char buf[4096];
    memset(buf, 'D', sizeof(buf));
    
    uint64_t end_time = get_time_ns() + ((uint64_t)args->duration * 1000000000ULL);
    
    while (get_time_ns() < end_time) {
        if (pwrite(fd, buf, sizeof(buf), 0) != sizeof(buf)) {
            break;
        }
        if (fdatasync(fd) < 0) {
            break;
        }
        args->ops++;
    }
    
    close(fd);
    unlink(path);
    return NULL;
}

void run_sqlite_benchmark(int num_threads, int duration) {
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    struct sqlite_args *args = malloc(sizeof(struct sqlite_args) * num_threads);
    if (!threads || !args) {
        if (threads) free(threads);
        if (args) free(args);
        return;
    }
    
    pr_info("Running SQLite WAL Emulation Benchmark (fsync stress) across %d thread(s) for %d seconds...\n", num_threads, duration);
    
    uint64_t start = get_time_ns();
    for (int i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].duration = duration;
        args[i].ops = 0;
        pthread_create(&threads[i], NULL, sqlite_worker, &args[i]);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    uint64_t stop = get_time_ns();
    
    uint64_t total_ops = 0;
    for (int i = 0; i < num_threads; i++) total_ops += args[i].ops;
    
    double time_sec = (double)(stop - start) / 1000000000.0;
    double throughput = (double)total_ops / time_sec;
    
    pr_info("Total SQLite Transactions: %llu\n", (unsigned long long)total_ops);
    pr_info("Throughput: %.2f ops/sec\n", throughput);
    
    report_add_metric("sqlite", "transaction_throughput", throughput, "ops/sec");
    
    if (throughput < 100.0) {
        report_add_heuristic("sqlite", "Severe fsync bottleneck detected. If on EXT4, consider F2FS or enabling EXT4 fast_commit.");
    }
    
    double j = energy_get_joules();
    if (j > 0.0) {
        pr_info("Energy consumed: %.4f Joules\n", j);
        pr_info("Efficiency: %.2f ops/Joule\n", throughput / j);
        report_add_metric("sqlite", "energy_joules", j, "J");
        report_add_metric("sqlite", "efficiency_ops_per_j", throughput / j, "ops/J");
    }
    
    free(threads);
    free(args);
}
