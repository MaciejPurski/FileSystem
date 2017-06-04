#include "utils.h"
#include <stdio.h>
#include <string.h>


int find_first_block(int16_t* tab, uint16_t nBlocks) {
  int i;
  for(i=0; i<nBlocks; ++i) {
    if(tab[i]==-2)
      return i;
  }

  return -1;
}

int find_free_dir(Directory* dir) {
  int i;
  if(dir->nFiles == FILES_IN_DIR) /*no more files*/
     return -1;
  for(i=0; i<FILES_IN_DIR; ++i) {
    if(!OCCUPIED(dir->files[i].flags))
     return i;
   }
   return -1;
}

int16_t find_in_dir(Directory* dir, char* name) {
  int i;
  if(dir->nFiles == 0) { /*no files*/
     return -1;
  }
  for(i=0; i<FILES_IN_DIR; ++i) {
    if(OCCUPIED(dir->files[i].flags) && (strcmp(name, dir->files[i].fileName) == 0))
     return i;
   }
  return -1; /*not found*/
}


 /*Function returns block number of corresponding directory and changes dir to found directory*/

int find_dir(Directory* dir, FILE* file, char* path, uint16_t blockOffset) {
  char temp[20];
  char* stringEnd;
  int16_t index; /* index in directory table */
  uint8_t block;

  /*root directory*/
  
  ++path;
  stringEnd = path;

  while((*stringEnd) != '/' && (*stringEnd) != '\0'){
      ++stringEnd;
    }


    strncpy(temp, path, stringEnd - path);
    temp[stringEnd - path] = '\0';


    index = find_in_dir(dir, temp);
    if(index == -1) /*not found*/
      return -1;

    if((dir->files[index].flags & DIR) == 0) /* not a directory*/
      return -1;

    block = dir->files[index].blockNumber;
    fseek(file, (long) blockOffset + block*BLOCK_SIZE, 0);
    fread(dir, sizeof(Directory), 1, file); /* load new directory */
    
    
    if( (*stringEnd) == '\0'|| (* (stringEnd+1)) == '\0')  /* reached end - stop recursion */
      return block;

    return find_dir(dir, file, stringEnd, blockOffset);
}


int save_in_dir(FILE *file, char *name, int newBlock, int blocksOffset, int size, uint8_t type, char *path) {

  Directory dir;
  int parentBlock, freeDir;
  
  fseek(file, (long) blocksOffset, 0);
  /* read first catalog */
  if(fread(&dir, sizeof(Directory), 1, file) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  /* get block number of the parent directory */
  parentBlock = (strcmp(path, "/") == 0) ? 0 : find_dir(&dir, file, path, blocksOffset);

  if ( parentBlock == -1) {
    fprintf(stderr, "Path not found\n");
    return -1;
  }

  /* cannot create directory/file  - not a unique name*/
  if(find_in_dir(&dir, name) != -1) {
    fprintf(stderr, "Directory or file with that name already exists in the directory\n");
    return -2;
   }      
    
   freeDir = find_free_dir(&dir);
   if (freeDir == -1) {
    fprintf(stderr, "Not enough space in directory\n");
    return -2;
   }

   dir.files[freeDir].flags = type;
    strcpy(dir.files[freeDir].fileName, name);
    dir.files[freeDir].created = time(NULL);
    dir.files[freeDir].fileSize = size;
    dir.files[freeDir].blockNumber = newBlock;
    dir.nFiles++;

      /* save changed directory */
  fseek(file, (long) blocksOffset + parentBlock*BLOCK_SIZE, 0); 
  if(fwrite(&dir, sizeof(Directory), 1, file) != 1)
     return -1;

    return 0;

}
