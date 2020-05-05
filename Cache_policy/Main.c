#include "Trace.h"
#include "Cache.h"

extern TraceParser *initTraceParser(const char * mem_file);
extern bool getRequest(TraceParser *mem_trace);

extern Cache* initCache(CP_Config *config);
extern bool accessBlock(Cache *cache, Request *req, uint64_t access_time);
extern bool insertBlock(Cache *cache, Request *req, uint64_t access_time, uint64_t *wb_addr, CP_Config* config);

typedef struct CP_TABLE {
    int row;
    unsigned* block_size; // Size of a cache line (in Bytes), try 64
    // TODO, you should try different size of cache, for example, 128KB, 256KB, 512KB, 1MB, 2MB
    unsigned* cache_size; // Size of a cache (in KB), initially 256
    // TODO, you should try different association configurations, for example 4, 8, 16
    unsigned* assoc; // initially 4
    unsigned * SHCT_size;
    unsigned * sat_counter_bits;
    char* name;
 }CP_TABLE;



int main(int argc, const char *argv[])
{	
    if (argc != 2)
    {
        printf("Usage: %s %s\n", argv[0], "<mem-file>");

        return 0;
    }

    CP_Config config = (CP_Config) {
        .block_size = 64, // Size of a cache line (in Bytes), try 64
    // TODO, you should try different size of cache, for example, 128KB, 256KB, 512KB, 1MB, 2MB
        .cache_size = 256, // Size of a cache (in KB), initially 256
    // TODO, you should try different association configurations, for example 4, 8, 16
        .assoc = 4, // initially 4
        .SHCT_size = 16384,
        .sat_counter_bits = 3,
        .cp_type = "",
    };

    CP_TABLE *lru_table = malloc(sizeof(CP_TABLE));
    lru_table->row = 25;
    lru_table->name = "LRU";
    unsigned bst_arr[] = {
                        64, 64, 64, 64, 64, 
                        64, 64, 64, 64, 64, 
                        64, 64, 64, 64, 64,  
                        64, 64, 64, 64, 64,
                        64, 64, 64, 64, 64};  
                        // 64, 64, 64, 64, 64};
                        
    unsigned cs_arr[] = {
                        128, 128, 128, 128, 128, 
                        256, 256, 256, 256, 256, 
                        512, 512, 512, 512, 512,
                        1024, 1024, 1024, 1024, 1024,
                        2048, 2048, 2048, 2048, 2048};
                        // 4096, 4096, 4096, 4096, 4096 };

    unsigned assoc_arr[]  = {
                            2, 4, 8, 16, 32,
                            2, 4, 8, 16, 32,
                            2, 4, 8, 16, 32,
                            2, 4, 8, 16, 32,
                            2, 4, 8, 16, 32};
                            // 2, 4, 8, 16, 32 };
    unsigned SHCT_size_arr[] = {
                            16384, 16384, 16384, 16384, 16384,
                            16384, 16384, 16384, 16384, 16384,
                            16384, 16384, 16384, 16384, 16384,
                            16384, 16384, 16384, 16384, 16384,
                            16384, 16384, 16384, 16384, 16384};
    unsigned sat_counter_bits_arr[] = {
                            3, 3, 3, 3, 3,
                            3, 3, 3, 3, 3,
                            3, 3, 3, 3, 3,
                            3, 3, 3, 3, 3,
                            3, 3, 3, 3, 3};

    lru_table->block_size = bst_arr;
    lru_table->assoc = assoc_arr;
    lru_table->cache_size = cs_arr;


    CP_TABLE *lfu_table = malloc(sizeof(CP_TABLE));
    lfu_table->row = 25;
    lfu_table->name = "LFU";
 
    lfu_table->block_size = bst_arr;
    lfu_table->assoc = assoc_arr;
    lfu_table->cache_size = cs_arr;
    
    CP_TABLE *lru_ship_table = malloc(sizeof(CP_TABLE));
    lru_ship_table->row = 25;
    lru_ship_table->name = "LRU_SHiP";
    lru_ship_table->block_size = bst_arr;
    lru_ship_table->assoc = assoc_arr;
    lru_ship_table->cache_size = cs_arr;
    lru_ship_table->SHCT_size = SHCT_size_arr;
    lru_ship_table->sat_counter_bits = sat_counter_bits_arr;

    CP_TABLE *srrip_ship_table = malloc(sizeof(CP_TABLE));
    srrip_ship_table->row = 25;
    srrip_ship_table->name = "SRRIP_SHiP";
    srrip_ship_table->block_size = bst_arr;
    srrip_ship_table->assoc = assoc_arr;
    srrip_ship_table->cache_size = cs_arr;
    srrip_ship_table->SHCT_size = SHCT_size_arr;
    srrip_ship_table->sat_counter_bits = sat_counter_bits_arr;
    
    CP_TABLE *cp_tables[] = {lru_ship_table, srrip_ship_table};
    int num_tables =(int) ( sizeof(cp_tables) / sizeof(cp_tables[0]));
    printf("\nTrace file used: %s\n\n", argv[1]);

    for (int t = 0; t < num_tables; t++) {
        config.cp_type = cp_tables[t]->name;
        for( int r = 0; r < cp_tables[t]->row; r++) {
            if ((!strcmp(cp_tables[t]->name, "LRU")) || (!strcmp(cp_tables[t]->name, "LFU"))) {
                config.block_size = cp_tables[t]->block_size[r];
                config.assoc = cp_tables[t]->assoc[r];
                config.cache_size = cp_tables[t]->cache_size[r];
                printf("%s, %u, %3u, %5u, ", config.cp_type, config.block_size, config.assoc, config.cache_size);
            } else if ((!strcmp(cp_tables[t]->name, "LRU_SHiP")) || (!strcmp(cp_tables[t]->name, "SRRIP_SHiP"))) {
                config.block_size = cp_tables[t]->block_size[r];
                config.assoc = cp_tables[t]->assoc[r];
                config.cache_size = cp_tables[t]->cache_size[r];
                config.SHCT_size = cp_tables[t]->SHCT_size[r];
                config.sat_counter_bits = cp_tables[t]->sat_counter_bits[r];
                printf("%s, %u, %3u, %5u, ", config.cp_type, config.block_size, config.assoc, config.cache_size);
            }

            // Initialize a CPU trace parser
            TraceParser *mem_trace = initTraceParser(argv[1]);

            // Initialize a Cache
            Cache *cache = initCache(&config);
            //printf("Done init\n");

            // Running the trace
            uint64_t num_of_reqs = 0;
            uint64_t hits = 0;
            uint64_t misses = 0;
            uint64_t num_evicts = 0;

            uint64_t cycles = 0;
            while (getRequest(mem_trace))
            {
                // Step one, accessBlock()
                if (accessBlock(cache, mem_trace->cur_req, cycles))
                {
                    // Cache hit
                    hits++;
                }
                else
                {
                    // Cache miss!
                    misses++;
                    // Step two, insertBlock()
                    //printf("Inserting: %"PRIu64"\n", mem_trace->cur_req->load_or_store_addr);
                    uint64_t wb_addr;
                    if (insertBlock(cache, mem_trace->cur_req, cycles, &wb_addr, &config))
                    {
                        num_evicts++;
                        //printf("Evicted: %"PRIu64"\n", wb_addr);
                    }
                }

                ++num_of_reqs;
                ++cycles;
            }

            double hit_rate = (double)hits / ((double)hits + (double)misses);
            printf("%3lf%%\n", hit_rate * 100);
            free(cache->blocks);
            free(cache->sets);
            free(cache);
        }
        printf("\n-----------------------------------------\n");
    }
    free(lru_table);
    free(lfu_table);
    free(srrip_ship_table);
    free(lru_ship_table);    
    
}
