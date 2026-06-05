// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include "cbench.h"
#include "thermal.h"
#include "report.h"

static int enabled = 0;

static int get_max_temp(void) {
    glob_t g;
    int max_temp = -1;
    if (glob("/sys/class/thermal/thermal_zone*/temp", 0, NULL, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) {
            FILE *f = fopen(g.gl_pathv[i], "r");
            if (f) {
                int temp = -1;
                if (fscanf(f, "%d", &temp) == 1) {
                    // some drivers report mC, some report C
                    if (temp > 0 && temp < 200) temp *= 1000;
                    if (temp > max_temp) max_temp = temp;
                }
                fclose(f);
            }
        }
        globfree(&g);
    }
    return max_temp;
}

void thermal_init(void) {
    int max_temp = get_max_temp();
    if (max_temp >= 0) {
        enabled = 1;
        pr_info("Thermal subsystem initialized (Current max temp: %.1f C).\n", max_temp / 1000.0);
    }
}

void thermal_start(void) {
    // No-op for start, peak is handled at stop.
}

void thermal_stop(const char *subsystem) {
    if (!enabled) return;
    int peak_temp = get_max_temp();
    if (peak_temp >= 0) {
        double temp_c = peak_temp / 1000.0;
        pr_info("--- Thermal Analysis (%s) ---\n", subsystem);
        pr_info("Peak Temperature: %.1f C\n", temp_c);
        report_add_metric(subsystem, "peak_temp_c", temp_c, "C");
        
        if (peak_temp >= 85000) {
            pr_info("💡 KERNEL PATCH ADVICE (%s): Severe Thermal Throttling detected (>=85C). The CPU governor has likely dropped frequencies, invalidating throughput metrics.\n", subsystem);
            report_add_heuristic(subsystem, "Severe Thermal Throttling detected. CPU governor likely dropped frequencies.");
        }
    }
}
