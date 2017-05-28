#include "filesystem.h"
#include <string.h>
#include <stdlib.h>

int create_disc(uint16_t size, char* name) {
  SuperBlock sBlock;
  FATTable table;
  int i;
  FILE* file;
  Directory mainDirectory;
  Block block;

  /*Initialization of superblock*/
  strcpy(sBlock.name, "MYFAT");
  sBlock.size = size;
  sBlock.blockSize = BLOCK_SIZE;
  sBlock.nBlocks = (size - sizeof(SuperBlock) - sizeof(FATTable)) /
    (sizeof(int16_t) + BLOCK_SIZE);

  if (sBlock.nBlocks > MAX_BLOCKS) /*too many blocks*/
    return -2;
  
  sBlock.emptyBlocks = sBlock.nBlocks - 1; /* need 1 block for main directory */
  
  sBlock.fatOffset = sizeof(SuperBlock);
  sBlock.blocksOffset = sizeof(SuperBlock) + sizeof(FATTable) + sBlock.nBlocks*sizeof(uint16_t);

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

  /*Writing data*/
  
  if(fwrite(&sBlock, sizeof(SuperBlock), 1, file) != 1)
     return -1;
  if(fwrite(&table, sizeof(FATTable), 1, file) != 1)
     return -1;
  if(fwrite(&mainDirectory, sizeof(Directory), 1, file) != 1)
     return -1;


  for(i=1; i<sBlock.nBlocks; i++) {
    if(fwrite(&block, sizeof(Block), 1, file) != 1)
     return -1;
  }

  fclose(file);

  return 0;
}


int show_fat() {
  SuperBlock sBlock;
  FATTable table;
  FILE* file;
  int i;

  file = fopen("Mydisk", "rb");
  if (file == NULL)
    return -1; /*failed to open file*/

  if(fread(&sBlock, sizeof(SuperBlock), 1, file) != 1)
     return -1;
  if(fread(&table, sizeof(FATTable), 1, file) != 1)
     return -1;  

  printf("Blocks occupied %d/%d. FATTable: \n", sBlock.nBlocks-sBlock.emptyBlocks, sBlock.nBlocks);

  printf("\nBlock nr     Next block\n");
  for(i=0; i<sBlock.nBlocks; i++)
    printf("%d     %d\n", i, table.tab[i]);

  fclose(file);

  return 0;


}
