#include "Trace.h"
#include "Cache.h"

extern TraceParser *initTraceParser(const char * mem_file);
extern bool getRequest(TraceParser *mem_trace);

extern Cache* initCache();
extern bool accessBlock(Cache *cache, Request *req, uint64_t access_time);
extern bool insertBlock(Cache *cache, Request *req, uint64_t access_time, uint64_t *wb_addr);

int main(int argc, char *argv[])
{	
    if (argc != 3)
    {
        printf("Usage: %s %s %s\n", argv[0], "<mem-file>", "<policy>");

        return 0;
    }

    int i, j;
    unsigned cache_size[] = {128, 256, 512, 1024, 2048};
    unsigned assoc[] = {4, 8, 16};

    int num_row = (int) (sizeof(cache_size)/sizeof(cache_size[0]));
    int num_col = (int) (sizeof(assoc)/sizeof(assoc[0]));
    char policy[3];
    strncpy(policy, argv[2], 3);

    for (i = 0; i < num_row; i++) {
        for (j = 0; j < num_col; j++) {
            // Initialize a CPU trace parser
            TraceParser *mem_trace = initTraceParser(argv[1]);

            // Initialize a Cache
            Cache *cache = initCache(cache_size[i], assoc[j], policy);
            
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
        //            printf("Inserting: %"PRIu64"\n", mem_trace->cur_req->load_or_store_addr);
                    uint64_t wb_addr;
                    if (insertBlock(cache, mem_trace->cur_req, cycles, &wb_addr))
                    {
                        num_evicts++;
        //                printf("Evicted: %"PRIu64"\n", wb_addr);
                    }
                }

                ++num_of_reqs;
                ++cycles;
            }

            double hit_rate = (double)hits / ((double)hits + (double)misses);
            printf("%d\t%d\t%lf%%\n", cache_size[i], assoc[j], (hit_rate * 100));
        }
    }
    printf("___________________\n");
}
