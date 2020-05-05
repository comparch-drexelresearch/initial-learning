#include "Cache.h"
#include <string.h>

/* Constants */
//const unsigned block_size = 64; // Size of a cache line (in Bytes)
// TODO, you should try different size of cache, for example, 128KB, 256KB, 512KB, 1MB, 2MB
//const unsigned cache_size = 256; // Size of a cache (in KB)
// TODO, you should try different association configurations, for example 4, 8, 16
//const unsigned assoc = 4;
int instShiftAmt = 2;

Cache *initCache(CP_Config *config)
{
    Cache *cache = (Cache *)malloc(sizeof(Cache));

    cache->blk_mask = config->block_size - 1;

    unsigned num_blocks = config->cache_size * 1024 / config->block_size;
    cache->num_blocks = num_blocks;
    //printf("Num of blocks: %u\n", cache->num_blocks);

    cache->blocks = (Cache_Block *)malloc(num_blocks * sizeof(Cache_Block));
    
    int i;
    for (i = 0; i < num_blocks; i++)
    {
        cache->blocks[i].tag = UINTMAX_MAX; 
        cache->blocks[i].valid = false;
        cache->blocks[i].dirty = false;
        cache->blocks[i].when_touched = 0;
        cache->blocks[i].frequency = 0;
        cache->blocks[i].outcome = 0;
        cache->blocks[i].prediction = 1; 
        cache->blocks[i].signature = 0;
        cache->blocks[i].RRPV = 2;
    }

    // Initialize Set-way variables
    unsigned num_sets = config->cache_size * 1024 / (config->block_size * config->assoc);
    cache->num_sets = num_sets;
    cache->num_ways = config->assoc;    
    //printf("Num of sets: %u\n", cache->num_sets);

    unsigned set_shift = log2(config->block_size);  
    cache->set_shift = set_shift;
    //printf("Set shift: %u\n", cache->set_shift);

    unsigned set_mask = num_sets - 1;
    cache->set_mask = set_mask;
    //printf("Set mask: %u\n", cache->set_mask);

    unsigned tag_shift = set_shift + log2(num_sets);
    cache->tag_shift = tag_shift;
    //printf("Tag shift: %u\n", cache->tag_shift);

    // Initialize Sets
    cache->sets = (Set *)malloc(num_sets * sizeof(Set));
    for (i = 0; i < num_sets; i++)
    {
        cache->sets[i].ways = (Cache_Block **)malloc(config->assoc * sizeof(Cache_Block *));
    }

    // Combine sets and blocks
    for (i = 0; i < num_blocks; i++)
    {
        Cache_Block *blk = &(cache->blocks[i]);
        
        uint32_t set = i / config->assoc;
        uint32_t way = i % config->assoc;

        blk->set = set;
        blk->way = way;

        cache->sets[set].ways[way] = blk;
    }

    // Initialize sat counters
    //if (!strcmp(config->cp_type, "LRU_SHiP")) {
        assert(checkPowerofTwo(config->SHCT_size));
        cache->SHCT_mask = config->SHCT_size - 1;
        cache->SHCT = (Sat_Counter *)malloc(config->SHCT_size * sizeof(Sat_Counter));
    //    printf("Sat counter bits is : %d\n", config->sat_counter_bits);
        for (i=0; i < config->SHCT_size; i++) {
            initSatCounter(&(cache->SHCT[i]), config->sat_counter_bits);
        }
    //    printf("Sat counter value is: %d\n", cache->SHCT[0].counter);
    //}

    return cache;
}

bool accessBlock(Cache *cache, Request *req, uint64_t access_time)
{
    bool hit = false;

    uint64_t blk_aligned_addr = blkAlign(req->load_or_store_addr, cache->blk_mask);

    Cache_Block *blk = findBlock(cache, blk_aligned_addr);

    if (blk != NULL) 
    {
        hit = true;
        // Update access time	
        blk->when_touched = access_time;
        // Increment frequency counter
        ++blk->frequency;
        //outcome = true, increment sat counter
        blk->outcome = 1;
        incrementCounter(&(cache->SHCT[blk->signature]));

        if (req->req_type == STORE)
        {
            blk->dirty = true;
        }
        blk->RRPV = 0;
    }
    return hit;
}

bool insertBlock(Cache *cache, Request *req, uint64_t access_time, uint64_t *wb_addr, CP_Config * config)
{
    // Step one, find a victim block
    uint64_t blk_aligned_addr = blkAlign(req->load_or_store_addr, cache->blk_mask);

    Cache_Block *victim = NULL;

    bool wb_required;
    if (!strcmp(config->cp_type, "LRU")) {
        wb_required = lru(cache, blk_aligned_addr, &victim, wb_addr);
    } else if (!strcmp(config->cp_type, "LFU")) {
        wb_required = lfu(cache, blk_aligned_addr, &victim, wb_addr);
    } else if (!strcmp(config->cp_type, "LRU_SHiP")) {
        wb_required = lru_ship(cache, blk_aligned_addr, &victim, wb_addr);
    } else if (!strcmp(config->cp_type, "SRRIP_SHiP")) {
         wb_required = srrip_ship(cache, blk_aligned_addr, &victim, wb_addr);
    }
    
    assert(victim != NULL);
    if (victim->outcome == 0) {
        decrementCounter(&(cache->SHCT[victim->signature]));
    }
    // Step two, insert the new block
    uint64_t tag = req->load_or_store_addr >> cache->tag_shift;
    victim->tag = tag;
    victim->valid = true;
    victim->signature = getSignature(req->PC, cache->SHCT_mask);

    victim->when_touched = access_time;
    ++victim->frequency;

    if (req->req_type == STORE)
    {
        victim->dirty = true;
    }

    //Get prediction from SHCT
    int prediction;
    if ((!strcmp(config->cp_type, "SHiP+LRU")) || (!strcmp(config->cp_type, "SHiP+SRRIP"))) {
        if (cache->SHCT[victim->signature].counter < 4) {
            victim->prediction = 1; // distant reference 
        } else {
            victim->prediction = 0; // immediate reference
        }
    }
    victim->RRPV = 2;

    return wb_required;
//    printf("Inserted: %"PRIu64"\n", req->load_or_store_addr);
}

// Helper Functions
inline uint64_t blkAlign(uint64_t addr, uint64_t mask)
{
    return addr & ~mask;
}

Cache_Block *findBlock(Cache *cache, uint64_t addr)
{
//    printf("Addr: %"PRIu64"\n", addr);

    // Extract tag
    uint64_t tag = addr >> cache->tag_shift;
//    printf("Tag: %"PRIu64"\n", tag);

    // Extract set index
    uint64_t set_idx = (addr >> cache->set_shift) & cache->set_mask;
//    printf("Set: %"PRIu64"\n", set_idx);

    Cache_Block **ways = cache->sets[set_idx].ways;
    int i;
    for (i = 0; i < cache->num_ways; i++)
    {
        if (tag == ways[i]->tag && ways[i]->valid == true)
        {
            return ways[i];
        }
    }

    return NULL;
}

// sat counter functions
inline void initSatCounter(Sat_Counter *sat_counter, unsigned counter_bits)
{
    sat_counter->counter_bits = counter_bits;
    sat_counter->counter = 0;
    sat_counter->max_val = (1 << counter_bits) - 1;
}

inline void incrementCounter(Sat_Counter *sat_counter)
{
    if (sat_counter->counter < sat_counter->max_val)
    {
        ++sat_counter->counter;
    }
}

inline void decrementCounter(Sat_Counter *sat_counter)
{
    if (sat_counter->counter > 0) 
    {
        --sat_counter->counter;
    }
}

inline unsigned getSignature(uint64_t branch_addr, unsigned index_mask)
{
    return (branch_addr >> instShiftAmt) & index_mask; // = modulo local_predictor_size
}

bool srrip_ship(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr) 
{
    uint64_t set_idx = (addr >> cache->set_shift) & cache->set_mask;
    //    printf("Set: %"PRIu64"\n", set_idx);
    Cache_Block **ways = cache->sets[set_idx].ways;

    // Step one, try to find an invalid block.
    int i;
    for (i = 0; i < cache->num_ways; i++) {
        if (ways[i]->valid == false)
        {
            *victim_blk = ways[i];
            return false; // No need to write-back, no need to update RRPV of block existing in cache
        }
    }

    //Step two, if there is no invalid block, victim is first block with highest RRPV
    //increment RRPV of all other blocks
    //Take into account of SHiP by prioritize SHiP over SRRIP
    Cache_Block *victim = ways[0];
    for (i=1; i< cache->num_ways; i++) {
        if (ways[i]->prediction >= victim->prediction) {
            if (ways[i]->RRPV > victim->RRPV) {
                victim = ways[i];
            }
            if (ways[i]->RRPV < 3) {
                ways[i]->RRPV++; //Update RRPV of all blocks when have to swap out a cache block
            }
        }
    }

    // Step three, need to write-back the victim block
    *wb_addr = (victim->tag << cache->tag_shift) | (victim->set << cache->set_shift);
//    uint64_t ori_addr = (victim->tag << cache->tag_shift) | (victim->set << cache->set_shift);
//    printf("Evicted: %"PRIu64"\n", ori_addr);

    // Step three, invalidate victim
    victim->tag = UINTMAX_MAX;
    victim->valid = false;
    victim->dirty = false;
    victim->frequency = 0;
    victim->when_touched = 0;


    *victim_blk = victim;

    return true; // Need to write-back

}





bool lru_ship(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr)
{
    uint64_t set_idx = (addr >> cache->set_shift) & cache->set_mask;
    //    printf("Set: %"PRIu64"\n", set_idx);
    Cache_Block **ways = cache->sets[set_idx].ways;

    // Step one, try to find an invalid block.
    int i;
    for (i = 0; i < cache->num_ways; i++)
    {
        if (ways[i]->valid == false)
        {
            *victim_blk = ways[i];
            return false; // No need to write-back
        }
    }

    //Step two, if there is no invalid block, locate the LFU block
    Cache_Block *victim = ways[0];
    for (i=1; i< cache->num_ways; i++) {
        if (ways[i]->prediction >= victim->prediction) {
            if (ways[i]->when_touched < victim->when_touched) {
                victim = ways[i];
            }
        }

    }


    // Step three, need to write-back the victim block
    *wb_addr = (victim->tag << cache->tag_shift) | (victim->set << cache->set_shift);
//    uint64_t ori_addr = (victim->tag << cache->tag_shift) | (victim->set << cache->set_shift);
//    printf("Evicted: %"PRIu64"\n", ori_addr);

    // Step three, invalidate victim
    victim->tag = UINTMAX_MAX;
    victim->valid = false;
    victim->dirty = false;
    victim->frequency = 0;
    victim->when_touched = 0;

    *victim_blk = victim;

    return true; // Need to write-back

}


bool lfu(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr)
{
    uint64_t set_idx = (addr >> cache->set_shift) & cache->set_mask;
    //    printf("Set: %"PRIu64"\n", set_idx);
    Cache_Block **ways = cache->sets[set_idx].ways;

    // Step one, try to find an invalid block.
    int i;
    for (i = 0; i < cache->num_ways; i++)
    {
        if (ways[i]->valid == false)
        {
            *victim_blk = ways[i];
            return false; // No need to write-back
        }
    }

    //Step two, if there is no invalid block, locate the LFU block
    Cache_Block *victim = ways[0];
    for (i=1; i< cache->num_ways; i++) {
        if (ways[i]->frequency < victim->frequency) {
            victim = ways[i];
        }
    }


    // Step three, need to write-back the victim block
    *wb_addr = (victim->tag << cache->tag_shift) | (victim->set << cache->set_shift);
//    uint64_t ori_addr = (victim->tag << cache->tag_shift) | (victim->set << cache->set_shift);
//    printf("Evicted: %"PRIu64"\n", ori_addr);

    // Step three, invalidate victim
    victim->tag = UINTMAX_MAX;
    victim->valid = false;
    victim->dirty = false;
    victim->frequency = 0;
    victim->when_touched = 0;

    *victim_blk = victim;

    return true; // Need to write-back

}


bool lru(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr)
{
    uint64_t set_idx = (addr >> cache->set_shift) & cache->set_mask;
    //    printf("Set: %"PRIu64"\n", set_idx);
    Cache_Block **ways = cache->sets[set_idx].ways;

    // Step one, try to find an invalid block.
    int i;
    for (i = 0; i < cache->num_ways; i++)
    {
        if (ways[i]->valid == false)
        {
            *victim_blk = ways[i];
            return false; // No need to write-back
        }
    }

    // Step two, if there is no invalid block. Locate the LRU block
    Cache_Block *victim = ways[0];
    for (i = 1; i < cache->num_ways; i++)
    {
        if (ways[i]->when_touched < victim->when_touched)
        {
            victim = ways[i];
        }
    }

    // Step three, need to write-back the victim block
    *wb_addr = (victim->tag << cache->tag_shift) | (victim->set << cache->set_shift);
//    uint64_t ori_addr = (victim->tag << cache->tag_shift) | (victim->set << cache->set_shift);
//    printf("Evicted: %"PRIu64"\n", ori_addr);

    // Step three, invalidate victim
    victim->tag = UINTMAX_MAX;
    victim->valid = false;
    victim->dirty = false;
    victim->frequency = 0;
    victim->when_touched = 0;

    *victim_blk = victim;

    return true; // Need to write-back
}

int checkPowerofTwo(unsigned x)
{
    //checks whether a number is zero or not
    if (x == 0)
    {
        return 0;
    }

    //true till x is not equal to 1
    while( x != 1)
    {
        //checks whether a number is divisible by 2
        if(x % 2 != 0)
        {
            return 0;
        }
        x /= 2;
    }
    return 1;
}