// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <glob.h>
#include "dvfs.h"
#include "cbench.h"
#include "topology.h"
#include "report.h"

// Max frequency states per CPU
#define MAX_FREQ_STATES 64

struct freq_state {
    uint64_t freq_khz;
    uint64_t time_tics; // typically 10ms ticks on Linux
};

static struct freq_state *start_states = NULL;
static struct freq_state *stop_states = NULL;
static int enabled = 0;

static void read_time_in_state(struct freq_state *states) {
    char path[256];
    char line[128];
    FILE *f;

    for (int i = 0; i < system_topo.total_cpus * MAX_FREQ_STATES; i++) {
        states[i].freq_khz = 0;
        states[i].time_tics = 0;
    }

    for (int cpu = 0; cpu < system_topo.total_cpus; cpu++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/stats/time_in_state", cpu);
        f = fopen(path, "r");
        if (!f) continue;
        
        int state = 0;
        while (fgets(line, sizeof(line), f) && state < MAX_FREQ_STATES) {
            uint64_t freq, time;
            if (sscanf(line, "%llu %llu", (unsigned long long*)&freq, (unsigned long long*)&time) == 2) {
                states[cpu * MAX_FREQ_STATES + state].freq_khz = freq;
                states[cpu * MAX_FREQ_STATES + state].time_tics = time;
                state++;
            }
        }
        fclose(f);
    }
}

void dvfs_init(void) {
    start_states = calloc(system_topo.total_cpus * MAX_FREQ_STATES, sizeof(struct freq_state));
    stop_states = calloc(system_topo.total_cpus * MAX_FREQ_STATES, sizeof(struct freq_state));
    if (start_states && stop_states) {
        enabled = 1;
        pr_info("DVFS subsystem initialized (time_in_state monitoring).\n");
    }
}

void dvfs_start(void) {
    if (!enabled) return;
    read_time_in_state(start_states);
}

void dvfs_stop(const char *subsystem) {
    if (!enabled) return;
    read_time_in_state(stop_states);

    pr_info("--- DVFS Scaling Analysis (%s) ---\n", subsystem);
    
    double total_avg_mhz = 0.0;
    int valid_cpus = 0;

    for (int cpu = 0; cpu < system_topo.total_cpus; cpu++) {
        uint64_t sum_weighted_freq = 0;
        uint64_t sum_time = 0;
        
        for (int state = 0; state < MAX_FREQ_STATES; state++) {
            int idx = cpu * MAX_FREQ_STATES + state;
            if (stop_states[idx].freq_khz > 0) {
                uint64_t diff = 0;
                if (stop_states[idx].time_tics >= start_states[idx].time_tics) {
                    diff = stop_states[idx].time_tics - start_states[idx].time_tics;
                }
                sum_weighted_freq += stop_states[idx].freq_khz * diff;
                sum_time += diff;
            }
        }
        
        if (sum_time > 0) {
            double avg_khz = (double)sum_weighted_freq / (double)sum_time;
            double avg_mhz = avg_khz / 1000.0;
            pr_info("CPU %d Avg Freq: %.1f MHz\n", cpu, avg_mhz);
            total_avg_mhz += avg_mhz;
            valid_cpus++;
        }
    }
    
    if (valid_cpus > 0) {
        double overall_avg = total_avg_mhz / valid_cpus;
        pr_info("Overall Avg Freq: %.1f MHz\n", overall_avg);
        report_add_metric(subsystem, "avg_freq_mhz", overall_avg, "MHz");
        
        if (overall_avg < 1000.0) {
            report_add_heuristic(subsystem, "Very low average frequency detected. Ensure CPU governor is scaling up properly.");
        }
    } else {
        pr_info("No DVFS state changes detected (Governor locked or hardware unsupported).\n");
    }
}
