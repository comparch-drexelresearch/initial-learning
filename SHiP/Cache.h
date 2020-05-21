#ifndef __CACHE_H__
#define __CACHE_H__

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <stdint.h>

#include "Cache_Blk.h"
#include "Request.h"
#include "SHCT.h"


/* Cache */
typedef struct Set
{
    Cache_Block **ways; // Block ways within a set
}Set;

typedef struct Cache
{
    char policy[3];   // Replacement policy implemented in cache

    uint64_t blk_mask;
    unsigned num_blocks;
    
    Cache_Block *blocks; // All cache blocks

    /* Set-Associative Information */
    unsigned num_sets; // Number of sets
    unsigned num_ways; // Number of ways within a set

    unsigned set_shift;
    unsigned set_mask; // To extract set index
    unsigned tag_shift; // To extract tag

    Set *sets; // All the sets of a cache
    
}Cache;

// Function Definitions
Cache *initCache(unsigned cache_size, unsigned assoc, char *policy);
bool accessBlock(Cache *cache, Request *req, uint64_t access_time, SHCT *signature_table);
bool insertBlock(Cache *cache, Request *req, uint64_t access_time, uint64_t *wb_addr, SHCT *signature_table);

// Helper Function
uint64_t blkAlign(uint64_t addr, uint64_t mask);
Cache_Block *findBlock(Cache *cache, uint64_t addr);

// Replacement Policies
bool lru(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr, SHCT *signature_table);
bool lfu(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr);
#endif
