// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include "telemetry.h"
#include "cpuidle.h"
#include "energy.h"
#include "profiler.h"
#include "thermal.h"
#include "softirq.h"
#include "vmstat.h"
#include "hwperf.h"

void telemetry_init(void) {
    cpuidle_init();
    energy_init();
    profiler_init();
    thermal_init();
    softirq_init();
    vmstat_init();
    hwperf_init();
}

void telemetry_start(void) {
    cpuidle_start();
    energy_start();
    profiler_start();
    thermal_start();
    softirq_start();
    vmstat_start();
    hwperf_start();
}

void telemetry_stop(const char *subsystem) {
    hwperf_stop(subsystem);
    vmstat_stop(subsystem);
    softirq_stop(subsystem);
    thermal_stop(subsystem);
    profiler_stop(subsystem);
    energy_stop();
    cpuidle_stop(subsystem);
}
