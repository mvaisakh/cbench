// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "energy.h"
#include "cbench.h"
#include "utils.h"

static int use_rapl = 0;
static int use_battery = 0;
static uint64_t start_energy_uj = 0;
static uint64_t stop_energy_uj = 0;

static uint64_t read_rapl_uj(void)
{
    FILE *f = fopen("/sys/class/powercap/intel-rapl:0/energy_uj", "r");
    if (!f) return 0;
    uint64_t val = 0;
    if (fscanf(f, "%llu", (unsigned long long*)&val) != 1) val = 0;
    fclose(f);
    return val;
}

/* Coarse battery reading if RAPL is missing */
static uint64_t read_battery_charge_now(void)
{
    FILE *f = fopen("/sys/class/power_supply/battery/charge_now", "r");
    if (!f) return 0;
    uint64_t val = 0;
    if (fscanf(f, "%llu", (unsigned long long*)&val) != 1) val = 0;
    fclose(f);
    return val;
}

static uint64_t read_battery_voltage_now(void)
{
    FILE *f = fopen("/sys/class/power_supply/battery/voltage_now", "r");
    if (!f) return 0;
    uint64_t val = 0;
    if (fscanf(f, "%llu", (unsigned long long*)&val) != 1) val = 0;
    fclose(f);
    return val;
}

void energy_init(void)
{
    if (read_rapl_uj() > 0) {
        use_rapl = 1;
        pr_info("Energy subsystem initialized: using intel-rapl\n");
    } else if (read_battery_charge_now() > 0) {
        use_battery = 1;
        pr_info("Energy subsystem initialized: using battery capacity\n");
    } else {
        pr_info("Energy subsystem: No standard power interfaces found.\n");
    }
}

void energy_start(void)
{
    if (use_rapl) {
        start_energy_uj = read_rapl_uj();
    } else if (use_battery) {
        start_energy_uj = read_battery_charge_now();
    }
}

void energy_stop(void)
{
    if (use_rapl) {
        stop_energy_uj = read_rapl_uj();
    } else if (use_battery) {
        stop_energy_uj = read_battery_charge_now();
    }
}

double energy_get_joules(void)
{
    if (use_rapl) {
        if (stop_energy_uj >= start_energy_uj) {
            return (double)(stop_energy_uj - start_energy_uj) / 1000000.0;
        } else {
            /* Handle wrap-around */
            return 0.0;
        }
    } else if (use_battery) {
        uint64_t diff_charge = (start_energy_uj >= stop_energy_uj) ? (start_energy_uj - stop_energy_uj) : 0;
        uint64_t voltage = read_battery_voltage_now(); /* uV */
        if (voltage == 0) voltage = 3800000; /* assume 3.8V */
        /* charge_now is uAh. Joules = (uAh * 3600 / 1000000) * (uV / 1000000) */
        double amphours = (double)diff_charge / 1000000.0;
        double volts = (double)voltage / 1000000.0;
        return amphours * 3600.0 * volts;
    }
    return 0.0;
}
