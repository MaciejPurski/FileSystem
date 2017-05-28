#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <time.h>
#include <stdint.h>

#define BLOCK_SIZE 1024
#define NAME_LENGTH 21
#define FILES_IN_DIR 31
#define MAX_BLOCKS 255

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
  uint16_t fileSize;
  uint8_t blockNumber;
  char fileName[NAME_LENGTH];
  time_t created;
} Attributes;

typedef struct directory {
  uint16_t nFiles;
  Attributes files[FILES_IN_DIR];
} Directory;

int make_directory(char* path, char* name);
int create_disc(uint16_t size, char* name);
int show_fat();

#endif
