// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include "topology.h"
#include "cbench.h"

struct topology system_topo;

static void parse_cpu_list(const char *buf, struct cpu_cluster *cl, int *mapped) {
    const char *p = buf;
    while (*p && *p != '\n') {
        int start, end;
        if (sscanf(p, "%d-%d", &start, &end) == 2) {
            for (int i = start; i <= end && i < MAX_CPUS; i++) {
                if (!mapped[i]) {
                    cl->cpus[cl->num_cpus++] = i;
                    mapped[i] = 1;
                }
            }
            while (*p && *p != ',' && *p != '\n') p++;
        } else if (sscanf(p, "%d", &start) == 1) {
            if (!mapped[start] && start < MAX_CPUS) {
                cl->cpus[cl->num_cpus++] = start;
                mapped[start] = 1;
            }
            while (*p && *p != ',' && *p != '\n' && *p != ' ') p++;
        } else {
            p++;
        }
        if (*p == ',' || *p == ' ') p++;
    }
}

void topology_init(void)
{
    system_topo.total_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    system_topo.num_clusters = 0;
    
    if (system_topo.total_cpus <= 0) {
        system_topo.total_cpus = 1;
    }

    int c;
    int mapped[MAX_CPUS] = {0};

    for (c = 0; c < system_topo.total_cpus; c++) {
        if (mapped[c]) continue;

        char path[256];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/topology/core_cpus_list", c);
        FILE *f = fopen(path, "r");
        if (!f) {
            snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/related_cpus", c);
            f = fopen(path, "r");
        }
        
        if (f) {
            char buf[256];
            if (fgets(buf, sizeof(buf), f)) {
                struct cpu_cluster *cl = &system_topo.clusters[system_topo.num_clusters];
                cl->id = system_topo.num_clusters;
                cl->num_cpus = 0;
                
                parse_cpu_list(buf, cl, mapped);
                if (cl->num_cpus > 0) {
                    system_topo.num_clusters++;
                }
            }
            fclose(f);
        } else {
            if (system_topo.num_clusters == 0) {
                system_topo.num_clusters = 1;
                system_topo.clusters[0].id = 0;
                system_topo.clusters[0].num_cpus = 0;
            }
            struct cpu_cluster *cl = &system_topo.clusters[0];
            cl->cpus[cl->num_cpus++] = c;
            mapped[c] = 1;
        }
    }
    
    pr_info("Topology: %d CPUs across %d cluster(s)\n", system_topo.total_cpus, system_topo.num_clusters);
}

void topology_bind_thread(int thread_id)
{
    if (system_topo.num_clusters == 0) return;
    
    int cluster_idx = thread_id % system_topo.num_clusters;
    struct cpu_cluster *cl = &system_topo.clusters[cluster_idx];
    if (cl->num_cpus == 0) return;
    
    int cpu = cl->cpus[(thread_id / system_topo.num_clusters) % cl->num_cpus];

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        pr_warn("Failed to set CPU affinity for thread %d (CPU %d)\n", thread_id, cpu);
    }
}
