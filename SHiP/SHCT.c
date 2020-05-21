#include "SHCT.h"

SHCT *init_SHCT(unsigned table_size, unsigned counter_bits) {
    SHCT *table = (SHCT *)malloc(sizeof(SHCT));
    table->signature_table_size = table_size;
    assert(checkPowerofTwo(table->signature_table_size));

    table->signature_table_mask = table->signature_table_size - 1;

    // Initialize Sat Counters
    table->counters = \
    (Sat_Counter *)malloc(table->signature_table_size*sizeof(Sat_Counter));

    int i = 0;
    for (i = 0; i < table->signature_table_size; i++) {
        initSatCounter(&(table->counters[i]), counter_bits);
    }
    return table;
}

inline void initSatCounter(Sat_Counter *sat_counter, unsigned counter_bits)
{
    sat_counter->counter_bits = counter_bits;
    sat_counter->counter = (1 << (counter_bits - 1)) - 1;
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