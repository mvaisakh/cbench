// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include "cbench.h"
#include "hwperf.h"
#include "report.h"
#include "topology.h"

struct hwperf_ctx {
    int fd_inst;
    int fd_miss;
};
static struct hwperf_ctx *ctxs = NULL;
static int enabled = 0;

static long sys_perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

void hwperf_init(void) {
    if (getuid() != 0) return;
    ctxs = calloc(system_topo.total_cpus, sizeof(struct hwperf_ctx));
    enabled = 1;
    pr_info("Hardware Perf Counters (IPC/Cache) subsystem initialized.\n");
}

void hwperf_start(void) {
    if (!enabled) return;
    for (int cpu = 0; cpu < system_topo.total_cpus; cpu++) {
        struct perf_event_attr attr_inst, attr_miss;
        memset(&attr_inst, 0, sizeof(attr_inst));
        attr_inst.type = PERF_TYPE_HARDWARE;
        attr_inst.size = sizeof(attr_inst);
        attr_inst.config = PERF_COUNT_HW_INSTRUCTIONS;
        attr_inst.disabled = 1;
        attr_inst.exclude_user = 1;
        
        memset(&attr_miss, 0, sizeof(attr_miss));
        attr_miss.type = PERF_TYPE_HARDWARE;
        attr_miss.size = sizeof(attr_miss);
        attr_miss.config = PERF_COUNT_HW_CACHE_MISSES;
        attr_miss.disabled = 1;
        attr_miss.exclude_user = 1;
        
        ctxs[cpu].fd_inst = sys_perf_event_open(&attr_inst, -1, cpu, -1, 0);
        ctxs[cpu].fd_miss = sys_perf_event_open(&attr_miss, -1, cpu, -1, 0);
        
        if (ctxs[cpu].fd_inst >= 0) {
            ioctl(ctxs[cpu].fd_inst, PERF_EVENT_IOC_RESET, 0);
            ioctl(ctxs[cpu].fd_inst, PERF_EVENT_IOC_ENABLE, 0);
        }
        if (ctxs[cpu].fd_miss >= 0) {
            ioctl(ctxs[cpu].fd_miss, PERF_EVENT_IOC_RESET, 0);
            ioctl(ctxs[cpu].fd_miss, PERF_EVENT_IOC_ENABLE, 0);
        }
    }
}

void hwperf_stop(const char *subsystem) {
    if (!enabled) return;
    uint64_t total_inst = 0;
    uint64_t total_miss = 0;
    int supported = 0;
    
    for (int cpu = 0; cpu < system_topo.total_cpus; cpu++) {
        if (ctxs[cpu].fd_inst >= 0) {
            ioctl(ctxs[cpu].fd_inst, PERF_EVENT_IOC_DISABLE, 0);
            uint64_t val = 0;
            if (read(ctxs[cpu].fd_inst, &val, sizeof(val)) == sizeof(val)) total_inst += val;
            close(ctxs[cpu].fd_inst);
            ctxs[cpu].fd_inst = -1;
            supported = 1;
        }
        if (ctxs[cpu].fd_miss >= 0) {
            ioctl(ctxs[cpu].fd_miss, PERF_EVENT_IOC_DISABLE, 0);
            uint64_t val = 0;
            if (read(ctxs[cpu].fd_miss, &val, sizeof(val)) == sizeof(val)) total_miss += val;
            close(ctxs[cpu].fd_miss);
            ctxs[cpu].fd_miss = -1;
            supported = 1;
        }
    }
    
    if (supported && total_inst > 0) {
        double miss_rate = (double)total_miss * 100.0 / total_inst;
        pr_info("--- Hardware Counter Analysis (%s) ---\n", subsystem);
        pr_info("Hardware Cache Miss Rate: %.2f%% (%llu misses / %llu instructions)\n", miss_rate, (unsigned long long)total_miss, (unsigned long long)total_inst);
        report_add_metric(subsystem, "hw_cache_miss_rate", miss_rate, "%");
        
        if (miss_rate > 5.0) {
            pr_info("💡 KERNEL PATCH ADVICE (%s): High hardware cache miss rate. CPU is stalling on memory. Optimize data structures for cache locality.\n", subsystem);
            report_add_heuristic(subsystem, "High hardware cache miss rate. Optimize data structures for cache locality.");
        }
    }
}
