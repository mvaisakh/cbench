// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_PROFILER_H
#define _CBENCH_PROFILER_H

void profiler_init(void);
void profiler_start(void);
void profiler_stop(const char *subsystem);

#endif /* _CBENCH_PROFILER_H */
