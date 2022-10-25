#include "../include/bitmap.h"
#include <math.h>
#include <stdio.h>
// data is an array of uint8_t and needs to be allocated in bitmap_create
//      and used in the remaining bitmap functions. You will use data for any bit operations and bit logic
// bit_count the number of requested bits, set in bitmap_create from n_bits
// byte_count the total number of bytes the data contains, set in bitmap_create


bitmap_t *bitmap_create(size_t n_bits){
   if (n_bits > 0 && n_bits < SIZE_MAX - 1){ // error check 
      size_t bytes; 
      size_t last; 

      bitmap_t *ptr = (bitmap_t*)malloc(sizeof(bitmap_t));  // allocate memory for bit array
      if (ptr == NULL){ // malloc check
         free(ptr);
         return NULL;
      }
      
      /* test 1 */ 
      if (n_bits % 8 == 0){   // if there are an exact # of bits
         bytes = ceil(n_bits/8); // round remaining bytes 
         ptr->data = (uint8_t*)malloc(sizeof(uint8_t) * bytes); // allocate memory for bitmap 
         last = bytes; 
      /* test 2 */  
      } else {
         bytes = ceil((n_bits/8)+1);
         ptr->data = (uint8_t*)malloc(sizeof(uint8_t) * bytes); // allocate memory for bitmap
         last = bytes + 1; 
      }
      size_t i; 
      for (i = 0; i < last; i++){ // loop through array till the last element and set all bits to 0
         ptr->data[i] = 0;   
      }
      ptr->bit_count = n_bits;   // set # of bits
      ptr->byte_count = bytes;   // set bytes calculated above
      return ptr; 
      
   }
   return NULL;
}

// most of this code is from an example from https://web.archive.org/web/20220103112233/http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
bool bitmap_set(bitmap_t *const bitmap, const size_t bit){
	if (bitmap && bit < SIZE_MAX && bit <= bitmap->bit_count){
      size_t i = bit/8; // get byte
      size_t pos = bit%8; // get bit position 
      uint8_t flag = 1; // from given link: flag = 0000.....00001
      flag = flag << pos; // shift bit position: flag = 0000....010....00000 
      bitmap->data[i] = bitmap->data[i] | flag; 
      return true; 
   }
   return false; 
}

// most of this code is from an example from https://web.archive.org/web/20220103112233/http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
bool bitmap_reset(bitmap_t *const bitmap, const size_t bit){
   if (bitmap && bit >= 0 && bit <= bitmap->bit_count){
      size_t i = bit/8;
      size_t pos = bit%8;
      uint8_t flag = 1; // flag: 0000.....00001
      flag = flag << pos; // shift flag k positions 
      flag = ~flag; // one's compliment (flip bits)
      bitmap->data[i] = bitmap->data[i] & flag; // reset bit to k-th position in data[i]
      return true; 
   }
   return false; 
}

// most of this code is from an example from https://web.archive.org/web/20220103112233/http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
bool bitmap_test(const bitmap_t *const bitmap, const size_t bit){
   if (bitmap && bit >= 0 && bit <= bitmap->bit_count){
      size_t i = bit/8; 
      size_t pos = bit%8; 
      uint8_t flag = 1; 
      flag = flag << pos; 
      if (bitmap->data[i] & flag){
         return true; 
      } 
   }
   return false; 
}

size_t bitmap_ffs(const bitmap_t *const bitmap){
   if (bitmap){ // check if present
      int i;   
      size_t total = bitmap->bit_count;   // total bits = bit_count
      for (i = total-1; i >= 0; i--){
         if (bitmap_test(bitmap, i)){
            return i; 
            // printf("infinite loop, in if"); 
         }
         // printf("infinite loop, out of if"); 
      } 
   }
   return SIZE_MAX;
}

size_t bitmap_ffz(const bitmap_t *const bitmap){
   if (bitmap){
      size_t i; 
      for (i = 0; i < bitmap->bit_count; i++){
         if (!bitmap_test(bitmap, i)) return i; 
      }
   }
   return SIZE_MAX;
}

bool bitmap_destroy(bitmap_t *bitmap){
   if (bitmap){   // check if present
      free(bitmap->data);  // free bitmap data
      free(bitmap); // free mmemory for bitmap_t struct
      return true; 
   }
   return false; 
}
