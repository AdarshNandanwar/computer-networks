// CLIENT

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 10000
#define DEBUG 0
#define PUBLIC_KEY 0
#define PRIVATE_KEY 1

int get_file_content(char * file_name, char * content, int len){
    FILE * fp = fopen(file_name, "r");
    if (fp == NULL){
        fprintf(stderr, "Error loading content from \"%s\".\n", file_name);
        fclose(fp);
        return -1;
    } else {
        fread(content, len, 1, fp);
    }
    fclose(fp);
    return 0;
}

int write_encrypted_content(char * file_name, char * content, int len){
    FILE * fp = fopen(file_name, "w");
    if (fp == NULL){
        fprintf(stderr, "Error writing into file \"%s\".\n", file_name);
        fclose(fp);
        return -1;
    } else {
        for(int i = 0; i<len; i++){
            fprintf(fp, "%c", content[i]);
        }
    }
    fclose(fp);
    return 0;
}

RSA * generate_rsa(unsigned char * key, int type){
    RSA * rsa = NULL;
    // creating a memory BIO using len bytes of data at buf.
    // if len is -1 then the buf is assumed to be nul terminated and its length is determined by strlen.
    BIO * bio_key = BIO_new_mem_buf(key, -1);
    if(bio_key == NULL)     {
        printf( "Error creating BIO key.\n");
        return 0;
    }
    if(type == PUBLIC_KEY){
        rsa = PEM_read_bio_RSA_PUBKEY(bio_key, &rsa, NULL, NULL);
    } else if (type == PRIVATE_KEY){
        rsa = PEM_read_bio_RSAPrivateKey(bio_key, &rsa, NULL, NULL);
    } else {
        printf("Invalid key type.\n");
    }
    if(rsa == NULL){
        printf( "Error creating RSA.\n");
    } 
    return rsa;
}

int check_decrypt(char * encrypted_data){
    char * decrypted_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char * private_key = (char *) malloc(BUFFER_SIZE * sizeof(char));
    get_file_content("private.pem", private_key, BUFFER_SIZE);
    RSA * rsa = generate_rsa(private_key, PRIVATE_KEY);
    int decrypted_data_length = RSA_private_decrypt(RSA_size(rsa), encrypted_data, decrypted_data, rsa, RSA_PKCS1_PADDING);
    if(decrypted_data_length == -1){
        printf("Decrypt fail\n");
        return 1;
    } 
    // printf("Encrypted Data: \n%s\n", encrypted_data);
    printf("Decrypted Data: \n%s\n", decrypted_data);
    printf("Decrypt Check Successful\n");
    return 0;
}

void print_errors(){
    FILE * fp = fopen("logs.txt", "wb");
    ERR_print_errors_fp(fp);
    fclose(fp);
}

int main(int argc, char * argv[]){

    if(argc != 4){
        printf("Number of parameters passed = %d.\n", argc-1);
        printf("Please pass the following parameters: {public_key, input_file_name, output_file_name}\n");
        return -1;
    }

    int status, encrypted_data_size;
    char * public_key_file_name = argv[1];
    char * input_file_name = argv[2];
    char * output_file_name = argv[3];


    // Reading public key
    printf("Reading public key from file \"%s\".\n", public_key_file_name);
    char * public_key = (char *) malloc(BUFFER_SIZE * sizeof(char));
    status = get_file_content(public_key_file_name, public_key, BUFFER_SIZE);
    if(status == -1) return 1;


    // Reading data from input file
    printf("Reading input from file \"%s\".\n", input_file_name);
    char * data = (char *) malloc(BUFFER_SIZE * sizeof(char));
    status = get_file_content(input_file_name, data, BUFFER_SIZE);
    if(status == -1) return 1;


    // ENCRYPTION using public key
    printf("Encrypting...\n");
    char * encrypted_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
    RSA * rsa = generate_rsa(public_key, PUBLIC_KEY);
    // PKCS #1 v1.5 padding. This currently is the most widely used mode.
    encrypted_data_size = RSA_public_encrypt(strlen(data), data, encrypted_data, rsa, RSA_PKCS1_PADDING);
    if(encrypted_data_size == -1){
        printf("Encryption failed.\n");
        // "logs.txt"
        print_errors();     
        return 1;
    }
    printf("Encryption Successsful.\n");


    // Storing the encrypted data in output file
    printf("Encrypted data stored in \"%s\".\n", output_file_name);
    status = write_encrypted_content(output_file_name, encrypted_data, RSA_size(rsa));
    if(status == -1) return 1;

    return 0;
}