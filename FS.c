#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "FS.h"

#define NUMBER_OF_BLOCKS 64  // number of blocks
#define BLOCK_SIZE 512 // In byte
#define MAX_FILES 192
#define MAX_FILE_SIZE 1536 // In byte

#define BIT_MAP_SIZE 8 // In byte

unsigned char D[NUMBER_OF_BLOCKS][BLOCK_SIZE]; // Disk
unsigned char M[BLOCK_SIZE]; // main memory
struct OFT_entry OFT[4]; // open file table

struct File_descriptor FDT[MAX_FILES]; // file descriptor table
unsigned char bit_map[BIT_MAP_SIZE];

// Disk access functions
void read_block(int b, void* dst){
    memcpy(dst, &D[b], BLOCK_SIZE);
}

void write_block(int b, void* src){
    memcpy(src, &D[b], BLOCK_SIZE);
}

// Bit map functions
int read_bit_map(int position){
    if(position < 0 || position > 63)
        perror("Bit map access out of boundary!");

    int byte_offset = position / BIT_MAP_SIZE;
    int bit_offset = position % BIT_MAP_SIZE;

    unsigned char mask = 128;
    mask >>= bit_offset;

    return bit_map[byte_offset] & mask;
}

// Value can be 0 or 1, assigned to the bit map
void write_bit_map(int position, int value){
    if(position < 0 || position > 63)
        perror("Bit map access out of boundary!");
    
    int byte_offset = position / BIT_MAP_SIZE;
    int bit_offset = position % BIT_MAP_SIZE;

    unsigned char mask = 128;
    mask >>= bit_offset;
    if(value){ // set to 1
        bit_map[byte_offset] |= mask;
    } else{ // set to 0
        mask = ~mask;
        bit_map[byte_offset] &= mask;
    }
}

// return the position of an empty block, memset the block to 0 and 
// mark in bit map, return -1 if it's full
int get_empty_block(){
    int byte_offset;
    int bit_offset;
    int result;
    unsigned char empty_block[BLOCK_SIZE];

    memset(empty_block, 0, BLOCK_SIZE);

    for(byte_offset=0; byte_offset<BIT_MAP_SIZE; byte_offset++){
        unsigned char byte = bit_map[byte_offset];
        unsigned char mask = 128;
        for (bit_offset = 0; bit_offset < 8; bit_offset++){
            if(!(byte & mask)){
                result = byte_offset * 8 + bit_offset;
                write_bit_map(result, 1);
                write_block(result, empty_block);
                return result;
            }
            mask >>= 1;
        }
    }

    return -1;
}

// Initialization

void init(){
    int i;

    // Block from 0 to 7 allocated, 0 is bit map, 1 - 6 are file 
    // descriptors, 7 is the directory's first block
    for(i=0; i < 8; i++)
        write_bit_map(i, 1);

    // Create the directory file descriptor
    struct File_descriptor *dir;
    dir = &FDT[0];
    dir->size = 0;
    dir->block[0] = 7;
    
    // set OFT[0] to the directory
    OFT[0].fd = 0;
    OFT[0].size = dir->size;
    OFT[0].current_position = 0;

    // set OFT current positon to -1 to mark free
    for(i = 1; i < 4; i++)
        OFT[i].current_position = -1;
    
    // set FDT size to -1 to mark free
    for(i = 1; i < MAX_FILES; i++)
        FDT[i].size = -1;

}


// OFT functions
void load_buffer(int OFT_index){
    struct File_descriptor *fd;
    struct OFT_entry *ofte;
    int block_number;

    ofte = &OFT[OFT_index];
    fd = &FDT[ofte->fd];

    block_number = ofte->current_position / BLOCK_SIZE;

    if(block_number < 3)
        read_block(fd->block[block_number], ofte->buffer);
}

// directory functions

// return the file descriptor if the name exists,
// otherwise, return 0.
int exists(char* name){
    struct Directory_entry* dir_e;
    struct OFT_entry* ofte;
    struct File_descriptor *fd;
    int i;

    ofte = &OFT[0];
    fd = &FDT[0];

    // Loop on the directory
    for(i = 0; i * sizeof(struct Directory_entry) < fd->size; i++){
        seek(0, i * sizeof(struct Directory_entry));
        dir_e = &ofte->buffer[ofte->current_position % BLOCK_SIZE];

        if(strncmp(dir_e->name, name, 4) == 0)
            return dir_e->fd;  
    }

    return 0;
}

// User Interface
void seek(int i, int p){
    if (p < 0){
        perror("Current position can not be negative!\n");
        return;
    }

    if(p > MAX_FILE_SIZE){
        perror("Current position is past the end of file!\n");
        return;
    }

    int block_number;
    int previous_block_number;
    struct OFT_entry *ofte;
    struct File_descriptor *fd;

    ofte = &OFT[i];
    fd = &FDT[ofte->fd];
    block_number = p / BLOCK_SIZE;
    previous_block_number = ofte->current_position / BLOCK_SIZE;

    if(block_number != previous_block_number){
        // Need to change r/w buffer
        write_block(fd->block[previous_block_number], ofte->buffer);
        read_block(fd->block[block_number], ofte->buffer);
    }
        
    ofte->current_position = p;
}

void create(char* name){
    if(exists(name)){
        perror("Duplicate file name!\n");
        return;
    }

    int fd_index;
    struct File_descriptor *dir;
    struct Directory_entry *dir_e;
    struct OFT_entry *ofte;
    int i;
    int block_number;

    ofte = &OFT[0];
    dir = &FDT[0];

    if(dir->size == MAX_FILE_SIZE){
        perror("No free directory entry found!\n");
        return;
    }

    // find an empty fd
    for(i = 0; i < MAX_FILES; i++){
        if(FDT[i].size == -1){
            fd_index = i;
            break;
        }
    }
    if(i == MAX_FILES){
        perror("Too many files");
        return;
    }

    for(i = 0; i * sizeof(struct Directory_entry) < dir->size; i++){
        seek(0, i * sizeof(struct Directory_entry));
        dir_e = &ofte->buffer[ofte->current_position % BLOCK_SIZE];
        if(*(int*)(dir_e->name) == 0){
            // find an empty directory entry
            memcpy(dir_e->name, name, 4);
            dir_e->fd = fd_index;
            FDT[fd_index].size = 0; // allocate the fd for the new file
            return;
        }
    }

    // if the directory size need to increase


    // if need to allocate a new block for the directory
    if(dir->size % BLOCK_SIZE == 0){
        // If this block is full
        // allocate a new block for the directory
        block_number = dir->size / BLOCK_SIZE + 1;
        if(dir->block[block_number] = get_empty_block() == -1){
            perror("Disk full!\n");
            return;
        }
    }

    // set up the directory entry
    seek(0, dir->size);
    dir_e = &ofte->buffer[dir->size % BLOCK_SIZE];
    memcpy(dir_e->name, name, 4);
    dir_e->fd = fd_index;
    FDT[fd_index].size = 0;
    
     // set up directory size
    dir->size += 8;
    ofte->size += 8;
    
}

void destroy(char* name){
    struct Directory_entry* dir_e;
    struct File_descriptor* fd;
    int fd_index;
    int i;

    fd_index = exists(name);
    if(fd_index == -1){
        // name does not exits;
        perror("File does not exist!\n");
        return;
    }

    // find the directory entry that should be deleted
    dir_e = &OFT[0].buffer[OFT[0].current_position % BLOCK_SIZE];
    // find the file descriptor that should be deleted
    fd = &FDT[fd_index];

    // check if the file is opened.
    for(i = 0; i < 4; i++){
        if(OFT[i].fd = fd_index){
            perror("Can not destroy file because it is opened!\n");
            return;
        }
    }

    fd->size = -1; // free the fd
    for(i = 0; i < 3; i++){
        // if block number is nonzero
        if(fd->block[i]){
            write_bit_map(fd->block[i], 0);
            fd->block[i] = 0;
        }
    }

    // mark the directory entry as free
    memset(dir_e->name, 0, 4);
}


// return the OFT index of a opened file, return -1 if error
int open(char* name){
    struct OFT_entry* ofte;
    struct File_descriptor* fd;
    int fd_index;
    int i;
    int j;

    if(fd_index = exists(name) == -1){
        perror("File does not exist!\n");
        return -1;
    }    

    for(j = 0; j < 4; j++){
        if(OFT[j].size == -1){
            ofte = &OFT[j];
            break;
        }
    }
    if(j == 4){
        perror("Too many files open!\n");
        return -1;
    }

    fd = &FDT[fd_index];
    ofte->fd = fd_index;
    ofte->size = fd->size;
    ofte->current_position = 0;

    // allocate a new block if size is 0
    if (ofte->size == 0)
        fd->block[0] = get_empty_block();

    read_block(fd->block[0], ofte->buffer);
    
    return j;
}

int main(){
   
}