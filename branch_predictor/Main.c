#include "Trace.h"
#include "Branch_Predictor.h"

extern TraceParser *initTraceParser(const char * trace_file);
extern bool getInstruction(TraceParser *cpu_trace);

extern Branch_Predictor *initBranchPredictor();
extern bool predict(Branch_Predictor *branch_predictor, Instruction *instr);

extern unsigned localPredictorSize;
extern unsigned localCounterBits;
extern unsigned localHistoryTableSize; 
extern unsigned globalPredictorSize;
extern unsigned globalCounterBits; 
extern unsigned choicePredictorSize; // Keep this the same as globalPredictorSize.
extern unsigned choiceCounterBits;
extern unsigned gsharePredictorSize;
extern unsigned gshareCounterBits; 

int main(int argc, const char *argv[])
{	
    if (argc != 2)
    {
        printf("Usage: %s %s\n", argv[0], "<trace-file>");

        return 0;
    }

    // Initialize a CPU trace parser
    TraceParser *cpu_trace = initTraceParser(argv[1]);

    // Initialize a branch predictor
    Branch_Predictor *branch_predictor = initBranchPredictor();

    // Running the trace
    uint64_t num_of_instructions = 0;
    uint64_t num_of_branches = 0;
    uint64_t num_of_correct_predictions = 0;
    uint64_t num_of_incorrect_predictions = 0;

    while (getInstruction(cpu_trace))
    {
        // We are only interested in BRANCH instruction
        if (cpu_trace->cur_instr->instr_type == BRANCH)
        {
            ++num_of_branches;

            if (predict(branch_predictor, cpu_trace->cur_instr))
            {
                ++num_of_correct_predictions;
            }
            else
            {
                ++num_of_incorrect_predictions;
            }
        }
        ++num_of_instructions;
    }

//    printf("Number of instructions: %"PRIu64"\n", num_of_instructions);
//    printf("Number of branches: %"PRIu64"\n", num_of_branches);
//    printf("Number of correct predictions: %"PRIu64"\n", num_of_correct_predictions);
//    printf("Number of incorrect predictions: %"PRIu64"\n", num_of_incorrect_predictions);

    float performance = (float)num_of_correct_predictions / (float)num_of_branches * 100;
    #ifdef TOURNAMENT
    printf("Tournament,%d,%d,%d,%d,%"PRIu64",%"PRIu64",%f%%\n",localPredictorSize, localHistoryTableSize, globalPredictorSize, choicePredictorSize, num_of_correct_predictions, num_of_incorrect_predictions, performance);
    #endif

    #ifdef GSHARE
    printf("Gshare,%d,%"PRIu64",%"PRIu64",%f%%\n", gsharePredictorSize,num_of_correct_predictions, num_of_incorrect_predictions, performance)
    #endif

    #ifdef TWO_BIT_LOCAL
    printf("2bitlocal,%d,%d,%"PRIu64",%"PRIu64",%f%%\n", localPredictorSize, localCounterBits, num_of_correct_predictions, num_of_incorrect_predictions, performance);
    #endif
}
