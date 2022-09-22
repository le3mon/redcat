#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SET_INPUT 0x1;
#define SET_OUTPUT 0x2;
#define SET_ENCRYPTION 0x4;
#define SET_DECRYPTION 0x8;

char *alog_list[] = {"md5", "sha-1", "sha-256"};


bool is_algorithm(char *algo) {
    for (int i = 0; i < (sizeof(alog_list)/sizeof(char*)); i++) {
        if(strcmp(algo, alog_list[i]) == 0) {
            return true;
        }
    }
    return false;
}

void help() {
    printf("-i      Input file/directory path\n");
    printf("-o      Output path\n");
    printf("-a      Algorithm used for encryption ex:[md5, ase, ...]\n");
    printf("-d      Decryption\n");
    printf("-h      Help\n");
}

int main(int argc, char *argv[]) {
    char opt;
    char *input, *output, *algo = NULL;
    int flag = 0;
    while ( -1 != (opt = getopt(argc, argv, "i:o:a:d:h:"))) {
        switch (opt) {
        case 'i':
            input = (char*)malloc(strlen(optarg)+1);
            flag |= SET_INPUT;
            break;
        case 'o':
            output = (char*)malloc(strlen(optarg)+1);
            flag |= SET_OUTPUT;
            break;
        case 'a':
            algo = (char*)malloc(strlen(optarg)+1);
            if(!is_algorithm(algo)) {
                printf("wrong algorithm\n");
                return -1;
            }
            flag |= SET_ENCRYPTION;
            break;
        case 'd':
            flag |= SET_DECRYPTION;
            break;
        case 'h':
            help();
            break;
        default:
            printf("wrong option\n");
            break;
        }   
    }
    
    
    return 0;
}