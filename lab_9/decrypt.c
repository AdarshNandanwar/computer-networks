// SERVER

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

int write_file_content(char * file_name, char * content){
    FILE * fp = fopen(file_name, "w");
    if (fp == NULL){
        fprintf(stderr, "Error writing into file \"%s\".\n", file_name);
        fclose(fp);
        return -1;
    } else {
        fprintf(fp, "%s", content);
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

int main(int argc, char * argv[]){

    if(argc != 4){
        printf("Number of parameters passed = %d.\n", argc-1);
        printf("Please pass the following parameters: {private_key, input_file_name, output_file_name}\n");
        return -1;
    }

    int status;
    char * private_key_file_name = argv[1];
    char * input_file_name = argv[2];
    char * output_file_name = argv[3];


    // Reading private key
    printf("Reading private key from file \"%s\".\n", private_key_file_name);
    char * private_key = (char *) malloc(BUFFER_SIZE * sizeof(char));
    status = get_file_content(private_key_file_name, private_key, BUFFER_SIZE);
    if(status == -1) return 1;


    // Reading data from encrypted input file
    printf("Reading encrypted input from file \"%s\".\n", input_file_name);
    char * encrypted_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
    status = get_file_content(input_file_name, encrypted_data, BUFFER_SIZE);
    if(status == -1) return 1;

    
    // DECRYPTION using private key
    printf("Decrypting...\n");
    char * decrypted_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
    RSA * rsa = generate_rsa(private_key, PRIVATE_KEY);
    int decrypted_data_length = RSA_private_decrypt(RSA_size(rsa), encrypted_data, decrypted_data, rsa, RSA_PKCS1_PADDING);
    if(decrypted_data_length == -1){
        printf("Decryption failed\n");
        return 1;
    }
    printf("Decryption Successsful.\n");


    // Storing the decrypted data in output file
    printf("Decrypted data stored in \"%s\".\n", output_file_name);
    status = write_file_content(output_file_name, decrypted_data);
    if(status == -1) return 1;
    
    return 0;
}