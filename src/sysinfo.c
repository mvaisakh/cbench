// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <string.h>
#include "sysinfo.h"
#include "report.h"

static void read_file_to_sysinfo(const char *key, const char *path) {
    FILE *f = fopen(path, "r");
    if (f) {
        char buf[128];
        if (fgets(buf, sizeof(buf), f)) {
            buf[strcspn(buf, "\n")] = 0;
            report_add_sysinfo(key, buf);
        }
        fclose(f);
    }
}

void collect_sysinfo(void) {
    read_file_to_sysinfo("kernel_release", "/proc/sys/kernel/osrelease");
    read_file_to_sysinfo("kernel_version", "/proc/sys/kernel/version");
    read_file_to_sysinfo("cpu_governor", "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
}
