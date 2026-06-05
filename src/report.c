// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "report.h"

#define MAX_SYSINFO 32
#define MAX_METRICS 64

struct sysinfo_entry {
    char key[64];
    char value[128];
};

struct metric_entry {
    char subsystem[32];
    char name[64];
    double value;
    char unit[16];
};

static struct sysinfo_entry sys_infos[MAX_SYSINFO];
static int sysinfo_count = 0;

static struct metric_entry metrics[MAX_METRICS];
static int metric_count = 0;

void report_init(void) {
    sysinfo_count = 0;
    metric_count = 0;
}

void report_add_sysinfo(const char *key, const char *value) {
    if (sysinfo_count >= MAX_SYSINFO) return;
    strncpy(sys_infos[sysinfo_count].key, key, 63);
    strncpy(sys_infos[sysinfo_count].value, value, 127);
    sysinfo_count++;
}

void report_add_metric(const char *subsystem, const char *metric, double value, const char *unit) {
    if (metric_count >= MAX_METRICS) return;
    strncpy(metrics[metric_count].subsystem, subsystem, 31);
    strncpy(metrics[metric_count].name, metric, 63);
    metrics[metric_count].value = value;
    strncpy(metrics[metric_count].unit, unit, 15);
    metric_count++;
}

void report_print_json(void) {
    int i;
    printf("{\n");
    
    /* Sysinfo */
    printf("  \"sysinfo\": {\n");
    for (i = 0; i < sysinfo_count; i++) {
        printf("    \"%s\": \"%s\"%s\n", sys_infos[i].key, sys_infos[i].value, (i == sysinfo_count - 1) ? "" : ",");
    }
    printf("  },\n");
    
    /* Metrics */
    printf("  \"metrics\": [\n");
    for (i = 0; i < metric_count; i++) {
        printf("    {\n");
        printf("      \"subsystem\": \"%s\",\n", metrics[i].subsystem);
        printf("      \"metric\": \"%s\",\n", metrics[i].name);
        printf("      \"value\": %f,\n", metrics[i].value);
        printf("      \"unit\": \"%s\"\n", metrics[i].unit);
        printf("    }%s\n", (i == metric_count - 1) ? "" : ",");
    }
    printf("  ]\n");
    printf("}\n");
}
