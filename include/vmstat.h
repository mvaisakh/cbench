// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_VMSTAT_H
#define _CBENCH_VMSTAT_H

void vmstat_init(void);
void vmstat_start(void);
void vmstat_stop(const char *subsystem);

#endif /* _CBENCH_VMSTAT_H */
