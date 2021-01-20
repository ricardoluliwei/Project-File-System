#ifndef FS_H
#define FS_H


void create();
void destroy();
int open();
void close();
void read();
void write();
void seek();
void directory();

struct File_descriptor{
    int size;
    int b0;
    int b1;
    int b2;
};

struct File_entry{
    char name[4];
    int fd;
};

#endif
