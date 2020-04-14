#ifndef __BRANCH_PREDICTOR_HH__
#define __BRANCH_PREDICTOR_HH__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "Instruction.h"

// // You can play around with these settings.
// #define localPredictorSize 2048
// #define localCounterBits 2
// #define localHistoryTableSize 4096 
// #define globalPredictorSize 16384
// #define globalCounterBits 2 
// #define choicePredictorSize 16384 // Keep this the same as globalPredictorSize.
// #define choiceCounterBits 2
// #define gsharePredictorSize 2048
// #define gshareCounterBits 2

// saturating counter
typedef struct Sat_Counter
{
    unsigned counter_bits;
    uint8_t max_val;
    uint8_t counter;
}Sat_Counter;

typedef struct BP_Config 
{
    unsigned local_predictor_size;
    unsigned local_history_table_size;
    unsigned local_counter_bits;
 
    unsigned global_predictor_size;
    unsigned global_counter_bits;    
    
    unsigned choice_predictor_size;
    unsigned choice_counter_bits;
    
    unsigned gshare_predictor_size;
    unsigned gshare_counter_bits;

    char* bp_type;
}BP_Config;

typedef struct Branch_Predictor
{

    unsigned local_predictor_sets; // Number of entries in a local predictor
    unsigned index_mask;

    unsigned local_predictor_size;
    unsigned local_predictor_mask;
    Sat_Counter *local_counters;

    unsigned local_history_table_size;
    unsigned local_history_table_mask;
    unsigned *local_history_table;

    unsigned global_predictor_size;
    unsigned global_history_mask;
    Sat_Counter *global_counters;

    unsigned choice_predictor_size;
    unsigned choice_history_mask;
    Sat_Counter *choice_counters;

    uint64_t global_history;

    unsigned gshare_predictor_size;
    unsigned gshare_predictor_mask;
    Sat_Counter *gshare_counters;
    uint64_t gshare_history;
    unsigned history_register_mask;

}Branch_Predictor;

// Initialization function
Branch_Predictor *initBranchPredictor(BP_Config *config);

// Counter functions
void initSatCounter(Sat_Counter *sat_counter, unsigned counter_bits);
void incrementCounter(Sat_Counter *sat_counter);
void decrementCounter(Sat_Counter *sat_counter);

// Branch predictor functions
bool predict(Branch_Predictor *branch_predictor, Instruction *instr, BP_Config *config);

unsigned getIndex(uint64_t branch_addr, unsigned index_mask);
bool getPrediction(Sat_Counter *sat_counter);

// Utility
int checkPowerofTwo(unsigned x);

#endif
