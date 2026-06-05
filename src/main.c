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
#include "bench_rng.h"
#include "bench_net.h"
#include "bench_futex.h"
#include "bench_crypto.h"
#include "bench_zero.h"
#include "report.h"
#include "sysinfo.h"
#include "topology.h"
#include "energy.h"
#include "profiler.h"
#include "cpuidle.h"

int num_threads = 0;
int benchmark_duration_sec = 10;

static void print_usage(const char *prog) {
    pr_info("Usage: %s [OPTIONS]\n", prog);
    pr_info("Options:\n");
    pr_info("  -a       Run all benchmarks\n");
    pr_info("  -d SEC   Duration per benchmark (default 10)\n");
    pr_info("  -t NUM   Number of threads (default: all topology cores)\n");
    pr_info("  -s       Run Syscall benchmark\n");
    pr_info("  -S       Run Scheduler benchmark\n");
    pr_info("  -m       Run Memory benchmark\n");
    pr_info("  -i       Run I/O benchmark\n");
    pr_info("  -r       Run RNG benchmark\n");
    pr_info("  -n       Run Network loopback benchmark\n");
    pr_info("  -f       Run Futex contention benchmark\n");
    pr_info("  -c       Run AF_ALG Crypto benchmark\n");
    pr_info("  -z       Run /dev/zero benchmark\n");
    pr_info("  -j       Output JSON report at the end\n");
    pr_info("  -h       Print this help\n");
}

int main(int argc, char **argv)
{
    int opt;
    int run_all = 0, run_syscall = 0, run_sched = 0, run_mem = 0, run_io = 0;
    int run_rng = 0, run_net = 0, run_futex = 0, run_crypto = 0, run_zero = 0;
    int output_json = 0;

    while ((opt = getopt(argc, argv, "ad:t:sSmijrnfczh")) != -1) {
        switch (opt) {
            case 'a': run_all = 1; break;
            case 'd': benchmark_duration_sec = atoi(optarg); break;
            case 't': num_threads = atoi(optarg); break;
            case 's': run_syscall = 1; break;
            case 'S': run_sched = 1; break;
            case 'm': run_mem = 1; break;
            case 'i': run_io = 1; break;
            case 'r': run_rng = 1; break;
            case 'n': run_net = 1; break;
            case 'f': run_futex = 1; break;
            case 'c': run_crypto = 1; break;
            case 'z': run_zero = 1; break;
            case 'j': output_json = 1; break;
            case 'h': print_usage(argv[0]); return 0;
            default: print_usage(argv[0]); return 1;
        }
    }

    if (run_all) {
        run_syscall = run_sched = run_mem = run_io = 1;
        run_rng = run_net = run_futex = run_crypto = run_zero = 1;
    }

    if (!run_all && !run_syscall && !run_sched && !run_mem && !run_io && 
        !run_rng && !run_net && !run_futex && !run_crypto && !run_zero) {
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

    if (run_syscall) {
        profiler_start();
        cpuidle_start();
        run_syscall_benchmark();
        cpuidle_stop("syscall");
        profiler_stop("syscall");
    }
    if (run_sched) {
        profiler_start();
        cpuidle_start();
        run_sched_benchmark();
        cpuidle_stop("sched");
        profiler_stop("sched");
    }
    if (run_mem) {
        profiler_start();
        cpuidle_start();
        run_mem_benchmark();
        cpuidle_stop("mem");
        profiler_stop("mem");
    }
    if (run_io) {
        profiler_start();
        cpuidle_start();
        run_io_benchmark();
        cpuidle_stop("io");
        profiler_stop("io");
    }
    if (run_rng) run_rng_benchmark(num_threads, benchmark_duration_sec);
    if (run_net) run_net_benchmark(num_threads, benchmark_duration_sec);
    if (run_futex) run_futex_benchmark(num_threads, benchmark_duration_sec);
    if (run_crypto) run_crypto_benchmark(num_threads, benchmark_duration_sec);
    if (run_zero) run_zero_benchmark(num_threads, benchmark_duration_sec);

    pr_info("Run complete.\n");

    if (output_json) {
        report_print_json();
    }

    return 0;
}
