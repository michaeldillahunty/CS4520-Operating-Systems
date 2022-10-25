#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../include/sys_prog.h"

// LOOK INTO OPEN, READ, WRITE, CLOSE, FSTAT/STAT, LSEEK
// GOOGLE FOR ENDIANESS HELP

/** Notes:
https://www.geeksforgeeks.org/input-output-system-calls-c-create-open-close-read-write/
   OPEN: int open (const char* Path, int flags [, int mode ]); 
      Path: path to file which you want to use 
      flags : How you like to use
         O_RDONLY: read only, O_WRONLY: write only, O_RDWR: read and write, O_CREAT: 
         create file if it doesn’t exist, O_EXCL: prevent creation if it already exists
   
   READ: size_t read (int fd, void* buf, size_t cnt);  
      fd: file descriptor
      buf: buffer to read data from
      cnt: length of buffer

   WRITE: size_t write (int fd, void* buf, size_t cnt); 
      fd: file descriptor
      buf: buffer to write data to
      cnt: length of buffer
      RETURN: 
         return Number of bytes written on success
         return 0 on reaching end of file
         return -1 on error
         return -1 on signal interrupt

   CLOSE: int close(int fd); 
         fd :file descriptor
         return 0 on success, -1 on fail 

   FSTAT: 
      int fildes	The file descriptor of the file that is being inquired.
      struct stat *buf	A structure where data about the file will be stored.
      return value	Returns a negative value on failure.
   http://codewiki.wikidot.com/c:system-calls:fstat

   LSEEK: off_t lseek(int fildes, off_t offset, int whence);
      int fildes : The file descriptor of the pointer that is going to be moved
      off_t offset : The offset of the pointer (measured in bytes).
      int whence : The method in which the offset is to be interpreted
   https://www.geeksforgeeks.org/lseek-in-c-to-read-the-alternate-nth-byte-and-write-it-in-another-file/
*/ 

/**
// Read contents from the passed into an destination
   \param input_filename the file containing the data to be copied into the destination
   \param dst the variable that will be contain the copied contents from the file
   \param offset the starting location in the file, how many bytes inside the file I start reading
   \param dst_size the total number of bytes the destination variable contains
   return true if operation was successful, else false
*/
bool bulk_read(const char *input_filename, void *dst, const size_t offset, const size_t dst_size){
   if (!input_filename || !dst || offset > dst_size || dst_size < 1){   // check params
      return false;
   }
   if (strcmp(input_filename, "\n") == 0 || strcmp(input_filename, "\0") == 0 || strcmp(input_filename, "") == 0){   // check file name
      return false;
   }

   int fd = open(input_filename, O_RDONLY);  // init file descriptor to read only | open(filepath, flag)
      if(fd == -1){  // check if file opened correctly
         return false;  // if failed return false
      } else {
         lseek(fd, offset, SEEK_SET);  // lseek(file, offset, offset method)
                                       // set to size of file + offset bytes 
         if(read(fd, dst, dst_size) == dst_size) { // read(file, buffer to read data from, length of buffer)
            if(close(fd) != -1){ // check if file closed correctly
               return true;
               }     
            }
        } 
   return false; 
}


bool bulk_write(const void *src, const char *output_filename, const size_t offset, const size_t src_size){
   if (!src || !output_filename || src_size < 1){ // param check
      return false;
   }
   if (strcmp(output_filename, "") == 0 || strcmp(output_filename, "\0") == 0 || strcmp(output_filename, "\n") == 0){ // file name check
      return false;
   }

   int fd = open(output_filename, O_WRONLY); // init file descriptor in write only 
      if(fd == -1){  // check if file opened correctly
         return false;  // if failed return false
      } else {
         // used to change the location of the read/write pointer of a file descriptor
         lseek(fd, offset, SEEK_SET);  // lseek(file, offset, offset method)
                                       // set to size of file + offset bytes 
         if(write(fd, src, src_size) == src_size) { // read(file, buffer to read data from, length of buffer)
            if(close(fd) != -1){ // check if file is closed correctly 
               return true;
            } 
         }
      }
   return false;
}
    

// some code in this func is from http://codewiki.wikidot.com/c:system-calls:fstat
// fstat() is identical to stat(), except that the file to be stat-ed is specified by the file descriptor fd.
bool file_stat(const char *query_filename, struct stat *metadata){
   if (query_filename && metadata && (strcmp(query_filename, "\0") != 0) && (strcmp(query_filename, "") != 0)){
      int file = 0; 
      if((file = open(query_filename, O_RDONLY)) == 0){  // error check for file 
         return true; 
      }
      if(fstat(file, metadata) == 0){ 
         return true; 
      }
   }
   return false;
}

bool endianess_converter(uint32_t *src_data, uint32_t *dst_data, const size_t src_count){
   if (src_data && dst_data && src_count >= 1){
      uint32_t swapped; 
      int i;
      for (i = 0; i < src_count; i++){
         // refernce: https://stackoverflow.com/questions/2182002/convert-big-endian-to-little-endian-in-c-without-using-provided-func
         swapped = ((src_data[i] & 0x000000ff) << 24u) | ((src_data[i] & 0x0000ff00) << 8u) | 
                  ((src_data[i] & 0x00ff0000) >> 8u) | ((src_data[i] & 0xff000000) >> 24u);
         dst_data[i] = swapped; 
      }
      return true; 
   }
   return false;
}

