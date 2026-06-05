// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cbench.h"
#include "softirq.h"
#include "report.h"

static int enabled = 0;
struct softirq_counts {
    uint64_t net_rx;
    uint64_t block;
};
static struct softirq_counts start_counts;

static void get_softirqs(struct softirq_counts *out) {
    out->net_rx = 0; out->block = 0;
    FILE *f = fopen("/proc/softirqs", "r");
    if (!f) return;
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "NET_RX:", 7) == 0) {
            char *p = line + 7;
            uint64_t val;
            int n;
            while (sscanf(p, "%lu%n", &val, &n) == 1) {
                out->net_rx += val;
                p += n;
            }
        } else if (strncmp(line, "BLOCK:", 6) == 0) {
            char *p = line + 6;
            uint64_t val;
            int n;
            while (sscanf(p, "%lu%n", &val, &n) == 1) {
                out->block += val;
                p += n;
            }
        }
    }
    fclose(f);
}

void softirq_init(void) {
    FILE *f = fopen("/proc/softirqs", "r");
    if (f) {
        enabled = 1;
        fclose(f);
        pr_info("SoftIRQ subsystem initialized.\n");
    }
}

void softirq_start(void) {
    if (!enabled) return;
    get_softirqs(&start_counts);
}

void softirq_stop(const char *subsystem) {
    if (!enabled) return;
    struct softirq_counts stop_counts;
    get_softirqs(&stop_counts);
    
    uint64_t net_rx_delta = stop_counts.net_rx - start_counts.net_rx;
    uint64_t block_delta = stop_counts.block - start_counts.block;
    
    pr_info("--- SoftIRQ Analysis (%s) ---\n", subsystem);
    pr_info("NET_RX SoftIRQs: %lu\n", (unsigned long)net_rx_delta);
    pr_info("BLOCK SoftIRQs: %lu\n", (unsigned long)block_delta);
    report_add_metric(subsystem, "softirq_net_rx", (double)net_rx_delta, "irqs");
    report_add_metric(subsystem, "softirq_block", (double)block_delta, "irqs");
    
    if (net_rx_delta > 1000000) {
        pr_info("💡 KERNEL PATCH ADVICE (%s): Network Interrupt Storm detected. Consider tuning RPS (Receive Packet Steering) or NAPI budgets.\n", subsystem);
        report_add_heuristic(subsystem, "Network Interrupt Storm detected. Tune RPS/NAPI.");
    }
}
