// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_TELEMETRY_H
#define _CBENCH_TELEMETRY_H

void telemetry_init(void);
void telemetry_start(void);
void telemetry_stop(const char *subsystem);
void telemetry_deinit(void);

#endif /* _CBENCH_TELEMETRY_H */
