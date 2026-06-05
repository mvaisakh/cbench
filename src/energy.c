// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "energy.h"
#include "cbench.h"
#include "utils.h"

static int use_rapl = 0;
static int use_battery_charge = 0;
static int use_battery_energy = 0;
static int use_battery_current = 0;
static uint64_t start_energy_val = 0;
static uint64_t stop_energy_val = 0;
static uint64_t start_time_ns = 0;
static uint64_t stop_time_ns = 0;
static char active_path[256];

static uint64_t read_sysfs_uint64(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    uint64_t val = 0;
    if (fscanf(f, "%llu", (unsigned long long*)&val) != 1) val = 0;
    fclose(f);
    return val;
}

static uint64_t read_battery_voltage_now(void)
{
    uint64_t v = read_sysfs_uint64("/sys/class/power_supply/battery/voltage_now");
    if (v == 0) v = read_sysfs_uint64("/sys/class/power_supply/bms/voltage_now");
    return v;
}

void energy_init(void)
{
    if (read_sysfs_uint64("/sys/class/powercap/intel-rapl:0/energy_uj") > 0) {
        use_rapl = 1;
        snprintf(active_path, sizeof(active_path), "/sys/class/powercap/intel-rapl:0/energy_uj");
        pr_info("Energy subsystem initialized: using intel-rapl\n");
        return;
    } 
    
    /* Try Android 'energy_now' (uWh) */
    const char *energy_paths[] = {
        "/sys/class/power_supply/battery/energy_now",
        "/sys/class/power_supply/bms/energy_now",
        NULL
    };
    for (int i=0; energy_paths[i]; i++) {
        if (read_sysfs_uint64(energy_paths[i]) > 0) {
            use_battery_energy = 1;
            snprintf(active_path, sizeof(active_path), "%s", energy_paths[i]);
            pr_info("Energy subsystem initialized: using Android battery energy (uWh)\n");
            return;
        }
    }

    /* Try Android 'charge_now' (uAh) */
    const char *charge_paths[] = {
        "/sys/class/power_supply/battery/charge_now",
        "/sys/class/power_supply/bms/charge_now",
        NULL
    };
    for (int i=0; charge_paths[i]; i++) {
        if (read_sysfs_uint64(charge_paths[i]) > 0) {
            use_battery_charge = 1;
            snprintf(active_path, sizeof(active_path), "%s", charge_paths[i]);
            pr_info("Energy subsystem initialized: using Android battery charge (uAh)\n");
            return;
        }
    }

    /* Try Android 'current_now' (uA) */
    const char *current_paths[] = {
        "/sys/class/power_supply/battery/current_now",
        "/sys/class/power_supply/bms/current_now",
        NULL
    };
    for (int i=0; current_paths[i]; i++) {
        if (read_sysfs_uint64(current_paths[i]) > 0) {
            use_battery_current = 1;
            snprintf(active_path, sizeof(active_path), "%s", current_paths[i]);
            pr_info("Energy subsystem initialized: using Android instantaneous current (uA)\n");
            return;
        }
    }

    pr_info("Energy subsystem: No standard power interfaces found (Root required?).\n");
}

void energy_start(void)
{
    start_time_ns = get_time_ns();
    if (use_rapl || use_battery_charge || use_battery_energy || use_battery_current) {
        start_energy_val = read_sysfs_uint64(active_path);
    }
}

void energy_stop(void)
{
    stop_time_ns = get_time_ns();
    if (use_rapl || use_battery_charge || use_battery_energy || use_battery_current) {
        stop_energy_val = read_sysfs_uint64(active_path);
    }
}

double energy_get_joules(void)
{
    if (use_rapl) {
        if (stop_energy_val >= start_energy_val) {
            return (double)(stop_energy_val - start_energy_val) / 1000000.0;
        }
    } else if (use_battery_energy) {
        uint64_t diff_uwh = (start_energy_val >= stop_energy_val) ? (start_energy_val - stop_energy_val) : 0;
        return (double)diff_uwh * 3600.0 / 1000000.0;
    } else if (use_battery_charge) {
        uint64_t diff_uah = (start_energy_val >= stop_energy_val) ? (start_energy_val - stop_energy_val) : 0;
        uint64_t voltage = read_battery_voltage_now(); /* uV */
        if (voltage == 0) voltage = 3800000;
        double amphours = (double)diff_uah / 1000000.0;
        double volts = (double)voltage / 1000000.0;
        return amphours * 3600.0 * volts;
    } else if (use_battery_current) {
        /* Approximate using average current over the elapsed duration */
        double duration_sec = (double)(stop_time_ns - start_time_ns) / 1000000000.0;
        
        int64_t start_c = (int64_t)start_energy_val;
        int64_t stop_c  = (int64_t)stop_energy_val;
        if (start_c < 0) start_c = -start_c;
        if (stop_c < 0) stop_c = -stop_c;
        
        uint64_t avg_current_ua = (start_c + stop_c) / 2;
        uint64_t voltage = read_battery_voltage_now();
        if (voltage == 0) voltage = 3800000;
        
        double amps = (double)avg_current_ua / 1000000.0;
        double volts = (double)voltage / 1000000.0;
        return amps * volts * duration_sec;
    }
    return 0.0;
}
