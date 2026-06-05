#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "cbench.h"
#include "utils.h"
#include "bench_mem.h"

#define MEM_SIZE (256 * 1024 * 1024) /* 256 MB */
#define PAGE_SIZE 4096

int run_mem_benchmark(void)
{
    uint64_t start, end;
    void *mem1, *mem2;
    size_t i;
    double total_ns, bw_mb_s;

    pr_info("Running Memory Subsystem Benchmarks...\n");

    /* 1. Page Fault / Anonymous Memory mapping */
    start = get_time_ns();
    mem1 = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem1 == MAP_FAILED) {
        pr_err("mmap failed\n");
        return -1;
    }
    
    /* Touch each page to trigger page faults */
    volatile char *ptr = (volatile char *)mem1;
    for (i = 0; i < MEM_SIZE; i += PAGE_SIZE) {
        ptr[i] = 1;
    }
    end = get_time_ns();
    
    total_ns = (double)(end - start);
    pr_info("Page Faults (Anonymous): %.2f ms to fault %u pages (256 MB)\n", total_ns / 1000000.0, MEM_SIZE / PAGE_SIZE);

    /* 2. Sequential Write Bandwidth */
    start = get_time_ns();
    memset(mem1, 0xAA, MEM_SIZE);
    end = get_time_ns();

    total_ns = (double)(end - start);
    bw_mb_s = (MEM_SIZE / (1024.0 * 1024.0)) / (total_ns / 1000000000.0);
    pr_info("Sequential Write (memset): %.2f MB/s\n", bw_mb_s);

    /* 3. Sequential Copy Bandwidth */
    mem2 = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem2 != MAP_FAILED) {
        /* Pre-fault the destination */
        memset(mem2, 0, MEM_SIZE);
        
        start = get_time_ns();
        memcpy(mem2, mem1, MEM_SIZE);
        end = get_time_ns();

        total_ns = (double)(end - start);
        bw_mb_s = (MEM_SIZE / (1024.0 * 1024.0)) / (total_ns / 1000000000.0);
        pr_info("Memory Copy (memcpy): %.2f MB/s\n", bw_mb_s);
        
        munmap(mem2, MEM_SIZE);
    }

    munmap(mem1, MEM_SIZE);
    return 0;
}
