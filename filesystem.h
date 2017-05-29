#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <time.h>
#include <stdint.h>

#define BLOCK_SIZE 1024
#define NAME_LENGTH 20
#define FILES_IN_DIR 31
#define MAX_BLOCKS 255
#define FIL 0x1
#define DIR 0x2
#define FREE 0
#define OCCUPIED(b) (0x3 & b)

typedef struct superBlock {
  char name[5];
  uint16_t size, blockSize, nBlocks, emptyBlocks, fatOffset, blocksOffset;
} SuperBlock;

typedef struct block {
  uint8_t data[BLOCK_SIZE];
} Block;

typedef struct fatTable {
  int16_t *tab;
} FATTable;

typedef struct attributes {
  uint8_t flags; /* file or dir or free */
  uint16_t fileSize;
  uint8_t blockNumber;
  char fileName[NAME_LENGTH];
  time_t created;
} Attributes;

typedef struct directory {
  uint16_t nFiles;
  Attributes files[FILES_IN_DIR];
} Directory;

int create_disc(uint16_t size, char* name);
int show_fat(char* discName);
int make_directory(char* discName, char* path, char* name);
int add_file(char* discName, char* fromPath, char* toPath);
int get_file(char* discName, char* fromPath);
int show_directory(char* discName, char* path);
int remove_file(char* discName, char* path);
int stats(char* discName, char* path);
int remove_disc(char* discName);

int find_first_block(int16_t* tab, uint16_t nBlocks);

#endif
