// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "sysinfo.h"
#include "report.h"

static void read_file_to_sysinfo(const char *key, const char *path) {
    FILE *f = fopen(path, "r");
    if (f) {
        char buf[128];
        if (fgets(buf, sizeof(buf), f)) {
            buf[strcspn(buf, "\r\n")] = 0;
            report_add_sysinfo(key, buf);
        }
        fclose(f);
    }
}

static void collect_cpuinfo(void) {
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f) return;
    char line[256];
    int found_model = 0;
    while (fgets(line, sizeof(line), f)) {
        if (!found_model && (strncmp(line, "model name", 10) == 0 ||
                             strncmp(line, "Processor", 9) == 0 ||
                             strncmp(line, "Hardware", 8) == 0 ||
                             strncmp(line, "CPU implementer", 15) == 0)) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                colon[strcspn(colon, "\r\n")] = 0;
                report_add_sysinfo("cpu_model", colon);
                found_model = 1;
            }
        }
    }
    fclose(f);
}

static void collect_meminfo(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++;
                while (*colon == ' ' || *colon == '\t') colon++;
                colon[strcspn(colon, "\r\n")] = 0;
                report_add_sysinfo("mem_total", colon);
                break;
            }
        }
    }
    fclose(f);
}

void collect_sysinfo(void) {
    struct utsname u;
    if (uname(&u) == 0) {
        report_add_sysinfo("sysname", u.sysname);
        report_add_sysinfo("release", u.release);
        report_add_sysinfo("version", u.version);
        report_add_sysinfo("machine", u.machine);
    }

    read_file_to_sysinfo("kernel_release", "/proc/sys/kernel/osrelease");
    read_file_to_sysinfo("kernel_version", "/proc/sys/kernel/version");
    read_file_to_sysinfo("cpu_governor", "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    read_file_to_sysinfo("scaling_min_freq", "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
    read_file_to_sysinfo("scaling_max_freq", "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");

    /* Device product / board name if available */
    read_file_to_sysinfo("product_name", "/sys/devices/virtual/dmi/id/product_name");
    read_file_to_sysinfo("board_name", "/sys/devices/virtual/dmi/id/board_name");

    collect_cpuinfo();
    collect_meminfo();
}
