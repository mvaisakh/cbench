// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_HWPERF_H
#define _CBENCH_HWPERF_H

void hwperf_init(void);
void hwperf_start(void);
void hwperf_stop(const char *subsystem);

#endif /* _CBENCH_HWPERF_H */
