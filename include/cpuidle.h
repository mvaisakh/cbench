// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_CPUIDLE_H
#define _CBENCH_CPUIDLE_H

void cpuidle_init(void);
void cpuidle_start(void);
void cpuidle_stop(const char *subsystem);

#endif /* _CBENCH_CPUIDLE_H */
