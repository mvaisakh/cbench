// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_DVFS_H
#define _CBENCH_DVFS_H

void dvfs_init(void);
void dvfs_start(void);
void dvfs_stop(const char *subsystem);

#endif /* _CBENCH_DVFS_H */
