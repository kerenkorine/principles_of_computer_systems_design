#include "cache.h"
#include <stdlib.h>
#include <string.h>

static size_t findEntry(Entry *data, size_t size, char ch) {
    for (size_t i = 0; i < size; i++) {
        if (data[i].ch == ch) {
            return i;
        }
    }
    return size + 1;
}

void destroyCache(Cache *cache) {
    free(cache->data);
    free(cache);
}

Cache *createCache(Configuration *config) {
    Cache *new_cache = malloc(sizeof(Cache));
    if (new_cache == NULL) {
        fprintf(stderr, "Memory Error: malloc returned null\n");
        return NULL;
    }

    switch (config->policy) {
    case FIFO: new_cache->insert = fifoInsert; break;
    case LRU: new_cache->insert = lruInsert; break;
    case CLOCK: new_cache->insert = clockInsert; break;
    default:
        fprintf(stderr, "Config Error: invalid cache policy\n");
        free(new_cache);
        return NULL;
    }

    for (int i = 0; i < CHAR_OPTS; i++) {
        new_cache->seen[i] = FALSE;
    }

    new_cache->capacity = config->cache_size;
    new_cache->size = 0;
    new_cache->clock_hand = 0;
    new_cache->data = malloc(config->cache_size * sizeof(Entry));
    if (new_cache->data == NULL) {
        fprintf(stderr, "Memory Error: malloc returned null\n");
        free(new_cache);
        return NULL;
    }
    for (size_t i = 0; i < new_cache->capacity; i++) {
        new_cache->data[i].referenced = FALSE;
    }
    return new_cache;
}

CacheResult fifoInsert(Cache *cache, char ch) {
    // have we seen ch before?
    boolean seenBefore = cache->seen[(int) ch];
    // mark that we've seen ch now
    cache->seen[(int) ch] = TRUE;

    // is already in cache?
    size_t index = findEntry(cache->data, cache->size, ch);
    if (index < cache->size) {
        return (CacheResult) { .insertion = HIT, .missType = NONE };
    }

    // is there still free space?
    if (cache->size < cache->capacity) {
        cache->data[cache->size].ch = ch;
        cache->size += 1;
        return (CacheResult) { .insertion = MISS, .missType = COMPULSORY };
    }

    // we must replace an old entry with the new one
    size_t move_size = (cache->size - 1) * sizeof(Entry);
    memmove(&cache->data[0], &cache->data[1], move_size);
    cache->data[cache->size - 1].ch = ch;

    return (CacheResult) { .insertion = MISS, .missType = seenBefore ? CAPACITY : COMPULSORY };
}

CacheResult lruInsert(Cache *cache, char ch) {
    // have we seen ch before?
    boolean seenBefore = cache->seen[(int) ch];
    // mark that we've seen ch now
    cache->seen[(int) ch] = TRUE;

    // is already in cache?
    size_t index = findEntry(cache->data, cache->size, ch);
    if (index < cache->size) {
        // move the entry to the last cell, if not already there
        if (index < cache->size - 1) {
            char cacheEntry = cache->data[index].ch;
            size_t move_size = (cache->size - index - 1) * sizeof(Entry);
            memmove(&cache->data[index], &cache->data[index + 1], move_size);
            cache->data[cache->size - 1].ch = cacheEntry;
        }
        return (CacheResult) { .insertion = HIT, .missType = NONE };
    }

    // is there still free space?
    if (cache->size < cache->capacity) {
        cache->data[cache->size].ch = ch;
        cache->size += 1;
        return (CacheResult) { .insertion = MISS, .missType = COMPULSORY };
    }

    // we must replace an old entry with the new one
    size_t move_size = (cache->size - 1) * sizeof(Entry);
    memmove(&cache->data[0], &cache->data[1], move_size);
    cache->data[cache->size - 1].ch = ch;

    return (CacheResult) { .insertion = MISS, .missType = seenBefore ? CAPACITY : COMPULSORY };
}

CacheResult clockInsert(Cache *cache, char ch) {
    // have we seen ch before?
    boolean seenBefore = cache->seen[(int) ch];
    // mark that we've seen ch now
    cache->seen[(int) ch] = TRUE;

    // is already in cache?
    size_t index = findEntry(cache->data, cache->size, ch);
    if (index < cache->size) {
        // set referenced
        cache->data[index].referenced = TRUE;
        return (CacheResult) { .insertion = HIT, .missType = NONE };
    }

    // is there still free space?
    if (cache->size < cache->capacity) {
        cache->data[cache->size].ch = ch;
        cache->size += 1;
        return (CacheResult) { .insertion = MISS, .missType = COMPULSORY };
    }

    size_t index_to_delete = cache->clock_hand;
    while (TRUE) {
        if (cache->data[index_to_delete].referenced) {
            cache->data[index_to_delete].referenced = FALSE;
        } else {
            cache->data[index_to_delete].ch = ch;
            cache->clock_hand = (index_to_delete + 1) % cache->size;
            break;
        }
        index_to_delete = (index_to_delete + 1) % cache->size;
    }
    return (CacheResult) { .insertion = MISS, .missType = seenBefore ? CAPACITY : COMPULSORY };
}
