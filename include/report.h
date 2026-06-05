#ifndef _CBENCH_REPORT_H
#define _CBENCH_REPORT_H

void report_init(void);
void report_add_sysinfo(const char *key, const char *value);
void report_add_metric(const char *subsystem, const char *metric, double value, const char *unit);
void report_print_json(void);

#endif /* _CBENCH_REPORT_H */
