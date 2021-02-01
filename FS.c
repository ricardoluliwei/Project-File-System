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

// return the position of an empty block, return -1 if it's full
int get_empty_block(){
    int byte_offset;
    int bit_offset;
    for(byte_offset=0; byte_offset<BIT_MAP_SIZE; byte_offset++){
        unsigned char byte = bit_map[byte_offset];
        unsigned char mask = 128;
        for (bit_offset = 0; bit_offset < 8; bit_offset++){
            if(!(byte & mask))
                return (byte_offset * 8 + bit_offset);
            mask >>= 1;
        }
    }

    return -1;
}

// Initialization

void init(){
    int i;

    // Block from 0 to 7 allocated, 0 is bit map, 1 - 6 are file 
    // descriptors, 7 is the directory
    for(int i=0; i < 8; i++)
        write_bit_map(i, 1);

    
    // Create the directory file descriptor
    struct File_descriptor *directory;
    directory = FDT;
    directory->size = 0;
    directory->b0 = 7;
    
}



int main(){
    printf("%d", get_empty_block());
}