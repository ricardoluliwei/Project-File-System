CC = gcc
CFLAGS = -g
.PHONY: all clean

FS: FS.c FS.h
	$(CC) $(CFLAGS) FS.c -o FS

all: FS
	@echo Done!

clean:
	rm -rf FS core*