#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int find_first_block(int16_t *tab, uint16_t nBlocks) {
    int i;
    for (i = 0; i < nBlocks; ++i) {
        if (tab[i] == -2)
            return i;
    }

    return -1;
}

int find_free_dir(Directory *dir) {
    int i;
    if (dir->n_entries == FILES_IN_DIR) /*no more files*/
        return -1;
    for (i = 0; i < FILES_IN_DIR; ++i) {
        if (!OCCUPIED(dir->entries[i].flags))
            return i;
    }
    return -1;
}

int16_t find_in_dir(Directory *dir, char *name) {
    int i;
    if (dir->n_entries == 0) { /*no files*/
        return -1;
    }
    for (i = 0; i < FILES_IN_DIR; ++i) {
        if (OCCUPIED(dir->entries[i].flags) && (strcmp(name, dir->entries[i].name) == 0))
            return i;
    }
    return -1; /*not found*/
}


/*Function returns block number of corresponding directory and changes dir to found directory*/

int find_dir(Directory *dir, FILE *file, char *path, uint16_t blockOffset) {
    char temp[20];
    char *stringEnd;
    int16_t index; /* index in directory table */
    uint8_t block;

    /*root directory*/

    ++path;
    stringEnd = path;

    while ((*stringEnd) != '/' && (*stringEnd) != '\0') {
        ++stringEnd;
    }


    strncpy(temp, path, stringEnd - path);
    temp[stringEnd - path] = '\0';


    index = find_in_dir(dir, temp);
    if (index == -1) /*not found*/
        return -1;

    if ((dir->entries[index].flags & DIR) == 0) /* not a directory*/
        return -1;

    block = dir->entries[index].block_number;
    fseek(file, (long) blockOffset + block * BLOCK_SIZE, 0);
    fread(dir, sizeof(Directory), 1, file); /* load new directory */


    if ((*stringEnd) == '\0' || (*(stringEnd + 1)) == '\0')  /* reached end - stop recursion */
        return block;

    return find_dir(dir, file, stringEnd, blockOffset);
}


int save_in_dir(FILE *file, char *name, int new_block, int blocks_offset, int size, uint8_t type, char *path) {

    Directory dir;
    int parentBlock, freeDir;

    fseek(file, (long) blocks_offset, 0);
    /* read first catalog */
    if (fread(&dir, sizeof(Directory), 1, file) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    /* get block number of the parent directory */
    parentBlock = (strcmp(path, "/") == 0) ? 0 : find_dir(&dir, file, path, blocks_offset);

    if (parentBlock == -1) {
        fprintf(stderr, "Path not found\n");
        return -1;
    }

    /* cannot create directory/file  - not a unique name*/
    if (find_in_dir(&dir, name) != -1) {
        fprintf(stderr, "Directory or file with that name already exists in the directory\n");
        return -2;
    }

    freeDir = find_free_dir(&dir);
    if (freeDir == -1) {
        fprintf(stderr, "Not enough space in directory\n");
        return -2;
    }

    dir.entries[freeDir].flags = type;
    strcpy(dir.entries[freeDir].name, name);
    dir.entries[freeDir].created = time(NULL);
    dir.entries[freeDir].size = size;
    dir.entries[freeDir].block_number = new_block;
    dir.n_entries++;

    /* save changed directory */
    fseek(file, (long) blocks_offset + parentBlock * BLOCK_SIZE, 0);
    if (fwrite(&dir, sizeof(Directory), 1, file) != 1)
        return -1;

    return 0;
}


int initialize(FILE *file, SuperBlock *s_block, FATTable *table, char *disk_name, char *mode) {
    file = fopen(disk_name, mode);
    if (file == NULL) {
        fprintf(stderr, "Failed to open file\n");
        return -1; /*failed to open file*/
    }

    if (fread(s_block, sizeof(SuperBlock), 1, file) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    table->tab = (int16_t *) malloc(s_block->n_blocks * sizeof(int16_t));
    if (fread(table->tab, sizeof(int16_t), s_block->n_blocks, file) != s_block->n_blocks) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    return 0;
}
