#include <stdio.h>
#include <stdint.h>
#include "bitmap.h"
#include "block_store.h"
// include more if you need
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

/** GIVEN CONSTANTS 
    #define BLOCK_STORE_NUM_BLOCKS 512  
    #define BLOCK_SIZE_BYTES 32         // 2^5 BYTES per block
    #define BITMAP_SIZE_BITS BLOCK_STORE_NUM_BLOCKS
    #define BITMAP_SIZE_BYTES (BITMAP_SIZE_BITS / 8)
    #define BLOCK_STORE_NUM_BYTES (BLOCK_STORE_NUM_BLOCKS * BLOCK_SIZE_BYTES)
    #define BITMAP_START_BLOCK 127
    #define BITMAP_NUM_BLOCKS (BITMAP_SIZE_BYTES/BLOCK_SIZE_BYTES)
*/

// create a new struct that holds an array of bytes
// using this struct along with block_store struct allows us to treat data like a 2D array
typedef struct block{
    uint8_t block_bytes[BLOCK_SIZE_BYTES];
}block_t; 

typedef struct block_store{
    bitmap_t*fbm; // keeps track of block in use 
    block_t num_blocks[BLOCK_STORE_NUM_BLOCKS];
}block_store_t; 



block_store_t *block_store_create(){
    block_store_t*bs = (block_store_t*)calloc(1, sizeof(block_store_t)); // allocate memory block for block store
    if (!bs){ // alloc check 
        return NULL;
    }
    // bs->fbm = bitmap_create(BLOCK_STORE_NUM_BLOCKS); // create bitmap space with size of available bs blocks 
    // create a bitmap with 512 bits, each representing a block, and set starting point at 127
    bs->fbm = bitmap_overlay(BITMAP_SIZE_BITS, &(bs->num_blocks[BITMAP_START_BLOCK]));
    if (bs->fbm == NULL){ // check for failed bitmap create
        return NULL;
    }
    // store fbm starting in block 127/128
    bitmap_set(bs->fbm, 127);
    bitmap_set(bs->fbm, 128);
    // size_t i = 0; 
    // while (i < BITMAP_SIZE_BYTES){ 
    //     if ((i >= BITMAP_START_BLOCK + BITMAP_NUM_BLOCKS) || (int)i < BITMAP_START_BLOCK){ 
    //         bitmap_reset(bs->fbm, i); 
    //     }
    //     i++;
    // }
    return bs; // return block store object 
}

void block_store_destroy(block_store_t *const bs){
    if (!bs){ // param check
        return; 
    }
    // destory bitmap object and free the block store device
    bitmap_destroy(bs->fbm); 
    free(bs);
}

size_t block_store_allocate(block_store_t *const bs){
    if (bs && bs->num_blocks){ // param check
        size_t ffz = bitmap_ffz(bs->fbm); // find first zero bit in free-block-map
        if (ffz >= SIZE_MAX || ffz > BLOCK_STORE_NUM_BLOCKS){ // error check size of returned block 
            return SIZE_MAX; 
        }    
        bitmap_set(bs->fbm, ffz); // else set ffz bit in fbm
        return ffz; 
    }
    return SIZE_MAX; // else return SIZE_MAX on error 
}

/** Attempts to allocate the requested block id
	\param bs the block store object
	\param block_id the requested block identifier
	\return boolean indicating succes of operation
*/
bool block_store_request(block_store_t *const bs, const size_t block_id){
    
    if (bs && block_id > 0 && block_id < BLOCK_STORE_NUM_BLOCKS){ // check params  
        if (bitmap_test(bs->fbm, block_id) == 1){ // check if bit is already set
            return false; 
        } // if bit is not set
        bitmap_set(bs->fbm, block_id); // set block bit at block_id
        if (bitmap_test(bs->fbm, block_id) == 0){ // check bit again after setting to ensure it was set correctly 
            return false; // if the current block wasn't set, return false
        } 
        return true;
    }
    return false;
}

/* Frees the specified block */
void block_store_release(block_store_t *const bs, const size_t block_id){ 
    if (bs && block_id < BLOCK_STORE_NUM_BLOCKS){ // param check 
        if (bitmap_test(bs->fbm, block_id) == 0){ // check if block that block_id is at is free
            return;
        }
        bitmap_reset(bs->fbm, block_id); // if block isn't free, reset block value (1 -> 0)
        return;
    }
    return; 
}

size_t block_store_get_used_blocks(const block_store_t *const bs){
    if (bs){ // param check
        size_t blocks_set = bitmap_total_set(bs->fbm); // count total # of bits in fbm
        return blocks_set; // return total # of bits
    } // else if theres no block storage device
    return SIZE_MAX; // return SIZE_MAX 
}

size_t block_store_get_free_blocks(const block_store_t *const bs){
    if (bs){ // param check
        bitmap_invert(bs->fbm); // flip all bits 
        // get the total # of set bits now that all bits are flipped 
        // bitmap_total_set() can only get SET blocks, thus temporarily flip bits so that bit = 1 means its free
        size_t blocks_free = bitmap_total_set(bs->fbm);
        bitmap_invert(bs->fbm); // flip bits back to correct values 
        return blocks_free; // return counted bits
    } // else if theres no block store device
    return SIZE_MAX; // return SIZE_MAX 
}

size_t block_store_get_total_blocks(){
    return BLOCK_STORE_NUM_BLOCKS; // return total blocks
}

/** Reads data from the specified block and writes it to the designated buffer
 \param bs BS device
 \param block_id Source block id
 \param buffer Data buffer to write to
 \return Number of bytes read, 0 on error
*/
size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer){
    if (bs && block_id <= BLOCK_STORE_NUM_BYTES && buffer){ // param check
        // copy data from specified (from block_id) in block-store device into buffer | copy 32 bytes into buffer at a time
        int*copy_data = memcpy(buffer, bs->num_blocks[block_id].block_bytes, BLOCK_SIZE_BYTES); 
        if (!copy_data){
            return 0; // return 0 if copy fails
        }
        return BLOCK_SIZE_BYTES; // return the # of bytes read
    }
    return 0; 
}

/** Reads data from the specified buffer and writes it to the designated block
 \param bs BS device
 \param block_id Destination block id
 \param buffer Data buffer to read from
 \return Number of bytes written, 0 on error
*/
size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer){
    if (bs && block_id < BLOCK_STORE_NUM_BYTES && buffer){ // check params
        // write data from buffer into block-store device at specified block_id location
        int*copy_data = memcpy(bs->num_blocks[block_id].block_bytes, buffer, BLOCK_SIZE_BYTES);
        if(!copy_data){
            return 0; // if copy failed return 0
        }
        return BLOCK_SIZE_BYTES; // else return the # of bytes written
    }
    return 0; 
}

/* ---- EXTRA CREDIT ---- */

/** Imports BS device from the given file - for grads/bonus
    \param filename The file to load
    \return Pointer to new BS device, NULL on error
*/
block_store_t *block_store_deserialize(const char *const filename){
    if (filename){
        // block_store_t*bs = block_store_create();
        block_store_t*bs = (block_store_t*)malloc(sizeof(block_store_t));
        if (bs == NULL){
            return NULL;
        }
    
        int fd = open(filename, O_RDONLY);
        if (fd < 0){
            return NULL;
        }

        // ssize_t buff = sizeof(block_t)*BLOCK_STORE_NUM_BLOCKS;
        // read up to 512 bytes from file, into the block store device
        ssize_t read_bytes = read(fd, bs, BLOCK_STORE_NUM_BYTES);
        if (read_bytes < 0){ // check if read failed
                close(fd);
                return NULL;
            }
        
        // allocate memory for data to be stored ;
        // **bs->num_blocks = (char*)malloc((BLOCK_STORE_NUM_BLOCKS - 1) * BLOCK_STORE_NUM_BLOCKS * sizeof(char));
        // read(fd, **bs->data, (BLOCK_STORE_NUM_BLOCKS - 1) * BLOCK_STORE_NUM_BLOCKS);
        
        // copy block data from bitmap, stored at the bitmap_start_block
        bs->fbm = bitmap_import(BITMAP_SIZE_BITS, &(bs->num_blocks[BITMAP_START_BLOCK])); 
        if (bs->fbm == NULL){ // check if data copied successfully
            return NULL;    // if failed, return NULL
        }
        
        int is_closed = close(fd); // close file 
        if (is_closed < 0){ // check if file closed successfully 
            return NULL;
        }
        return bs; // return block store device 
    }
    return NULL;
}

/** Writes the entirety of the BS device to file, overwriting it if it exists - for grads/bonus
 \param bs BS device
 \param filename The file to write to
 \return Number of bytes written, 0 on error
*/ 
size_t block_store_serialize(const block_store_t *const bs, const char *const filename){
    if (bs && filename){
        /* references: 
            http://www.ss64.com/bash/chmod.html
            https://stackoverflow.com/questions/15798450/open-with-o-creat-was-it-opened-or-created 
            https://stackoverflow.com/questions/18415904/what-does-mode-t-0644-mean
            
            0x644:
            * (owning) User: read & write
            * Group: read
            * Other: read               */

        // open file in write only mode
        // O_CREAT | O_TRUNC to return an error if the file already exists
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644); // open file 
        if (fd < 0){ // check if file opened succesfully 
            return 0;
        }
        ssize_t write_bytes = write(fd, bs, BLOCK_STORE_NUM_BYTES); // 
        if (write_bytes < 0){ // check write 
            return 0;
        }
        int is_closed = close(fd); // close file
        if (is_closed < 0){ // check if file closed successfully
            return 0;
        }
        return write_bytes; // return written bytes 
    }
    return 0;
}