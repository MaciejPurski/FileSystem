#include "filesystem.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int create_disc(uint16_t size, char* name) {
  FILE *file;
  SuperBlock sBlock;
  FATTable table;
  int i;
  Directory mainDirectory;
  Block block;

  /*Initialization of superblock*/
  strcpy(sBlock.name, "MYFAT");
  sBlock.size = size;
  sBlock.blockSize = BLOCK_SIZE;
  sBlock.nBlocks = (size - sizeof(SuperBlock)) /
    (sizeof(int16_t) + BLOCK_SIZE);
  printf("NBlocks: %d\n", sBlock.nBlocks);

  if (sBlock.nBlocks > MAX_BLOCKS) /*too many blocks*/
    return -2;
  
  sBlock.emptyBlocks = sBlock.nBlocks - 1; /* need 1 block for main directory */
  
  sBlock.fatOffset = sizeof(SuperBlock);
  sBlock.blocksOffset = sizeof(SuperBlock) + sBlock.nBlocks*sizeof(uint16_t);

  /*FAT table initialization*/
  table.tab = malloc(sBlock.nBlocks*sizeof(int16_t));
  table.tab[0] = -1; /* first block is occupied by the main directory*/

  for(i=1; i<sBlock.nBlocks; i++)
    table.tab[i] = -2; /*set all the blocks empty*/

  file = fopen(name, "wb+");
  if (file == NULL)
    return -1; /*failed to create file*/
 
  /* Initialize main directory */
  mainDirectory.nFiles = 0;
  for(i=0; i<FILE_IN_DIR; ++i) {
    mainDirectory.files[i].flags=FREE;

  /*Writing data*/
  
  if(fwrite(&sBlock, sizeof(SuperBlock), 1, file) != 1)
     return -1;
  if(fwrite(table.tab, sizeof(int16_t), sBlock.nBlocks, file) != sBlock.nBlocks)
     return -1;
  if(fwrite(&mainDirectory, sizeof(Directory), 1, file) != 1)
     return -1;


  for(i=1; i<sBlock.nBlocks; i++) {
    if(fwrite(&block, sizeof(Block), 1, file) != 1)
     return -1;
  }

  free(table.tab);
  fclose(file);

  return 0;
}


int show_fat(char* diskName) {
  SuperBlock sBlock;
  FATTable table;
  FILE *file;
  int i;

  file = fopen(diskName, "rb");
  if (file == NULL) {
    fprintf(stderr, "Failed to open file\n");
    return -1; /*failed to open file*/
  }

  if(fread(&sBlock, sizeof(SuperBlock), 1, file) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  table.tab = (int16_t *) malloc(sBlock.nBlocks*sizeof(int16_t));
  if(fread(table.tab, sizeof(int16_t), sBlock.nBlocks, file) != sBlock.nBlocks) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  printf("Blocks occupied %d/%d. FATTable: \n", sBlock.nBlocks-sBlock.emptyBlocks, sBlock.nBlocks);

  printf("\nBlock nr \tNext block\n");
  for(i=0; i<sBlock.nBlocks; ++i)
    printf("%3d %20d\n", i, table.tab[i]);

  free(table.tab);
  fclose(file);

  return 0;

}

int make_directory(char* diskName, char* path, char* name) {
  FILE* file;
  int freeBlock;
  SuperBlock sBlock;
  FATTable table;
  Directory main, newDir;
  int levels;
  int freeDir;
  
  file = fopen(diskName, "rb+");
  if (file == NULL) {
    fprintf(stderr, "Failed to open file\n");
    return -1; /*failed to open file*/
  }

  /*open superblock*/
  if(fread(&sBlock, sizeof(SuperBlock), 1, file) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  if(sBlock.emptyBlocks == 0) {
    fprintf(stderr, "Not enough free space\n");
    return -2;
  }

  
    
  /*open table*/
  table.tab = (int16_t *) malloc(sBlock.nBlocks*sizeof(int16_t));
  if(fread(table.tab, sizeof(int16_t), sBlock.nBlocks, file) != sBlock.nBlocks) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

    /*update fattable*/
  freeBlock = find_first_block(table.tab, superBlock.nBlocks);
  table.tab[freeBlock]=-1;

  /*find catalog*/
  fseek(file, (long) sBlock.blocksOffset, 0);
  if(fread(main, sizeof(Directory), 1, file) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  levels = get_levels(path);
  if(levels == 1) {
    if(find_in_dir(&main, name) != -1) {
     fprintf(stderr, "Directory or file with that name already exists in the directory\n");
     return -2;
    }      
    
    freeDir = find_free_dir(&main);
    if (freeDir == -1) {
     fprintf(stderr, "Not enough space in directory\n");
     return -2;
    }

    main.files[freeDir].flags = DIR;
    strcpy(main.files[freeDir].name, name);
    main.files[freeDir].created = time(NULL);
    main.files[freeDir].fileSize = 1024;
    main.files[freeDir].blockNumber = freeBlock;
    

    

  
  



}

inline
int find_first_block(int16_t* tab, uint16_t nBlocks) {
  int i;
  for(i=0; i<nBlocks; ++i) {
    if(tab[i]==-2)
      return i;
  }

  return -1;
}


inline int get_levels(char* path) {
  int result = 0;
  char tmp = path[0];
  while(tmp != '\0') {
    if (tmp == '/')
      result++;
    path++;
  }

  return result;
}

inline int find_free_dir(Directory* dir) {
  int i;
  if(dir->nFiles == FILES_IN_DIR) /*no more files*/
     return -1;
  for(i=0; i<FILES_IN_DIR; ++i) {
    if(FREE(dir->files[i].flags))
     return i;
   }
   return -1;
}

 inline int find_in_dir(Directory* dir, char* name) {
  int i;
  if(dir->nFiles == 0) /*no files*/
     return -1;
  for(i=0; i<FILES_IN_DIR; ++i) {
    if(strcmp(name, dir->files[i].name) == 0)
     return i;
   }
  return -1; /*not found*/

}
