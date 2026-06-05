// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_THERMAL_H
#define _CBENCH_THERMAL_H

void thermal_init(void);
void thermal_start(void);
void thermal_stop(const char *subsystem);

#endif /* _CBENCH_THERMAL_H */
