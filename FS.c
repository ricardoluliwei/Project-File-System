#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "FS.h"

#define NUMBER_OF_BLOCKS 1000  // number of blocks
#define BLOCK_SIZE 512
#define MAX_FILES 10

unsigned char D[NUMBER_OF_BLOCKS][BLOCK_SIZE];

