#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

// added library to use INT_MAX macro
#include <limits.h>

#include "../include/sstring.h"

bool string_valid(const char *str, const size_t length){
   if (str && length){  // check if present
      int compare = 0;
      compare = strcmp(str+length-1,"\0"); // compare last index to null terminate val
      if (compare == 0){               
         return true; 
      }
   }
   return false;  // else return false
}

/**/ 
char *string_duplicate(const char *str, const size_t length){ /* return an allocated pointer to new string, else NULL */ 
   if (str && length){  // check if present
      char *str2 = malloc(length*sizeof(char)); // allocate memory for pointer to new string 
      if(str2){   // if str2 is present
         memcpy(str2, str, length*sizeof(char)); // copy str to str2
         return str2; 
      }
   }
   return NULL;
}

bool string_equal(const char *str_a, const char *str_b, const size_t length){
   if (str_a && str_b && length){   // check if present 
      int compare = memcmp(str_a, str_b, length); // compare the bytes of length to str_a and str_b
      if (compare == 0){ 
         return true; 
      }
   }
   return false; 
}

int string_length(const char *str, const size_t length){
   if (str && length){  // if present 
      return strlen(str); // return length of str
   }
   return -1; 
}

/** 
 \param str the string that will be used for tokenization
 \param delims the delimiters that will be used for splitting the str into segments
 \param str_length the lengt of the str
 \param tokens the string array that is pre-allocated and will contain the parsed tokens
 \param max_token_length the max length of a token string in the tokens string array with null terminator
 \param requested_tokens the number of possible strings that tokens string array can contain
 
*/ 
int string_tokenize(const char *str, const char *delims, const size_t str_length, char **tokens, const size_t max_token_length, const size_t requested_tokens){
   if (str && delims && str_length && tokens && max_token_length && requested_tokens){ // error check 
      if (string_valid(str, str_length)){ // check if string is null terminated 
         
         char arr[str_length];
         int i = 0; 
         while (i < str_length + 1){
            arr[i] = str[i];
            i++;
         }

         int j = 0;   
         while (j < requested_tokens){
            if (tokens[j] == NULL){ 
               return -1;  // failed to find token
            }
            char*ptr; 
            // reference for using strtok() https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
            if (j == 0){
               // strtok(string thats divided into tokens, c string containing delimiters)
               ptr = strtok(arr, ","); // if found, get the token 
            } else {
               ptr = strtok(NULL, ",");
            }
            strcpy(tokens[j], ptr); // copy contents of ptr to tokens arr
            j++; 
         }
      }
      return requested_tokens; 
   }
   return false;
}

//https://www.tutorialspoint.com/c_standard_library/c_function_atoi.htm (atoi())
/* long int strtol(const char *str, char **endptr, int base)
   str = string containing the representation of an integeral num
   endptr = value is set by the function to the next character in str after the numerical val
   base = must be between 2 and 36 inclusive, or special val 0 
*/
// https://www.ibm.com/docs/en/i/7.1?topic=lf-strtol-strtoll-convert-character-string-long-long-long-integer
bool string_to_int(const char *str, int *converted_value){
   if (str && converted_value != NULL){   // first check
      // strtol(const char*nptr, char**endptr, int base);
      if (INT_MAX <= strtol(str, (char**)NULL, 10)){
         return false; 
      } 
      *converted_value = atoi(str);
      return true;
   }
   return false; 
}
