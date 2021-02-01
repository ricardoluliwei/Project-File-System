#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "FS.h"

#define NUMBER_OF_BLOCKS 64  // number of blocks
#define BLOCK_SIZE 512
#define MAX_FILES 192

// Disk
unsigned char D[NUMBER_OF_BLOCKS][BLOCK_SIZE];

unsigned char I[BLOCK_SIZE]; // Input buffer
unsigned char O[BLOCK_SIZE]; // Output buffer
unsigned char M[BLOCK_SIZE]; // main memory
struct OFT_entry OFT[BLOCK_SIZE]; // open file table

