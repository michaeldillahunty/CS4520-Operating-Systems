#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../include/arrays.h"

// LOOK INTO MEMCPY, MEMCMP, FREAD, and FWRITE

/**
  MEMCPY: https://www.tutorialspoint.com/c_standard_library/c_function_memcpy.htm

  MEMCMP: https://www.tutorialspoint.com/c_standard_library/c_function_memcmp.htm
   int memcmp(const void *str1, const void *str2, size_t n)
   - if Return value < 0 then it indicates str1 is less than str2.
   - if Return value > 0 then it indicates str2 is less than str1.
   - if Return value = 0 then it indicates str1 is equal to str2.
  FREAD: 
   
  FWRITE:
*/ 

bool array_copy(const void *src, void *dst, const size_t elem_size, const size_t elem_count){
   // error check 
   if (!src || !dst || !elem_size || !elem_count){ // check if present
      return false; 
   } 
   if (src && dst && elem_size && elem_count){  // if they are present 
      memcpy(dst, src, elem_count * elem_size); // copy src to dst 
      return true; 
   }
   return false;  // else return false
}

bool array_is_equal(const void *data_one, void *data_two, const size_t elem_size, const size_t elem_count){
   if (data_one && data_two && elem_size && elem_count){
      if (memcmp(data_one, data_two, elem_size * elem_count) == 0){
         return true;
      }
   } 
   return false; // else { return false; }
}
 
ssize_t array_locate(const void *data, const void *target, const size_t elem_size, const size_t elem_count){
   if (data && target && elem_size && elem_count){ // check if present 
      for (int i=0; i<elem_count; i++){   // 
         if (memcmp(data, target, elem_size) == 0){   // compare target to data, 
            return i; // return index of target
         }
         data += elem_size; 
      }
   } 
   return -1; // else failed
}

bool array_serialize(const void *src_data, const char *dst_file, const size_t elem_size, const size_t elem_count){
   // error check 
   if (!src_data || !dst_file || !elem_size || !elem_count || strcmp(dst_file, "") == 0 || strcmp (dst_file, "\n") == 0 || strcmp (dst_file, "\0") == 0){ 
      return false; 
   }
   if (src_data && dst_file && elem_size && elem_count){ // check if present
      FILE*fp = fopen(dst_file, "w");
      if (fp == NULL){
         return false; 
      } else {
         if (fwrite(src_data, elem_size, elem_count, fp) == elem_count){
            fclose(fp);
            return true;   
         }
      }
   }
   return false; 
}

bool array_deserialize(const char *src_file, void *dst_data, const size_t elem_size, const size_t elem_count){
   // error check
   if (src_file && dst_data && elem_size && elem_count && strcmp(src_file, "") != 0 && strcmp(src_file,"\n") != 0){
      FILE*fp = fopen(src_file, "rb"); // reading binary files
      if (fp == NULL){ 
         return false; 
      }
      fread(dst_data, elem_size, elem_count, fp); 
      return true; 
   } else {
      return false; 
   }
   return false; 
}

