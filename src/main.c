// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <math.h>
#include <unistd.h>
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
#include "bench_rcu.h"
#include "bench_neon.h"
#include "bench_sqlite.h"
#include "bench_zram.h"
#include "bench_eas.h"
#include "report.h"
#include "sysinfo.h"
#include "telemetry.h"
#include "topology.h"

int num_threads = 0;
int repeat_count = 1;

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
    pr_info("  -u       Run Kernel VFS RCU stress benchmark\n");
    pr_info("  -N       Run NEON Thermal Throttling benchmark\n");
    pr_info("  -q       Run SQLite WAL Emulation benchmark\n");
    pr_info("  -Z       Run ZRAM Compression Stress benchmark\n");
    pr_info("  -E       Run EAS Ping-Pong benchmark\n");
    pr_info("  -j       Output JSON report at the end (to stdout)\n");
    pr_info("  -o FILE  Write JSON report directly to specified file\n");
    pr_info("  -h       Print this help\n");
}

int main(int argc, char **argv)
{
    int failures = 0;
    int total_runs = 0;
    int opt;
    int run_all = 0, run_syscall = 0, run_sched = 0, run_mem = 0, run_io = 0;
    int run_rng = 0, run_net = 0, run_futex = 0, run_crypto = 0, run_zero = 0;
    int run_rcu = 0, run_neon = 0, run_sqlite = 0, run_zram = 0, run_eas = 0;
    int output_json = 0;
    char *output_file = NULL;

    while ((opt = getopt(argc, argv, "ad:t:o:R:sSmijrnfczuNqZEh")) != -1) {
        switch (opt) {
            case 'a': run_all = 1; break;
            case 'R': repeat_count = atoi(optarg); break;
            case 'd': benchmark_duration_sec = atoi(optarg); break;
            case 't': num_threads = atoi(optarg); break;
            case 'o': output_file = optarg; break;
            case 's': run_syscall = 1; break;
            case 'S': run_sched = 1; break;
            case 'm': run_mem = 1; break;
            case 'i': run_io = 1; break;
            case 'r': run_rng = 1; break;
            case 'n': run_net = 1; break;
            case 'f': run_futex = 1; break;
            case 'c': run_crypto = 1; break;
            case 'z': run_zero = 1; break;
            case 'u': run_rcu = 1; break;
            case 'N': run_neon = 1; break;
            case 'q': run_sqlite = 1; break;
            case 'Z': run_zram = 1; break;
            case 'E': run_eas = 1; break;
            case 'j': output_json = 1; break;
            case 'h': print_usage(argv[0]); return 0;
            default: print_usage(argv[0]); return 1;
        }
    }

    if (run_all) {
        run_syscall = run_sched = run_mem = run_io = 1;
        run_rng = run_net = run_futex = run_crypto = run_zero = run_rcu = 1;
        run_neon = run_sqlite = run_zram = run_eas = 1;
    }

    if (!run_all && !run_syscall && !run_sched && !run_mem && !run_io && 
        !run_rng && !run_net && !run_futex && !run_crypto && !run_zero && !run_rcu &&
        !run_neon && !run_sqlite && !run_zram && !run_eas) {
        pr_info("No benchmarks selected. Use -a to run all or -h for help.\n");
        return 0;
    }

    if (benchmark_duration_sec <= 0) {
        benchmark_duration_sec = 10;
    }

    if (geteuid() != 0) {
        pr_warn("cbench must be run as root. Some benchmarks may fail.\n");
        return CBENCH_EXIT_PERMISSION;
    }
    
    signal(SIGPIPE, SIG_IGN);

    report_init();
    collect_sysinfo();
    topology_init();
    telemetry_init();

    if (num_threads <= 0) {
        num_threads = system_topo.total_cpus;
    }
    report_set_metadata(benchmark_duration_sec, num_threads);
    pr_info("Starting Cerium Benchmarking (cbench) with %d thread(s), %d seconds per test...\n", num_threads, benchmark_duration_sec);

    if (run_syscall) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            if (run_syscall_benchmark() != 0) local_fails++;
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("syscall");
    }
    if (run_sched) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            if (run_sched_benchmark() != 0) local_fails++;
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("sched");
    }
    if (run_mem) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            if (run_mem_benchmark() != 0) local_fails++;
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("mem");
    }
    if (run_io) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            if (run_io_benchmark() != 0) local_fails++;
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("io");
    }
    if (run_rng) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            run_rng_benchmark(num_threads, benchmark_duration_sec);
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("rng");
    }
    if (run_net) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            run_net_benchmark(num_threads, benchmark_duration_sec);
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("net");
    }
    if (run_futex) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            run_futex_benchmark(num_threads, benchmark_duration_sec);
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("futex");
    }
    if (run_crypto) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            run_crypto_benchmark(num_threads, benchmark_duration_sec);
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("crypto");
    }
    if (run_zero) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            run_zero_benchmark(num_threads, benchmark_duration_sec);
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("zero");
    }
    
    if (run_rcu) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            if (run_rcu_benchmark(num_threads, benchmark_duration_sec) != 0) local_fails++;
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("rcu");
    }
    if (run_neon) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            run_neon_benchmark(num_threads, benchmark_duration_sec);
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("neon");
    }
    if (run_sqlite) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            run_sqlite_benchmark(num_threads, benchmark_duration_sec);
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("sqlite");
    }
    if (run_zram) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            run_zram_benchmark(num_threads, benchmark_duration_sec);
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("zram");
    }
    if (run_eas) {
        pr_info("\n================================================================================\n");
        telemetry_start();
        report_begin_iteration();
        int local_fails = 0;
        for (int i = 0; i < repeat_count; i++) {
            report_start_benchmark_timer();
            run_eas_benchmark(benchmark_duration_sec);
            report_stop_benchmark_timer();
        }
        report_process_iterations(repeat_count);
        if (local_fails > 0) failures++;
        total_runs++;
        telemetry_stop("eas");
    }

    pr_info("\nRun complete.\n");

    /* Print user-friendly terminal scorecard */
    report_print_summary();

    if (output_file) {
        report_write_json(output_file);
    }

    if (output_json) {
        report_print_json();
    }

    telemetry_deinit();

    if (failures == 0) return CBENCH_EXIT_SUCCESS;
    else if (failures < total_runs) return CBENCH_EXIT_PARTIAL;
    else return CBENCH_EXIT_ERROR;
}
