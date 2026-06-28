// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "bench_neon.h"
#include "cbench.h"
#include "utils.h"
#include "topology.h"
#include "energy.h"
#include "report.h"

#ifdef __aarch64__
#include <arm_neon.h>
#endif

struct neon_args {
    int thread_id;
    int duration;
    uint64_t ops;
};

static void *neon_worker(void *arg) {
    struct neon_args *args = arg;
    topology_bind_thread(args->thread_id);
    
    uint64_t end_time = get_time_ns() + ((uint64_t)args->duration * 1000000000ULL);
    
#ifdef __aarch64__
    float32x4_t a = vdupq_n_f32(1.0001f);
    float32x4_t b = vdupq_n_f32(1.0002f);
    float32x4_t c = vdupq_n_f32(0.0f);
    
    while (get_time_ns() < end_time) {
        for (int i = 0; i < 10000; i++) {
            // Unroll loop for maximum IPC and heat generation
            c = vmlaq_f32(c, a, b);
            c = vmlaq_f32(c, a, b);
            c = vmlaq_f32(c, a, b);
            c = vmlaq_f32(c, a, b);
            c = vmlaq_f32(c, a, b);
            c = vmlaq_f32(c, a, b);
            c = vmlaq_f32(c, a, b);
            c = vmlaq_f32(c, a, b);
        }
        args->ops += 80000; // 8 ops per iteration * 10000
    }
    // Prevent compiler from optimizing away the result
    volatile float res = vgetq_lane_f32(c, 0);
    (void)res;
#else
    float a = 1.0001f;
    float b = 1.0002f;
    float c = 0.0f;
    
    while (get_time_ns() < end_time) {
        for (int i = 0; i < 10000; i++) {
            c += a * b;
            c += a * b;
            c += a * b;
            c += a * b;
            c += a * b;
            c += a * b;
            c += a * b;
            c += a * b;
        }
        args->ops += 80000;
    }
    volatile float res = c;
    (void)res;
#endif

    return NULL;
}

void run_neon_benchmark(int num_threads, int duration) {
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    struct neon_args *args = malloc(sizeof(struct neon_args) * num_threads);
    if (!threads || !args) {
        if (threads) free(threads);
        if (args) free(args);
        return;
    }
    
    pr_info("Running Thermal Throttling Matrix Benchmark (SIMD/NEON) across %d thread(s) for %d seconds...\n", num_threads, duration);
    
    uint64_t start = get_time_ns();
    for (int i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].duration = duration;
        args[i].ops = 0;
        pthread_create(&threads[i], NULL, neon_worker, &args[i]);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    uint64_t stop = get_time_ns();
    
    uint64_t total_ops = 0;
    for (int i = 0; i < num_threads; i++) total_ops += args[i].ops;
    
    double time_sec = (double)(stop - start) / 1000000000.0;
    double throughput = (double)total_ops / time_sec;
    
    pr_info("Total Matrix Operations: %llu\n", (unsigned long long)total_ops);
    pr_info("Throughput: %.2f ops/sec\n", throughput);
    
    report_add_metric("neon", "matrix_ops_throughput", throughput, "ops/sec");
    
    double j = energy_get_joules();
    if (j > 0.0) {
        pr_info("Energy consumed: %.4f Joules\n", j);
        pr_info("Efficiency: %.2f ops/Joule\n", throughput / j);
        report_add_metric("neon", "energy_joules", j, "J");
        report_add_metric("neon", "efficiency_ops_per_j", throughput / j, "ops/J");
    }
    
    free(threads);
    free(args);
}
