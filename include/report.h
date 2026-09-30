// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_REPORT_H
#define _CBENCH_REPORT_H

#define CBENCH_VERSION "2.0.0"

enum heuristic_severity {
    HEURISTIC_INFO = 0,
    HEURISTIC_WARN = 1,
    HEURISTIC_CRITICAL = 2,
};

void report_init(void);
void report_set_metadata(int duration_sec, int threads);
void report_add_sysinfo(const char *key, const char *value);
void report_start_benchmark_timer(void);
void report_stop_benchmark_timer(void);
void report_add_metric_stats(const char *subsystem, const char *metric, double mean, double stddev, double min, double max, int iterations, const char *unit);
void report_begin_iteration(void);
void report_end_iteration(void);
void report_process_iterations(int iterations);
void report_add_metric(const char *subsystem, const char *metric, double value, const char *unit);
void report_add_heuristic(const char *subsystem, const char *message);
void report_add_heuristic_severity(const char *subsystem, const char *message, enum heuristic_severity severity);
void report_print_summary(void);
void report_print_json(void);
int report_write_json(const char *filename);

#endif /* _CBENCH_REPORT_H */
