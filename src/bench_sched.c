// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include "cbench.h"
#include "utils.h"
#include "bench_sched.h"
#include "report.h"
#include "topology.h"
#include "energy.h"

#define PING_PONG_ITERS_PER_THREAD 50000

static void *sched_worker(void *arg)
{
    int thread_id = *(int *)arg;
    int p1[2], p2[2];
    pid_t pid;
    char msg = 'c';
    int i;

    topology_bind_thread(thread_id);

    if (pipe(p1) == -1 || pipe(p2) == -1) {
        return NULL;
    }

    pid = fork();
    if (pid == -1) {
        return NULL;
    }

    if (pid == 0) {
        /* Child */
        topology_bind_thread(thread_id); /* Bind child to same CPU/cluster */
        for (i = 0; i < PING_PONG_ITERS_PER_THREAD; i++) {
            if (read(p1[0], &msg, 1) != 1) exit(1);
            if (write(p2[1], &msg, 1) != 1) exit(1);
        }
        exit(0);
    } else {
        /* Parent */
        for (i = 0; i < PING_PONG_ITERS_PER_THREAD; i++) {
            if (write(p1[1], &msg, 1) != 1) break;
            if (read(p2[0], &msg, 1) != 1) break;
        }
        wait(NULL);
    }
    
    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);

    return NULL;
}

int run_sched_benchmark(void)
{
    pthread_t *threads;
    int *tids;
    uint64_t start, end;
    int i;

    pr_info("Running Scheduler Context Switch Benchmark (Pipe Ping-Pong) across %d thread(s)...\n", num_threads);

    threads = malloc(sizeof(pthread_t) * num_threads);
    tids = malloc(sizeof(int) * num_threads);
    if (!threads || !tids) return -1;

    energy_start();
    start = get_time_ns();

    for (i = 0; i < num_threads; i++) {
        tids[i] = i;
        pthread_create(&threads[i], NULL, sched_worker, &tids[i]);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    end = get_time_ns();
    energy_stop();

    uint64_t total_ns = end - start;
    uint64_t total_iters = (uint64_t)PING_PONG_ITERS_PER_THREAD * num_threads;
    double throughput = (double)(total_iters * 2) / (total_ns / 1000000000.0); /* 2 switches per iter */
    double joules = energy_get_joules();

    pr_info("Pipe Ping-Pong: %llu total iterations (2 context switches per iteration)\n", (unsigned long long)total_iters);
    pr_info("Total wall time: %llu ns\n", (unsigned long long)total_ns);
    pr_info("Throughput: %.2f context_switches/sec\n", throughput);

    report_add_metric("sched", "context_switch_throughput", throughput, "switches/sec");

    if (joules > 0.0) {
        pr_info("Energy consumed: %.4f Joules\n", joules);
        pr_info("Efficiency: %.2f switches/Joule\n", throughput / joules);
        report_add_metric("sched", "energy_joules", joules, "J");
        report_add_metric("sched", "efficiency_switches_per_j", throughput / joules, "switches/J");
    }

    free(threads);
    free(tids);

    return 0;
}
