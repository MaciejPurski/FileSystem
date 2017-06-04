#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "filesystem.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Too few arguments\n");
        return -1;
    }

    if (strcmp(argv[1], "create") == 0) {
        if (argc != 4) {
            printf("Invalid list of arguments for command 'create'. Use: create <disc_size> <disc_name> \n");
            return -1;
        }
        create_disc(atoi(argv[2]), argv[3]);
    } else if (strcmp(argv[1], "mkdir") == 0) {
        if (argc != 5) {
            return -1;
        }
        make_directory(argv[2], argv[3], argv[4]);
    } else if (strcmp(argv[1], "add") == 0) {
        if (argc != 5) {
            printf("Invalid list of arguments for command 'add'. Use: add <disc_name> <external_file_name> <disc_path> \n");
            return -1;
        }
        add_file(argv[2], argv[3], argv[4]);
    } else if (strcmp(argv[1], "get") == 0) {
        if (argc != 5) {
            printf("Invalid list of arguments for command 'get'. Use: get <disc_name> <disc_path> <file_name> \n");
            return -1;
        }
        get_file(argv[2], argv[3], argv[4]);
    } else if (strcmp(argv[1], "rm") == 0) {
        if (argc != 5) {
            printf("Invalid list of arguments for command 'rm'. Use: get <disc_name> <disc_path> <file_name> \n");
            return -1;
        }
        remove_file(argv[2], argv[3], argv[4]);
    } else if (strcmp(argv[1], "stat") == 0) {
        if (argc != 5) {
            printf("Invalid list of arguments for command 'stat'. Use: get <disc_name> <disc_path> <file_name> \n");
            return -1;
        }
        stats(argv[2], argv[3], argv[4]);
    } else if (strcmp(argv[1], "ls") == 0) {
        if (argc != 4) {
            printf("Invalid list of arguments for command 'ls'. Use: get <disc_name> <disc_path> \n");
            return -1;
        }
        show_directory(argv[2], argv[3]);
    } else if (strcmp(argv[1], "fat") == 0) {
        if (argc != 3) {
            printf("Invalid list of arguments for command 'fat'. Use: get <disc_name> \n");
            return -1;
        }
        show_fat(argv[2]);
    } else
        printf("Unknown command \n");

    return 0;
}
