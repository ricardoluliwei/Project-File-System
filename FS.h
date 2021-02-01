#ifndef FS_H
#define FS_H

#define NUMBER_OF_BLOCKS 1000  // number of blocks
#define BLOCK_SIZE 512
#define MAX_FILES 10


// User Interface
void create();
void destroy();
int open();
void close();
void read();
void write();
void seek();
void directory();

// Disk access functions
void read_block(int b);
void write_block(int b);

// Initialization
void init();

struct File_descriptor{
    int size; // 
    int b0;
    int b1;
    int b2;
};

// entry in the directory file
struct Directory_entry{
    char name[4];
    int fd;
};

// entry in the open file table
struct OFT_entry{
    unsigned char buffer[BLOCK_SIZE];
    int current_position; // current_position in the file from 0 to 1536
    int file_size;
    int fd;
};

#endif
