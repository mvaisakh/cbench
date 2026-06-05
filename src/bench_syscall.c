#include <unistd.h>
#include <sys/syscall.h>
#include "cbench.h"
#include "utils.h"
#include "bench_syscall.h"

#define ITERS 10000000

int run_syscall_benchmark(void)
{
    uint64_t start, end;
    int i;

    pr_info("Running Syscall Latency Benchmark (SYS_gettid)...\n");

    start = get_time_ns();
    for (i = 0; i < ITERS; i++) {
        syscall(SYS_gettid);
    }
    end = get_time_ns();

    uint64_t total_ns = end - start;
    double avg_ns = (double)total_ns / ITERS;

    pr_info("Syscall SYS_gettid: %d iterations\n", ITERS);
    pr_info("Total time: %llu ns\n", (unsigned long long)total_ns);
    pr_info("Average latency per syscall: %.2f ns\n", avg_ns);

    return 0;
}
