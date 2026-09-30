// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "report.h"
#include "cbench.h"

#define MAX_SYSINFO 64
#define MAX_METRICS 256
#define MAX_HEURISTICS 64

struct sysinfo_entry {
    char key[64];
    char value[128];
};

struct metric_entry {
    char subsystem[32];
    char name[64];
    double value;
    char unit[16];
    double elapsed_ms;
    double stddev;
    double min;
    double max;
    int iterations;
};

struct heuristic_entry {
    char subsystem[32];
    char message[256];
    enum heuristic_severity severity;
};

static struct sysinfo_entry sys_infos[MAX_SYSINFO];
static int sysinfo_count = 0;

static struct metric_entry metrics[MAX_METRICS];
static int metric_count = 0;

static struct heuristic_entry heuristics[MAX_HEURISTICS];
static int heuristic_count = 0;

static struct timespec bench_start_time;
static double last_bench_elapsed_ms = 0.0;
static int iteration_start_count = 0;


static int meta_duration_sec = 10;
static int meta_threads = 1;
static time_t meta_start_time = 0;

void report_init(void) {
    sysinfo_count = 0;
    metric_count = 0;
    heuristic_count = 0;
    meta_start_time = time(NULL);
}

void report_set_metadata(int duration_sec, int threads) {
    meta_duration_sec = duration_sec;
    meta_threads = threads;
}

void report_add_sysinfo(const char *key, const char *value) {
    if (sysinfo_count >= MAX_SYSINFO) return;
    strncpy(sys_infos[sysinfo_count].key, key, 63);
    sys_infos[sysinfo_count].key[63] = '\0';
    strncpy(sys_infos[sysinfo_count].value, value, 127);
    sys_infos[sysinfo_count].value[127] = '\0';
    sysinfo_count++;
}


void report_start_benchmark_timer(void) {
    clock_gettime(CLOCK_MONOTONIC, &bench_start_time);
}

void report_stop_benchmark_timer(void) {
    struct timespec bench_end_time;
    clock_gettime(CLOCK_MONOTONIC, &bench_end_time);
    last_bench_elapsed_ms = (bench_end_time.tv_sec - bench_start_time.tv_sec) * 1000.0 +
                            (bench_end_time.tv_nsec - bench_start_time.tv_nsec) / 1000000.0;
}

void report_begin_iteration(void) {
    iteration_start_count = metric_count;
}

void report_end_iteration(void) {
    // Only needed to keep track of where iteration started
}


void report_process_iterations(int iterations) {
    if (iterations <= 1) return;
    int added = metric_count - iteration_start_count;
    if (added == 0 || added % iterations != 0) return; // something went wrong
    int m = added / iterations;
    
    for (int i = 0; i < m; i++) {
        double sum = 0, sq_sum = 0, min_val = -1, max_val = -1;
        int first = iteration_start_count + i;
        for (int j = 0; j < iterations; j++) {
            double val = metrics[iteration_start_count + j * m + i].value;
            sum += val;
            sq_sum += val * val;
            if (j == 0 || val < min_val) min_val = val;
            if (j == 0 || val > max_val) max_val = val;
        }
        double mean = sum / iterations;
        double variance = (sq_sum / iterations) - (mean * mean);
        double stddev = variance > 0 ? sqrt(variance) : 0.0;
        
        // Overwrite the first iteration's entry with aggregated stats
        metrics[first].value = mean;
        metrics[first].stddev = stddev;
        metrics[first].min = min_val;
        metrics[first].max = max_val;
        metrics[first].iterations = iterations;
    }
    // reset metric_count to drop the extra iterations
    metric_count = iteration_start_count + m;
}
void report_add_metric_stats(const char *subsystem, const char *metric, double mean, double stddev, double min, double max, int iterations, const char *unit) {
    if (metric_count >= MAX_METRICS) return;
    strncpy(metrics[metric_count].subsystem, subsystem, 31);
    metrics[metric_count].subsystem[31] = '\0';
    strncpy(metrics[metric_count].name, metric, 63);
    metrics[metric_count].name[63] = '\0';
    metrics[metric_count].value = mean;
    strncpy(metrics[metric_count].unit, unit, 15);
    metrics[metric_count].unit[15] = '\0';
    metrics[metric_count].elapsed_ms = last_bench_elapsed_ms;
    metrics[metric_count].stddev = stddev;
    metrics[metric_count].min = min;
    metrics[metric_count].max = max;
    metrics[metric_count].iterations = iterations;
    metric_count++;
}
void report_add_metric(const char *subsystem, const char *metric, double value, const char *unit) {
    if (metric_count >= MAX_METRICS) return;
    strncpy(metrics[metric_count].subsystem, subsystem, 31);
    metrics[metric_count].subsystem[31] = '\0';
    strncpy(metrics[metric_count].name, metric, 63);
    metrics[metric_count].name[63] = '\0';
    metrics[metric_count].value = value;
    strncpy(metrics[metric_count].unit, unit, 15);
    metrics[metric_count].unit[15] = '\0';
    metric_count++;
}

void report_add_heuristic(const char *subsystem, const char *message) {
    report_add_heuristic_severity(subsystem, message, HEURISTIC_WARN);
}

void report_add_heuristic_severity(const char *subsystem, const char *message, enum heuristic_severity severity) {
    if (heuristic_count >= MAX_HEURISTICS) return;
    strncpy(heuristics[heuristic_count].subsystem, subsystem, 31);
    heuristics[heuristic_count].subsystem[31] = '\0';
    strncpy(heuristics[heuristic_count].message, message, 255);
    heuristics[heuristic_count].message[255] = '\0';
    heuristics[heuristic_count].severity = severity;
    heuristic_count++;
}

static const char *severity_str(enum heuristic_severity s) {
    switch (s) {
        case HEURISTIC_CRITICAL: return "critical";
        case HEURISTIC_INFO:     return "info";
        case HEURISTIC_WARN:
        default:                 return "warning";
    }
}

static void escape_json_string(const char *input, char *output, size_t max_len) {
    size_t j = 0;
    for (size_t i = 0; input[i] != '\0' && j + 2 < max_len; i++) {
        if (input[i] == '"' || input[i] == '\\') {
            output[j++] = '\\';
            output[j++] = input[i];
        } else if (input[i] == '\n') {
            output[j++] = '\\';
            output[j++] = 'n';
        } else if (input[i] == '\r') {
            output[j++] = '\\';
            output[j++] = 'r';
        } else if (input[i] == '\t') {
            output[j++] = '\\';
            output[j++] = 't';
        } else {
            output[j++] = input[i];
        }
    }
    output[j] = '\0';
}

static void format_json_report(FILE *stream) {
    int i;
    char esc_val[512];
    char esc_msg[512];

    fprintf(stream, "{\n");
    fprintf(stream, "  \"version\": \"%s\",\n", CBENCH_VERSION);
    fprintf(stream, "  \"timestamp\": %ld,\n", (long)meta_start_time);
    fprintf(stream, "  \"duration_sec\": %d,\n", meta_duration_sec);
    fprintf(stream, "  \"threads\": %d,\n", meta_threads);

    /* Sysinfo */
    fprintf(stream, "  \"sysinfo\": {\n");
    for (i = 0; i < sysinfo_count; i++) {
        escape_json_string(sys_infos[i].value, esc_val, sizeof(esc_val));
        fprintf(stream, "    \"%s\": \"%s\"%s\n",
                sys_infos[i].key, esc_val,
                (i == sysinfo_count - 1) ? "" : ",");
    }
    fprintf(stream, "  },\n");

    /* Metrics */
    fprintf(stream, "  \"metrics\": [\n");
    for (i = 0; i < metric_count; i++) {
        fprintf(stream, "    {\n");
        fprintf(stream, "      \"subsystem\": \"%s\",\n", metrics[i].subsystem);
        fprintf(stream, "      \"metric\": \"%s\",\n", metrics[i].name);
        fprintf(stream, "      \"value\": %.4f,\n", metrics[i].value);
        fprintf(stream, "      \"unit\": \"%s\",\n", metrics[i].unit);
        fprintf(stream, "      \"elapsed_ms\": %.4f", metrics[i].elapsed_ms);
        if (metrics[i].iterations > 1) {
            fprintf(stream, ",\n");
            fprintf(stream, "      \"iterations\": %d,\n", metrics[i].iterations);
            fprintf(stream, "      \"stddev\": %.4f,\n", metrics[i].stddev);
            fprintf(stream, "      \"min\": %.4f,\n", metrics[i].min);
            fprintf(stream, "      \"max\": %.4f\n", metrics[i].max);
        } else {
            fprintf(stream, "\n");
        }
        fprintf(stream, "    }%s\n", (i == metric_count - 1) ? "" : ",");
    }
    fprintf(stream, "  ],\n");

    /* Heuristics */
    fprintf(stream, "  \"heuristics\": [\n");
    for (i = 0; i < heuristic_count; i++) {
        escape_json_string(heuristics[i].message, esc_msg, sizeof(esc_msg));
        fprintf(stream, "    {\n");
        fprintf(stream, "      \"subsystem\": \"%s\",\n", heuristics[i].subsystem);
        fprintf(stream, "      \"severity\": \"%s\",\n", severity_str(heuristics[i].severity));
        fprintf(stream, "      \"message\": \"%s\"\n", esc_msg);
        fprintf(stream, "    }%s\n", (i == heuristic_count - 1) ? "" : ",");
    }
    fprintf(stream, "  ]\n");
    fprintf(stream, "}\n");
}

void report_print_json(void) {
    format_json_report(stdout);
}

int report_write_json(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        pr_err("Failed to open report output file: %s\n", filename);
        return -1;
    }
    format_json_report(f);
    fclose(f);
    pr_info("Report successfully written to %s\n", filename);
    return 0;
}

void report_print_summary(void) {
    printf("\n");
    printf("================================================================================\n");
    printf("                  PROJECT CERIUM BENCHMARK REPORT SUMMARY                      \n");
    printf("================================================================================\n");
    printf(" Duration: %d sec/test | Threads: %d | Total Metrics: %d\n",
           meta_duration_sec, meta_threads, metric_count);
    printf("--------------------------------------------------------------------------------\n");
    printf(" %-12s | %-26s | %10s | %10s | %8s | %-8s\n", "SUBSYSTEM", "METRIC", "VALUE", "ELAPSED(ms)", "STDDEV", "UNIT");
    printf("--------------------------------------------------------------------------------\n");
    for (int i = 0; i < metric_count; i++) {
        char val_str[32];
        snprintf(val_str, sizeof(val_str), "%.2f", metrics[i].value);
        char el_str[32];
        snprintf(el_str, sizeof(el_str), "%.2f", metrics[i].elapsed_ms);
        char std_str[32] = "";
        if (metrics[i].iterations > 1) {
            snprintf(std_str, sizeof(std_str), "%.2f", metrics[i].stddev);
        }
        printf(" %-12s | %-26s | %10s | %10s | %8s | %-8s\n",
               metrics[i].subsystem, metrics[i].name, val_str, el_str, std_str, metrics[i].unit);
    }
    printf("================================================================================\n");

    if (heuristic_count > 0) {
        printf("\n------------------------- ACTIONABLE KERNEL ADVICE -----------------------------\n");
        for (int i = 0; i < heuristic_count; i++) {
            const char *badge = "[WARN]";
            if (heuristics[i].severity == HEURISTIC_CRITICAL) badge = "[CRITICAL]";
            else if (heuristics[i].severity == HEURISTIC_INFO) badge = "[INFO]";
            printf(" %-10s [%s] %s\n", badge, heuristics[i].subsystem, heuristics[i].message);
        }
        printf("--------------------------------------------------------------------------------\n");
    }
    printf("\n");
}
