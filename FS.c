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
unsigned char bit_map[BLOCK_SIZE];

// Disk access functions
void read_block(int b, void* dst){
    memcpy(dst, &D[b], BLOCK_SIZE);
}

void write_block(int b, void* src){
    memcpy(&D[b], src, BLOCK_SIZE);
}

// Bit map functions
int read_bit_map(int position){
    if(position < 0 || position > 63)
        printf("error\n");

    int byte_offset = position / BIT_MAP_SIZE;
    int bit_offset = position % BIT_MAP_SIZE;

    unsigned char mask = 128;
    mask >>= bit_offset;

    return bit_map[byte_offset] & mask;
}

// Value can be 0 or 1, assigned to the bit map
void write_bit_map(int position, int value){
    if(position < 0 || position > 63)
        printf("error\n");
    
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

    memset(D, 0, BLOCK_SIZE * NUMBER_OF_BLOCKS);
    memset(M, 0, BLOCK_SIZE);
    memset(FDT, 0, MAX_FILES * sizeof(struct File_descriptor));
    memset(bit_map, 0, BLOCK_SIZE);
    memset(OFT, 0, 4 * sizeof(struct OFT_entry));
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
    
    printf("system initialized\n");
}

// memory access functions
void read_memory(int m, int n){
    if(m + n > BLOCK_SIZE){
        n = BLOCK_SIZE - m;
    }

    char buf[n + 1];
    memcpy(buf, &M[m], n);
    buf[n] = 0;
    printf("%s\n", buf);
}

void write_memory(int m, char* s){
    int n;
    for(n = 0; s[n]; n++);

    if(m + n > BLOCK_SIZE){
        n = BLOCK_SIZE - m;
    }

    memcpy(&M[m], s, n);
    
    printf("%d bytes written to M\n", n);
}

void save(char* f){
    FILE *fp;
    void *block_pointer;
    int i;

    fp = fopen(f, "w"); 
    block_pointer = &FDT[0];
    // wirte first 7 blocks into D
    write_block(0, bit_map);

    for (i = 1; i < 7; i++){
        write_block(i, block_pointer);
        block_pointer += BLOCK_SIZE;
    }
    
    fwrite(D, BLOCK_SIZE, NUMBER_OF_BLOCKS, fp);
    fclose(fp);
}

void restore(char* f){
    FILE *fp;
    void *block_pointer;
    int i;
    struct File_descriptor *dir;

    fp = fopen(f, "r");
    memset(D, 0, BLOCK_SIZE * NUMBER_OF_BLOCKS);
    memset(M, 0, BLOCK_SIZE);
    memset(FDT, 0, MAX_FILES * sizeof(struct File_descriptor));
    memset(bit_map, 0, BLOCK_SIZE);
    memset(OFT, 0, 4 * sizeof(struct OFT_entry));

    fread(D, BLOCK_SIZE, NUMBER_OF_BLOCKS, fp); 
    block_pointer = &FDT[0];

    read_block(0, bit_map);

    for (i = 1; i < 7; i++){
        read_block(i, block_pointer);
        block_pointer += BLOCK_SIZE;
    }
    
    // restore directory
    dir = &FDT[0];
    // set OFT[0] to the directory
    OFT[0].fd = 0;
    OFT[0].size = dir->size;
    OFT[0].current_position = 0;

    // set OFT current positon to -1 to mark free
    for(i = 1; i < 4; i++)
        OFT[i].current_position = -1;
    
    fclose(fp);
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
        dir_e = (struct Directory_entry*)&ofte->buffer[ofte->current_position % BLOCK_SIZE];

        if(strncmp(dir_e->name, name, 4) == 0)
            return dir_e->fd;  
    }

    return 0;
}


// User Interface
int seek(int i, int p){
    if (p < 0){
        printf("error\n");
        return 0;
    }

    int block_number;
    int previous_block_number;
    struct OFT_entry *ofte;
    struct File_descriptor *fd;

    ofte = &OFT[i];
    fd = &FDT[ofte->fd];
    block_number = p / BLOCK_SIZE;
    previous_block_number = ofte->current_position / BLOCK_SIZE;

    if(p > ofte->size){
        printf("error\n");
        return 0;
    }

    if(block_number >= 3){
        write_block(fd->block[previous_block_number], ofte->buffer);
    } else if(block_number != previous_block_number){
        // Need to change r/w buffer
        write_block(fd->block[previous_block_number], ofte->buffer);
        read_block(fd->block[block_number], ofte->buffer);
    }
        
    ofte->current_position = p;

    return 1;
}

void seek_with_message(int i, int p){
    if(seek(i, p)){
        printf("position is %d\n", p);
    }
}

void create(char* name){
    if(exists(name)){
        printf("error\n");
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
        printf("error\n");
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
        printf("error\n");
        return;
    }

    for(i = 0; i * sizeof(struct Directory_entry) < dir->size; i++){
        seek(0, i * sizeof(struct Directory_entry));
        dir_e = (struct Directory_entry*)&ofte->buffer[ofte->current_position % BLOCK_SIZE];
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
    if((dir->size + 8) / BLOCK_SIZE != dir->size / BLOCK_SIZE){
        // If this block is full
        // allocate a new block for the directory
        block_number = dir->size / BLOCK_SIZE + 1;
        if((dir->block[block_number] = get_empty_block()) == -1){
            printf("error\n");
            return;
        }
    }

    // set up the directory entry
    seek(0, dir->size);
    dir_e = (struct Directory_entry*)&ofte->buffer[dir->size % BLOCK_SIZE];
    memcpy(dir_e->name, name, 4);
    dir_e->fd = fd_index;
    FDT[fd_index].size = 0;
    
     // set up directory size
    dir->size += 8;
    ofte->size += 8;
    
    printf("%s created\n", name);
}

void destroy(char* name){
    struct Directory_entry* dir_e;
    struct File_descriptor* fd;
    int fd_index;
    int i;

    fd_index = exists(name);
    if(fd_index == -1){
        // name does not exits;
        printf("error\n");
        return;
    }

    // find the directory entry that should be deleted
    dir_e = (struct Directory_entry*)&OFT[0].buffer[OFT[0].current_position % BLOCK_SIZE];
    // find the file descriptor that should be deleted
    fd = &FDT[fd_index];

    // check if the file is opened.
    for(i = 0; i < 4; i++){
        if(OFT[i].fd == fd_index){
            printf("error\n");
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

    printf("%s destroyed\n", name);
}


// return the OFT index of a opened file, return -1 if error
int fs_open(char* name){
    struct OFT_entry* ofte;
    struct File_descriptor* fd;
    int fd_index;
    int i;
    int j;

    if((fd_index = exists(name)) == -1){
        printf("error\n");
        return -1;
    }

    for(j = 0; j < 4; j++){
        if(OFT[j].current_position == -1){
            ofte = &OFT[j];
            break;
        }
    }
    if(j == 4){
        printf("error\n");
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

    printf("%s opened %d\n", name, j);
    return j;
}

void fs_close(int i){
    if(i < 1 || i > 3){
        printf("error\n");
        return;
    }

    struct File_descriptor* fd;
    struct OFT_entry* ofte;

    ofte = &OFT[i];
    fd = &FDT[ofte->fd];

    write_block(fd->block[ofte->current_position / BLOCK_SIZE], ofte->buffer);
    memset(ofte->buffer, 0, BLOCK_SIZE);
    fd->size = ofte->size;
    ofte->current_position = -1;
    ofte->fd = 0;
    ofte->size = 0;

    printf("%d closed\n", i);
    return;
}

void fs_read(int i, int m, int n){
    if(i < 1 || i > 3){
        printf("error\n");
        return;
    }

    if(m + n > BLOCK_SIZE){
        printf("error\n");
        return;
    }

    int byte_copied; // how many bytes already copied
    int bytes; // how many bytes need to be copied in one loop
    int current_block;
    struct File_descriptor* fd;
    struct OFT_entry* ofte;

    ofte = &OFT[i];
    fd = &FDT[ofte->fd];
    
    seek(ofte->fd, ofte->current_position);
    byte_copied = 0;

    // if n is larger than the end of file, read the whole file.
    if(ofte->current_position + n > ofte->size)
        n = ofte->size - ofte->current_position;

    while (byte_copied < n){ 
        bytes = 0;
        current_block = ofte->current_position / BLOCK_SIZE;
        
        if(current_block != 
        (ofte->current_position + n - byte_copied) / BLOCK_SIZE){
            // if we need to copy next block
            bytes = BLOCK_SIZE - (ofte->current_position % BLOCK_SIZE);
        } else{
            // if we don't need to copy next block
            bytes = n - byte_copied;
        }

        // copy
        memcpy(&M[m], &(ofte->buffer[ofte->current_position % BLOCK_SIZE]), bytes);
        seek(ofte->fd, ofte->current_position + bytes);
        byte_copied += bytes;
        m += bytes;
    }
    
    printf("%d bytes read from %d\n", byte_copied, i);
}

void fs_write(int i, int m, int n){
    if(i < 1 || i > 3){
        printf("error\n");
        return;
    }

    int byte_copied; // how many bytes already copied
    int bytes; // how many bytes need to be copied in one loop
    int current_block;
    struct File_descriptor* fd;
    struct OFT_entry* ofte;

    ofte = &OFT[i];
    fd = &FDT[ofte->fd];
    
    seek(ofte->fd, ofte->current_position);
    byte_copied = 0;

    // if current position + n is larger than the
    // max file size, write until max size reached.
    if(ofte->current_position + n > MAX_FILE_SIZE)
        n = MAX_FILE_SIZE - ofte->current_position;

    // if m + n is larger than 512,
    // write until the memory ends/
    if(m + n > BLOCK_SIZE)
        n = BLOCK_SIZE - m;
    
    while (byte_copied < n){ 
        bytes = 0;
        current_block = ofte->current_position / BLOCK_SIZE;
        
        if(current_block != 
        (ofte->current_position + n - byte_copied) / BLOCK_SIZE){
            // if we need to copy next block
            bytes = BLOCK_SIZE - (ofte->current_position % BLOCK_SIZE);
            // check if the next block exists
            if(!fd->block[current_block + 1]){
                fd->block[current_block + 1] = get_empty_block();
            }
        } else{
            // if we don't need to copy next block
            bytes = n - byte_copied;
        }
        // change the file size if need
        if(ofte->current_position + bytes > ofte->size){
            ofte->size = ofte->current_position + bytes;
            fd->size = ofte->size;
        }
        // copy
        memcpy(&(ofte->buffer[ofte->current_position % BLOCK_SIZE]), &M[m], bytes);
        seek(ofte->fd, ofte->current_position + bytes);
        byte_copied += bytes;
        m += bytes;
    }

    printf("%d bytes written to %d\n", byte_copied, i);
}

void directory(){
    struct File_descriptor* fd;
    struct OFT_entry* ofte;
    struct Directory_entry* dir_e;

    ofte = &OFT[0];
    seek(0, 0);

    while (ofte->current_position < ofte->size){
        dir_e = (struct Directory_entry*)&ofte->buffer[ofte->current_position % BLOCK_SIZE];
        if(*(int *)(dir_e->name)){
            fd = &FDT[dir_e->fd];
            printf("%s %d ", dir_e->name, fd->size);
        }
        seek(0, ofte->current_position + sizeof(struct Directory_entry));
    }
    printf("\n");
}


// presentation shell
int main(){
    char buffer[BLOCK_SIZE];
    char* token;
    char* spliter = " \n";
    while (1){
        if(!fgets(buffer, BLOCK_SIZE, stdin)){
            break;
        }
        
        token = strtok(buffer, spliter);
        if(!token){
            printf("\n");
            continue;
        }
        if(strcmp(token, "cr") == 0){
            token = strtok(NULL, spliter);
            create(token);
            continue;
        }
        if(strcmp(token, "de") == 0){
            token = strtok(NULL, spliter);
            destroy(token);
            continue;
        }
        if(strcmp(token, "op") == 0){
            token = strtok(NULL, spliter);
            fs_open(token);
            continue;
        }
        if(strcmp(token, "cl") == 0){
            token = strtok(NULL, spliter);
            int i = atoi(token);
            fs_close(i);
            continue;
        }
        if(strcmp(token, "rd") == 0){
            int i, m, n;
            token = strtok(NULL, spliter);
            i = atoi(token);
            token = strtok(NULL, spliter);
            m = atoi(token);
            token = strtok(NULL, spliter);
            n = atoi(token);
            fs_read(i, m, n);
            continue;
        }
        if(strcmp(token, "wr") == 0){
            int i, m, n;
            token = strtok(NULL, spliter);
            i = atoi(token);
            token = strtok(NULL, spliter);
            m = atoi(token);
            token = strtok(NULL, spliter);
            n = atoi(token);
            fs_write(i, m, n);
            continue;
        }
        if(strcmp(token, "sk") == 0){
            int i, p;
            token = strtok(NULL, spliter);
            i = atoi(token);
            token = strtok(NULL, spliter);
            p = atoi(token);
            seek_with_message(i, p);
            continue;
        }
        if(strcmp(token, "dr") == 0){
            directory();
            continue;
        }
        if(strcmp(token, "in") == 0){
            init();
        }
        if(strcmp(token, "rm") == 0){
            int m, n;
            token = strtok(NULL, spliter);
            m = atoi(token);
            token = strtok(NULL, spliter);
            n = atoi(token);
            read_memory(m, n);
            continue;
        }
        if(strcmp(token, "wm") == 0){
            int m;
            char* s;
            token = strtok(NULL, spliter);
            m = atoi(token);
            s = strtok(NULL, spliter);
            write_memory(m, s);
            continue;
        }
        if(strcmp(token, "sv") == 0){
            char* f;
            f = strtok(NULL, spliter);
            save(f);
            continue;
        }
        if(strcmp(token, "ld") == 0){
            char* f;
            f = strtok(NULL, spliter);
            restore(f);
            continue;
        }
        if(strcmp(token, "quit") == 0){
            break;
        }
    }
   
}
