#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "Trace.h"
#include "Branch_Predictor.h"

extern TraceParser *initTraceParser(const char * trace_file);
extern bool getInstruction(TraceParser *cpu_trace);

extern Branch_Predictor *initBranchPredictor();
extern bool predict(Branch_Predictor *branch_predictor, Instruction *instr, predictor_args *args);

int main(int argc, const char *argv[])
{	
    if (argc != 2)
    {
        printf("Usage: %s %s\n", argv[0], "<trace-file>");

        return 0;
    }

    // Get current time of collecting result
    time_t t = time(NULL);
    struct tm time = *localtime(&t);
    printf("\t\t%02d-%02d-%d, %02d:%02d:%02d\n", time.tm_mon+1, time.tm_mday, time.tm_year+1900,
                time.tm_hour, time.tm_min, time.tm_sec);

    // Print the testing file's name 
    printf("\tTesting on file: %s\n", argv[1]);

    predictor_args config = (predictor_args) {
        .perceptron_table_size = 2048,
        .history_length = 12
    };

    // Argument of configuration of perceptron-based predictor
    args_table *perceptron_args_table = malloc(sizeof(args_table));
    unsigned perceptron_table_size[] = {2048, 4096, 8192, 16384, 32768, 65536};
    unsigned history_length[] = {12, 22, 32, 42, 52, 62};
    perceptron_args_table->num_row = (int) (sizeof(perceptron_table_size)/sizeof(perceptron_table_size[0]));
    perceptron_args_table->num_col = (int) (sizeof(history_length)/sizeof(history_length[0]));
    perceptron_args_table->perceptron_table_size = perceptron_table_size;
    perceptron_args_table->history_length = history_length;

    for (int i = 0; i < perceptron_args_table->num_row; i++) {
        printf("\t___________________________________________________________\n");
        config.perceptron_table_size = perceptron_args_table->perceptron_table_size[i];
        for (int j = 0; j < perceptron_args_table->num_col; j++) {
            config.history_length = perceptron_args_table->history_length[j];
            printf("\tperceptron, %u, %u, ", config.perceptron_table_size, config.history_length);
            
            // Initialize a CPU trace parser
            TraceParser *cpu_trace = initTraceParser(argv[1]);

            // Initialize a branch predictor
            Branch_Predictor *branch_predictor = initBranchPredictor(&config);

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

                    if (predict(branch_predictor, cpu_trace->cur_instr, &config))
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

            // printf("Number of instructions: %"PRIu64"\n", num_of_instructions);
            // printf("Number of branches: %"PRIu64"\n", num_of_branches);
            // printf("Number of correct predictions: %"PRIu64"\n", num_of_correct_predictions);
            // printf("Number of incorrect predictions: %"PRIu64"\n", num_of_incorrect_predictions);

            float performance = (float)num_of_correct_predictions / (float)num_of_branches * 100;
            printf("%"PRIu64", %"PRIu64", %"PRIu64", %"PRIu64", %f%%\n", num_of_instructions, num_of_branches, 
                                                num_of_correct_predictions, num_of_incorrect_predictions, performance);
            
        }
    }
    printf("---------------------------------------------------------------------------\n");
}
