#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "cbench.h"
#include "utils.h"
#include "bench_sched.h"

#define PING_PONG_ITERS 100000

int run_sched_benchmark(void)
{
    int p1[2], p2[2];
    pid_t pid;
    char msg = 'c';
    uint64_t start, end;
    int i;

    pr_info("Running Scheduler Context Switch Benchmark (Pipe Ping-Pong)...\n");

    if (pipe(p1) == -1 || pipe(p2) == -1) {
        pr_err("Failed to create pipes\n");
        return -1;
    }

    start = get_time_ns();

    pid = fork();
    if (pid == -1) {
        pr_err("Fork failed\n");
        return -1;
    }

    if (pid == 0) {
        /* Child */
        for (i = 0; i < PING_PONG_ITERS; i++) {
            if (read(p1[0], &msg, 1) != 1) exit(1);
            if (write(p2[1], &msg, 1) != 1) exit(1);
        }
        exit(0);
    } else {
        /* Parent */
        for (i = 0; i < PING_PONG_ITERS; i++) {
            if (write(p1[1], &msg, 1) != 1) break;
            if (read(p2[0], &msg, 1) != 1) break;
        }
        wait(NULL);
    }

    end = get_time_ns();

    uint64_t total_ns = end - start;
    /* Two context switches per iteration (parent->child, child->parent) */
    double avg_ns = (double)total_ns / (PING_PONG_ITERS * 2);

    pr_info("Pipe Ping-Pong: %d iterations (2 context switches per iteration)\n", PING_PONG_ITERS);
    pr_info("Total time: %llu ns\n", (unsigned long long)total_ns);
    pr_info("Average context switch latency: %.2f ns\n", avg_ns);

    return 0;
}
