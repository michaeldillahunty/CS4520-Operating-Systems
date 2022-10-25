#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dyn_array.h"
#include "processing_scheduling.h"


// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

// private function
void virtual_cpu(ProcessControlBlock_t *process_control_block) 
{
    // decrement the burst time of the pcb
    --process_control_block->remaining_burst_time;
}

/* MILESTONE 1 */
/** REFERENCES: 
    https://www.geeksforgeeks.org/program-for-fcfs-cpu-scheduling-set-1/
    https://www.tutorialspoint.com/c-program-for-fcfs-scheduling
    */
bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result) {
    if (ready_queue == NULL || result == NULL){ // paramater check
        return false; 
    }
    result->total_run_time = 0;
    
    uint32_t size = dyn_array_size(ready_queue); // size of dynamic array
    uint32_t turnaround = 0, wt = 0, rt = 0; // turnaround time
    ProcessControlBlock_t pcb;
    for (uint32_t i = 0; i < size; i++){ // while not at max size
        wt = wt + rt; 
        dyn_array_extract_back(ready_queue, &pcb);
        rt = rt + pcb.remaining_burst_time;
        turnaround = turnaround + rt;
        turnaround = turnaround - pcb.arrival;
        wt = wt - pcb.arrival; 

        while (pcb.remaining_burst_time > 0){   // while the cpu burst is not done
            virtual_cpu(&pcb); // decrement pcb burst time
        }
    }
    // AWT, ATaT & Total Run Time calculations
    result->average_waiting_time = ((float)wt) / ((float)size);
    result->average_turnaround_time = ((float)turnaround) / ((float)size);
    result->total_run_time = rt;
    return true;
}

/* MILESTONE 1 */
/** Reads the PCB burst time values from the binary file into ProcessControlBlock_t remaining_burst_time field
    for N number of PCB burst time stored in the file.
    \param input_file the file containing the PCB burst times
    \return a populated dyn_array of ProcessControlBlocks if function ran successful else NULL for an error 
*/
dyn_array_t *load_process_control_blocks(const char *input_file) {
    if (input_file == NULL){ // param check for input_file
        return NULL;
    }

    if (strcmp(input_file, "") == 0 || strcmp(input_file, "\n") == 0){ // file check
        return NULL;
    }

    int fd = open(input_file, O_RDONLY); // open file in read only
    if(fd < 0) { // file open check
        close(fd);
        return NULL;
    }

    char *fe = strrchr(input_file, '.'); // file extension check
    if(!fe || strcmp(fe, ".bin")){
        return NULL;
    }

    uint32_t buf;
    int read_buf = read(fd, &buf, sizeof(uint32_t));
    if(read_buf <= 0) {
        close(fd);
        return NULL;
    }

    ProcessControlBlock_t *pcb = malloc(sizeof(ProcessControlBlock_t)*buf);
    for(uint32_t i = 0; i < buf; i++) {
        // store file data in process control blocks
        read_buf = read(fd, &pcb[i].remaining_burst_time, sizeof(uint32_t));
        read_buf = read(fd, &pcb[i].priority, sizeof(uint32_t));
        read_buf = read(fd, &pcb[i].arrival, sizeof(uint32_t));
    }
    
    // create array of process control blocks
    dyn_array_t *dyn_array = dyn_array_import((void*)pcb, sizeof(buf), sizeof(ProcessControlBlock_t), NULL); 

    close(fd); // close file
    free(pcb); // free malloced process control blocks
    return dyn_array; // return array 
}


/* ------------- MILESTONE 2 ------------- */

/** Runs the Shortest Job First Scheduling algorithm over the incoming ready_queue
    \param ready queue a dyn_array of type ProcessControlBlock_t that contain be up to N elements
    \param result used for shortest job first stat tracking \ref ScheduleResult_t
    \return true if function ran successful else false for an error 
    */
/* resource(s): 
    https://www.geeksforgeeks.org/program-for-shortest-job-first-or-sjf-cpu-scheduling-set-1-non-preemptive/
    https://www.edureka.co/blog/sjf-scheduling-in-c/
*/
bool shortest_job_first(dyn_array_t *ready_queue, ScheduleResult_t *result) {
    if (ready_queue == NULL|| result == NULL){ // param check 
        return false; 
    }
    // init variables
    result->average_waiting_time = 0;
    result->average_turnaround_time = 0;
    result->total_run_time = 0;
    uint32_t size = dyn_array_size(ready_queue);
    uint32_t clock = 0; // clock tick time
    uint32_t latency = 0; // total latency 
    uint32_t turnaround = 0; // turnaround time 
    bool extracted = false; // check if pcb is correctly extracted
    int sum = 0; // arrival time summation 

    ProcessControlBlock_t*pcb = malloc(sizeof(ProcessControlBlock_t)); // malloc process control blocks
    pcb = (ProcessControlBlock_t*)(dyn_array_export(ready_queue));
    dyn_array_t*da = dyn_array_create(0, size, NULL); // init dynamic array

    for (uint32_t i = 0; i < size; i++){ 
        for (uint32_t j = 0; j < size; j++){
            if (pcb[j].arrival <= clock){ // if the process has arrived
                if (pcb[j].started == false){ // if the process hasn't been scheduled
                    extracted = dyn_array_push_back(da, &pcb[j]); // add the process to the back of the queue
                    if (extracted == false){ // if process push failed
                        return false;
                    }
                    pcb[j].started = true; // change scheduled status of process to true  
                    sum = sum + pcb[j].arrival; // add the arrival time to the arrival sum
                }
            }
        }
        if (dyn_array_size(da) != 0){ // if the queue is not empty
            dyn_array_sort(da, compare_burst_times); // sort the array based on burst times
            while (dyn_array_size(da) > 0){ // loop through queue while its not empty
                ProcessControlBlock_t temp_pcb; // init temp pcb var
                extracted = dyn_array_extract_back(da, &temp_pcb); // extract last process in queue
                if (extracted == false){ // if extraction failed
                    return false;
                }
                temp_pcb.started = true; // change status
                latency = latency + clock; // update latency time
                while (temp_pcb.remaining_burst_time > 0){ // while scheduling algo is not done
                    virtual_cpu(&temp_pcb); // add process to virtual cpu
                    clock++; // increment clock ticks
                }
                temp_pcb.started = false; // update status
                turnaround = turnaround + clock; // update turnaround time
            }
        }
    }
    // update results 
    result->average_turnaround_time = (float)(turnaround - sum) / size; 
    result->average_waiting_time = (float)(latency - sum) / size; 
    result->total_run_time = clock; 
    return true;
}

// function to compare the burst times of 2 different processes
int compare_burst_times(const void*x, const void*y){
/**
    compare(x,y) < 0 iff x < y
    compare(x,y) = 0 iff x == y
    compare(x,y) > 0 iff y > x
*/
    // if process1 burst time is > process2 burst time
    if (((ProcessControlBlock_t*)x)->remaining_burst_time > ((ProcessControlBlock_t*)y)->remaining_burst_time){
        return -1; 
    // else if process1 burst time is < process2 burst time
    } else if (((ProcessControlBlock_t*)x)->remaining_burst_time < ((ProcessControlBlock_t*)y)->remaining_burst_time){
        return 1; 
    }
    return 0; // else if process1 burst time = process2 burst time
}

bool priority(dyn_array_t *ready_queue, ScheduleResult_t *result) {
    UNUSED(ready_queue);
    UNUSED(result);
    return false;   
}

/** Runs the Round Robin Process Scheduling algorithm over the incoming ready_queue
    \param ready_queue a dyn_array of type ProcessControlBlock_t that contain be up to N elements
    \param result used for round robin stat tracking \ref ScheduleResult_t
    \param quantum the quantum
    \return true if function ran successful else false for an error
    */
/* references: 
    https://www.edureka.co/blog/round-robin-scheduling-in-c/
*/
bool round_robin(dyn_array_t *ready_queue, ScheduleResult_t *result, size_t quantum) {
    if (ready_queue == NULL || result == NULL || quantum == 0){ // param check
        return false; 
    }
    // init variables 
    result->average_turnaround_time = 0;
    result->average_waiting_time = 0;
    result->total_run_time = 0; 
    uint32_t size = dyn_array_size(ready_queue); // size of ready queue
    uint32_t wait = 0;  // wait time 
    uint32_t turnaround = 0; // turnaround time
    uint32_t rt = 0; // run time
    uint32_t curr_rt = 0; // current run time
    bool extracted = false; 
    dyn_array_t*da = dyn_array_create(0, sizeof(ProcessControlBlock_t), NULL);

    for (uint32_t i = 0; i < size; i++){ // loop through ready queue
        ProcessControlBlock_t*temp_pcb = ((ProcessControlBlock_t*)dyn_array_at(ready_queue, i)); // find first arrival
        temp_pcb->priority = temp_pcb->arrival; // store the priority
    }
    check_pcb(ready_queue, da, rt);
    while (dyn_array_size(da) > 0){ // while dyn array is not empty
        ProcessControlBlock_t pcb; 
        extracted = dyn_array_extract_front(da, &pcb); // extract from front of dyn array and store in pcb
        if (extracted == false){ // check if extraction failed
            return false; 
        }
        // printf("Prirority: %u\n", pcb.priority);
        wait = wait + rt - pcb.arrival; // update wait time
        if (pcb.remaining_burst_time <= quantum){ // if the remaining burst time is <= quantum value
            curr_rt = pcb.remaining_burst_time; // finish the processes burst
        } else {
            curr_rt = quantum; // else limit the burst run time to the quantum value
        }
        rt = rt + curr_rt; // update runtime val
        while (curr_rt > 0){ // while the processes have more burst time
            virtual_cpu(&pcb); // put process on virtual cpu and run (decrement time)
            curr_rt--; 
        }
        pcb.arrival = rt; // update the arrival time 
        check_pcb(ready_queue, da, rt); // check the pcb queue
        if (pcb.remaining_burst_time != 0){ // if process burst time is not 0
            dyn_array_push_back(da, &pcb); // put process in the back of the dynamic array
        } else { // else if remaining burst time is 0
            turnaround = turnaround + rt - pcb.priority; // calculate turnaround time (total run time - )
        }
        // printf("Arrival time: %u\n", pcb.arrival);
        // printf("Burst time: %u\n", pcb.remaining_burst_time);
        
    }
    result->average_waiting_time = (float)wait / size; 
    // printf("AWT: %f\n",result->average_waiting_time);
    result->average_turnaround_time = (float)turnaround / size; 
    // printf("ATA: %f\n",result->average_turnaround_time);
    result->total_run_time = rt; 
    dyn_array_destroy(da);
    return true;
}

void check_pcb(dyn_array_t*ready_queue, dyn_array_t*array_queue, uint32_t curr_time){
    size_t size = dyn_array_size(ready_queue);
    for(size_t i = 0; i < size; i++){ // while less than the size of the ready queue
        ProcessControlBlock_t pcb; 
        dyn_array_extract_back(ready_queue, &pcb); // remove process from array and store in PCB
        if(pcb.arrival <= curr_time){ // if the arrival time of current process is <= the current, total run time
            dyn_array_push_back(array_queue, &pcb); // copy process to back of dyn array
        } else { // else if arrival time > curr time 
            dyn_array_push_front(ready_queue, &pcb); // copy process to front of dyn array
        }
    }
}


/** Runs the Shortest Remaining Time First Process Scheduling algorithm over the incoming ready_queue
    \param ready_queue a dyn_array of type ProcessControlBlock_t that contain be up to N elements
    \param result used for shortest job first stat tracking \ref ScheduleResult_t
    \return true if function ran successful else false for an error
*/
bool shortest_remaining_time_first(dyn_array_t *ready_queue, ScheduleResult_t *result) {
    if (ready_queue == NULL || result == NULL){ // error check params
        return false; 
    }

    dyn_array_t*da = dyn_array_create(0, sizeof(ProcessControlBlock_t), NULL); // create dynamic array
    result->average_turnaround_time = 0;
    result->average_waiting_time = 0;
    result->total_run_time = 0; 
    uint32_t wait = 0; // wait time
    uint32_t turnaround = 0; // turnaround time 
    uint32_t rt = 0; // run time
    bool extracted = true; 

    uint32_t size = dyn_array_size(ready_queue);
    for (uint32_t i = 0; i < size; i++){ // loop through ready queue and find first arrival
        ProcessControlBlock_t*temp_pcb = (ProcessControlBlock_t*)dyn_array_at(ready_queue, i);
        temp_pcb->priority = temp_pcb->arrival; 
    }
    dyn_array_sort(ready_queue, calculate_arrival); // sort array based on arrival time
    check_pcb(ready_queue, da, rt); // use check_pcb to check queue for processes
    dyn_array_sort(da, compare_remaining_burst_time); // sort processes by burst times
    while (dyn_array_size(da) != 0){ 
        ProcessControlBlock_t pcb; // init pcb
        extracted = dyn_array_extract_front(da, &pcb); // remove process from array and store in pcb
        if (extracted == false){ // extraction fail check
            return false; 
        }

        wait += rt - pcb.arrival; // update wait time
        uint32_t ready_queue_size = dyn_array_size(ready_queue);
        uint32_t curr_rt;
        if(ready_queue_size != 0){ // if ready queue isn't full
            uint32_t temp_arrival;
            temp_arrival = ((ProcessControlBlock_t*)dyn_array_at(ready_queue, dyn_array_size(ready_queue) - 1))->arrival; 
            curr_rt = temp_arrival - rt;
        } else {
            curr_rt = pcb.remaining_burst_time;
        }
        rt = rt + curr_rt; // increment runtime + current runtime
        while (curr_rt > 0){ 
            virtual_cpu(&pcb); // add to virtual cpu 
            curr_rt--; // decrement run time 
        }
        pcb.arrival = rt; 
        if(pcb.remaining_burst_time == 0){
            turnaround += rt - pcb.priority;
        } else {
            dyn_array_push_back(da, &pcb);
        }
        check_pcb(ready_queue, da, rt); // use check_pcb to check queue for processes
        dyn_array_sort(da, compare_remaining_burst_time); // sort processes by burst times
    }
    // printf("turnaround: %u\n", turnaround);
    // printf("size: %u\n", size);
    // printf("wait: %u\n", wait);
    result->average_waiting_time = (float)wait/size;
    result->average_turnaround_time = (float)turnaround/size;
    result->total_run_time = rt; 
    dyn_array_destroy(da);
    return true; 
}

/** 
    ARRIVAL CALC HELPER
    */
int calculate_arrival(const void*process1, const void*process2){
    uint32_t x = ((ProcessControlBlock_t*)process1)->arrival; // get arrival time of process1
    uint32_t y = ((ProcessControlBlock_t*)process2)->arrival; // get arrival time of process2 
    if (x == y){ // if arrival times are equal 
        // compare remaining burst time and return 
        int calc = (((ProcessControlBlock_t*)process1)->remaining_burst_time) - (((ProcessControlBlock_t*)process2)->remaining_burst_time);
        return calc; 
    }
    return y-x; // else if x ≠ y, return the difference of arrival times
}

int compare_remaining_burst_time(const void* block1, const void* block2)
{
    //sorting the ready queue by remaining_burst_time
    if(((ProcessControlBlock_t*)block1)->remaining_burst_time > ((ProcessControlBlock_t*)block2)->remaining_burst_time){
        return 1;
    }
    else if (((ProcessControlBlock_t*)block1)->remaining_burst_time < ((ProcessControlBlock_t*)block2)->remaining_burst_time){
        return -1;
    }else if (((ProcessControlBlock_t*)block1)->remaining_burst_time == ((ProcessControlBlock_t*)block2)->remaining_burst_time){
        return 0;
    }
	return 0;
}
