#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>

typedef enum { FIFO = 'F', LRU = 'L', CLOCK = 'C' } Policy;

typedef struct {
    Policy policy;
    size_t cache_size;
} Configuration;

int parseArgs(int argc, char *argv[], Configuration *config);

#endif
