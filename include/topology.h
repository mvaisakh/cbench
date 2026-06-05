// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_TOPOLOGY_H
#define _CBENCH_TOPOLOGY_H

#define MAX_CPUS 256
#define MAX_CLUSTERS 16

struct cpu_cluster {
    int id;
    int cpus[MAX_CPUS];
    int num_cpus;
};

struct topology {
    int total_cpus;
    struct cpu_cluster clusters[MAX_CLUSTERS];
    int num_clusters;
};

extern struct topology system_topo;

void topology_init(void);
void topology_bind_thread(int thread_id);

#endif /* _CBENCH_TOPOLOGY_H */
