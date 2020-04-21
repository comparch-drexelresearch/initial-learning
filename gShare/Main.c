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
    printf("\t%02d-%02d-%d, %02d:%02d:%02d\n", time.tm_mon+1, time.tm_mday, time.tm_year+1900,
                time.tm_hour, time.tm_min, time.tm_sec);

    // Print the testing file's name 
    printf("Testing on file: %s\n", argv[1]);

    predictor_args config = (predictor_args) {
        .local_predictor_size = 2048,
        .local_history_table_size = 2048,
        .choice_predictor_size = 8192,
        .global_predictor_size = 8192,
        .local_counter_bits = 2,
        .global_counter_bits = 2,
        .choice_counter_bits = 2,
        .predictor_type = ""
    };

    // Argument of configurations of two-bit local predictor
    args_table *two_bit_local_table = malloc(sizeof(args_table));
    unsigned local_predictor_size_array[] = {2048, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288};
    unsigned local_counter_bits_array[] = {1, 2, 2, 2, 2, 2, 2, 2, 2, 2};
    two_bit_local_table->predictor_type = "two_bit_local";
    two_bit_local_table->num_row = (int) (sizeof(local_predictor_size_array)/sizeof(local_predictor_size_array[0]));
    two_bit_local_table->local_predictor_size = local_predictor_size_array;
    two_bit_local_table->local_counter_bits = local_counter_bits_array;

    // Argument of configurations of tournament predictor
    args_table *tournament_table = malloc(sizeof(args_table));
    unsigned local_histtable_size_array[] = {2048, 4096, 4096, 8192, 16384, 32768, 65536};
    unsigned global_predictor_size_array[] = {8192, 8192, 16384, 32768, 65536, 131072, 262144};
    unsigned choice_predictor_size_array[] = {8192, 8192, 16384, 32768, 65536, 131072, 262144};
    tournament_table->predictor_type = "tournament";
    tournament_table->num_row = (int) (sizeof(local_histtable_size_array)/sizeof(local_histtable_size_array[0]));
    tournament_table->local_history_table_size = local_histtable_size_array;
    tournament_table->global_predictor_size = global_predictor_size_array;
    tournament_table->choice_predictor_size = choice_predictor_size_array;

    // Argument of configurations of gshare predictor
    args_table *gshare_table = malloc(sizeof(args_table));
    unsigned gshare_global_predictor_size_array[] = {2048, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288};
    unsigned global_counter_bits_array[] = {1, 2, 2, 2, 2, 2, 2, 2, 2, 2};
    gshare_table->predictor_type = "gshare";
    gshare_table->num_row = (int) (sizeof(gshare_global_predictor_size_array)/sizeof(gshare_global_predictor_size_array[0]));
    gshare_table->global_predictor_size = gshare_global_predictor_size_array;
    gshare_table->global_counter_bits = global_counter_bits_array;

    args_table *argument_tables[] = {two_bit_local_table, tournament_table, gshare_table};
    int num_table = (int) (sizeof(argument_tables)/sizeof(argument_tables[0]));

    for (int i = 0; i<num_table; i++) {
        config.predictor_type = argument_tables[i]->predictor_type;
        printf("___________________________________________________________\n");
        for (int j = 0; j<argument_tables[i]->num_row; j++) {

            // Fetch argument values to predictor's configuration structure
            if (!strcmp(config.predictor_type, "two_bit_local")) {
                config.local_predictor_size = argument_tables[i]->local_predictor_size[j];
                config.local_counter_bits = argument_tables[i]->local_counter_bits[j];
                printf("%s, %u, %u, ", config.predictor_type, config.local_predictor_size, config.local_counter_bits);
            }
            else if (!strcmp(config.predictor_type, "tournament")) {
                config.local_history_table_size = argument_tables[i]->local_history_table_size[j];
                config.global_predictor_size = argument_tables[i]->global_predictor_size[j];
                config.choice_predictor_size = argument_tables[i]->choice_predictor_size[j];
                printf("%s, %u, %u, %u, ", config.predictor_type, config.local_history_table_size, config.global_predictor_size, config.choice_predictor_size);            
            }
            else if (!strcmp(config.predictor_type, "gshare")) {
                config.global_predictor_size = argument_tables[i]->global_predictor_size[j];
                config.global_counter_bits = argument_tables[i]->global_counter_bits[j];
                printf("%s, %u, %u, ", config.predictor_type, config.global_predictor_size, config.global_counter_bits);
            }
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
