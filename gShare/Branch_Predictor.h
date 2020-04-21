#ifndef __BRANCH_PREDICTOR_HH__
#define __BRANCH_PREDICTOR_HH__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "Instruction.h"

// saturating counter
typedef struct Sat_Counter
{
    unsigned counter_bits;
    uint8_t max_val;
    uint8_t counter;
}Sat_Counter;

// One set of arguments for all type of predictors in this simulator
// A specific predictor may not use all these values
typedef struct predictor_args {
    unsigned local_predictor_size;
    unsigned local_history_table_size;
    unsigned choice_predictor_size;
    unsigned global_predictor_size;

    unsigned local_counter_bits;
    unsigned global_counter_bits;
    unsigned choice_counter_bits;
    char *predictor_type;
}predictor_args;

// Table to keep track of all configurations for all types of predictor
typedef struct args_table {
    int num_row;

    unsigned *local_predictor_size;
    unsigned *local_history_table_size;
    unsigned *choice_predictor_size;
    unsigned *global_predictor_size;

    unsigned *local_counter_bits;
    unsigned *global_counter_bits;
    unsigned *choice_counter_bits;
    char *predictor_type;
}args_table;

typedef struct Branch_Predictor
{
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
    unsigned history_register_mask;
}Branch_Predictor;

// Initialization function
Branch_Predictor *initBranchPredictor(predictor_args *config);

// Counter functions
void initSatCounter(Sat_Counter *sat_counter, unsigned counter_bits);
void incrementCounter(Sat_Counter *sat_counter);
void decrementCounter(Sat_Counter *sat_counter);

// Branch predictor functions
bool predict(Branch_Predictor *branch_predictor, Instruction *instr, predictor_args *args);

unsigned getIndex(uint64_t branch_addr, unsigned index_mask);
bool getPrediction(Sat_Counter *sat_counter);

// Utility
int checkPowerofTwo(unsigned x);

#endif
