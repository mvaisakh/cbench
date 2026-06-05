// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cpuidle.h"
#include "cbench.h"
#include "topology.h"

struct cstate_record {
    char name[32];
    uint64_t time_us;
};

static struct cstate_record *start_states = NULL;
static struct cstate_record *stop_states = NULL;
static int max_cstates = 10;

static void read_cstates(struct cstate_record *states)
{
    int cpu, state;
    char path[256];
    char name_buf[32];
    uint64_t time_us;
    FILE *f;

    for (int i=0; i<system_topo.total_cpus * max_cstates; i++) {
        states[i].time_us = 0;
        states[i].name[0] = '\0';
    }

    for (cpu = 0; cpu < system_topo.total_cpus; cpu++) {
        for (state = 0; state < max_cstates; state++) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpuidle/state%d/name", cpu, state);
            f = fopen(path, "r");
            if (!f) continue;
            if (fgets(name_buf, sizeof(name_buf), f)) {
                name_buf[strcspn(name_buf, "\n")] = 0;
                snprintf(states[cpu * max_cstates + state].name, 32, "%s", name_buf);
            }
            fclose(f);

            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpuidle/state%d/time", cpu, state);
            f = fopen(path, "r");
            if (!f) continue;
            if (fscanf(f, "%llu", (unsigned long long*)&time_us) == 1) {
                states[cpu * max_cstates + state].time_us = time_us;
            }
            fclose(f);
        }
    }
}

void cpuidle_init(void)
{
    start_states = calloc(system_topo.total_cpus * max_cstates, sizeof(struct cstate_record));
    stop_states = calloc(system_topo.total_cpus * max_cstates, sizeof(struct cstate_record));
}

void cpuidle_start(void)
{
    if (start_states) read_cstates(start_states);
}

void cpuidle_stop(const char *subsystem)
{
    if (!stop_states || !start_states) return;
    read_cstates(stop_states);

    pr_info("--- CPU Idle State Analysis (%s) ---\n", subsystem);
    uint64_t agg_diff[max_cstates];
    char agg_name[max_cstates][32];
    memset(agg_diff, 0, sizeof(agg_diff));
    
    int state, cpu;
    for (state = 0; state < max_cstates; state++) {
        agg_name[state][0] = '\0';
        for (cpu = 0; cpu < system_topo.total_cpus; cpu++) {
            int idx = cpu * max_cstates + state;
            if (stop_states[idx].name[0] != '\0') {
                if (agg_name[state][0] == '\0') strcpy(agg_name[state], stop_states[idx].name);
                uint64_t diff = (stop_states[idx].time_us >= start_states[idx].time_us) ? 
                                (stop_states[idx].time_us - start_states[idx].time_us) : 0;
                agg_diff[state] += diff;
            }
        }
    }

    uint64_t total_idle_time = 0;
    for (state = 0; state < max_cstates; state++) {
        total_idle_time += agg_diff[state];
    }

    if (total_idle_time > 0) {
        for (state = 0; state < max_cstates; state++) {
            if (agg_name[state][0] != '\0' && agg_diff[state] > 0) {
                double pct = (double)agg_diff[state] * 100.0 / total_idle_time;
                pr_info("C-State %s: %.2f%% (%.2f ms)\n", agg_name[state], pct, (double)agg_diff[state]/1000.0);
            }
        }
    } else {
        pr_info("No significant idle residency detected.\n");
    }

    /* Automated Conclusions */
    uint64_t expected_total_us = (uint64_t)benchmark_duration_sec * num_threads * 1000000ULL;
    double idle_pct = 0.0;
    if (expected_total_us > 0) {
        idle_pct = (double)total_idle_time * 100.0 / expected_total_us;
    }

    pr_info("--- Actionable Insight (%s) ---\n", subsystem);
    if (idle_pct > 25.0) {
        pr_info("💡 BOTTLENECK: Massive idle residency (%.1f%% of available CPU time).\n", idle_pct);
        pr_info("   Threads are blocking and sleeping instead of processing. This usually\n");
        pr_info("   indicates severe lock contention (e.g., spinlocks, mutexes) or hitting\n");
        pr_info("   hard hardware limits (like disk I/O wait). Check VFS or scheduling locks!\n");
    } else if (idle_pct > 10.0) {
        pr_info("💡 WARNING: Moderate idle residency (%.1f%% of available CPU time).\n", idle_pct);
        pr_info("   Some threads are occasionally yielding. Check the kernel profiler\n");
        pr_info("   results below to see if 'schedule' or 'mutex_spin_on_owner' is high.\n");
    } else {
        pr_info("💡 EXCELLENT: Near-perfect CPU utilization (Total Idle: %.1f%%).\n", idle_pct);
        pr_info("   The kernel is scaling flawlessly across all threads without blocking.\n");
        pr_info("   Any bottlenecks here are purely algorithmic. Look at the kernel\n");
        pr_info("   profiler to optimize the specific functions taking the most time.\n");
    }
}
