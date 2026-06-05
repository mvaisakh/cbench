// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include "bench_rng.h"
#include "cbench.h"
#include "utils.h"
#include "topology.h"
#include "energy.h"
#include "profiler.h"
#include "report.h"
#include "cpuidle.h"

struct rng_args {
    int thread_id;
    int duration;
    uint64_t bytes_read;
};

static void *rng_worker(void *arg) {
    struct rng_args *args = arg;
    topology_bind_thread(args->thread_id);
    
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return NULL;
    
    char buf[4096];
    uint64_t end_time = get_time_ns() + ((uint64_t)args->duration * 1000000000ULL);
    
    while (get_time_ns() < end_time) {
        ssize_t ret = read(fd, buf, sizeof(buf));
        if (ret > 0) args->bytes_read += ret;
    }
    close(fd);
    return NULL;
}

void run_rng_benchmark(int num_threads, int duration) {
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    struct rng_args *args = malloc(sizeof(struct rng_args) * num_threads);
    
    pr_info("Running RNG Subsystem Benchmark (/dev/urandom) across %d thread(s) for %d seconds...\n", num_threads, duration);
    
    cpuidle_start();
    energy_start();
    profiler_start();
    
    uint64_t start = get_time_ns();
    for (int i=0; i<num_threads; i++) {
        args[i].thread_id = i;
        args[i].duration = duration;
        args[i].bytes_read = 0;
        pthread_create(&threads[i], NULL, rng_worker, &args[i]);
    }
    for (int i=0; i<num_threads; i++) pthread_join(threads[i], NULL);
    uint64_t stop = get_time_ns();
    
    profiler_stop("rng");
    energy_stop();
    cpuidle_stop("rng");
    
    uint64_t total_bytes = 0;
    for (int i=0; i<num_threads; i++) total_bytes += args[i].bytes_read;
    double time_sec = (double)(stop - start) / 1000000000.0;
    double bw = (double)total_bytes / (1024.0 * 1024.0) / time_sec;
    
    pr_info("Total /dev/urandom read bandwidth: %.2f MB/s\n", bw);
    
    double j = energy_get_joules();
    if (j > 0.0) {
        pr_info("Energy consumed: %.4f Joules\n", j);
        pr_info("Efficiency: %.2f MB_urandom/Joule\n", bw * time_sec / j);
        report_add_metric("rng", "energy_joules", j, "J");
        report_add_metric("rng", "efficiency_mb_per_j", bw * time_sec / j, "MB/J");
    }
    
    report_add_metric("rng", "urandom_bw", bw, "MB/s");
    
    free(threads);
    free(args);
}
