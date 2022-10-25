#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "dyn_array.h"
#include "processing_scheduling.h"

#define FCFS "FCFS"
#define P "PSRJF"
#define RR "RR"
#define SJF "SJF"

/* prototypes */
int my_atoi(char*str);
void print_data(FILE *fp, const char *sr_alg, ScheduleResult_t *sr);

// Add and comment your analysis code in this function.
// THIS IS NOT FINISHED.
int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) { // argc check
        printf("Input in the order: \n");
        printf("%s <PCBs_bin_file> <schedule algorithm> [quantum]\n", argv[0]);
        return EXIT_FAILURE;
    }

    dyn_array_t*da = load_process_control_blocks(argv[1]); // have pcb read from first argv
    ScheduleResult_t*result = (ScheduleResult_t*)malloc(sizeof(ScheduleResult_t)); // malloc schedule result object
    size_t quantum = 4; // set given quantum size
    char roundRobin[19] = ""; 

    /* reference for snprintf: https://www.geeksforgeeks.org/snprintf-c-library/*/
    if(argv[3]) {  // if quantum is given as a command line argument 
        quantum = my_atoi(argv[3]); 
        snprintf(roundRobin, 19, "RR (Q = %d)", (int)quantum);
    } else {
        snprintf(roundRobin, 19, "RR (Q = %d)", 4);
    }
    
    /* reference for stat struct: https://codeforwin.org/2018/03/c-program-find-file-properties-using-stat-function.html*/
    struct stat stats; // init stats struct to add to readme
    FILE *fp = fopen("../readme.md", "a"); // open readme file in append mode
    stat("../readme.md", &stats); // add stats to readme

    switch(*argv[2]) { // use switch cases to get argv[2] user input 
        case 'F': // First Come First Serve 
            if(first_come_first_serve(da, result))
                print_data(fp, FCFS, result);
            break;
        case 'P': // Preemptive Shortest Remaining Job First
            if(shortest_remaining_time_first(da, result))
                print_data(fp, P, result);
            break;
        case 'R': // Round Robin 
            if(argc == 4 && sscanf(argv[3], "%zu", &quantum)) {
                if(round_robin(da, result, quantum))
                    print_data(fp, roundRobin, result);
            } else
                fprintf(stderr, "ERROR: invalid quantum value\n");
            break;
        case 'S': // Shortest Job First
            if(shortest_job_first(da, result))
                print_data(fp, SJF, result);
            break;
        default:
            printf("%s <pcb_file> <algorithm> [quantum]\n", argv[0]);
            break;
    }

    fclose(fp); // close file
    dyn_array_destroy(da); // destory dynamic array
    free(result);

    return EXIT_SUCCESS;
}


/* reference: https://www.geeksforgeeks.org/write-your-own-atoi/*/
int my_atoi(char *str) {
    int result = 0;
    /* iterate through all characters of input strings and update result */
    for (int i = 0; str[i] != '\0'; ++i){
        result = result * 10 + str[i] - '0'; 
    } 
    return result;
}

void print_data(FILE *fp, const char*sr_algorithm, ScheduleResult_t *sr) {
    // fprintf(fp, "Scheduling Algorithm | Average Turnaround Time | Average Waiting Time | Total Clock Time\n");
    fprintf(fp, "\n%-20s | %-23f | %-20f | %-16lu  ", sr_algorithm, sr->average_turnaround_time, sr->average_waiting_time, sr->total_run_time); // print to readme file
    /* printing data results */
    printf("\nScheduling Algorithm | Average Turnaround Time | Average Waiting Time | Total Clock Time\n");
    printf("%-20s | %-23f | %-20f | %-16lu |\n", sr_algorithm, sr->average_turnaround_time, sr->average_waiting_time, sr->total_run_time);
}