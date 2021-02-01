#ifndef FS_H
#define FS_H

#define NUMBER_OF_BLOCKS 64  // number of blocks
#define BLOCK_SIZE 512 // In byte
#define MAX_FILES 192

#define BIT_MAP_SIZE 8 // In byte


// User Interface
void create(char* name);
void destroy();
int open();
void close();
void read();
void write();
void seek();
void directory();

// Disk access functions
void read_block(int b, void* dst);
void write_block(int b, void* src);

// Initialization
void init();

// Bit map functions
int read_bit_map(int position);
void write_bit_map(int position, int value);
int get_empty_block();

// memory access functions
void read_memory();
void write_memory();

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
