#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "cbench.h"
#include "utils.h"

static void usage(const char *progname)
{
    printf("Usage: %s [options]\n", progname);
    printf("Options:\n");
    printf("  -h, --help       Show this message\n");
    printf("  -a, --all        Run all benchmarks\n");
}

int main(int argc, char **argv)
{
    int opt;
    int run_all = 0;

    static struct option long_options[] = {
        {"help",    no_argument, 0, 'h'},
        {"all",     no_argument, 0, 'a'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "ha", long_options, NULL)) != -1) {
        switch (opt) {
        case 'a':
            run_all = 1;
            break;
        case 'h':
        default:
            usage(argv[0]);
            return 0;
        }
    }

    pr_info("Starting Cerium Benchmarking (cbench)...\n");

    if (!run_all) {
        pr_info("No benchmarks selected. Use -a to run all.\n");
        return 0;
    }

    pr_info("Run complete.\n");
    return 0;
}
