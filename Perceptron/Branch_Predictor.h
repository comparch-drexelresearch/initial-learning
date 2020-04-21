#ifndef __BRANCH_PREDICTOR_HH__
#define __BRANCH_PREDICTOR_HH__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "Instruction.h"

// Prediction values
#define TAKEN 1
#define NOT_TAKEN -1

// saturating counter
typedef struct Sat_Counter
{
    unsigned counter_bits;
    uint8_t max_val;
    uint8_t counter;
}Sat_Counter;

typedef struct Perceptron 
{
    signed *weights;
    unsigned max_val;
}Perceptron;

// Arguments for perceptron predictor
typedef struct predictor_args {
    unsigned perceptron_table_size;
    unsigned history_length;
}predictor_args;

// Table to keep track of all configurations for perceptron predictor
typedef struct args_table {
    int num_row;
    int num_col;
    unsigned *perceptron_table_size;
    unsigned *history_length;
}args_table;

typedef struct Branch_Predictor
{
    unsigned threshold;
    unsigned weight_bits;
    unsigned perceptron_table_size;
    signed *global_history;
    unsigned global_history_mask;
    Perceptron *Perceptron_table;
}Branch_Predictor;

// Initialization function
Branch_Predictor *initBranchPredictor(predictor_args *config);

// Counter functions
void initSatCounter(Sat_Counter *sat_counter, unsigned counter_bits);
void incrementCounter(Sat_Counter *sat_counter);
void decrementCounter(Sat_Counter *sat_counter);

// Perceptron functions
void initPerceptron(Perceptron *perceptron, unsigned num_weights, unsigned weight_bits, unsigned threshold);
signed get_dot_product(signed *weights, signed *global_history, unsigned history_length);
void train_perceptron(Perceptron *perceptron, signed *global_history, unsigned history_length, signed result);

// Branch predictor functions
bool predict(Branch_Predictor *branch_predictor, Instruction *instr, predictor_args *args);

unsigned getIndex(uint64_t branch_addr, unsigned index_mask);
bool getPrediction(Sat_Counter *sat_counter);

// Utility
int checkPowerofTwo(unsigned x);

#endif
