// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_SOFTIRQ_H
#define _CBENCH_SOFTIRQ_H

void softirq_init(void);
void softirq_start(void);
void softirq_stop(const char *subsystem);

#endif /* _CBENCH_SOFTIRQ_H */
