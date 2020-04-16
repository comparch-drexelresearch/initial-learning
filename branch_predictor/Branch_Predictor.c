#include "Branch_Predictor.h"
#include <string.h>

// The traces we are using, x86-based, variable-length
// RISC architecture: risc-v/arm, 4 bytes offset
// CICS architecture, variable offset
// instShiftAmt, experimental number
// branch_adddr >> instShiftAmt, 
const unsigned instShiftAmt = 2; // Number of bits to shift a PC by


Branch_Predictor *initBranchPredictor(BP_Config *config)
{
    Branch_Predictor *branch_predictor = (Branch_Predictor *)malloc(sizeof(Branch_Predictor));

    if (!strcmp(config->bp_type, "twobitlocal")) {
        branch_predictor->local_predictor_sets = config->local_predictor_size;
        assert(checkPowerofTwo(branch_predictor->local_predictor_sets));

        branch_predictor->index_mask = branch_predictor->local_predictor_sets - 1;

        // Initialize sat counters
        branch_predictor->local_counters =
            (Sat_Counter *)malloc(branch_predictor->local_predictor_sets * sizeof(Sat_Counter));

        int i = 0;
        for (i; i < branch_predictor->local_predictor_sets; i++) {
            initSatCounter(&(branch_predictor->local_counters[i]), config->local_counter_bits);
        }

    } else if (!strcmp(config->bp_type, "gshare")) {
        assert(checkPowerofTwo(config->gshare_predictor_size));
        branch_predictor->gshare_predictor_size = config->gshare_predictor_size;

        branch_predictor->gshare_counters = 
            (Sat_Counter *)malloc(config->gshare_predictor_size *sizeof(Sat_Counter));
        
        for ( int i=0; i < config->gshare_predictor_size; i++) {
            initSatCounter(&(branch_predictor->gshare_counters[i]), config->gshare_counter_bits);
        }

        branch_predictor->gshare_history = 0;
        branch_predictor->gshare_predictor_mask = config->gshare_predictor_size - 1;

    } else if (!strcmp(config->bp_type, "perceptron")) {

        assert(checkPowerofTwo(config->perceptron_table_size));
        unsigned weight_bits = ceil((1 + log2(1.93*config->global_history_bits + 14)));

        branch_predictor->threshold = ceil(1.93*config->global_history_bits + 14); 
        branch_predictor->perceptron_table_size = config->perceptron_table_size;
        if ((branch_predictor->perceptron_table = (Perceptron *)malloc(config->perceptron_table_size*sizeof(Perceptron))) == NULL) {
            printf("Malloc failed!");
        };
        
        for (int i=0; i<config->perceptron_table_size; i++) {
            initPerceptron(&(branch_predictor->perceptron_table[i]), weight_bits, config->global_history_bits);
        }
        branch_predictor->global_history = 0; 
        branch_predictor->global_history_mask = (1 << config->global_history_bits) - 1;

    } else if (!strcmp(config->bp_type, "tournament")) {
        assert(checkPowerofTwo(config->local_predictor_size));
        assert(checkPowerofTwo(config->local_history_table_size));
        assert(checkPowerofTwo(config->global_predictor_size));
        assert(checkPowerofTwo(config->choice_predictor_size));
        assert(config->global_predictor_size == config->choice_predictor_size);

        branch_predictor->local_predictor_size = config->local_predictor_size;
        branch_predictor->local_history_table_size = config->local_history_table_size;
        branch_predictor->global_predictor_size = config->global_predictor_size;
        branch_predictor->choice_predictor_size = config->choice_predictor_size;
    
        // Initialize local counters 
        branch_predictor->local_counters =
            (Sat_Counter *)malloc(config->local_predictor_size* sizeof(Sat_Counter));

        int i = 0;
        for (i; i < config->local_predictor_size; i++)
        {
            initSatCounter(&(branch_predictor->local_counters[i]), config->local_counter_bits);
        }

        branch_predictor->local_predictor_mask = config->local_predictor_size - 1;

        // Initialize local history table
        branch_predictor->local_history_table = 
            (unsigned *)malloc(config->local_history_table_size * sizeof(unsigned));

        for (i = 0; i < config->local_history_table_size; i++)
        {
            branch_predictor->local_history_table[i] = 0;
        }

        branch_predictor->local_history_table_mask = config->local_history_table_size - 1;

        // Initialize global counters
        branch_predictor->global_counters = 
            (Sat_Counter *)malloc(config->global_predictor_size * sizeof(Sat_Counter));

        for (i = 0; i < config->global_predictor_size; i++)
        {
            initSatCounter(&(branch_predictor->global_counters[i]), config->global_counter_bits);
        }

        branch_predictor->global_history_mask = config->global_predictor_size - 1;

        // Initialize choice counters
        branch_predictor->choice_counters = 
            (Sat_Counter *)malloc(config->choice_predictor_size * sizeof(Sat_Counter));

        for (i = 0; i < config->choice_predictor_size; i++)
        {
            initSatCounter(&(branch_predictor->choice_counters[i]), config->choice_counter_bits);
        }

        branch_predictor->choice_history_mask = config->choice_predictor_size - 1;

        // global history register
        branch_predictor->global_history = 0;

        // We assume choice predictor size is always equal to global predictor size.
        branch_predictor->history_register_mask = config->choice_predictor_size - 1;
    }

    return branch_predictor;
}

inline void initPerceptron(Perceptron *perceptron, unsigned weight_bits, unsigned num_weights) {
    perceptron->weights = (signed *)malloc(num_weights *sizeof(signed int));
    perceptron->weights[0]=1;
    for (int i=1; i< num_weights; i++) {
        perceptron->weights[i]=0;
    }
    perceptron->weight_bits = weight_bits;
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
bool predict(Branch_Predictor *branch_predictor, Instruction *instr, BP_Config * config)
{
    uint64_t branch_address = instr->PC;

    if (!strcmp(config->bp_type, "twobitlocal")) {     
        // Step one, get prediction
        unsigned local_index = getIndex(branch_address, 
                                        branch_predictor->index_mask);

        bool prediction = getPrediction(&(branch_predictor->local_counters[local_index]));

        // Step two, update counter
        if (instr->taken)
        {
            // printf("Correct: %u -> ", branch_predictor->local_counters[local_index].counter);
            incrementCounter(&(branch_predictor->local_counters[local_index]));
            // printf("%u\n", branch_predictor->local_counters[local_index].counter);
        }
        else
        {
            // printf("Incorrect: %u -> ", branch_predictor->local_counters[local_index].counter);
            decrementCounter(&(branch_predictor->local_counters[local_index]));
            // printf("%u\n", branch_predictor->local_counters[local_index].counter);
        }

        return prediction == instr->taken;
    }

    else if (!strcmp(config->bp_type, "gshare")) {
        //step 1, get global prediction
        // unsigned PC_index = (branch_address >> ((int)(64 - log2(branch_predictor->gshare_predictor_size))));
        uint64_t PC_index = branch_address >> instShiftAmt;
        unsigned gshare_predictor_idx = 
            (branch_predictor->gshare_history ^ PC_index) & branch_predictor->gshare_predictor_mask;

        bool gshare_prediction = 
            getPrediction(&(branch_predictor->gshare_counters[gshare_predictor_idx]));
        
        bool prediction_correct = gshare_prediction == instr->taken;

        //step 2, update counter
        if (instr->taken) {
            incrementCounter(&(branch_predictor->gshare_counters[gshare_predictor_idx]));
        }
        else {
            decrementCounter(&(branch_predictor->gshare_counters[gshare_predictor_idx]));
        }

        //step 3, update global history register
        branch_predictor->gshare_history = branch_predictor->gshare_history << 1 | instr->taken;
        
        return prediction_correct;

    } else if (!strcmp(config->bp_type, "perceptron")) {
        unsigned perceptron_table_idx = getIndex(branch_address, 
            branch_predictor->perceptron_table_size - 1);
        
        signed output = dotProduct(
            branch_predictor->perceptron_table[perceptron_table_idx].weights, 
            branch_predictor->global_history, config->global_history_bits);

        bool prediction;
        signed xi;        
        
        if (output >= 0) {
            prediction = 1; 
            xi = 1;
        } else {
            prediction = 0; 
            xi = -1;
        }
        
        bool prediction_correct = prediction == instr->taken;
        signed t; //t is actual outcome of branch, t is 1 when taken and -1 when not taken
        if (instr->taken) { t = 1; } else { t = -1;}
        
        if ((instr->taken ^ prediction) || (abs(output) <= branch_predictor->threshold)) {
            for (int i=0; i < config->global_history_bits; i++) {
                branch_predictor->perceptron_table[perceptron_table_idx].weights[i] = branch_predictor->perceptron_table[perceptron_table_idx].weights[i] + t*xi;
            }
        }

        branch_predictor->global_history = branch_predictor->global_history << 1 | instr->taken;

        return prediction_correct;
    }
    else if (!strcmp(config->bp_type, "tournament")) {
        // Step one, get local prediction.
        unsigned local_history_table_idx = getIndex(branch_address,
                branch_predictor->local_history_table_mask);
        
        unsigned local_predictor_idx = 
            branch_predictor->local_history_table[local_history_table_idx] & 
            branch_predictor->local_predictor_mask;

        bool local_prediction = 
            getPrediction(&(branch_predictor->local_counters[local_predictor_idx]));

        // Step two, get global prediction.
        unsigned global_predictor_idx = 
            branch_predictor->global_history & branch_predictor->global_history_mask;

        bool global_prediction = 
            getPrediction(&(branch_predictor->global_counters[global_predictor_idx]));

        // Step three, get choice prediction. Initially, same as global prediction
        unsigned choice_predictor_idx = 
            branch_predictor->global_history & branch_predictor->choice_history_mask;

        bool choice_prediction = 
            getPrediction(&(branch_predictor->choice_counters[choice_predictor_idx]));


        // Step four, final prediction.
        bool final_prediction;
        if (choice_prediction)
        {
            final_prediction = global_prediction;
        }
        else
        {
            final_prediction = local_prediction;
        }

        bool prediction_correct = final_prediction == instr->taken;
        // Step five, update counters
        if (local_prediction != global_prediction)
        {
            if (local_prediction == instr->taken)
            {
                // Should be more favorable towards local predictor.
                decrementCounter(&(branch_predictor->choice_counters[choice_predictor_idx]));
            }
            else if (global_prediction == instr->taken)
            {
                // Should be more favorable towards global predictor.
                incrementCounter(&(branch_predictor->choice_counters[choice_predictor_idx]));
            }
        }

        if (instr->taken)
        {
            incrementCounter(&(branch_predictor->global_counters[global_predictor_idx]));
            incrementCounter(&(branch_predictor->local_counters[local_predictor_idx]));
        }
        else
        {
            decrementCounter(&(branch_predictor->global_counters[global_predictor_idx]));
            decrementCounter(&(branch_predictor->local_counters[local_predictor_idx]));
        }

        // Step six, update global history register
        branch_predictor->global_history = branch_predictor->global_history << 1 | instr->taken;
        branch_predictor->local_history_table[local_history_table_idx] = branch_predictor->local_history_table[local_history_table_idx] << 1 | instr->taken;
        // exit(0);
        //
        return prediction_correct;
    }
}

inline unsigned getIndex(uint64_t branch_addr, unsigned index_mask)
{
    return (branch_addr >> instShiftAmt) & index_mask; // = modulo local_predictor_size
}

inline bool getPrediction(Sat_Counter *sat_counter)
{
    uint8_t counter = sat_counter->counter;
    unsigned counter_bits = sat_counter->counter_bits;

    // MSB determins the direction
    return (counter >> (counter_bits - 1));
}

inline signed dotProduct(signed *weights, unsigned global_history_register, unsigned global_history_bits)
{
    signed dot_product = 0;
    signed input = 0;
    for (int i=1; i < global_history_bits; i++) {
        input = (global_history_register >> i) & 1;
        dot_product = dot_product + weights[i]*input;
    }
    return dot_product + weights[0] * 1; // bias weight gets input =1
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
