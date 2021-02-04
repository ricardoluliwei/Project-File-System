#ifndef FS_H
#define FS_H

#define NUMBER_OF_BLOCKS 64  // number of blocks
#define BLOCK_SIZE 512 // In byte
#define MAX_FILES 192

#define BIT_MAP_SIZE 8 // In byte


// User Interface
int seek(int i, int p);
void seek_with_message(int i, int p);
void create(char* name);
void destroy(char* name);
int fs_open(char* name);
void fs_close(int i);
void fs_read(int i, int m, int n);
void fs_write(int i, int m, int n);
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

// OFT functions
void load_buffer(int OFT_index);

// directory functions
int exists(char* name);


// memory access functions
void read_memory(int m, int n);
void write_memory(int m, char* s);

void save(char* f);
void restore(char* f);

struct File_descriptor{
    int size; // 
    int block[3];
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
    int size;
    int fd;
};

#endif
