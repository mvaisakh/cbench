#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "cbench.h"
#include "utils.h"
#include "bench_syscall.h"
#include "bench_sched.h"
#include "bench_mem.h"

static void usage(const char *progname)
{
    printf("Usage: %s [options]\n", progname);
    printf("Options:\n");
    printf("  -h, --help       Show this message\n");
    printf("  -a, --all        Run all benchmarks\n");
    printf("  -s, --syscall    Run Syscall Latency benchmark\n");
    printf("  -c, --sched      Run Scheduler Context Switch benchmark\n");
    printf("  -m, --mem        Run Memory Subsystem benchmark\n");
}

int main(int argc, char **argv)
{
    int opt;
    int run_all = 0;
    int run_syscall = 0;
    int run_sched = 0;
    int run_mem = 0;

    static struct option long_options[] = {
        {"help",    no_argument, 0, 'h'},
        {"all",     no_argument, 0, 'a'},
        {"syscall", no_argument, 0, 's'},
        {"sched",   no_argument, 0, 'c'},
        {"mem",     no_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "hascm", long_options, NULL)) != -1) {
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
        case 'h':
        default:
            usage(argv[0]);
            return 0;
        }
    }

    pr_info("Starting Cerium Benchmarking (cbench)...\n");

    if (!run_all && !run_syscall && !run_sched && !run_mem) {
        pr_info("No benchmarks selected. Use -a to run all or -h for help.\n");
        return 0;
    }

    if (run_all || run_syscall) {
        run_syscall_benchmark();
    }
    
    if (run_all || run_sched) {
        run_sched_benchmark();
    }
    
    if (run_all || run_mem) {
        run_mem_benchmark();
    }

    pr_info("Run complete.\n");
    return 0;
}
