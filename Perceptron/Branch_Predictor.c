#include "Branch_Predictor.h"

const unsigned instShiftAmt = 2; // Number of bits to shift a PC by

Branch_Predictor *initBranchPredictor(predictor_args *config)
{
    Branch_Predictor *branch_predictor = (Branch_Predictor *)malloc(sizeof(Branch_Predictor));

    branch_predictor->perceptron_table_size = config->perceptron_table_size;
    assert(checkPowerofTwo(branch_predictor->perceptron_table_size));
    branch_predictor->threshold = (unsigned) ceil(1.93 * (config->history_length) + 14);
    branch_predictor->weight_bits = (unsigned) ceil(1 + log2(branch_predictor->threshold));

    // Initialize Perceptron table
    branch_predictor->Perceptron_table = 
        (Perceptron *)malloc(branch_predictor->perceptron_table_size * sizeof(Perceptron));

    int i;
    for (i = 0; i < branch_predictor->perceptron_table_size; i++) {
        initPerceptron(&(branch_predictor->Perceptron_table[i]), 
                config->history_length, branch_predictor->weight_bits, branch_predictor->threshold);
    }

    // Initialize Global history register
    branch_predictor->global_history = (signed *)malloc(config->history_length * sizeof(signed));
    for (i = 0; i < config->history_length; i++) {
        branch_predictor->global_history[i] = NOT_TAKEN;
    }

    return branch_predictor;
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

// Branch Predictor functions
inline void initPerceptron(Perceptron *perceptron, unsigned num_weights, unsigned weight_bits, unsigned threshold) {
    perceptron->weights = (signed *)malloc(num_weights * sizeof(signed));
    int i;
    time_t t;
    srand((unsigned) time(&t));
    for (i = 0; i < num_weights; i++) {
        perceptron->weights[i] = 0;
        // perceptron->weights[i] = ((signed)rand() % 2*threshold) - threshold;
        perceptron->max_val = (1 << weight_bits) - 1;
    }
}

bool predict(Branch_Predictor *branch_predictor, Instruction *instr, predictor_args *config)
{
    uint64_t branch_address = instr->PC;
    int i = 0;


    unsigned perceptron_index = getIndex(branch_address, branch_predictor->perceptron_table_size - 1);
    signed y = get_dot_product(
        branch_predictor->Perceptron_table[perceptron_index].weights,
        branch_predictor->global_history, config->history_length);
        
    bool prediction;

    // Predict branch direction
    if (y >= 0) {
        prediction = 1;
    }
    else {
        prediction = 0;
    }

    signed t;
    if (instr->taken) {
        t = TAKEN;
    }
    else {
        t = NOT_TAKEN;
    }
    // Training selected perceptron if necessary
    if ((instr->taken ^ prediction) || (abs(y) <= branch_predictor->threshold)) {
        train_perceptron(&(branch_predictor->Perceptron_table[perceptron_index]), 
            branch_predictor->global_history, config->history_length, t);
    }

    // Update Global History Register
    for (i = config->history_length - 1; i > 0; i--) {
        branch_predictor->global_history[i] = branch_predictor->global_history[i-1];
    }
    if (instr->taken) {
        branch_predictor->global_history[0] = TAKEN;
    }
    else {
        branch_predictor->global_history[0] = NOT_TAKEN;
    }

    return prediction == instr->taken;
}

inline signed get_dot_product(signed *weights, signed *global_history, unsigned history_length) {
    signed dotProduct = weights[0];
    for (int i = 1; i < history_length; i++) {
        dotProduct += weights[i] * global_history[i];
    }
    return dotProduct;
}

inline void train_perceptron(Perceptron *perceptron, signed *global_history, unsigned history_length, signed result) {
    signed sum = perceptron->weights[0] + result;
    if (abs(sum) < perceptron->max_val) {
        perceptron->weights[0] = sum;
    }
    for (int i = 1; i < history_length; i++) {
        sum = perceptron->weights[i] + result * global_history[i];
        if (abs(sum) < perceptron->max_val) {
            perceptron->weights[i] = sum;
        }
    }
    return;
}

inline unsigned getIndex(uint64_t branch_addr, unsigned index_mask)
{
    return (branch_addr >> instShiftAmt) & index_mask;
}

inline bool getPrediction(Sat_Counter *sat_counter)
{
    uint8_t counter = sat_counter->counter;
    unsigned counter_bits = sat_counter->counter_bits;

    // MSB determins the direction
    return (counter >> (counter_bits - 1));
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
