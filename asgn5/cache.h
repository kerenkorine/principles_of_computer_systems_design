#ifndef CACHE_H
#define CACHE_H

#include "bool.h"
#include "config.h"

#define CHAR_OPTS (256)

typedef enum { HIT, MISS } InsertionResult;

typedef enum { NONE, COMPULSORY, CAPACITY } MissType;

typedef struct {
    InsertionResult insertion;
    MissType missType;
} CacheResult;

typedef struct {
    char ch;
    // used for clock algorithm
    boolean referenced;
} Entry;

typedef struct _Cache {
    Entry *data;
    boolean seen[CHAR_OPTS];
    size_t size;
    size_t capacity;
    // used for clock algorithm
    size_t clock_hand;
    CacheResult (*insert)(struct _Cache *, char);
} Cache;

CacheResult fifoInsert(Cache *cache, char ch);
CacheResult lruInsert(Cache *cache, char ch);
CacheResult clockInsert(Cache *cache, char ch);
Cache *createCache(Configuration *config);
void destroyCache(Cache *cache);

#endif
