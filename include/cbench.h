// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_H
#define _CBENCH_H

#include <stdio.h>

#define pr_info(fmt, ...)  printf("cbench: " fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...)   fprintf(stderr, "cbench: ERROR: " fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...)  fprintf(stderr, "cbench: WARNING: " fmt, ##__VA_ARGS__)

extern int num_threads;

#endif /* _CBENCH_H */
