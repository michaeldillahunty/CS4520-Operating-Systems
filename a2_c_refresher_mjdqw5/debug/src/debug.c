#include "../include/debug.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// protected function, that only this .c can use
int comparator_func(const void *a, const void *b) {
    return *(uint8_t*)a - *(uint8_t *)b;
}

bool terrible_sort(uint16_t*data_array, const size_t value_count) {
    if (data_array == NULL || value_count <= 0){ // CHANGE: add error check for params
        return false;
    }
    
    uint16_t *sorting_array = (uint16_t*)malloc(value_count * sizeof(uint16_t)); // CHANGE: *data_array -> uint16_t
    if (sorting_array == NULL){ // CHANGE: added malloc test
        return false; 
    } 

    uint16_t i; // CHANGE: add uint16 counter var
    for (i = 0; i < value_count; ++i) { // loop through data_array then store in sorting_array
        sorting_array[i] = data_array[i];
    }
    
    qsort(sorting_array, value_count, sizeof(uint16_t), comparator_func);   // CHANGE: redundant arithmetic sizeof(value_count)/sizeof(uint16_t) because we allocated memory for value_count * sizeof(uint16_t)
                                                                            // then quicksort sorting_array

    bool sorted = true; // init variable to true
    for (i = 0; i < value_count-1; ++i) { // loop through array and check if it's sorted
        // &= -> bit wise AND
        // sorted &= sorting_array is the same as sorted = sorted & sorting_array
        sorted &= sorting_array[i] <= sorting_array[i + 1];                                                           
    }

    if (sorted) {  // if sorted = true
        memcpy(data_array, sorting_array, value_count*sizeof(uint16_t)); // copy contents of sorting_array into data_array
    }

    free(sorting_array);
    return sorted;
}
    


