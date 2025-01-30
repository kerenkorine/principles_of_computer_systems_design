#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "config.h"
#include "bool.h"

#define BUFFER_SIZE (256)

int main(int argc, char *argv[]) {
    Configuration config = { FIFO, 0 };

    if (parseArgs(argc, argv, &config) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    Cache *cache = createCache(&config);
    if (cache == NULL) {
        return EXIT_FAILURE;
    }

    size_t compulsory_misses = 0;
    size_t capacity_misses = 0;
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (strlen(line) < 1) {
            continue;
        }
        // we can assume that every entry is a single character;
        char entry = line[0];
        CacheResult result = cache->insert(cache, entry);
        printf("%s\n", result.insertion == MISS ? "MISS" : "HIT");
        switch (result.missType) {
        case COMPULSORY: compulsory_misses += 1; break;
        case CAPACITY: capacity_misses += 1; break;
        case NONE: break;
        }
    }
    printf("%lu %lu\n", compulsory_misses, capacity_misses);

    destroyCache(cache);
    return EXIT_SUCCESS;
}
