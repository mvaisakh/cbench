// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Project Cerium

#ifndef _CBENCH_H
#define _CBENCH_H

#define CBENCH_EXIT_SUCCESS      0
#define CBENCH_EXIT_ERROR        1
#define CBENCH_EXIT_PARTIAL      2
#define CBENCH_EXIT_PERMISSION   3


#include <stdio.h>

#define pr_info(fmt, ...)  printf("cbench: " fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...)   fprintf(stderr, "cbench: ERROR: " fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...)  fprintf(stderr, "cbench: WARNING: " fmt, ##__VA_ARGS__)

extern int num_threads;
extern int benchmark_duration_sec;

#endif /* _CBENCH_H */
