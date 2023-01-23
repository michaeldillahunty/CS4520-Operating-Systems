#include "dyn_array.h"
#include "bitmap.h"
#include "block_store.h"
#include "FS.h"

#define BLOCK_STORE_NUM_BLOCKS 65536    // 2^16 blocks.
#define BLOCK_STORE_AVAIL_BLOCKS 65534  // Last 2 blocks consumed by the FBM
#define BLOCK_SIZE_BITS 32768           // 2^12 BYTES per block *2^3 BITS per BYTES
#define BLOCK_SIZE_BYTES 4096           // 2^12 BYTES per block
#define BLOCK_STORE_NUM_BYTES (BLOCK_STORE_NUM_BLOCKS * BLOCK_SIZE_BYTES)  // 2^16 blocks of 2^12 bytes.


// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

/// Formats (and mounts) an FS file for use
/// \param fname The file to format
/// \return Mounted FS object, NULL on error
///
FS_t *fs_format(const char *path)
{
    if(path != NULL && strlen(path) != 0)
    {
        FS_t * ptr_FS = (FS_t *)calloc(1, sizeof(FS_t));	// get started
        ptr_FS->BlockStore_whole = block_store_create(path);				// pointer to start of a large chunck of memory

        // reserve the 1st block for bitmap of inode
        size_t bitmap_ID = block_store_allocate(ptr_FS->BlockStore_whole);
        //		printf("bitmap_ID = %zu\n", bitmap_ID);

        // 2rd - 5th block for inodes, 4 blocks in total
        size_t inode_start_block = block_store_allocate(ptr_FS->BlockStore_whole);
        //		printf("inode_start_block = %zu\n", inode_start_block);		
        for(int i = 0; i < 3; i++)
        {
            block_store_allocate(ptr_FS->BlockStore_whole);
            //			printf("all the way with block %zu\n", block_store_allocate(ptr_FS->BlockStore_whole));
        }

        // install inode block store inside the whole block store
        ptr_FS->BlockStore_inode = block_store_inode_create(block_store_Data_location(ptr_FS->BlockStore_whole) + bitmap_ID * BLOCK_SIZE_BYTES, block_store_Data_location(ptr_FS->BlockStore_whole) + inode_start_block * BLOCK_SIZE_BYTES);

        // the first inode is reserved for root dir
        block_store_sub_allocate(ptr_FS->BlockStore_inode);
        //		printf("first inode ID = %zu\n", block_store_sub_allocate(ptr_FS->BlockStore_inode));

        // update the root inode info.
        uint8_t root_inode_ID = 0;	// root inode is the first one in the inode table
        inode_t * root_inode = (inode_t *) calloc(1, sizeof(inode_t));
        //		printf("size of inode_t = %zu\n", sizeof(inode_t));
        root_inode->vacantFile = 0x00000000;
        root_inode->fileType = 'd';								
        root_inode->inodeNumber = root_inode_ID;
        root_inode->linkCount = 1;
        //		root_inode->directPointer[0] = root_data_ID;	// not allocate date block for it until it has a sub-folder or file
        block_store_inode_write(ptr_FS->BlockStore_inode, root_inode_ID, root_inode);		
        free(root_inode);

        // now allocate space for the file descriptors
        ptr_FS->BlockStore_fd = block_store_fd_create();

        return ptr_FS;
    }

    return NULL;	
}



///
/// Mounts an FS object and prepares it for use
/// \param fname The file to mount

/// \return Mounted FS object, NULL on error

///
FS_t *fs_mount(const char *path)
{
    if(path != NULL && strlen(path) != 0)
    {
        FS_t * ptr_FS = (FS_t *)calloc(1, sizeof(FS_t));	// get started
        ptr_FS->BlockStore_whole = block_store_open(path);	// get the chunck of data	

        // the bitmap block should be the 1st one
        size_t bitmap_ID = 0;

        // the inode blocks start with the 2nd block, and goes around until the 5th block, 4 in total
        size_t inode_start_block = 1;

        // attach the bitmaps to their designated place
        ptr_FS->BlockStore_inode = block_store_inode_create(block_store_Data_location(ptr_FS->BlockStore_whole) + bitmap_ID * BLOCK_SIZE_BYTES, block_store_Data_location(ptr_FS->BlockStore_whole) + inode_start_block * BLOCK_SIZE_BYTES);

        // since file descriptors are allocated outside of the whole blocks, we can simply reallocate space for it.
        ptr_FS->BlockStore_fd = block_store_fd_create();

        return ptr_FS;
    }

    return NULL;		
}




///
/// Unmounts the given object and frees all related resources
/// \param fs The FS object to unmount
/// \return 0 on success, < 0 on failure
///
int fs_unmount(FS_t *fs)
{
    if(fs != NULL)
    {	
        block_store_inode_destroy(fs->BlockStore_inode);

        block_store_destroy(fs->BlockStore_whole);
        block_store_fd_destroy(fs->BlockStore_fd);

        free(fs);
        return 0;
    }
    return -1;
}


// check if the input filename is valid or not
bool isValidFileName(const char *filename)
{
    if(!filename || strlen(filename) == 0 || strlen(filename) > 127 || '/'==filename[0])
    {
        return false;
    }

    return true;
}



// use str_split to decompose the input string into filenames along the path, '/' as delimiter
char** str_split(char* a_str, const char a_delim, size_t * count)
{
    if(*a_str != '/')
    {
        return NULL;
    }
    char** result    = 0;
    char* tmp        = a_str;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = '\0';

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            (*count)++;
        }
        tmp++;
    }

    result = (char**)calloc(1, sizeof(char*) * (*count));
    for(size_t i = 0; i < (*count); i++)
    {
        *(result + i) = (char*)calloc(1, 200);
    }

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            strcpy(*(result + idx++), token);
            //    *(result + idx++) = strdup(token);
            token = strtok(NULL, delim);
        }

    }
    return result;
}



///
/// Creates a new file at the specified location
///   Directories along the path that do not exist are not created
/// \param fs The FS containing the file
/// \param path Absolute path to file to create
/// \param type Type of file to create (regular/directory)
/// \return 0 on success, < 0 on failure
///
/** MILESTONE 2 **
------------------------------
    - fs_create
    - fs_open
    - fs_close
    - fs_get_dir

    typedef enum { FS_REGULAR, FS_DIRECTORY } file_t;
*/

/** Function to split the given path, using delimiters, into individual tokens
    Majority of the code used in this function is taken from this stackoverflow thread
    - https://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c
*/
char **tokenize(const char *path, size_t *token_count){
    if (*path != '/'){
        return NULL;
    }
    char**result = 0;
    char *path_copy = calloc(1, 300);
    strcpy(path_copy, path);
    char *temp = path_copy;
    while (*temp){
        if (*temp == '/'){
            (*token_count)++;
        }
        temp++;
    }
    result = (char**)calloc(1, sizeof(char*) * (*token_count));
    for (size_t i = 0; i < (*token_count); i++){
        *(result + i) = (char*)calloc(1, 300);
    }
    size_t index = 0;
    char *token = strtok(path_copy, "/");
    while (token){
        strcpy(result[index], token);
        token = strtok(NULL, "/");    
        index++;
    }
    free(path_copy);
    return result;
}

/** Creates a new file at the specified location
     Directories along the path that do not exist are not created
    \param fs The FS containing the file
    \param path Absolute path to file to create
    \param type Type of file to create (regular/directory)
    \return 0 on success, < 0 on failure
*/
int fs_create(FS_t *fs, const char *path, file_t type) {
    // param checks
    if (fs != NULL && path != NULL && strlen(path) != 0 && (type == FS_REGULAR || type == FS_DIRECTORY)){ 
        inode_t*new_inode = calloc(1, sizeof(inode_t)); // allocate space for a new inode
        if (new_inode == NULL){ // malloc check
            return -1;
        }
        size_t new_inode_num = 0; // initialize a new corresponding inode number
        block_store_inode_read(fs->BlockStore_inode, 0, new_inode); // read the new inode into the first space -> becomes parent inode temporarily

        directoryFile_t*directory = calloc(1, BLOCK_SIZE_BYTES); // allocate space for a new directory
        if (directory == NULL){ // malloc check
            return -1;
        }
        size_t dir_counter = 0; // the depth/level in the directory (counter to help keep track of position)        
        block_store_read(fs->BlockStore_whole, new_inode->directPointer[0], directory); // read inode data into the new directory

        size_t token_count = 0; // # of tokens
        char**tokens_arr; // array of 'tokens' created by splitting the path using a delimiter 
        tokens_arr = tokenize(path, &token_count); // tokenize the path
        if (tokens_arr == NULL){ // check if tokenize was successful
            return -1; 
        }
        size_t n = 0;
        while (n < token_count){ // loop through array of tokens
            if (strlen(tokens_arr[n]) == 0 || strchr(tokens_arr[n], '!') != NULL){ // check for empty tokens or tokens that contain invalid chars
                for (size_t i = 0; i < token_count; i++){ // if empty string or invalid char, free all tokens
                    free(tokens_arr[i]);
                }
                free(tokens_arr);
                return -1; 
            }
            n++;
        }

        fs->bitmap = bitmap_overlay(32, &(new_inode->vacantFile)); // create bitmap using the parent inode data
        for (size_t i = 0; i < token_count - 1; i++){ // loop through tokens array - 1
            // REMEMBER: have to be in a dir to create a new file/dir
            if ( (new_inode->fileType) == 'd'){ // extra layer check in case dir/file have the same name
                int curr_dir = 0;
                while (curr_dir < folder_number_entries){
                    block_store_read(fs->BlockStore_whole, new_inode->directPointer[0], directory); // update directory data with parent inode data        
                    if (strcmp((directory + curr_dir)->filename, tokens_arr[i]) == 0){ // if the 2 strings/tokens are equal
                        new_inode_num = (directory + curr_dir)->inodeNumber; // update inode number
                        block_store_inode_read(fs->BlockStore_inode, new_inode_num, new_inode); // read updated inode number into parent inode
                        dir_counter++;
                    }
                    curr_dir++;                    
                }
            }
        }
        
        /*
            printf("Path:%s\n", path);
            printf("# of Subpaths = %zu\n", dir_counter);
            printf("Parent Inode # = %lu\n", new_inode_num);
            printf("# of Tokens = %zu\n", token_count);
            printf("------------------------\n");
        */

        if (new_inode->fileType == 'd' && dir_counter == token_count - 1 ){  // check that we're in a dir -> cannot create a file inside a file
            size_t first_zero = bitmap_ffz(fs->bitmap); // find first 0 bit to store to be created directory in
            for (size_t i = 0; i < first_zero; i++){ // loop through non-set bits
                // check if the filename trying to be created already exists 
                if (strcmp((directory + i)->filename, tokens_arr[token_count - 1]) == 0){ // if they're the same, error and free
                    free(directory);
                    free(new_inode);
                    for (size_t j = 0; j < token_count; j++){ 
                        free(tokens_arr[j]);
                    }
                    free(tokens_arr);
                    return -1; 
                }
            }

            if (first_zero < folder_number_entries){ // check that theres not more than the # of allowed entries
                if (first_zero == 0){ // double check that bit found is not set
                    size_t temp_block_id = block_store_allocate(fs->BlockStore_whole); // allocate a new bs device for the inode table 
                    if (temp_block_id > BLOCK_STORE_AVAIL_BLOCKS){ // if there are no blocks availible -> free everything
                        free(directory); 
                        free(new_inode); 
                        for(size_t i = 0; i < token_count; i++){
                            free(tokens_arr[i]);
                        }
                        free(tokens_arr);
                        bitmap_destroy(fs->bitmap);
                        return -1;
                    } 
                    // else, store created block data in inode table
                    new_inode->directPointer[0] = temp_block_id;
                }
                bitmap_set(fs->bitmap, first_zero); // change bit to set and update bitmap
                block_store_inode_write(fs->BlockStore_inode, new_inode_num, new_inode); // update inode with new data

                size_t temp_file_id = block_store_allocate(fs->BlockStore_inode); // allocate memory for file to be created
                if (temp_file_id == SIZE_MAX){ // if failed to allocate new block
                    free(new_inode);
                    free(directory);
                    for(size_t i = 0; i < token_count; i++){
                        free(tokens_arr[i]);
                    }
                    free(tokens_arr);
                    bitmap_destroy(fs->bitmap);
                    return -1; 
                }
                strcpy((directory + first_zero)->filename, tokens_arr[token_count - 1]); // update filename
                (directory + first_zero)->inodeNumber = temp_file_id; // update the inode num to the new file block's id 
                block_store_write(fs->BlockStore_whole, new_inode->directPointer[0], directory); // write the inode data to the directory
                
                inode_t*fs_inode = (inode_t*)calloc(1, sizeof(inode_t)); // allocate a new inode for the file or directory
                if (fs_inode == NULL){ // malloc check
                    free(fs_inode);
                    return -1; 
                }
                // set the filetype data for the new inode
                if (type == FS_REGULAR){
                    fs_inode->fileType = 'r';
                } else if (type == FS_DIRECTORY) {
                    fs_inode->fileType = 'd'; 
                }
                block_store_inode_write(fs->BlockStore_inode, temp_file_id, fs_inode); // update new inode with file block id data

                free(directory);
                free(new_inode);
                free(fs_inode);
                for (size_t i = 0; i < token_count; i++){
                    free(tokens_arr[i]);
                }
                free(tokens_arr); 
                bitmap_destroy(fs->bitmap);
                return 0;
            }
        }
        free(new_inode); 
        free(directory);
        for (size_t i = 0; i < token_count; i++){
            free(tokens_arr[i]);
        }
        free(tokens_arr);
    }
    return -1;
}

/** Opens the specified file for use
      R/W position is set to the beginning of the file (BOF)
      Directories cannot be opened
    \param fs The FS containing the file
    \param path path to the requested file
    \return file descriptor to the requested file, < 0 on error
*/
int fs_open(FS_t *fs, const char *path) {
    if (fs != NULL && path != NULL && strlen(path) > 0) {
        inode_t*new_inode = (inode_t*)calloc(1, sizeof(inode_t)); // create new inode 
        if (new_inode == NULL){ // malloc check
            return -1;
        }
        size_t new_inode_num = 0; // start at the first inode
        block_store_inode_read(fs->BlockStore_inode, 0, new_inode); // read the first inode to our new parent inode

        directoryFile_t*directory = (directoryFile_t*)calloc(1, BLOCK_SIZE_BYTES); // create a new directory
        if (directory == NULL){ // malloc check 
            return -1; 
        }
        size_t dir_counter = 0; // counter variable to track depth in directory (should be the same as the token)
        block_store_read(fs->BlockStore_whole, new_inode->directPointer[0], directory);  // read the directory data from new inode to the directory

        size_t token_count = 0;
        char**tokens_arr;
        tokens_arr = tokenize(path, &token_count);
        if (tokens_arr == NULL){ // check tokenize worked as expected
            return -1;
        }
        
        for (size_t i = 0; i < token_count; i++){ // loop through tokens array
            if (new_inode->fileType == 'd'){ // check that it's a directory -> because we can't open a file the way we would a directory
                int curr_dir = 0; 
                while (curr_dir < folder_number_entries){ // loop through the entire directory
                    /* NOT ENTERING IF BLOCK
                        Loop 1: 
                            token[0] = folder
                                |folder|file|0|0|0|0|
                            directory[0]->filename = file
                                |file|0|0|0|0|0|
                                >> 1 = |folder|file|0|0|0|0|
                     */
                        // printf("Path = %s\n", path);
                        // printf("tokens[%zu]    = %s\n", i, tokens[i]);
                        // printf("directory[%zu] = %s\n", i, directory->filename);
                        // printf("Dir Counter = %zu\n", dir_counter);
                        // printf("Current dir = %x\n", curr_dir);
                        // printf("------------------------------\n"); 
                    
                    block_store_read(fs->BlockStore_whole, new_inode->directPointer[0], directory); // update directory data
                    if (strcmp((directory + curr_dir)->filename, tokens_arr[i]) == 0) { // check that the current directory name and token name match 
                        new_inode_num = (directory + curr_dir)->inodeNumber; // set inode position to current and increment
                        dir_counter++; 
                    } 
                    curr_dir++; 
                }
            }
            block_store_inode_read(fs->BlockStore_inode, new_inode_num, new_inode); // update inode number
        }
        free(directory); 

        /*
            printf("Path: %s\n", path);
            printf("# of Directories = %zu\n", dir_counter);  
            printf("Parent Inode ID  = %lu\n", new_inode_num);
            printf("# of Tokens      = %zu\n", token_count);
            for (size_t test = 0; test < token_count; test++){
            printf("Token %zu)         = %s\n", test, tokens[test]);
            }
            printf("------------------------\n"); 
        */

        if (dir_counter == token_count){  // if the dir counter and token count values are = continue 
            size_t fd_table = block_store_sub_allocate(fs->BlockStore_fd); // allocate file descriptor table in empty block
            if (fd_table < number_fd){ // check that we have not exceeded the limit for # of file descriptors
                if ((new_inode->fileType) == 'd'){ // if the parent inode corresponds to a dir, free and error -> we are trying to open a file
                    free(new_inode); 
                        for (size_t i = 0; i < token_count; i++){
                            free(tokens_arr[i]);
                        }
                    free(tokens_arr);
                    return -1; 
                }
                size_t fd_inode_num = new_inode_num; // create new fd inode num and set it to the current inode number
                inode_t*fd_inode = (inode_t*)calloc(1, sizeof(inode_t)); // allocate a new inode
                if (fd_inode == NULL){ // malloc check
                    return -1;
                }
                fileDescriptor_t*new_fd = (fileDescriptor_t*)calloc(1, sizeof(fileDescriptor_t)); // allocate a new fd
                if (new_fd == NULL){ // malloc check 
                    return -1; 
                }
                block_store_inode_read(fs->BlockStore_inode, fd_inode_num, fd_inode); // read fd_inode data with the fd inode num
                new_fd->inodeNum = new_inode_num; // update the inode number to the current inode #
                block_store_fd_write(fs->BlockStore_fd, fd_table, new_fd); // add the file descriptor table to the file descriptor 
                free(new_inode);
                free(new_fd);
                for (size_t j = 0; j < token_count; j++){
                    free(tokens_arr[j]);
                }
                free(tokens_arr);
                return fd_table; 
            }
        } // else free all tokens
        for (size_t i = 0; i < token_count; i++){
            free(tokens_arr[i]);
        }
        free(tokens_arr);
    }
    return -1; 
}

/** Closes the given file descriptor
    \param fs The FS containing the file
    \param fd The file to close
    \return 0 on success, < 0 on failure
*/
int fs_close(FS_t *fs, int fd){
    // param check
    if(fs != NULL && fd >= 0 && fd < number_fd && block_store_sub_test(fs->BlockStore_fd, fd)){
        // if the fd is being used, "release" it 
        block_store_sub_release(fs->BlockStore_fd, fd);
        return 0;   
    } // else nothing to close -> error
    return -1;
}

/** Populates a dyn_array with information about the files in a directory
      Array contains up to 15 file_record_t structures
    \param fs The FS containing the file
    \param path Absolute path to the directory to inspect
    \return dyn_array of file records, NULL on error
*/
dyn_array_t *fs_get_dir(FS_t *fs, const char *path) {
    // param check
    if (fs != NULL && path != NULL && strlen(path) > 0){
        inode_t*new_inode = (inode_t*)calloc(1, sizeof(inode_t)); // create a new inode 
        if (new_inode == NULL){
            return NULL;
        }
        size_t new_inode_num = 0; // initialize the inode num
        block_store_inode_read(fs->BlockStore_inode, 0, new_inode); // set the inode position to the first block 

        directoryFile_t*directory = (directoryFile_t*)calloc(1, BLOCK_SIZE_BYTES); // create a new directory
        if (directory == NULL){
            return NULL;
        }
        size_t dir_counter = 0; 
        block_store_read(fs->BlockStore_whole, new_inode->directPointer[0], directory); // add inode data to directory

        size_t token_count = 0;
        char**tokens_arr; 
        tokens_arr = tokenize(path, &token_count);
        if (strlen(*tokens_arr) == 0){
            token_count = token_count - 1;
        } 
        
        size_t j = 0;
        while (j < token_count){ // loop through array of tokens
            if (strlen(tokens_arr[j]) == 0 || strchr(tokens_arr[j], '!') != NULL){ // check for empty tokens or tokens that contain invalid chars
                for (size_t i = 0; i < token_count; i++){ // if empty string or invalid char, free all tokens
                    free(tokens_arr[i]);
                }
                free(tokens_arr);
                return NULL; 
            }
            j++;
        }

        for (size_t i = 0; i < token_count; i++){
            if (new_inode->fileType == 'd'){
                int curr_dir = 0;
                while (curr_dir < folder_number_entries){
                    if (strcmp((directory + curr_dir)->filename, tokens_arr[i]) == 0){
                        block_store_read(fs->BlockStore_whole, new_inode->directPointer[0], directory);
                        new_inode_num = (directory + curr_dir)->inodeNumber;
                        dir_counter++;
                    }
                    curr_dir++;
                }
            }
            block_store_inode_read(fs->BlockStore_whole, new_inode_num, new_inode);
        }
        fs->bitmap = bitmap_overlay(32, &(new_inode->vacantFile));
        free(new_inode);
        free(directory);

        // check current position and token # match | check the parent inode is a directory
        if (dir_counter == token_count){ // check current position and token # match (aka we're in the right spot)
            inode_t*dir_inode = (inode_t*)calloc(1, sizeof(inode_t)); // allocate a new inode for the file
            if (dir_inode == NULL){
                return NULL;
            }
            directoryFile_t*dir_file = (directoryFile_t*)calloc(1, BLOCK_SIZE_BYTES); // create a directory file to hold the data
            if (dir_file == NULL){
                return NULL;
            }
            dyn_array_t*dyn_arr = dyn_array_create(20, sizeof(file_record_t), NULL); // create dynamic array to hold up to 20 directory file objects

            block_store_inode_read(fs->BlockStore_inode, new_inode_num, dir_inode); // read the parent inode data into the file inode
            if (dir_inode->fileType == 'd'){ // double check that we're at a directory -> files can't contain other files
                block_store_read(fs->BlockStore_whole, dir_inode->directPointer[0], dir_file); // once we know we're at a dir - read data from inode to directory file
                int curr_dir = 0;
                while (curr_dir < folder_number_entries){ // loop through directory
                    if (bitmap_test(fs->bitmap, curr_dir) == 1){  // look for a populated bit (aka contains a file)
                        file_record_t*file_data = (file_record_t*)calloc(1, sizeof(file_record_t)); // allocate space for file metadata
                        strcpy(file_data->name, (dir_file + curr_dir)->filename); // copy current dir filename to the files data 
                        dyn_array_push_front(dyn_arr, file_data); // add the file data at the front of the dynamic array
                        free(file_data);
                    }
                    curr_dir++; 
                }
            }
            free(dir_inode);
            free(dir_file);
            return dyn_arr;
        }
        bitmap_destroy(fs->bitmap);
    }
    return NULL;
}

/** Moves the R/W position of the given descriptor to the given location
      Files cannot be seeked past EOF or before BOF (beginning of file)
      Seeking past EOF will seek to EOF, seeking before BOF will seek to BOF
    \param fs The FS containing the file
    \param fd The descriptor to seek
    \param offset Desired offset relative to whence
    \param whence Position from which offset is applied
    \return offset from BOF, < 0 on error 
*/
off_t fs_seek(FS_t *fs, int fd, off_t offset, seek_t whence) {
    if (fs != NULL && (whence == FS_SEEK_CUR || whence == FS_SEEK_SET)){ // initial param check
        if (block_store_sub_test(fs->BlockStore_fd, fd) == false){ // check file descriptor table block which fd corresponds to
            return -1;
        }
        if (offset < 0){
            return 0;
        }
        
        fileDescriptor_t new_fd;
        block_store_fd_read(fs->BlockStore_fd, fd, &new_fd);
        
        // if we surpass max offset, seek to the max offset 
        if (offset > 259981311){
            new_fd.locate_order = 0;
            new_fd.locate_offset = 0;
            offset = 259981311;
            // updateFileDescriptor(&fileDescriptor, 63472 * 4096 - 1);
            size_t remaining = 4096 - new_fd.locate_offset;
            new_fd.locate_offset += (63472 * 4096) - 1;  // offset - 1
            if (remaining < (63472 * 4096) - 1){
                new_fd.locate_offset += ((63472 * 4096) - 1) / 4096;
                new_fd.locate_offset = ((63472 * 4096) - 1) % 4096;
            }
            block_store_fd_write(fs->BlockStore_fd, fd, &new_fd);
            return offset;
        } else {
            inode_t inode;
            block_store_inode_read(fs->BlockStore_inode, new_fd.inodeNum, &inode);
            if (whence == FS_SEEK_CUR) {
                off_t prev_offset = 0;
                if (new_fd.locate_offset != 0) {
                    prev_offset += new_fd.locate_order * BLOCK_SIZE_BYTES;
                    prev_offset += new_fd.locate_offset;
                }
                if (offset > (int)inode.fileSize){
                    offset = inode.fileSize;
                } else {
                    offset = offset + prev_offset;
                }
            }
            new_fd.locate_order = 0;
            new_fd.locate_offset = 0;
            // updateFileDescriptor(&fileDescriptor, offset);
            int remaining = 4096 - new_fd.locate_offset;
            new_fd.locate_offset += offset;
            if (remaining < offset) {
                new_fd.locate_order += offset / 4096;
                new_fd.locate_offset = offset % 4096;
            }
            block_store_fd_write(fs->BlockStore_fd, fd, &new_fd);
            return offset;
        }
    }
    return -1;
}

/** Reads data from the file linked to the given descriptor
      Reading past EOF returns data up to EOF
      R/W position in incremented by the number of bytes read
    \param fs The FS containing the file
    \param fd The file to read from 
    \param dst The buffer to write to
    \param nbyte The number of bytes to read
    \return number of bytes read (< nbyte IFF read passes EOF), < 0 on error
*/
/* Notes: 
    - int fd = index into file descriptor table / fd ID
        - allows us to use block_store_fd_read() on that int to access the corresponding fd struct
    - idea: fs_open() does the path traversal to find the inode number for the file that's being opened
        - stores that inode number in the file descriptor so other functions, like fs_read() and fs_write(),
        don't have to traverse the path
        - Inode number in a file descriptor should be the inode ID for the inode that represents the file 
*/
ssize_t fs_read(FS_t *fs, int fd, void *dst, size_t nbyte){
    if (fs != NULL && fd < number_fd && dst != NULL){ // error check params
        fileDescriptor_t new_fd;
        block_store_fd_read(fs->BlockStore_fd, fd, &new_fd);
        inode_t fd_inode;
        block_store_inode_read(fs->BlockStore_inode, new_fd.inodeNum, &fd_inode);
        off_t prev_offset = 0;

        if (new_fd.locate_order == 0) {
            prev_offset = new_fd.locate_offset;
        } else if (new_fd.locate_offset != 0) {
            prev_offset += ((new_fd.locate_order - 1) * 4096);
            prev_offset += new_fd.locate_offset;
        } else {
            prev_offset += (new_fd.locate_order * 4096);
        }
        if (prev_offset + nbyte > fd_inode.fileSize){
            nbyte = fd_inode.fileSize - prev_offset;
        }
        uint16_t block_id = fd_inode.directPointer[new_fd.locate_order];
        uint8_t block_buff[4096];
        block_store_read(fs->BlockStore_whole, block_id, block_buff);
        memcpy(dst, block_buff + new_fd.locate_offset, nbyte);
        int remaining = 4096 - new_fd.locate_offset;
        new_fd.locate_offset += nbyte; 
        if (remaining > (int)nbyte){
            new_fd.locate_order += nbyte / 4096;
            new_fd.locate_offset = nbyte % 4096;
        }
        block_store_fd_write(fs->BlockStore_fd, fd, &new_fd);
        return nbyte;
    }        
    return -1; 
}

ssize_t write_indirect_ptr(FS_t*fs, uint16_t fd_order, uint16_t fd_offset, const void*src, size_t nbyte, size_t block_num){
    uint16_t block_buff[2048]; // buffer holder 
    block_store_read(fs->BlockStore_whole, block_num, block_buff); // add the indirect block id to the buffer
    ssize_t retBytes = 0;
    int avail = (fd_order - fd_size) % 2048;
    while (nbyte > 0 && avail < 2048){ // check that there is available space and data to write
        size_t block_space = BLOCK_SIZE_BYTES - fd_offset; // calculate space available in block
        if (block_space > nbyte){
            block_space = nbyte;
        }
        // update data
        avail = avail + 1; 
        nbyte = nbyte - block_space;
        src = src + block_space;
        retBytes = retBytes + block_space;
        fd_order += 1;
    }
    block_store_write(fs->BlockStore_whole, block_num, block_buff);
    return retBytes;
}

ssize_t write_double_indirect_ptr(FS_t *fs, inode_t *inode, uint16_t fd_order, uint16_t fd_offset, const void *src, size_t nbyte){
    uint16_t block_buff[2048];
    block_store_read(fs->BlockStore_whole, inode->doubleIndirectPointer, block_buff);
    size_t block_num = block_buff[(fd_order - (6 + 2048)) / 2048]; // udpate the new indirect block number for indirect ptr
    
    return write_indirect_ptr(fs, fd_order, fd_offset, src, nbyte, block_num + 2048);
}

ssize_t write_direct_ptr(FS_t*fs, inode_t*inode, uint16_t fd_order, uint16_t fd_offset, const void*src, size_t nbyte){
    uint8_t block_buff[BLOCK_SIZE_BYTES]; // a buffer for the current block we are writing to block id
    memset(block_buff, '\0', BLOCK_SIZE_BYTES);
    // calculate availible space that we can write to with the fd offset
    size_t available_space = 4096 - fd_offset; 
    if (available_space > nbyte){ // if there is more space than we need
        available_space = nbyte; // set the space to exact # we want to write
    } 
    if (inode->directPointer[fd_order] == 0){ // if inode  doesn't already have direct pointers allocated
        inode->directPointer[fd_order] = block_store_allocate(fs->BlockStore_whole);;
        memcpy(block_buff, src, BLOCK_SIZE_BYTES); // src path to buff holder
    } else { // if direct pointers are already there
        block_store_read(fs->BlockStore_whole, inode->directPointer[fd_order], block_buff); // read the updated block id into the buffer
        memcpy(block_buff + fd_offset, src, available_space); // copy contents of src to the buffer at the current fd offset
    }
    // update direct pointer with new data
    block_store_write(fs->BlockStore_whole, inode->directPointer[fd_order], block_buff); 
    nbyte = nbyte - available_space; // update nbytes
    if (nbyte == 0){ // if all bytes have been written
        return available_space;
    }
    if (fd_order + 1 < 6){ // if theres still more to be bytes to be written use next availible direct pointer
        fd_order = fd_order + 1;
        return available_space + write_direct_ptr(fs, inode, fd_order, 0, src + available_space, nbyte);
    } 
    return available_space;
}

void update_order_offset(fileDescriptor_t*new_fd, size_t nbyte){
    size_t remaining = 4096 - new_fd->locate_offset;
    if (remaining < nbyte){
        nbyte = nbyte - remaining ;
        new_fd->locate_order = new_fd->locate_order + 1;
        new_fd->locate_order += nbyte / 4096;
        new_fd->locate_offset = nbyte % 4096;
    } else {
        new_fd->locate_offset += nbyte;
        if (new_fd->locate_offset == 1024){
            new_fd->locate_order = new_fd->locate_order + 1;
            new_fd->locate_offset = 0;
        }
    }
}

/** Writes data from given buffer to the file linked to the descriptor
      Writing past EOF extends the file
      Writing inside a file overwrites existing data
      R/W position in incremented by the number of bytes written
    \param fs The FS containing the file
    \param fd The file to write to
    \param dst The buffer to read from
    \param nbyte The number of bytes to write
    \return number of bytes written (< nbyte IFF out of space), < 0 on error
*/
ssize_t fs_write(FS_t *fs, int fd, const void *src, size_t nbyte) {
    if (fs != NULL && src != NULL){ // param check 
        if (!block_store_sub_test(fs->BlockStore_fd, fd)){
            return -1;
        }
		fileDescriptor_t new_fd; // create file descriptor
		block_store_fd_read(fs->BlockStore_fd, fd, &new_fd); // update file descriptor with fd value 
        uint16_t fd_order = new_fd.locate_order;
        uint16_t fd_offset = new_fd.locate_offset;

		inode_t fd_inode;
		block_store_inode_read(fs->BlockStore_inode, new_fd.inodeNum, &fd_inode);

		ssize_t num_bytes_written = 0;
		if (fd_order < fd_size){
			num_bytes_written = write_direct_ptr(fs, &fd_inode, fd_order, fd_offset, src, nbyte);
            update_order_offset(&new_fd, nbyte);
		    
        } else if (fd_order >= fd_size && fd_order < (fd_size + 2048)){ // write to the indirect pointer
			size_t block_num;
			if (fd_inode.indirectPointer[0] == 0){
                uint16_t block_buff[fd_size + 2048];
                block_num = block_store_allocate(fs->BlockStore_whole);
                if (block_num == SIZE_MAX){
                    return 0;
                }
                memset(block_buff, '\0', BLOCK_SIZE_BYTES);
                block_store_write(fs->BlockStore_whole, block_num, block_buff);
				fd_inode.indirectPointer[0] = block_num;
			} 
			block_num = fd_inode.indirectPointer[0];
			num_bytes_written = write_indirect_ptr(fs, fd_order, fd_offset, src, nbyte, block_num);

		} else if(fd_order >= (fd_size + 2048)) { // allocate the double indirect pointers
			num_bytes_written = write_double_indirect_ptr(fs, &fd_inode, fd_order, fd_offset, src, nbyte);
            update_order_offset(&new_fd, nbyte);
		}
        block_store_fd_write(fs->BlockStore_fd, fd, &new_fd);
        fd_inode.fileSize += num_bytes_written;
        block_store_inode_write(fs->BlockStore_inode, new_fd.inodeNum, &fd_inode);
		return num_bytes_written;
    }
    return -1;
}

/** tokenize helper function I used in A5.2 */
char **tokenize(const char *path, size_t *token_count){
    if (*path != '/'){
        return NULL;
    }
    char**result = 0;
    char *path_copy = calloc(1, 65535);
    strcpy(path_copy, path);
    char *temp = path_copy;
    while (*temp){
        if (*temp == '/'){
            (*token_count)++;
        }
        temp++;
    }
    result = (char**)calloc(1, sizeof(char*) * (*token_count));
    for (size_t i = 0; i < (*token_count); i++){
        *(result + i) = (char*)calloc(1, 65535);
    }
    size_t index = 0;
    char *token = strtok(path_copy, "/");
    while (token){
        strcpy(result[index], token);
        token = strtok(NULL, "/");    
        index++;
    }
    free(path_copy);
    return result;
}


/*
    bool isValidFileName(const char *filename)
    {
        if(!filename || strlen(filename) == 0 || strlen(filename) > 127 || '/'==filename[0])
        {
            return false;
        }

        return true;
    }
*/

void free_tokens(char** tokens, size_t token_count) {
    for (size_t i = 0; i < token_count; i++){
        free(tokens[i]);
    }
    free(tokens);
}

/** Deletes the specified file and closes all open descriptors to the file
      Directories can only be removed when empty
    \param fs The FS containing the file
    \param path Absolute path to file to remove
    \return 0 on success, < 0 on error
*/
/**** NOTE ****
    A LOT OF THIS CODE FOR THIS FUNCTION
    WAS TAKEN FROM PREVIOUS FUNCTIONS WHICH
     JIMR GAVE US SOLUTIONS TO 
     -> THUS A LOT IS COPY PASTED JIMR CODE
*/
int fs_remove(FS_t *fs, const char *path) {
    if(fs != NULL && path != NULL && strlen(path) != 0) {
        size_t token_count = 0;
        char**tokens;
        tokens = tokenize(path, &token_count);
        if(tokens == NULL) {
            return -1;
        }

        directoryFile_t*parent_data = (directoryFile_t*)calloc(1, BLOCK_SIZE_BYTES);
        if (parent_data == NULL){
            free(parent_data);
            return -1; 
        }
        size_t dir_counter = 0;
        
        inode_t*parent_inode = (inode_t*)calloc(1, sizeof(inode_t));	
        if (parent_inode == NULL){
            free(parent_inode);
            return -1;
        }
        size_t parent_inode_ID = 0;	// start from the 1st inode, ie., the inode for root directory

        /* 
         *
         * FOLLOWING CODE IS TAKEN FROM JIMRS 'fs_create()' FUNCTION SOLUTION 
         *
         */

        // we declare parent_inode and parent_data here since it will still be used after the for loop
        

        for(size_t i = 0; i < token_count - 1; i++){
            block_store_inode_read(fs->BlockStore_inode, parent_inode_ID, parent_inode); // read inode number to parent inode
            if(parent_inode->fileType == 'd') { // check case where file and dir have same name
                block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);
                int j = 0;
                while (j < folder_number_entries){
                    if( ((parent_inode->vacantFile >> j) & 1) == 1 && strcmp((parent_data + j) -> filename, *(tokens + i)) == 0 ){
                        parent_inode_ID = (parent_data + j) -> inodeNumber;
                        dir_counter++;
                    }
                    j++;
                }
            }					
        }
        block_store_inode_read(fs->BlockStore_inode, parent_inode_ID, parent_inode);


        /* 
         *
         * FOLLOWING CODE IS TAKEN FROM JIMRS 'fs_create()' FUNCTION SOLUTION 
         *
         */

        // for deleting empty directories  
        if(dir_counter == token_count - 1) {
            inode_t*new_inode = calloc(1, sizeof(inode_t));
            if (new_inode == NULL){
                return -1; 
            }
            int curr_dir = 0;
            int temp_counter = 0;
            if (parent_inode->fileType == 'd'){
                // same file or dir name in the same path is intolerable
                int curr_pos = 0;
                while (curr_dir < folder_number_entries){
                    if( ((parent_inode->vacantFile >> curr_dir) & 1) == 1){
                        block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);
                        block_store_inode_read(fs->BlockStore_inode, (parent_data + curr_dir) -> inodeNumber, new_inode);
                        // if (strcmp((parent_data + curr_dir) -> filename, *(tokens + token_count - 1)) == 0)
                        curr_pos = strcmp((parent_data + curr_dir) -> filename, *(tokens + token_count - 1)) == 0;
                        // strcmp((parent_data + curr_dir) -> filename, *(tokens + token_count - 1)) == 0;
                        // block_store_inode_read(fs->BlockStore_inode, (parent_data + curr_dir) -> inodeNumber, new_inode);
                        for(size_t i = 0; i < folder_number_entries; i++){
                            if(((new_inode->vacantFile >> curr_dir)) != 0 && new_inode->fileType == 'd') {
                                temp_counter++;       
                            }
                        }
                        if( strcmp((parent_data + curr_dir) -> filename, *(tokens + token_count - 1)) == 0 ){
                            break;											
                        }
                    }
                    curr_dir++;
                }

                if (curr_pos == 1 && temp_counter == 0){
                    block_store_sub_release(fs->BlockStore_inode, (parent_data + curr_dir)->inodeNumber);
                    bitmap_t*temp_bm = bitmap_overlay(31,&(parent_inode->vacantFile));
                    bitmap_reset(temp_bm, curr_dir);

                    parent_inode->directPointer[curr_dir] = 0;
                    block_store_inode_write(fs->BlockStore_inode, parent_inode_ID, parent_inode);
                    memset((parent_data + curr_dir)->filename, 0, 127);
                    block_store_write(fs->BlockStore_whole, parent_inode->directPointer[curr_dir], parent_data);

                    free_tokens(tokens, token_count);
                    free(parent_inode);	
                    free(parent_data);
                    return 0;

                } 
                // if we are not currently at a directory 
                free_tokens(tokens, token_count);
                free(parent_inode);	
                free(parent_data);
                return -1;
            }
        }

        /* 
         *
         * FOLLOWING CODE IS TAKEN FROM JIMRS 'fs_create()' FUNCTION SOLUTION 
         *
         */
        if (dir_counter == token_count - 1 && parent_inode->fileType == 'd'){
            // same file or dir name in the same path is intolerable
            int k = 0;
            int curr_dir = 0;
            while (curr_dir < folder_number_entries){
                if ( ((parent_inode->vacantFile >> curr_dir) & 1) == 1){
                    if ( strcmp((parent_data + curr_dir) -> filename, *(tokens + token_count - 1)) == 0 ){
                        free(parent_inode);	
                        free(parent_data);
                        free_tokens(tokens, token_count);
                        return -1;   			
                    }
                    block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);
                    k = strcmp((parent_data + curr_dir) -> filename, *(tokens + token_count - 1)) == 0;
                }
                curr_dir++;
            }

            if (k == 1){ // if a file is found
                block_store_sub_release(fs->BlockStore_inode, (parent_data + curr_dir)->inodeNumber); // release the block at the current inode
                bitmap_t*temp_bm = bitmap_overlay(folder_number_entries, &(parent_inode->vacantFile));
                bitmap_reset(temp_bm, curr_dir);
                
                parent_inode->directPointer[curr_dir]=0;
                block_store_inode_write(fs->BlockStore_inode, parent_inode_ID, parent_inode);
    
                memset((parent_data+curr_dir)->filename, 0, 127);
                block_store_write(fs->BlockStore_whole, parent_inode->directPointer[curr_dir], parent_data);
                free_tokens(tokens, token_count);
                free(parent_inode);	
                free(parent_data);
                return 0;
            }
            free_tokens(tokens, token_count);
            free(parent_inode);	
            free(parent_data);
        }
    }
    return -1;
}

/** Moves the file from one location to the other
      Moving files does not affect open descriptors
    \param fs The FS containing the file
    \param src Absolute path of the file to move
    \param dst Absolute path to move the file to
    \return 0 on success, < 0 on error

    **************
    **** NOTE ****
    **************
    A LOT OF THIS CODE FOR THIS FUNCTION
    WAS TAKEN FROM PREVIOUS FUNCTIONS WHICH
     JIMR GAVE US SOLUTIONS TO 
     -> THUS A LOT IS COPY PASTED JIMR CODE
*/
int fs_move(FS_t *fs, const char *src, const char *dst) {
    if (fs != NULL && src != NULL && strlen(src) != 0 && dst != NULL && strlen(dst)!=0) {
        directoryFile_t * parent_data = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);
        if (parent_data == NULL){
            return -1;
        }
        size_t dir_counter = 0;
        
        inode_t * parent_inode = (inode_t *) calloc(1, sizeof(inode_t));	
        if (parent_inode == NULL){
            return -1; 
        }
        size_t parent_inode_ID = 0;	// start from the 1st inode, ie., the inode for root directory

        size_t count = 0;
        char**tokens;
        tokens = tokenize(src, &count);
        if(tokens == NULL) {
            free_tokens(tokens, count);
            return -1;
        }
        // let's check if the filenames are valid or not
        /* SEG FAULT WITHOUT CHECK */
        for(size_t n = 0; n < count; n++){	
            if(isValidFileName(tokens[n]) == false){
                free_tokens(tokens, n);
                return -1;
            }
        }

        char token_save[127];
        strcpy(token_save, tokens[count-1]);
    
        /* 
         *
         * FOLLOWING CODE IS TAKEN FROM JIMRS 'fs_create()' FUNCTION SOLUTION 
         *
         */
        for(size_t i = 0; i < count - 1; i++){
            block_store_inode_read(fs->BlockStore_inode, parent_inode_ID, parent_inode);
            if(parent_inode->fileType == 'd'){ // in case file and dir has the same name
                block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);
                int curr_dir = 0; 
                while (curr_dir < folder_number_entries){
                    if( ((parent_inode->vacantFile >> curr_dir) & 1) == 1 && strcmp((parent_data + curr_dir) -> filename, tokens[i]) == 0 ) {
                        parent_inode_ID = (parent_data + curr_dir) -> inodeNumber;
                        dir_counter++;
                    }
                    curr_dir++;
                }
            }					
        }
        block_store_inode_read(fs->BlockStore_inode, parent_inode_ID, parent_inode);

        
        int target_src, src_num;
        if (dir_counter == count - 1 && parent_inode->fileType == 'd') {
            int flag = 0;
            int curr_dir = 0;
            while (curr_dir < folder_number_entries) {
                if( ((parent_inode->vacantFile >> curr_dir) & 1) == 1) {
                    block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);
                    flag = strcmp((parent_data + curr_dir) -> filename, tokens[count - 1]) == 0;
                    if( strcmp((parent_data + curr_dir) -> filename, tokens[count - 1]) == 0) {
                        target_src = flag;
                        src_num = curr_dir;
                    }
                }
                curr_dir++;
            }
        }


        /* clear token values and tokenize dst string now */
        count = 0; 
        tokens = tokenize(dst, &count);
        if(tokens == NULL){
            return -1;
        }
        for(size_t n = 0; n < count; n++){	
            if(isValidFileName(tokens[n]) == false){
                free_tokens(tokens, n);
                return -1;
            }
        }

        
        
        directoryFile_t * dst_directory = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);
        if (dst_directory == NULL){
            return -1;
        }
        dir_counter = 0;

        inode_t * dst_inode = (inode_t *) calloc(1, sizeof(inode_t));	
        if (dst_inode == NULL){
            return -1;
        }
        size_t dst_inode_id = 0;	

        for(size_t i = 0; i < count - 1; i++) {
            block_store_inode_read(fs->BlockStore_inode, dst_inode_id, dst_inode);	// read out the parent inode
            if(dst_inode->fileType == 'd'){
                block_store_read(fs->BlockStore_whole, dst_inode->directPointer[0], dst_directory);
                int curr_dir = 0;
                while (curr_dir < folder_number_entries) {
                    if( ((dst_inode->vacantFile >> curr_dir) & 1) == 1 && strcmp((dst_directory + curr_dir) -> filename, tokens[i]) == 0 ) {
                        dst_inode_id = (dst_directory + curr_dir) -> inodeNumber;
                        dir_counter++;
                    }
                    curr_dir++;            
                }
            }					
        }
        
        // read out the parent inode
        block_store_inode_read(fs->BlockStore_inode, dst_inode_id, dst_inode);
        bool dst_flag;
        //int entryNumberDst;
        uint8_t inode_dst_stat;

        if(dir_counter == count - 1 && dst_inode->fileType == 'd') {
            // same file or dir name in the same path is intolerable

            bool temp_flag = false;
            int curr_dir = 0;
            while (curr_dir < folder_number_entries){
                if( ((dst_inode->vacantFile >> curr_dir) & 1) == 1){
                    block_store_read(fs->BlockStore_whole, dst_inode->directPointer[0], dst_directory);
                    temp_flag = strcmp((dst_directory + curr_dir) -> filename, *(tokens + count - 1)) == 0;
                    if(strcmp((dst_directory + curr_dir)->filename, token_save) == 0){
                        dst_flag = true;
                        break;
                    }
                    if( strcmp((dst_directory + curr_dir) -> filename, *(tokens + count - 1)) == 0 ){
                        dst_flag = temp_flag;
                        inode_dst_stat = (dst_directory + curr_dir)->inodeNumber;
                        break;
                    }
                }
                curr_dir++;
            } 
        } else {
            dst_flag = true;
        }
        
        

        if(dst_flag == false && target_src == true) {
            bitmap_t* src_store = bitmap_overlay(folder_number_entries, &(parent_inode->vacantFile));
            bitmap_reset(src_store, src_num);
            
            memset((parent_data + src_num)->filename,0,127);
            block_store_write(fs->BlockStore_whole,parent_inode->directPointer[0],parent_data);
            block_store_inode_write(fs->BlockStore_inode,parent_inode_ID,parent_inode);
           
            bitmap_t* dst_store =bitmap_overlay(31,&(dst_inode->vacantFile));
            size_t temp_ffz = bitmap_ffz(dst_store);
            bitmap_set(dst_store, temp_ffz);
            (dst_directory + temp_ffz)->inodeNumber = inode_dst_stat;
            strcpy((dst_directory + temp_ffz)->filename, tokens[count - 1]);
            block_store_write(fs->BlockStore_whole, dst_inode->directPointer[0], dst_directory);
            block_store_inode_write(fs->BlockStore_inode, dst_inode_id, dst_inode);

            free_tokens(tokens, count);
            free(parent_inode);	
            free(parent_data);
            return 0;
        }
        free_tokens(tokens, count);
        free(parent_inode);	
        free(parent_data);
    }
    return -1;
}


/** Link the dst with the src
     dst and src should be in the same File type, say, both are files or both are directories
    \param fs The F18FS containing the file
    \param src Absolute path of the source file
    \param dst Absolute path to link the source to
    \return 0 on success, < 0 on error
*/
int fs_link(FS_t *fs, const char *src, const char *dst){
    if (fs && src != NULL && strlen(src) > 0 && dst != NULL && strlen(dst) > 0){
        return 0;
    }
    return -1;
}



























