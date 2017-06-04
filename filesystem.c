#include "filesystem.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

/*Function used to create a new disc with given size and name */
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
  for(i=0; i<FILES_IN_DIR; ++i) {
    mainDirectory.files[i].flags=FREE;
  }

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
  int newBlock, i; /* newBlock - block of the new directory*/
  SuperBlock sBlock;
  FATTable table;
  Directory dir, newDir;
  int parentBlock, freeDir; /* parentBlock - block of parent directory, freeDir - index in the table of directory*/
  /* TODO check name length*/
  
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

  /*one block less will be empty */
  --sBlock.emptyBlocks;

  /*open table*/
  table.tab = (int16_t *) malloc(sBlock.nBlocks*sizeof(int16_t));
  if(fread(table.tab, sizeof(int16_t), sBlock.nBlocks, file) != sBlock.nBlocks) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  /*update fattable*/
  newBlock = find_first_block(table.tab, sBlock.nBlocks);
  table.tab[newBlock]=-1;

  /*find catalog*/

  if(save_in_dir(file, name, newBlock, sBlock.blocksOffset, BLOCK_SIZE, DIR, path) < 0)
    return -2; /*error*/
  
   newDir.nFiles = 0;
     for(i=0; i<FILES_IN_DIR; ++i) {
    newDir.files[i].flags=FREE;
  }

  rewind(file);

    /* write changes */
  if(fwrite(&sBlock, sizeof(SuperBlock), 1, file) != 1)
     return -1;
  if(fwrite(table.tab, sizeof(int16_t), sBlock.nBlocks, file) != sBlock.nBlocks)
     return -1;
  /* save new directory */
  fseek(file, (long) sBlock.blocksOffset + newBlock*BLOCK_SIZE, 0); 
  if(fwrite(&newDir, sizeof(Directory), 1, file) != 1)
     return -1;

  fclose(file);
  free(table.tab);

  return 0;
}



int add_file(char* discName, char* fromPath, char* toPath) {
  uint8_t buffer[BLOCK_SIZE];
  SuperBlock sBlock;
  FATTable table;
  FILE *input, *output;
  int size, block, pBlock, nBlocks, i, toRead; /* first free block in which the file shall be written */
  struct stat st;
  
  input = fopen(fromPath, "rb");
  if (input == NULL) {
    fprintf(stderr, "Failed to open file\n");
    return -1; /*failed to open file*/
  }

  output = fopen(discName, "rb+");
  if (output == NULL) {
    fprintf(stderr, "Failed to open file\n");
    return -1; /*failed to open file*/
  }

  /* get file size */
  stat(fromPath, &st);
  size = st.st_size;

  /* count nBlocks */
  nBlocks = size / BLOCK_SIZE;
  if((size % BLOCK_SIZE) != 0 )
    nBlocks++;

    /*open superblock*/
  if(fread(&sBlock, sizeof(SuperBlock), 1, output) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

   /*one block less will be empty */
   sBlock.emptyBlocks-=nBlocks;
  
   if(sBlock.emptyBlocks <  0) {
    fprintf(stderr, "Not enough free space\n");
    return -2;
  }


   /*open table*/
  table.tab = (int16_t *) malloc(sBlock.nBlocks*sizeof(int16_t));
  if(fread(table.tab, sizeof(int16_t), sBlock.nBlocks, output) != sBlock.nBlocks) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  /*first block*/
  pBlock = find_first_block(table.tab, sBlock.nBlocks);
  table.tab[pBlock] = -1;
  block = pBlock;

  /*find catalog*/
  if(save_in_dir(output, fromPath, pBlock, sBlock.blocksOffset, size, FIL, toPath) < 0)
    return -2; /*error*/

  /* find other blocks */
  for(i = 0; i < nBlocks; ++i) {
    toRead = (size/BLOCK_SIZE) ? BLOCK_SIZE : size;
    size-=toRead;
    fseek(output, sBlock.blocksOffset + BLOCK_SIZE * block, 0);
    fread(buffer, 1, toRead, input);
    fwrite(buffer, 1, toRead, output);
    
    block = find_first_block(table.tab, sBlock.nBlocks);
    table.tab[block] = -1;
    table.tab[pBlock] = block;
    pBlock = block;
  }

    rewind(output);

    /* write changes */
  if(fwrite(&sBlock, sizeof(SuperBlock), 1, output) != 1)
     return -1;
  if(fwrite(table.tab, sizeof(int16_t), sBlock.nBlocks, output) != sBlock.nBlocks)
     return -1;

  free(table.tab);
  fclose(input);
  fclose(output);

  return 0;
}

int remove_file(char* discName, char* path, char* name) {
  Directory dir;
  SuperBlock sBlock;
  FATTable table;
  FILE *file;
  int size, dirBlock, index, block, pBlock, nBlocks, i; /* first free block in which the file shall be written */
  struct stat st;

  file = fopen(discName, "rb+");
  if (file == NULL) {
    fprintf(stderr, "Failed to open file\n");
    return -1; /*failed to open file*/
  }

  /*open superblock*/
  if(fread(&sBlock, sizeof(SuperBlock), 1, file) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

     /*open table*/
  table.tab = (int16_t *) malloc(sBlock.nBlocks*sizeof(int16_t));
  if(fread(table.tab, sizeof(int16_t), sBlock.nBlocks, file) != sBlock.nBlocks) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

    /*find catalog*/
  fseek(file, (long) sBlock.blocksOffset, 0);
  /* read first catalog */
  if(fread(&dir, sizeof(Directory), 1, file) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  /* recursive search through directories */
   if(strcmp(path, "/" ) == 0)
     dirBlock = 0;
  else	  /* recursive search through directories */
     dirBlock = find_dir(&dir, file, path, sBlock.blocksOffset);
     
  if ( dirBlock == -1) {
    fprintf(stderr, "Path not found\n");
    return -1;
  }

  /* find file in directory */
  if((index = find_in_dir(&dir, name)) < 0) {
    fprintf(stderr, "File not found\n");
    return -1;
   }

  if(dir.files[index].flags == DIR) {
    fprintf(stderr, "Not a file\n");
    return -1;
  }
  
  /* remove file from dir */
  block = dir.files[index].blockNumber;
  dir.files[index].flags = FREE;


  nBlocks=0; 

  do {
    printf("block: %d\n", block);
    pBlock = block;
    block = table.tab[block];
    table.tab[pBlock] = -2;
    ++nBlocks;
   } while (block != -1);
  

  /*blocks got free  */
   sBlock.emptyBlocks+=nBlocks;

    rewind(file);

    /* write changes */
  if(fwrite(&sBlock, sizeof(SuperBlock), 1, file) != 1)
     return -1;
  if(fwrite(table.tab, sizeof(int16_t), sBlock.nBlocks, file) != sBlock.nBlocks)
     return -1;

  fseek(file, sBlock.blocksOffset + dirBlock * BLOCK_SIZE, 0);
  fwrite(&dir, sizeof(Directory), 1, file);

  

  free(table.tab);
  fclose(file);


  return 0;

}


int get_file(char* discName, char* path, char* name) {
  uint8_t buffer[BLOCK_SIZE];
  Directory dir;
  SuperBlock sBlock;
  FATTable table;
  FILE *file, *output;
  int size, dirBlock, index, block, toRead; /* first free block in which the file shall be written */
  char outName[24] = "Result_"; 
  strcat(outName, name);

  /*create new file*/
  output = fopen(outName, "wb+");
  if (output == NULL) {
    fprintf(stderr, "Failed to open file\n");
    return -1; /*failed to open file*/
  }


  file = fopen(discName, "rb");
  if (file == NULL) {
    fprintf(stderr, "Failed to open file\n");
    return -1; /*failed to open file*/
  }

  /*open superblock*/
  if(fread(&sBlock, sizeof(SuperBlock), 1, file) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

     /*open table*/
  table.tab = (int16_t *) malloc(sBlock.nBlocks*sizeof(int16_t));
  if(fread(table.tab, sizeof(int16_t), sBlock.nBlocks, file) != sBlock.nBlocks) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

    /*find catalog*/
  fseek(file, (long) sBlock.blocksOffset, 0);
  /* read first catalog */
  if(fread(&dir, sizeof(Directory), 1, file) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  /* recursive search through directories */
   if(strcmp(path, "/" ) == 0)
     dirBlock = 0;
  else	  /* recursive search through directories */
     dirBlock = find_dir(&dir, file, path, sBlock.blocksOffset);
     
  if ( dirBlock == -1) {
    fprintf(stderr, "Path not found\n");
    return -1;
  }

  /* find file in directory */
  if((index = find_in_dir(&dir, name)) < 0) {
    fprintf(stderr, "File not found\n");
    return -1;
   }

  if(dir.files[index].flags == DIR) {
    fprintf(stderr, "Not a file\n");
    return -1;
  }

  block = dir.files[index].blockNumber;
  size = dir.files[index].fileSize;

  /* read and write data */

  do {
    toRead = (size/BLOCK_SIZE) ? BLOCK_SIZE : size;
    size-=toRead;
    fseek(file, sBlock.blocksOffset + BLOCK_SIZE * block, 0);
    fread(buffer, 1, toRead, file);
    fwrite(buffer, 1, toRead, output);
    block = table.tab[block];
  } while (block != -1);
  


  free(table.tab);
  fclose(file);
  fclose(output);


  return 0;

}
  
int show_directory(char* diskName, char* path) {
  FILE* file;
  int freeBlock, i;
  SuperBlock sBlock;
  Directory dir;
  int dirBlock, freeDir;
  /* TODO check name length*/
  
  file = fopen(diskName, "rb");
  if (file == NULL) {
    fprintf(stderr, "Failed to open file\n");
    return -1; /*failed to open file*/
  }

  /*open superblock*/
  if(fread(&sBlock, sizeof(SuperBlock), 1, file) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  /*find catalog*/
  fseek(file, (long) sBlock.blocksOffset, 0);
  /* read first catalog */
  if(fread(&dir, sizeof(Directory), 1, file) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  /* recursive search through directories */
   if(strcmp(path, "/" ) == 0)
     dirBlock = 0;
  else	  /* recursive search through directories */
     dirBlock = find_dir(&dir, file, path, sBlock.blocksOffset);
     
  if ( dirBlock == -1) {
    fprintf(stderr, "Path not found\n");
    return -1;
  }

  printf("Directory %s :\n", path); 
  for(i=0; i<FILES_IN_DIR; i++) {
    if(OCCUPIED(dir.files[i].flags))
      printf("%s\t", dir.files[i].fileName);
  }
  printf("\n");

  fclose(file);

  return 0;
}


int stats(char* diskName, char* path, char* name) {
   FILE* file;
   int fileIndex;
  SuperBlock sBlock;
  Directory dir;
  int dirBlock;
  /* TODO check name length*/
  
  file = fopen(diskName, "rb");
  if (file == NULL) {
    fprintf(stderr, "Failed to open file\n");
    return -1; /*failed to open file*/
  }

  /*open superblock*/
  if(fread(&sBlock, sizeof(SuperBlock), 1, file) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  /*find catalog*/
  fseek(file, (long) sBlock.blocksOffset, 0);
  /* read first catalog */
  if(fread(&dir, sizeof(Directory), 1, file) != 1) {
     fprintf(stderr, "Failed to read data\n");
     return -1;
  }

  /* recursive search through directories */
   if(strcmp(path, "/" ) == 0)
     dirBlock = 0;
  else	  /* recursive search through directories */
     dirBlock = find_dir(&dir, file, path, sBlock.blocksOffset);
     
  if ( dirBlock == -1) {
    fprintf(stderr, "Path not found\n");
    return -1;
  }

  if(( fileIndex = find_in_dir(&dir, name) ) == -1 ) {
    fprintf(stderr, "File or directory does not exist\n");
    return -1;
  }

  printf("Type: \t%s", (dir.files[fileIndex].flags & DIR) ? "Directory\n" : "File\n");
  printf("Name: \t%s\n", dir.files[fileIndex].fileName);
  printf("Size: \t%d\n", dir.files[fileIndex].fileSize);
  printf("Created: \t%s\n", ctime(&dir.files[fileIndex].created));
	 fclose(file);
	 return 0;
}
  
  

  
  
