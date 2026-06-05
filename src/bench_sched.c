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

struct sched_worker_args {
    int thread_id;
    uint64_t iters;
};

static void *sched_worker(void *arg)
{
    struct sched_worker_args *args = (struct sched_worker_args *)arg;
    int p1[2], p2[2];
    pid_t pid;
    char msg = 'c';
    int i;

    args->iters = 0;
    topology_bind_thread(args->thread_id);

    if (pipe(p1) == -1 || pipe(p2) == -1) {
        return NULL;
    }

    pid = fork();
    if (pid == -1) {
        return NULL;
    }

    if (pid == 0) {
        /* Child */
        topology_bind_thread(args->thread_id);
        uint64_t end_time = get_time_ns() + ((uint64_t)benchmark_duration_sec * 1000000000ULL);
        while (get_time_ns() < end_time) {
            for (i = 0; i < 1000; i++) {
                if (read(p1[0], &msg, 1) != 1) exit(1);
                if (write(p2[1], &msg, 1) != 1) exit(1);
            }
        }
        exit(0);
    } else {
        /* Parent */
        uint64_t end_time = get_time_ns() + ((uint64_t)benchmark_duration_sec * 1000000000ULL);
        while (get_time_ns() < end_time) {
            for (i = 0; i < 1000; i++) {
                if (write(p1[1], &msg, 1) != 1) break;
                if (read(p2[0], &msg, 1) != 1) break;
            }
            args->iters += 1000;
        }
        close(p1[1]); /* Close write end so child's read fails and it exits cleanly */
        close(p2[0]);
        wait(NULL);
    }
    
    close(p1[0]);
    close(p2[1]);

    return NULL;
}

int run_sched_benchmark(void)
{
    pthread_t *threads;
    struct sched_worker_args *args;
    uint64_t start, end;
    int i;

    pr_info("Running Scheduler Context Switch Benchmark across %d thread(s) for %d seconds...\n", num_threads, benchmark_duration_sec);

    threads = malloc(sizeof(pthread_t) * num_threads);
    args = malloc(sizeof(struct sched_worker_args) * num_threads);
    if (!threads || !args) return -1;

    energy_start();
    start = get_time_ns();

    for (i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        pthread_create(&threads[i], NULL, sched_worker, &args[i]);
    }

    uint64_t total_iters = 0;
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        total_iters += args[i].iters;
    }

    end = get_time_ns();
    energy_stop();

    uint64_t total_ns = end - start;
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
    free(args);

    return 0;
}
