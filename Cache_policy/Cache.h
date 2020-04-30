#ifndef __CACHE_H__
#define __CACHE_H__

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>

#include <math.h>
#include <stdint.h>

#include "Cache_Blk.h"
#include "Request.h"

#define LRU


typedef struct CP_Config 
{
    unsigned block_size; // Size of a cache line (in Bytes), try 64
    // TODO, you should try different size of cache, for example, 128KB, 256KB, 512KB, 1MB, 2MB
    unsigned cache_size; // Size of a cache (in KB), initially 256
    // TODO, you should try different association configurations, for example 4, 8, 16
    unsigned assoc; // initially 4
    char* cp_type;
}CP_Config;


/* Cache */
typedef struct Set
{
    Cache_Block **ways; // Block ways within a set
}Set;

typedef struct Cache
{
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
Cache *initCache(CP_Config *config);
bool accessBlock(Cache *cache, Request *req, uint64_t access_time, CP_Config *config);
bool insertBlock(Cache *cache, Request *req, uint64_t access_time, uint64_t *wb_addr, CP_Config *config);

// Helper Function
uint64_t blkAlign(uint64_t addr, uint64_t mask);
Cache_Block *findBlock(Cache *cache, uint64_t addr);

// Replacement Policies
bool lru(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr);
bool lfu(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr);
#endif
