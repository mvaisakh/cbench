// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "bench_net.h"
#include "cbench.h"
#include "utils.h"
#include "topology.h"
#include "energy.h"
#include "profiler.h"
#include "report.h"
#include "cpuidle.h"

struct net_args {
    int thread_id;
    int duration;
    uint64_t bytes_sent;
};

static void *net_worker(void *arg) {
    struct net_args *args = arg;
    topology_bind_thread(args->thread_id);
    
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345 + args->thread_id);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    char buf[1400] = {0}; // Standard MTU-ish payload
    uint64_t end_time = get_time_ns() + ((uint64_t)args->duration * 1000000000ULL);
    
    while (get_time_ns() < end_time) {
        ssize_t ret = sendto(fd, buf, sizeof(buf), 0, (struct sockaddr *)&addr, sizeof(addr));
        if (ret > 0) args->bytes_sent += ret;
    }
    close(fd);
    return NULL;
}

void run_net_benchmark(int num_threads, int duration) {
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    struct net_args *args = malloc(sizeof(struct net_args) * num_threads);
    if (!threads || !args) {
        if (threads) free(threads);
        if (args) free(args);
        return;
    }
    
    pr_info("Running Network Subsystem Benchmark (UDP Loopback) across %d thread(s) for %d seconds...\n", num_threads, duration);
    
    
    uint64_t start = get_time_ns();
    for (int i=0; i<num_threads; i++) {
        args[i].thread_id = i;
        args[i].duration = duration;
        args[i].bytes_sent = 0;
        pthread_create(&threads[i], NULL, net_worker, &args[i]);
    }
    for (int i=0; i<num_threads; i++) pthread_join(threads[i], NULL);
    uint64_t stop = get_time_ns();
    
    
    uint64_t total_bytes = 0;
    for (int i=0; i<num_threads; i++) total_bytes += args[i].bytes_sent;
    double time_sec = (double)(stop - start) / 1000000000.0;
    double bw = (double)total_bytes / (1024.0 * 1024.0) / time_sec;
    
    pr_info("Total UDP loopback bandwidth: %.2f MB/s\n", bw);
    
    double j = energy_get_joules();
    if (j > 0.0) {
        pr_info("Energy consumed: %.4f Joules\n", j);
        pr_info("Efficiency: %.2f MB_udp/Joule\n", bw * time_sec / j);
        report_add_metric("net", "energy_joules", j, "J");
        report_add_metric("net", "efficiency_mb_per_j", bw * time_sec / j, "MB/J");
    }
    
    report_add_metric("net", "udp_loopback_bw", bw, "MB/s");
    
    free(threads);
    free(args);
}
