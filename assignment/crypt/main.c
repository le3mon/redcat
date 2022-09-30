#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <dirent.h>
#include <sys/stat.h>
// #include <errno.h>

#define SET_INPUT 0x1
#define SET_OUTPUT 0x2
#define SET_ENCRYPTION 0x4
#define SET_DECRYPTION 0x8
#define SET_DIGEST 0x10
#define SET_CIPHER 0x20

char *digest_list[] = {"MD5", "SHA1", "SHA256", "SHA512"};
char *cipher_list[] = {"AES-128-CBC", "AES-192-CBC", "AES-256-CBC", "DES-CBC"};

void help() {
    printf("-i [path]           Input file/directory path\n");
    printf("-o [path]           Output path\n");
    printf("-e [algorithm]      Encryption ex:[md5, ase, ...]\n");
    printf("-d [algorithm]      Decryption\n");
    printf("-h                  Help\n");
    printf("\nalgorithm list :");
    for (int i = 0; i < 4; i++)
        printf(" \'%s\' \'%s\'", digest_list[i], cipher_list[i]);
}

bool is_directory(char *path) {
    DIR *dp = NULL;
    if(!(dp = opendir(path))) {
        return false;
    }
    closedir(dp);
    return true;
}

int check_algorithm(int *flag, char *algo) {
    for (int i = 0; i < 4; i++) {
        if(strcmp(algo, digest_list[i]) == 0) {
            *flag |= SET_DIGEST;
            return 0;
        }
        
        if(strcmp(algo, cipher_list[i]) == 0) {
            *flag |= SET_CIPHER;
            return 0;
        }
    }
    return -1;
}

int check_arg(int *flag, char *algo) {
    if(!(*flag)) {
        help();
        return -1;
    }
    if(!(*flag & SET_INPUT)) {
        printf("set input file/directory\n\n");
        return -1;
    }
    if(!(*flag & SET_OUTPUT)) {
        printf("set output file/directory\n\n");
        return -1;
    }
    if(!((*flag & SET_ENCRYPTION) | (*flag & SET_DECRYPTION))) {
        printf("set encryption or decryption\n\n");
        return -1;
    }
    if(check_algorithm(flag, algo)) {
        printf("wrong algorithm\n");
        return -1;
    }
    return 0;
}

int digest(FILE *in, FILE *out, char *algo) {
    const EVP_MD *digest;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned char buf[BUFSIZ];
    int buf_len;
    unsigned char dig_buf[EVP_MAX_MD_SIZE];
    unsigned int dig_len;
    
    OpenSSL_add_all_digests();
    
    EVP_MD_CTX_init(ctx);
    digest = EVP_get_digestbyname(algo);
    EVP_DigestInit_ex(ctx, digest, NULL);

    while((buf_len = fread(buf, 1, sizeof(buf), in)) > 0) {
        if(!EVP_DigestUpdate(ctx, buf, buf_len)) {
            printf("error\n");
            EVP_MD_CTX_free(ctx);
            return -1;
        }
    }
    EVP_DigestFinal_ex(ctx, dig_buf, &dig_len);
    fwrite(dig_buf, 1, dig_len, out);
    EVP_MD_CTX_free(ctx);
    return 0;
}

int cipher(FILE *in, FILE *out, char *algo) {
    const EVP_CIPHER *cipher;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    unsigned char key[EVP_MAX_KEY_LENGTH] = {0, };
    unsigned char iv[EVP_MAX_IV_LENGTH] = {0, };
    
    OpenSSL_add_all_ciphers();

    EVP_CIPHER_CTX_init(ctx);
    cipher = EVP_get_cipherbyname(algo);
    memset(key, 0x41, EVP_CIPHER_key_length(cipher));
    memset(iv, 0x42, EVP_CIPHER_iv_length(cipher));

    EVP_EncryptInit_ex(ctx, cipher, NULL, key, iv);
    
    unsigned char *buf = (unsigned char*)malloc(BUFSIZ);
    unsigned char *cip_buf = (unsigned char*)malloc(BUFSIZ+EVP_MAX_BLOCK_LENGTH);
    int buf_len, cip_len = 0;
    
    while((buf_len = fread(buf, 1, sizeof(buf), in)) > 0) {
        if(!EVP_EncryptUpdate(ctx, cip_buf, &cip_len, buf, buf_len)) {
            printf("error\n");
            EVP_CIPHER_CTX_cleanup(ctx);
            return -1;
        }
        fwrite(cip_buf, 1, cip_len, out);
    }
    EVP_CipherFinal_ex(ctx, cip_buf, &cip_len);
    fwrite(cip_buf, 1, cip_len, out);

    EVP_CIPHER_CTX_cleanup(ctx);
    return 0;
}

int decrypt(FILE *in, FILE *out, char *algo) {
    const EVP_CIPHER *cipher;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    unsigned char key[EVP_MAX_KEY_LENGTH] = {0, };
    unsigned char iv[EVP_MAX_IV_LENGTH] = {0, };

    OpenSSL_add_all_ciphers();

    EVP_CIPHER_CTX_init(ctx);
    cipher = EVP_get_cipherbyname(algo);
    memset(key, 0x41, EVP_CIPHER_key_length(cipher));
    memset(iv, 0x42, EVP_CIPHER_iv_length(cipher));

    EVP_DecryptInit_ex(ctx, cipher, NULL, key, iv);

    unsigned char *buf = (unsigned char*)malloc(BUFSIZ);
    unsigned char *dec_buf = (unsigned char*)malloc(BUFSIZ+EVP_MAX_BLOCK_LENGTH);
    int buf_len, dec_len = 0;
    
    while((buf_len = fread(buf, 1, sizeof(buf), in)) > 0) {
        if(!EVP_DecryptUpdate(ctx, dec_buf, &dec_len, buf, buf_len)) {
            printf("error\n");
            EVP_CIPHER_CTX_cleanup(ctx);
            return -1;
        }
        fwrite(dec_buf, 1, dec_len, out);
    }
    if(!EVP_DecryptFinal_ex(ctx, dec_buf, &dec_len)) {
        printf("final error\n");
    }
    fwrite(dec_buf, 1, dec_len, out);

    EVP_CIPHER_CTX_cleanup(ctx);
    return 0;
}


int init(char *input, char *output, int flag, char *algo) {
    FILE *i_fp, *o_fp = NULL;
    if((i_fp = fopen(input, "rb")) == NULL) {
        printf("input file error\n");
        return -1;
    }

    if((o_fp = fopen(output, "wb")) == NULL) {
        printf("output file error\n");
        return -1;
    }
    
    if(flag & SET_DECRYPTION)
        decrypt(i_fp, o_fp, algo);

    else if(flag & SET_DIGEST)
        digest(i_fp, o_fp, algo);

    else if(flag & SET_CIPHER)
        cipher(i_fp, o_fp, algo);

    fclose(i_fp);
    fclose(o_fp);
    return 0;
}

int main(int argc, char *argv[]) {
    char opt;
    char *input, *output, *algo = NULL;
    int flag = 0;
    while ( -1 != (opt = getopt(argc, argv, "i:o:e:d:h"))) {
        switch (opt) {
        case 'i':
            input = (char*)malloc(strlen(optarg)+1);
            strcpy(input, optarg);
            flag |= SET_INPUT;
            break;
        case 'o':
            output = (char*)malloc(strlen(optarg)+1);
            strcpy(output, optarg);
            flag |= SET_OUTPUT;
            break;
        case 'e':
            algo = (char*)malloc(strlen(optarg)+1);
            strcpy(algo, optarg);
            flag |= SET_ENCRYPTION;
            break;
        case 'd':
            algo = (char*)malloc(strlen(optarg)+1);
            strcpy(algo, optarg);
            flag |= SET_DECRYPTION;
            break;
        case 'h':
            help();
            return 0;
        default:
            printf("wrong option\n");
            break;
        }   
    }

    if(check_arg(&flag, algo))
        return -1;

    if(is_directory(input) && is_directory(output)) {
        DIR *dp = NULL;
        struct dirent *entry = NULL;
        struct stat buf;
        
        if((dp = opendir(input)) == NULL) {
            printf("opendir error\n");
            return -1;
        }

        while ((entry = readdir(dp)) != NULL) {
            char *in_path = (char*)malloc(strlen(entry->d_name) + strlen(input));
            sprintf(in_path, "%s/%s", input, entry->d_name);
            if(lstat(in_path, &buf)) {
                printf("stat error");
                free(in_path);
                break;
            }

            if(S_ISREG(buf.st_mode)) {
                char *out_path = (char*)malloc(strlen(entry->d_name) + strlen(output) + 4);
                if(flag & SET_DECRYPTION) {
                    sprintf(out_path, "%s/%s_dec", output, entry->d_name);   
                    init(in_path, out_path, flag, algo);    
                }
                else {
                    sprintf(out_path, "%s/%s_enc", output, entry->d_name);
                    init(in_path, out_path, flag, algo);
                }
                free(out_path);
            }
            free(in_path);
        }
        closedir(dp);
    }
    else {
        init(input, output, flag, algo);
    }

    return 0;
}