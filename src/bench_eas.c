// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include "bench_eas.h"
#include "cbench.h"
#include "utils.h"
#include "topology.h"
#include "energy.h"
#include "report.h"

struct eas_ctx {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int turn;
    int duration;
    uint64_t ping_pongs;
    int running;
};

static void *eas_worker_little(void *arg) {
    struct eas_ctx *ctx = arg;
    // Bind to the very first core (usually LITTLE on big.LITTLE architectures)
    topology_bind_thread(0);
    
    while (ctx->running) {
        pthread_mutex_lock(&ctx->lock);
        while (ctx->turn != 0 && ctx->running) {
            pthread_cond_wait(&ctx->cond, &ctx->lock);
        }
        if (!ctx->running) {
            pthread_mutex_unlock(&ctx->lock);
            break;
        }
        ctx->turn = 1;
        pthread_cond_signal(&ctx->cond);
        pthread_mutex_unlock(&ctx->lock);
    }
    return NULL;
}

static void *eas_worker_big(void *arg) {
    struct eas_ctx *ctx = arg;
    // Bind to the very last core (usually prime/big on big.LITTLE architectures)
    topology_bind_thread(system_topo.total_cpus - 1);
    
    while (ctx->running) {
        pthread_mutex_lock(&ctx->lock);
        while (ctx->turn != 1 && ctx->running) {
            pthread_cond_wait(&ctx->cond, &ctx->lock);
        }
        if (!ctx->running) {
            pthread_mutex_unlock(&ctx->lock);
            break;
        }
        ctx->ping_pongs++;
        ctx->turn = 0;
        pthread_cond_signal(&ctx->cond);
        pthread_mutex_unlock(&ctx->lock);
    }
    return NULL;
}

void run_eas_benchmark(int duration) {
    if (system_topo.total_cpus < 2) {
        pr_info("Skipping EAS Ping-Pong benchmark (requires at least 2 cores).\n");
        return;
    }

    struct eas_ctx ctx;
    pthread_mutex_init(&ctx.lock, NULL);
    pthread_cond_init(&ctx.cond, NULL);
    ctx.turn = 0;
    ctx.duration = duration;
    ctx.ping_pongs = 0;
    ctx.running = 1;
    
    pthread_t thread_little, thread_big;
    
    pr_info("Running EAS Inter-Core Ping-Pong Benchmark (Core 0 <-> Core %d) for %d seconds...\n", 
            system_topo.total_cpus - 1, duration);
    
    uint64_t start = get_time_ns();
    pthread_create(&thread_little, NULL, eas_worker_little, &ctx);
    pthread_create(&thread_big, NULL, eas_worker_big, &ctx);
    
    // Main thread acts as timer
    uint64_t end_time = start + ((uint64_t)duration * 1000000000ULL);
    while (get_time_ns() < end_time) {
        // Sleep in chunks to avoid burning CPU
        struct timespec ts = {0, 10000000}; // 10ms
        nanosleep(&ts, NULL);
    }
    
    pthread_mutex_lock(&ctx.lock);
    ctx.running = 0;
    pthread_cond_broadcast(&ctx.cond);
    pthread_mutex_unlock(&ctx.lock);
    
    pthread_join(thread_little, NULL);
    pthread_join(thread_big, NULL);
    uint64_t stop = get_time_ns();
    
    double time_sec = (double)(stop - start) / 1000000000.0;
    double throughput = (double)ctx.ping_pongs / time_sec;
    double latency_ns = 1000000000.0 / throughput;
    
    pr_info("Total Ping-Pongs: %llu\n", (unsigned long long)ctx.ping_pongs);
    pr_info("Throughput: %.2f ops/sec\n", throughput);
    pr_info("Avg Latency (round-trip): %.2f ns\n", latency_ns);
    
    report_add_metric("eas", "ping_pong_throughput", throughput, "ops/sec");
    report_add_metric("eas", "ping_pong_latency", latency_ns, "ns");
    
    if (latency_ns > 20000.0) {
        report_add_heuristic("eas", "High inter-cluster wakeup latency detected. Check WALT/PELT tuning or governor ramp-up rates.");
    }
    
    pthread_mutex_destroy(&ctx.lock);
    pthread_cond_destroy(&ctx.cond);
}
