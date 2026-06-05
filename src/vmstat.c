// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cbench.h"
#include "vmstat.h"
#include "report.h"

static int enabled = 0;
static uint64_t start_pgmajfault = 0;

static uint64_t get_pgmajfault(void) {
    FILE *f = fopen("/proc/vmstat", "r");
    if (!f) return 0;
    char line[256];
    uint64_t val = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "pgmajfault ", 11) == 0) {
            sscanf(line + 11, "%lu", &val);
            break;
        }
    }
    fclose(f);
    return val;
}

void vmstat_init(void) {
    FILE *f = fopen("/proc/vmstat", "r");
    if (f) {
        enabled = 1;
        fclose(f);
        pr_info("VMStat subsystem initialized.\n");
    }
}

void vmstat_start(void) {
    if (!enabled) return;
    start_pgmajfault = get_pgmajfault();
}

void vmstat_stop(const char *subsystem) {
    if (!enabled) return;
    uint64_t stop_pgmajfault = get_pgmajfault();
    uint64_t delta = stop_pgmajfault - start_pgmajfault;
    
    pr_info("--- VMStat Analysis (%s) ---\n", subsystem);
    pr_info("Major Page Faults: %lu\n", (unsigned long)delta);
    report_add_metric(subsystem, "pgmajfaults", (double)delta, "faults");
    
    if (delta > 0) {
        pr_info("💡 KERNEL PATCH ADVICE (%s): Major page faults detected. The device is actively swapping to zRAM or Disk, causing massive latency spikes.\n", subsystem);
        report_add_heuristic(subsystem, "Major page faults detected. Active zRAM/Disk swapping causing latency spikes.");
    }
}
