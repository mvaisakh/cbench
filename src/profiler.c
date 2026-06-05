// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include "profiler.h"
#include "cbench.h"
#include "topology.h"

#define PERF_PAGES 8
#define PERF_PAGE_SIZE 4096
#define MMAP_SIZE ((PERF_PAGES + 1) * PERF_PAGE_SIZE)

struct perf_ctx {
    int fd;
    struct perf_event_mmap_page *base;
};

static struct perf_ctx *ctxs = NULL;
static int enabled = 0;

struct ksym {
    uint64_t addr;
    char name[64];
};

static struct ksym *ksyms = NULL;
static int num_ksyms = 0;

struct ip_count {
    uint64_t ip;
    int count;
};

#define MAX_SAMPLES 500000
static struct ip_count samples[MAX_SAMPLES];
static int num_samples = 0;

static int ksym_cmp(const void *a, const void *b) {
    return ((struct ksym *)a)->addr > ((struct ksym *)b)->addr ? 1 : -1;
}

static void load_kallsyms(void) {
    FILE *f = fopen("/proc/kallsyms", "r");
    if (!f) return;
    int capacity = 50000;
    ksyms = malloc(sizeof(struct ksym) * capacity);
    if (!ksyms) { fclose(f); return; }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        uint64_t addr;
        char type;
        char name[64];
        if (sscanf(line, "%lx %c %63s", &addr, &type, name) == 3) {
            if (type == 't' || type == 'T') {
                if (name[0] == '$') continue; /* Ignore ARM mapping symbols like $d.18 */
                if (num_ksyms >= capacity) {
                    capacity *= 2;
                    ksyms = realloc(ksyms, sizeof(struct ksym) * capacity);
                }
                ksyms[num_ksyms].addr = addr;
                strncpy(ksyms[num_ksyms].name, name, 63);
                ksyms[num_ksyms].name[63] = 0;
                num_ksyms++;
            }
        }
    }
    fclose(f);
    qsort(ksyms, num_ksyms, sizeof(struct ksym), ksym_cmp);
}

static const char *resolve_ip(uint64_t ip) {
    if (!ksyms || num_ksyms == 0) return "unknown_kernel_ptr";
    int l = 0, r = num_ksyms - 1;
    while (l <= r) {
        int m = l + (r - l) / 2;
        if (ksyms[m].addr <= ip && (m == num_ksyms - 1 || ksyms[m+1].addr > ip)) {
            return ksyms[m].name;
        }
        if (ksyms[m].addr < ip) l = m + 1;
        else r = m - 1;
    }
    return "unknown_kernel_ptr";
}

static void add_sample(uint64_t ip) {
    if (num_samples < MAX_SAMPLES) {
        samples[num_samples].ip = ip;
        samples[num_samples].count = 1;
        num_samples++;
    }
}

static long sys_perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                                int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

void profiler_init(void) {
    if (getuid() != 0) {
        pr_warn("Kernel profiling requires root privileges. Profiler disabled.\n");
        return;
    }
    load_kallsyms();
    if (num_ksyms == 0) {
        pr_warn("Could not load /proc/kallsyms (kptr_restrict=2?). Profiler disabled.\n");
        return;
    }
    
    ctxs = calloc(system_topo.total_cpus, sizeof(struct perf_ctx));
    enabled = 1;
    pr_info("Kernel profiler initialized (%d symbols loaded).\n", num_ksyms);
}

void profiler_start(void) {
    if (!enabled) return;
    num_samples = 0;

    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.type = PERF_TYPE_SOFTWARE;
    attr.size = sizeof(attr);
    attr.config = PERF_COUNT_SW_CPU_CLOCK;
    attr.sample_freq = 99; /* 99 Hz to avoid lockstep */
    attr.freq = 1;
    attr.sample_type = PERF_SAMPLE_IP;
    attr.exclude_user = 1;
    attr.disabled = 1;
    attr.wakeup_events = 1;

    for (int cpu = 0; cpu < system_topo.total_cpus; cpu++) {
        ctxs[cpu].fd = sys_perf_event_open(&attr, -1, cpu, -1, 0);
        if (ctxs[cpu].fd >= 0) {
            ctxs[cpu].base = mmap(NULL, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, ctxs[cpu].fd, 0);
            ioctl(ctxs[cpu].fd, PERF_EVENT_IOC_RESET, 0);
            ioctl(ctxs[cpu].fd, PERF_EVENT_IOC_ENABLE, 0);
        }
    }
}

struct sym_count {
    const char *name;
    int count;
};

static int sym_count_cmp(const void *a, const void *b) {
    return ((struct sym_count *)b)->count - ((struct sym_count *)a)->count;
}

void profiler_stop(const char *subsystem) {
    if (!enabled) return;

    for (int cpu = 0; cpu < system_topo.total_cpus; cpu++) {
        if (ctxs[cpu].fd >= 0) {
            ioctl(ctxs[cpu].fd, PERF_EVENT_IOC_DISABLE, 0);
            
            struct perf_event_mmap_page *p = ctxs[cpu].base;
            if (p != MAP_FAILED) {
                uint64_t head = p->data_head;
                __sync_synchronize();
                uint64_t tail = p->data_tail;
                unsigned char *data = (unsigned char *)p + PERF_PAGE_SIZE;
                
                while (tail < head) {
                    struct perf_event_header *hdr = (struct perf_event_header *)(data + (tail % (PERF_PAGES * PERF_PAGE_SIZE)));
                    if (hdr->type == PERF_RECORD_SAMPLE) {
                        uint64_t ip;
                        // PERF_SAMPLE_IP is the first 64bit field
                        memcpy(&ip, (unsigned char *)hdr + sizeof(struct perf_event_header), sizeof(uint64_t));
                        add_sample(ip);
                    }
                    tail += hdr->size;
                }
                p->data_tail = head;
            }
            
            if (ctxs[cpu].base != MAP_FAILED) munmap(ctxs[cpu].base, MMAP_SIZE);
            close(ctxs[cpu].fd);
            ctxs[cpu].fd = -1;
        }
    }

    int max_agg = 1000;
    struct sym_count agg[max_agg];
    int num_agg = 0;

    for (int i = 0; i < num_samples; i++) {
        const char *name = resolve_ip(samples[i].ip);
        int found = 0;
        for (int j = 0; j < num_agg; j++) {
            if (agg[j].name == name) {
                agg[j].count++;
                found = 1;
                break;
            }
        }
        if (!found && num_agg < max_agg) {
            agg[num_agg].name = name;
            agg[num_agg].count = 1;
            num_agg++;
        }
    }

    qsort(agg, num_agg, sizeof(struct sym_count), sym_count_cmp);

    pr_info("--- Kernel Hotspot Profile (%s) ---\n", subsystem);
    if (num_samples == 0) {
        pr_info("No kernel samples collected.\n");
        return;
    }
    for (int i = 0; i < 5 && i < num_agg; i++) {
        double pct = (double)agg[i].count * 100.0 / num_samples;
        pr_info("%5.2f%% : %s\n", pct, agg[i].name);
    }
}
