#ifndef __SHCT_H__
#define __SHCT_H__

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <stdint.h>


/* Signature History Counter Table (SHCT) */
typedef struct Sat_Counter
{
    unsigned counter_bits;
    uint8_t max_val;
    uint8_t counter;
}Sat_Counter;

typedef struct SHCT
{
    unsigned signature_table_size;
    unsigned signature_table_mask;
    Sat_Counter *counters;
}SHCT;

// Function Definitions

SHCT *init_SHCT(unsigned table_size, unsigned counter_bits);

// Counter function
void initSatCounter(Sat_Counter *sat_counter, unsigned counter_bits);
void incrementCounter(Sat_Counter *sat_counter);
void decrementCounter(Sat_Counter *sat_counter);

// Utility
int checkPowerofTwo(unsigned x);

#endif
