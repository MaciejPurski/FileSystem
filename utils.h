#ifndef UTIL_H
#define UTIL_H

#include "filesystem.h"
#include <stdint.h>


/* Function finds first free block in FATTable */
int find_first_block(int16_t *tab, uint16_t nBlocks);

/* returns index of a free entry in directory table or -1 if there is no*/
int find_free_dir(Directory *dir);

/* returns index of entry of a given name in directory or -1 if there is no*/
int16_t find_in_dir(Directory *dir, char *name);

/*Function returns block number of directory indicated in "path" variable and changes dir to suite that directory*/
int find_dir(Directory *dir, FILE *file, char *path, uint16_t blockOffset);

/*Function finds directory of given path and saves information in the directory*/
int save_in_dir(FILE *file, char *name, int new_block, int blocks_offset, int size, uint8_t type, char *path);

/*function opens file and reads superblock and fat table. Returns -1 on failure */
int initialize(FILE *file, SuperBlock *s_block, FATTable *table, char *disk_name, char *mode);


#endif

