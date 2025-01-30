#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include "config.h"
#include "bool.h"

int parseArgs(int argc, char *argv[], Configuration *config) {
    const char *const usage_error = "USAGE:\n\t./cacher [-N size] <policy>\n";
    boolean received_policy = FALSE;
    boolean received_cache_size = FALSE;
    char *endptr = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "FLCN:")) != -1) {
        switch (opt) {
        case 'N':
            config->cache_size = strtoull(optarg, &endptr, 10);
            if (endptr && *endptr != '\0') {
                fprintf(stderr, usage_error);
                return EXIT_FAILURE;
            }
            received_cache_size = TRUE;
            break;
        case 'F':
        case 'L':
        case 'C':
            received_policy = TRUE;
            config->policy = opt;
            break;
        }
    }

    if (!received_policy || !received_cache_size) {
        fprintf(stderr, usage_error);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
