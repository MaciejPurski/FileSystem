#include "filesystem.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>


int create_disc(uint16_t size, char *name) {
    FILE *file;
    SuperBlock s_block;
    FATTable table;
    int i;
    Directory main_directory;
    Block block;

    /*Initialization of superblock*/
    strcpy(s_block.name, "MYFAT");
    s_block.size = size;
    s_block.block_size = BLOCK_SIZE;
    s_block.n_blocks = (size - sizeof(SuperBlock)) /
                       (sizeof(int16_t) + BLOCK_SIZE);

    if (s_block.n_blocks > MAX_BLOCKS) /*too many blocks*/
        return -2;

    s_block.empty_blocks = s_block.n_blocks - 1; /* need 1 block for main directory */

    s_block.fat_offset = sizeof(SuperBlock);
    s_block.blocks_offset = sizeof(SuperBlock) + s_block.n_blocks * sizeof(uint16_t);

    /*FAT table initialization*/
    table.tab = malloc(s_block.n_blocks * sizeof(int16_t));
    table.tab[0] = -1; /* first block is occupied by the main directory*/

    for (i = 1; i < s_block.n_blocks; i++)
        table.tab[i] = -2; /*set all the blocks empty*/

    file = fopen(name, "wb+");
    if (file == NULL)
        return -1; /*failed to create file*/

    /* Initialize main directory */
    main_directory.n_entries = 0;
    for (i = 0; i < FILES_IN_DIR; ++i) {
        main_directory.entries[i].flags = FREE;
    }

    /*Writing data*/

    if (fwrite(&s_block, sizeof(SuperBlock), 1, file) != 1)
        return -1;
    if (fwrite(table.tab, sizeof(int16_t), s_block.n_blocks, file) != s_block.n_blocks)
        return -1;
    if (fwrite(&main_directory, sizeof(Directory), 1, file) != 1)
        return -1;


    for (i = 1; i < s_block.n_blocks; i++) {
        if (fwrite(&block, sizeof(Block), 1, file) != 1)
            return -1;
    }

    free(table.tab);
    fclose(file);

    return 0;
}


int show_fat(char *disk_name) {
    SuperBlock s_block;
    FATTable table;
    FILE *file;
    int i;

    /* read filesystem structures */
    file = fopen(disk_name, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file\n");
        return -1; /*failed to open file*/
    }

    if (fread(&s_block, sizeof(SuperBlock), 1, file) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    table.tab = (int16_t *) malloc(s_block.n_blocks * sizeof(int16_t));
    if (fread(table.tab, sizeof(int16_t), s_block.n_blocks, file) != s_block.n_blocks) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }


    printf("Blocks occupied %d/%d. FATTable: \n", s_block.n_blocks - s_block.empty_blocks, s_block.n_blocks);
    printf("\nBlock nr \tNext block\n");
    for (i = 0; i < s_block.n_blocks; ++i)
        printf("%3d %20d\n", i, table.tab[i]);

    free(table.tab);
    fclose(file);

    return 0;
}

int make_directory(char *disk_name, char *path, char *name) {
    FILE *file;
    int new_block, i; /* new_block - block of the new directory*/
    SuperBlock s_block;
    FATTable table;
    Directory new_dir;

    /* TODO check name length*/
    /* read data structures */
    file = fopen(disk_name, "rb+");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file\n");
        return -1; /*failed to open file*/
    }

    if (fread(&s_block, sizeof(SuperBlock), 1, file) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    table.tab = (int16_t *) malloc(s_block.n_blocks * sizeof(int16_t));
    if (fread(table.tab, sizeof(int16_t), s_block.n_blocks, file) != s_block.n_blocks) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }


    if (s_block.empty_blocks == 0) {
        fprintf(stderr, "Not enough free space\n");
        return -2;
    }

    /*one block less will be empty */
    --s_block.empty_blocks;

    /*update fat table*/
    new_block = find_first_block(table.tab, s_block.n_blocks);
    table.tab[new_block] = -1;

    /*find parent directory and update it*/
    if (save_in_dir(file, name, new_block, s_block.blocks_offset, BLOCK_SIZE, DIR, path) < 0)
        return -2; /*error*/

    /*initialize new directory */
    new_dir.n_entries = 0;
    for (i = 0; i < FILES_IN_DIR; ++i) {
        new_dir.entries[i].flags = FREE;
    }

    rewind(file);

    /* write changes in super block and fat table*/
    if (fwrite(&s_block, sizeof(SuperBlock), 1, file) != 1)
        return -1;
    if (fwrite(table.tab, sizeof(int16_t), s_block.n_blocks, file) != s_block.n_blocks)
        return -1;
    /* save new directory */
    fseek(file, (long) s_block.blocks_offset + new_block * BLOCK_SIZE, 0);
    if (fwrite(&new_dir, sizeof(Directory), 1, file) != 1)
        return -1;

    fclose(file);
    free(table.tab);

    return 0;
}


int add_file(char *disc_name, char *toPath, char *fromPath) {
    uint8_t buffer[BLOCK_SIZE];
    SuperBlock s_block;
    FATTable table;
    FILE *input, *output;
    int size, block, p_block, n_blocks, i, to_read; /* first free block in which the file shall be written */
    struct stat st;

    input = fopen(fromPath, "rb");
    if (input == NULL) {
      fprintf(stderr, "Failed to open input file %s\n", fromPath);
        return -1; /*failed to open file*/
    }

    output = fopen(disc_name, "rb+");
    if (output == NULL) {
        fprintf(stderr, "Failed to open disc file\n");
        return -1; /*failed to open file*/
    }

    if (fread(&s_block, sizeof(SuperBlock), 1, output) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    table.tab = (int16_t *) malloc(s_block.n_blocks * sizeof(int16_t));
    if (fread(table.tab, sizeof(int16_t), s_block.n_blocks, output) != s_block.n_blocks) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    /* get file size */
    stat(fromPath, &st);
    size = st.st_size;

    /* count n_blocks */
    n_blocks = size / BLOCK_SIZE;
    if ((size % BLOCK_SIZE) != 0)
        n_blocks++;



    if (s_block.empty_blocks < n_blocks) {
        fprintf(stderr, "Not enough free space\n");
        return -2;
    }

    
    /*change number of empty blocks */
    s_block.empty_blocks -= n_blocks;

    /*first block*/
    block = find_first_block(table.tab, s_block.n_blocks);

    /*find directory*/
    if (save_in_dir(output, fromPath, block, s_block.blocks_offset, size, FIL, toPath) < 0)
        return -2; /*error*/

    /* find other blocks and save data*/
    for (i = 0; i < n_blocks; ++i) {
      
        table.tab[block] = -1;
        to_read = (size / BLOCK_SIZE) ? BLOCK_SIZE : size;
        size -= to_read;
        fseek(output, s_block.blocks_offset + BLOCK_SIZE * block, 0);
        fread(buffer, 1, to_read, input);
        fwrite(buffer, 1, to_read, output);
	if (i != 0)
	  table.tab[p_block] = block;

     	p_block = block;
	block = find_first_block(table.tab, s_block.n_blocks);
    }

    

    rewind(output);

    /* write changes */
    if (fwrite(&s_block, sizeof(SuperBlock), 1, output) != 1)
        return -1;
    if (fwrite(table.tab, sizeof(int16_t), s_block.n_blocks, output) != s_block.n_blocks)
        return -1;

    free(table.tab);
    fclose(input);
    fclose(output);

    return 0;
}

int remove_file(char *disc_name, char *path, char *name) {
    Directory dir;
    SuperBlock s_block;
    FATTable table;
    FILE *file;
    int size, dir_block, index, block, p_block, n_blocks, i; /* first free block in which the file shall be written */
    struct stat st;

    file = fopen(disc_name, "rb+");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file\n");
        return -1; /*failed to open file*/
    }

    if (fread(&s_block, sizeof(SuperBlock), 1, file) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    table.tab = (int16_t *) malloc(s_block.n_blocks * sizeof(int16_t));
    if (fread(table.tab, sizeof(int16_t), s_block.n_blocks, file) != s_block.n_blocks) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    /*find catalog*/
    fseek(file, (long) s_block.blocks_offset, 0);
    /* read first catalog */
    if (fread(&dir, sizeof(Directory), 1, file) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    /* recursive search through directories */
    if (strcmp(path, "/") == 0)
        dir_block = 0;
    else
        dir_block = find_dir(&dir, file, path, s_block.blocks_offset);

    if (dir_block == -1) {
        fprintf(stderr, "Path not found\n");
        return -1;
    }

    /* find file in directory */
    if ((index = find_in_dir(&dir, name)) < 0) {
        fprintf(stderr, "File not found\n");
        return -1;
    }

    if (dir.entries[index].flags == DIR) {
        fprintf(stderr, "Not a file\n");
        return -1;
    }

    /* remove file from dir */
    block = dir.entries[index].block_number;
    dir.entries[index].flags = FREE;


    n_blocks = 0;
    do {
        p_block = block;
        block = table.tab[block];
        table.tab[p_block] = -2;
        ++n_blocks;
    } while (block != -1);


    /*blocks got free  */
    s_block.empty_blocks += n_blocks;

    rewind(file);

    /* write changes */
    if (fwrite(&s_block, sizeof(SuperBlock), 1, file) != 1)
        return -1;
    if (fwrite(table.tab, sizeof(int16_t), s_block.n_blocks, file) != s_block.n_blocks)
        return -1;

    fseek(file, s_block.blocks_offset + dir_block * BLOCK_SIZE, 0);
    fwrite(&dir, sizeof(Directory), 1, file);


    free(table.tab);
    fclose(file);


    return 0;
}


int get_file(char *disc_name, char *path, char *name) {
    uint8_t buffer[BLOCK_SIZE];
    Directory dir;
    SuperBlock s_block;
    FATTable table;
    FILE *file, *output;
    int size, dir_block, index, block, toRead; /* first free block in which the file shall be written */
    char outName[24] = "Result_";
    strcat(outName, name);

    /*create new file*/
    output = fopen(outName, "wb+");
    if (output == NULL) {
        fprintf(stderr, "Failed to open file\n");
        return -1; /*failed to open file*/
    }


    file = fopen(disc_name, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file\n");
        return -1; /*failed to open file*/
    }

    if (fread(&s_block, sizeof(SuperBlock), 1, file) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    table.tab = (int16_t *) malloc(s_block.n_blocks * sizeof(int16_t));
    if (fread(table.tab, sizeof(int16_t), s_block.n_blocks, file) != s_block.n_blocks) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    /*find catalog*/
    fseek(file, (long) s_block.blocks_offset, 0);
    /* read first catalog */
    if (fread(&dir, sizeof(Directory), 1, file) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    /* recursive search through directories */
    if (strcmp(path, "/") == 0)
        dir_block = 0;
    else      /* recursive search through directories */
        dir_block = find_dir(&dir, file, path, s_block.blocks_offset);

    if (dir_block == -1) {
        fprintf(stderr, "Path not found\n");
        return -1;
    }

    /* find file in directory */
    if ((index = find_in_dir(&dir, name)) < 0) {
        fprintf(stderr, "File not found\n");
        return -1;
    }

    if (dir.entries[index].flags == DIR) {
        fprintf(stderr, "Not a file\n");
        return -1;
    }

    block = dir.entries[index].block_number;
    size = dir.entries[index].size;

    /* read and write data */

    do {
        toRead = (size / BLOCK_SIZE) ? BLOCK_SIZE : size;
        size -= toRead;
        fseek(file, s_block.blocks_offset + BLOCK_SIZE * block, 0);
        fread(buffer, 1, toRead, file);
        fwrite(buffer, 1, toRead, output);
        block = table.tab[block];
    } while (block != -1);


    free(table.tab);
    fclose(file);
    fclose(output);


    return 0;

}

int show_directory(char *diskName, char *path) {
    FILE *file;
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
    if (fread(&sBlock, sizeof(SuperBlock), 1, file) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    /*find catalog*/
    fseek(file, (long) sBlock.blocks_offset, 0);
    /* read first catalog */
    if (fread(&dir, sizeof(Directory), 1, file) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    /* recursive search through directories */
    if (strcmp(path, "/") == 0)
        dirBlock = 0;
    else      /* recursive search through directories */
        dirBlock = find_dir(&dir, file, path, sBlock.blocks_offset);

    if (dirBlock == -1) {
        fprintf(stderr, "Path not found\n");
        return -1;
    }

    printf("Directory %s :\n", path);
    for (i = 0; i < FILES_IN_DIR; i++) {
        if (OCCUPIED(dir.entries[i].flags))
            printf("%s\t", dir.entries[i].name);
    }
    printf("\n");

    fclose(file);

    return 0;
}


int stats(char *diskName, char *path, char *name) {
    FILE *file;
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
    if (fread(&sBlock, sizeof(SuperBlock), 1, file) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    /*find catalog*/
    fseek(file, (long) sBlock.blocks_offset, 0);
    /* read first catalog */
    if (fread(&dir, sizeof(Directory), 1, file) != 1) {
        fprintf(stderr, "Failed to read data\n");
        return -1;
    }

    /* recursive search through directories */
    if (strcmp(path, "/") == 0)
        dirBlock = 0;
    else      /* recursive search through directories */
        dirBlock = find_dir(&dir, file, path, sBlock.blocks_offset);

    if (dirBlock == -1) {
        fprintf(stderr, "Path not found\n");
        return -1;
    }

    if ((fileIndex = find_in_dir(&dir, name)) == -1) {
        fprintf(stderr, "File or directory does not exist\n");
        return -1;
    }

    printf("Type: \t%s", (dir.entries[fileIndex].flags & DIR) ? "Directory\n" : "File\n");
    printf("Name: \t%s\n", dir.entries[fileIndex].name);
    printf("Size: \t%d\n", dir.entries[fileIndex].size);
    printf("Created: \t%s\n", ctime(&dir.entries[fileIndex].created));
    fclose(file);
    return 0;
}
  
  

  
  
