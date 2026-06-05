// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_ENERGY_H
#define _CBENCH_ENERGY_H

#include <stdint.h>

void energy_init(void);
void energy_start(void);
void energy_stop(void);
double energy_get_joules(void);

#endif /* _CBENCH_ENERGY_H */
