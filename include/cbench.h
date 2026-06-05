#ifndef _CBENCH_H
#define _CBENCH_H

#include <stdio.h>

#define pr_info(fmt, ...)  printf("cbench: " fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...)   fprintf(stderr, "cbench: ERROR: " fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...)  fprintf(stderr, "cbench: WARNING: " fmt, ##__VA_ARGS__)

#endif /* _CBENCH_H */
