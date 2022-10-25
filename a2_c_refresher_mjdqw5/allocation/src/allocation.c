#include "../include/allocation.h"
#include <stdlib.h>
#include <stdio.h>

/** 
  Allocates an array of n elements of size member_size in bytes utilizing calloc if the clear flag is set, malloc otherwise.
  Testing with certain values will show different behavior between calloc and malloc.
  \param size_t - member size in bytes (can be gotten using the sizeof operator e.g. sizeof(int)
  \param size_t - number of members in the array
  \param bool -   1 will clear memory (calling calloc), 0 will not (calling malloc).
  \return void* - pointer to allocated memory.
*/
void* allocate_array(size_t member_size, size_t nmember, bool clear/*clear flag*/) {
  if (member_size <= 0 || nmember <= 0){  // error check for invalid values of params member_size and nmember
    return NULL; 
  } else {  
      void*ptr = NULL; // pass by value
      if (clear == true){ // if clear flag is set use callloc
        ptr = (void*)calloc(nmember, member_size);  // allocate nmembers with size memeber_size
      } else {  // else use malloc 
        ptr = (void*)malloc(nmember*member_size); // allocate a memory block of size: nmember*memeber_size 
      }
    return ptr;
  } 
}

/**
  Simple wrapper around realloc.
  \param void* - pointer to memory to resize.
  \param size_t - size of memory to allocate
  \return void* - pointer to reallocated memory region, may be same as original pointer.
*/ 
void* reallocate_array(void* ptr, size_t size){
  if (ptr && size > 0){ // error check params
    ptr = (void*)realloc(ptr, size);  // resize previous memory block and set new size to size variable
    return ptr; 
  }
  return NULL; // else if ptr isn't present or if size is < 0 , return NULL
}

void deallocate_array(void** ptr){
  if (*ptr != NULL){  // if ptr is valid
    free(*ptr); // dereference and free ptr 
    *ptr = NULL;  // set ptr to NULL and return 
    return; 
  }
  return; 
}

/** HEAP:
 - Local variables are allocated on the stack
 - Large local variable can overflow the stack as stack space is limited (the stack is shared with the functions your program calls as well)
 - When a stack variable leaves scope it is popped from the stack meaning you cannot return a local variable (stack variable) from a function.
 - Heap variables are allocated in memory (or in other places, e.g. using MMAP)
 - Heap variables can be vastly larger than stack variables
 - A heap variable remains available until it is freed, it is the programmers job to do so.
 - A heap variable that is not freed results in a memory leak, such leaks can be found using valgrind.
 - An overflow of the heap can cause serious issues in other parts of the program that may not be easily found. Such issues can usually be found with valgrind -v
 - Allocation of stack variables is faster as you only need to alter the stack pointer. */

/** func notes: 
  - Takes a file name and reads a line into a newly allocated buffer
  \param char* : filename to read from
  \return char* : return ptr to malloced head space containing buffer
*/
char* read_line_to_buffer(char* filename){
  if (filename){  // if filename is present 
    FILE*fp = fopen(filename, "r"); // open file in read only
    if (fp == NULL){  // validate file pointer 
      return NULL;
    }
    // reference: https://stackoverflow.com/questions/55186410/what-is-the-special-macro-of-buffer-size-in-standard-c-library
    char*buffer = (char*)malloc(sizeof(char) * BUFSIZ);
    // buffer = pointer to array of chars
    // BUFSIZ = max number of chars to be read 
    fgets(buffer, BUFSIZ, fp);  
    fclose(fp); // close file 
    return buffer;
  }
  return NULL;  // else return NULL;
}
