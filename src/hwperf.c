// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include "cbench.h"
#include "hwperf.h"
#include "report.h"
#include "topology.h"

static int enabled = 0;
static int mode_amu = 0;
static int mode_pmu_mrs = 0;
static int mode_perf = 0;

#ifdef __aarch64__
static sigjmp_buf sigill_env;
static void sigill_handler(int sig) {
    (void)sig;
    siglongjmp(sigill_env, 1);
}
#endif

// Global start values for mrs fallback
static uint64_t start_cycles = 0;
static uint64_t start_inst = 0;

struct hwperf_ctx {
    int fd_cycles;
    int fd_inst;
    int fd_miss;
};
static struct hwperf_ctx *ctxs = NULL;

static long sys_perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

static uint64_t read_amu_cycles(void) {
    uint64_t val = 0;
#ifdef __aarch64__
    asm volatile("mrs %0, S3_3_C15_C2_0" : "=r"(val));
#endif
    return val;
}

static uint64_t read_amu_inst(void) {
    uint64_t val = 0;
#ifdef __aarch64__
    asm volatile("mrs %0, S3_3_C15_C2_1" : "=r"(val));
#endif
    return val;
}

static uint64_t read_pmu_cycles(void) {
    uint64_t val = 0;
#ifdef __aarch64__
    asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(val));
#endif
    return val;
}

void hwperf_init(void) {
    if (getuid() != 0) return;
    enabled = 1;

#ifdef __aarch64__
    struct sigaction sa, old_sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigill_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGILL, &sa, &old_sa);

    // Test AMU
    if (sigsetjmp(sigill_env, 1) == 0) {
        read_amu_cycles();
        mode_amu = 1;
        pr_info("Hardware Perf: AMU direct read ENABLED! Zero-overhead telemetry.\n");
    } else {
        // Test PMU
        if (sigsetjmp(sigill_env, 1) == 0) {
            read_pmu_cycles();
            mode_pmu_mrs = 1;
            pr_info("Hardware Perf: AMU trapped. PMU direct read ENABLED!\n");
        } else {
            mode_perf = 1;
            pr_info("Hardware Perf: Direct assembly reads blocked (SIGILL). Falling back to perf_event_open raw IDs.\n");
        }
    }
    sigaction(SIGILL, &old_sa, NULL);
#else
    mode_perf = 1;
    pr_info("Hardware Perf: Non-ARM architecture detected. Using perf_event_open.\n");
#endif

    if (mode_perf) {
        ctxs = calloc(system_topo.total_cpus, sizeof(struct hwperf_ctx));
    }
}

void hwperf_start(void) {
    if (!enabled) return;

    if (mode_amu) {
        start_cycles = read_amu_cycles();
        start_inst = read_amu_inst();
    } else if (mode_pmu_mrs) {
        start_cycles = read_pmu_cycles();
    } else if (mode_perf) {
        for (int cpu = 0; cpu < system_topo.total_cpus; cpu++) {
            struct perf_event_attr attr_cyc, attr_inst, attr_miss;
            
            memset(&attr_cyc, 0, sizeof(attr_cyc));
            attr_cyc.type = PERF_TYPE_RAW;
            attr_cyc.size = sizeof(attr_cyc);
            attr_cyc.config = 0x11; // CPU Cycles
            attr_cyc.disabled = 1;
            attr_cyc.exclude_user = 1;

            memset(&attr_inst, 0, sizeof(attr_inst));
            attr_inst.type = PERF_TYPE_RAW;
            attr_inst.size = sizeof(attr_inst);
            attr_inst.config = 0x08; // Instructions
            attr_inst.disabled = 1;
            attr_inst.exclude_user = 1;
            
            memset(&attr_miss, 0, sizeof(attr_miss));
            attr_miss.type = PERF_TYPE_RAW;
            attr_miss.size = sizeof(attr_miss);
            attr_miss.config = 0x13; // L1D Cache Miss
            attr_miss.disabled = 1;
            attr_miss.exclude_user = 1;
            
            ctxs[cpu].fd_cycles = sys_perf_event_open(&attr_cyc, -1, cpu, -1, 0);
            ctxs[cpu].fd_inst = sys_perf_event_open(&attr_inst, -1, cpu, -1, 0);
            ctxs[cpu].fd_miss = sys_perf_event_open(&attr_miss, -1, cpu, -1, 0);
            
            if (ctxs[cpu].fd_cycles >= 0) {
                ioctl(ctxs[cpu].fd_cycles, PERF_EVENT_IOC_RESET, 0);
                ioctl(ctxs[cpu].fd_cycles, PERF_EVENT_IOC_ENABLE, 0);
            }
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
}

void hwperf_stop(const char *subsystem) {
    if (!enabled) return;
    uint64_t total_cycles = 0;
    uint64_t total_inst = 0;
    uint64_t total_miss = 0;
    int supported = 0;
    
    if (mode_amu) {
        total_cycles = read_amu_cycles() - start_cycles;
        total_inst = read_amu_inst() - start_inst;
        pr_info("--- AMU Hardware Counters (%s) ---\n", subsystem);
        pr_info("Total CPU Cycles: %llu\n", (unsigned long long)total_cycles);
        pr_info("Total Instructions: %llu\n", (unsigned long long)total_inst);
        if (total_cycles > 0) pr_info("IPC: %.2f\n", (double)total_inst / total_cycles);
        
        report_add_metric(subsystem, "amu_cycles", (double)total_cycles, "cycles");
        report_add_metric(subsystem, "amu_instructions", (double)total_inst, "insts");
    } else if (mode_pmu_mrs) {
        total_cycles = read_pmu_cycles() - start_cycles;
        pr_info("--- PMU Direct Read (%s) ---\n", subsystem);
        pr_info("Total CPU Cycles: %llu\n", (unsigned long long)total_cycles);
        
        report_add_metric(subsystem, "pmu_cycles", (double)total_cycles, "cycles");
    } else if (mode_perf) {
        for (int cpu = 0; cpu < system_topo.total_cpus; cpu++) {
            if (ctxs[cpu].fd_cycles >= 0) {
                ioctl(ctxs[cpu].fd_cycles, PERF_EVENT_IOC_DISABLE, 0);
                uint64_t val = 0;
                if (read(ctxs[cpu].fd_cycles, &val, sizeof(val)) == sizeof(val)) total_cycles += val;
                close(ctxs[cpu].fd_cycles);
                ctxs[cpu].fd_cycles = -1;
                supported = 1;
            }
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
        
        if (supported) {
            pr_info("--- Raw Perf Event Analysis (%s) ---\n", subsystem);
            if (total_cycles > 0) {
                pr_info("Total CPU Cycles: %llu\n", (unsigned long long)total_cycles);
                pr_info("Total Instructions: %llu\n", (unsigned long long)total_inst);
                pr_info("IPC: %.2f\n", (double)total_inst / total_cycles);
                
                report_add_metric(subsystem, "hw_cycles", (double)total_cycles, "cycles");
                report_add_metric(subsystem, "hw_instructions", (double)total_inst, "insts");
            }
            if (total_inst > 0) {
                double miss_rate = (double)total_miss * 100.0 / total_inst;
                pr_info("Hardware Cache Miss Rate: %.2f%% (%llu misses / %llu instructions)\n", miss_rate, (unsigned long long)total_miss, (unsigned long long)total_inst);
                report_add_metric(subsystem, "hw_cache_miss_rate", miss_rate, "%");
                
                if (miss_rate > 5.0) {
                    pr_info("💡 KERNEL PATCH ADVICE (%s): High hardware cache miss rate. CPU is stalling on memory. Optimize data structures for cache locality.\n", subsystem);
                    report_add_heuristic(subsystem, "High hardware cache miss rate. Optimize data structures for cache locality.");
                }
            }
        }
    }
}
