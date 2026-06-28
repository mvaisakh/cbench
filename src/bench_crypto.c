// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if_alg.h>
#include <string.h>
#include "bench_crypto.h"
#include "cbench.h"
#include "utils.h"
#include "topology.h"
#include "energy.h"
#include "profiler.h"
#include "report.h"
#include "cpuidle.h"

#ifndef AF_ALG
#define AF_ALG 38
#endif

struct crypto_args {
    int thread_id;
    int duration;
    uint64_t bytes_hashed;
    int supported;
};

static void *crypto_worker(void *arg) {
    struct crypto_args *args = arg;
    topology_bind_thread(args->thread_id);
    args->supported = 0;
    
    int tfmfd = socket(AF_ALG, SOCK_SEQPACKET, 0);
    if (tfmfd < 0) return NULL;
    
    struct sockaddr_alg sa;
    memset(&sa, 0, sizeof(sa));
    sa.salg_family = AF_ALG;
    strncpy((char *)sa.salg_type, "hash", 13);
    strncpy((char *)sa.salg_name, "sha256", 63);
    
    if (bind(tfmfd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        close(tfmfd);
        return NULL;
    }
    
    int opfd = accept(tfmfd, NULL, 0);
    if (opfd < 0) {
        close(tfmfd);
        return NULL;
    }
    args->supported = 1;
    
    char buf[4096] = {0};
    char hash[32];
    uint64_t end_time = get_time_ns() + ((uint64_t)args->duration * 1000000000ULL);
    
    while (get_time_ns() < end_time) {
        if (write(opfd, buf, sizeof(buf)) != sizeof(buf)) break;
        if (read(opfd, hash, sizeof(hash)) != sizeof(hash)) break;
        args->bytes_hashed += sizeof(buf);
    }
    
    close(opfd);
    close(tfmfd);
    return NULL;
}

void run_crypto_benchmark(int num_threads, int duration) {
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    struct crypto_args *args = malloc(sizeof(struct crypto_args) * num_threads);
    if (!threads || !args) {
        if (threads) free(threads);
        if (args) free(args);
        return;
    }
    
    pr_info("Running Crypto Subsystem Benchmark (AF_ALG sha256) across %d thread(s) for %d seconds...\n", num_threads, duration);
    
    
    uint64_t start = get_time_ns();
    for (int i=0; i<num_threads; i++) {
        args[i].thread_id = i;
        args[i].duration = duration;
        args[i].bytes_hashed = 0;
        pthread_create(&threads[i], NULL, crypto_worker, &args[i]);
    }
    for (int i=0; i<num_threads; i++) pthread_join(threads[i], NULL);
    uint64_t stop = get_time_ns();
    
    
    if (!args[0].supported) {
        pr_warn("Kernel Crypto API (AF_ALG) not supported or sha256 not available on this kernel.\n");
        free(threads);
        free(args);
        return;
    }
    
    uint64_t total_bytes = 0;
    for (int i=0; i<num_threads; i++) total_bytes += args[i].bytes_hashed;
    double time_sec = (double)(stop - start) / 1000000000.0;
    double bw = (double)total_bytes / (1024.0 * 1024.0) / time_sec;
    
    pr_info("Total AF_ALG sha256 bandwidth: %.2f MB/s\n", bw);
    
    double j = energy_get_joules();
    if (j > 0.0) {
        pr_info("Energy consumed: %.4f Joules\n", j);
        pr_info("Efficiency: %.2f MB_crypto/Joule\n", bw * time_sec / j);
        report_add_metric("crypto", "energy_joules", j, "J");
        report_add_metric("crypto", "efficiency_mb_per_j", bw * time_sec / j, "MB/J");
    }
    
    report_add_metric("crypto", "crypto_bw", bw, "MB/s");
    
    free(threads);
    free(args);
}
