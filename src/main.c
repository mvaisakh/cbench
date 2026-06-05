// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include "cbench.h"
#include "utils.h"
#include "bench_syscall.h"
#include "bench_sched.h"
#include "bench_mem.h"
#include "bench_io.h"
#include "report.h"
#include "sysinfo.h"
#include "topology.h"
#include "energy.h"
#include "profiler.h"
#include "cpuidle.h"

int num_threads = 0;
int benchmark_duration_sec = 10;

static void usage(const char *progname)
{
    printf("Usage: %s [options]\n", progname);
    printf("Options:\n");
    printf("  -h, --help       Show this message\n");
    printf("  -a, --all        Run all benchmarks\n");
    printf("  -s, --syscall    Run Syscall Latency benchmark\n");
    printf("  -c, --sched      Run Scheduler Context Switch benchmark\n");
    printf("  -m, --mem        Run Memory Subsystem benchmark\n");
    printf("  -i, --io         Run I/O Subsystem benchmark\n");
    printf("  -t, --threads N  Number of threads (default: all cores)\n");
    printf("  -d, --duration S Duration in seconds per benchmark (default: 10)\n");
    printf("  -j, --json       Output results in JSON format at the end\n");
}

int main(int argc, char **argv)
{
    int opt;
    int run_all = 0;
    int run_syscall = 0;
    int run_sched = 0;
    int run_mem = 0;
    int run_io = 0;
    int json_out = 0;

    static struct option long_options[] = {
        {"help",    no_argument, 0, 'h'},
        {"all",     no_argument, 0, 'a'},
        {"syscall", no_argument, 0, 's'},
        {"sched",   no_argument, 0, 'c'},
        {"mem",     no_argument, 0, 'm'},
        {"io",      no_argument, 0, 'i'},
        {"threads", required_argument, 0, 't'},
        {"duration", required_argument, 0, 'd'},
        {"json",    no_argument, 0, 'j'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "hascmit:d:j", long_options, NULL)) != -1) {
        switch (opt) {
        case 'a':
            run_all = 1;
            break;
        case 's':
            run_syscall = 1;
            break;
        case 'c':
            run_sched = 1;
            break;
        case 'm':
            run_mem = 1;
            break;
        case 'i':
            run_io = 1;
            break;
        case 't':
            num_threads = atoi(optarg);
            break;
        case 'd':
            benchmark_duration_sec = atoi(optarg);
            break;
        case 'j':
            json_out = 1;
            break;
        case 'h':
        default:
            usage(argv[0]);
            return 0;
        }
    }

    if (!run_all && !run_syscall && !run_sched && !run_mem && !run_io) {
        pr_info("No benchmarks selected. Use -a to run all or -h for help.\n");
        return 0;
    }

    if (benchmark_duration_sec <= 0) {
        benchmark_duration_sec = 10;
    }

    signal(SIGPIPE, SIG_IGN);

    report_init();
    collect_sysinfo();
    topology_init();
    energy_init();
    profiler_init();
    cpuidle_init();

    if (num_threads <= 0) {
        num_threads = system_topo.total_cpus;
    }
    pr_info("Starting Cerium Benchmarking (cbench) with %d thread(s), %d seconds per test...\n", num_threads, benchmark_duration_sec);

    if (run_all || run_syscall) {
        profiler_start();
        cpuidle_start();
        run_syscall_benchmark();
        cpuidle_stop("syscall");
        profiler_stop("syscall");
    }
    
    if (run_all || run_sched) {
        profiler_start();
        cpuidle_start();
        run_sched_benchmark();
        cpuidle_stop("sched");
        profiler_stop("sched");
    }
    
    if (run_all || run_mem) {
        profiler_start();
        cpuidle_start();
        run_mem_benchmark();
        cpuidle_stop("mem");
        profiler_stop("mem");
    }
    
    if (run_all || run_io) {
        profiler_start();
        cpuidle_start();
        run_io_benchmark();
        cpuidle_stop("io");
        profiler_stop("io");
    }

    pr_info("Run complete.\n");

    if (json_out) {
        report_print_json();
    }

    return 0;
}
