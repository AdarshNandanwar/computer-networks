// CLIENT

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <pthread.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1250
#define DEBUG 0
#define PUBLIC_KEY 0
#define PRIVATE_KEY 1

int is_connected = 1;

pthread_mutex_t mutex;

typedef struct ParamStruct {
    int server_socket_descriptor;
    char * private_key;
} param_struct;


int get_file_content(char * file_name, char * content, int len){
    FILE * fp = fopen(file_name, "r");
    if (fp == NULL){
        fprintf(stderr, "error loading content from \"%s\".\n", file_name);
        return -1;
    } else {
        fread(content, len, 1, fp);
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
        printf( "error creating BIO key.\n");
        return 0;
    }
    if(type == PUBLIC_KEY){
        rsa = PEM_read_bio_RSA_PUBKEY(bio_key, &rsa, NULL, NULL);
    } else if (type == PRIVATE_KEY){
        rsa = PEM_read_bio_RSAPrivateKey(bio_key, &rsa, NULL, NULL);
    } else {
        printf("invalid key type.\n");
    }
    if(rsa == NULL){
        printf( "error creating RSA.\n");
    } 
    return rsa;
}


void print_errors(){
    FILE * fp = fopen("logs.txt", "wb");
    if (fp == NULL){
        fprintf(stderr, "error opening \"%s\".\n", "logs.txt");
    } else {
        ERR_print_errors_fp(fp);
        fclose(fp);
    }
}


int decrypt(char * response_data, char * decrypted_data, char * private_key){
    if(DEBUG) printf("decrypting...\n");
    RSA * rsa = generate_rsa(private_key, PRIVATE_KEY);
    int decrypted_data_length = RSA_private_decrypt(RSA_size(rsa), response_data, decrypted_data, rsa, RSA_PKCS1_PADDING);
    if(decrypted_data_length == -1){
        if(DEBUG) printf("decryption failed\n");
        return -1;
    }
    if(DEBUG) printf("decryption successsful. data length = %d\n", decrypted_data_length);
    return decrypted_data_length;
}


void * receiver_runner(void * param){
    param_struct * p = (param_struct *) param;
    int server_socket_descriptor = p->server_socket_descriptor;
    char * private_key = p->private_key;

    if(DEBUG) printf("starting receiver thread fd %d\n", server_socket_descriptor);

    while(1){
        // receive response data
        if(DEBUG) printf("waiting for response from server ...\n");
        char * response_data = (char *) malloc(10*BUFFER_SIZE * sizeof(char));
        int bytes_received = recv(server_socket_descriptor, response_data, 10*BUFFER_SIZE, 0);
        if(bytes_received == -1){
            printf("error receiving response data\n");
            printf("[error %d]: %s\n", errno, strerror(errno));
            continue;
        }
        if(DEBUG) printf("data received (%d bytes)\n", bytes_received);

        // printing the response message
        if(bytes_received && strcmp(response_data, "exit")){

            printf("\n[CLIENT] ciphertext:\n");
            for(int k = 0; k<BUFFER_SIZE; k++) printf("%c", response_data[k]);
            printf("\n");
            if(DEBUG){
                printf("<END>\n");
                for(int k = BUFFER_SIZE; k<10*BUFFER_SIZE; k++) printf("%c", response_data[k]);
                printf("<END>\n");
            }

            // decryption using private key
            char * decrypted_data = (char *) malloc(10*BUFFER_SIZE * sizeof(char));
            int dec_len = decrypt(response_data, decrypted_data, private_key);
            if(dec_len == -1) continue;
            decrypted_data[dec_len] = '\0';
            printf("[CLIENT] plaintext: %s\n", decrypted_data);
            if(DEBUG) printf("[CLIENT] data size: %ld\n", strlen(decrypted_data));

        } else {
            printf("[CLIENT] server disconnected.\n");
            if(DEBUG) printf("[error %d]: %s\n", errno, strerror(errno));
            pthread_mutex_lock(&mutex);
            is_connected = 0;
            pthread_mutex_unlock(&mutex);
            break;
        }
    }
    if(DEBUG) printf("closing receiver thread %d\n", server_socket_descriptor);
    close(server_socket_descriptor);
    exit(0);
}


int main(int argc, char * argv[]){

    if(argc != 5){
        printf("please pass the following as parameters: {server_IP_address, port_number, private_key_file, public_key_file}\n");
        return -1;
    }

    int status;
    int server_port = atoi(argv[2]);
    char * server_ip_address = argv[1];
    char * private_key_file_name = argv[3];
    char * public_key_file_name = argv[4];

    char * message_send = (char *) malloc(BUFFER_SIZE * sizeof(char));

    // reading public key
    if(DEBUG) printf("reading public key from file \"%s\".\n", public_key_file_name);
    char * public_key = (char *) malloc(10*BUFFER_SIZE * sizeof(char));
    status = get_file_content(public_key_file_name, public_key, 10*BUFFER_SIZE);
    if(status == -1) return 1;

    // reading private key
    if(DEBUG) printf("reading private key from file \"%s\".\n", private_key_file_name);
    char * private_key = (char *) malloc(10*BUFFER_SIZE * sizeof(char));
    status = get_file_content(private_key_file_name, private_key, 10*BUFFER_SIZE);
    if(status == -1) return 1;
    
    // thread configuration
    int attr_state;
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr); 
    pthread_attr_getdetachstate(&attr,&attr_state);
    if(attr_state == PTHREAD_CREATE_JOINABLE){
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }
    param_struct * param = (param_struct *) malloc(sizeof(param_struct));

    // creating the socket
    int socket_descriptor;
    socket_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(socket_descriptor == -1){
        printf("error creating socket\n");
        printf("[error %d]: %s\n", errno, strerror(errno));
        return 1;
    }
    if(DEBUG) printf("socket created successfully\n");

    // configuring socket
    struct sockaddr_in * socket_config = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
    socket_config->sin_family = AF_INET;
    socket_config->sin_port = htons(server_port);
    socket_config->sin_addr.s_addr = inet_addr(server_ip_address);

    // connecting the socket
    int connection = connect(socket_descriptor, (struct sockaddr *) socket_config,  sizeof(struct sockaddr_in));
    if(connection == -1){
        printf("error connecting the server. server is not reachable.\n");
        printf("[error %d]: %s\n", errno, strerror(errno));
        return 2;
    }
    if(DEBUG) printf("connection successful\n");

    // receive confirmation data
    if(DEBUG) printf("waiting for confirmation from server ...\n");
    char * confirmation_message = (char *) malloc(BUFFER_SIZE * sizeof(char));
    int bytes_received = recv(socket_descriptor, confirmation_message, BUFFER_SIZE, 0);
    if(bytes_received == -1){
        printf("Error receiving confirmation data\n");
        printf("[error %d]: %s\n", errno, strerror(errno));
        close(socket_descriptor);
        return 4;
    }
    if(DEBUG) printf("confirmation received (%d bytes)\n", bytes_received);

    // checking if server limit exceeded
    if(strcmp(confirmation_message, "1")){
        printf("[CLIENT] server's client limit reached. try again in a while.\n");
        close(socket_descriptor);
        return 4;
    } else {
        printf("[CLIENT] connected to the server.\n");
    }

    param->server_socket_descriptor = socket_descriptor;
    param->private_key = private_key;
    if(pthread_create(&tid, &attr, &receiver_runner, param)){
        printf("error creating receiver thread\n");
        printf("[error %d]: %s\n", errno, strerror(errno));
        close(socket_descriptor);
        return 3;
    }

    int disconnected = 0;
    while(1){

        // taking the input message from STDIN
        do {
            fgets(message_send, BUFFER_SIZE, stdin);
            message_send[strlen(message_send)-1] = '\0';

            // checking the connection
            pthread_mutex_lock(&mutex);
            disconnected = !is_connected;
            pthread_mutex_unlock(&mutex);
            if(disconnected) break;
        } while(strlen(message_send) == 0);

        if(disconnected) break;

        // encryption using public key
        if(DEBUG) printf("encrypting...\n");
        char * encrypted_data = (char *) malloc(10*BUFFER_SIZE * sizeof(char));
        RSA * rsa = generate_rsa(public_key, PUBLIC_KEY);
        // PKCS #1 v1.5 padding. This currently is the most widely used mode.
        int encrypted_data_size = RSA_public_encrypt(strlen(message_send), message_send, encrypted_data, rsa, RSA_PKCS1_PADDING);
        if(encrypted_data_size == -1){
            if(DEBUG) printf("encryption failed.\n");
            // "logs.txt"
            print_errors();     
            continue;
        }
        if(DEBUG) printf("encryption successsful.\n");
        if(DEBUG){
            printf("[CLIENT] ciphertext:\n");
            for(int k = 0; k<BUFFER_SIZE; k++) printf("%c", encrypted_data[k]);
            printf("\n");
            printf("<END>\n");
            for(int k = BUFFER_SIZE; k<10*BUFFER_SIZE; k++) printf("%c", encrypted_data[k]);
            printf("<END>\n");
        }

        // send request data
        if(DEBUG) printf("sending the message to server ... (\"%s\")\n", message_send);
        char * request_data = (char *) malloc(10*BUFFER_SIZE * sizeof(char));
        if(strcmp(message_send, "exit")){
            for(int k = 0; k<10*BUFFER_SIZE; k++) request_data[k] = encrypted_data[k];
        } else {
            strcpy(request_data, "exit");
        }
        int bytes_sent = send(socket_descriptor, request_data, 10*BUFFER_SIZE, 0);
        if(bytes_sent != 10*BUFFER_SIZE){
            printf("error sending request\n");
            printf("[error %d]: %s\n", errno, strerror(errno));
            return 4;
        }
        printf("[CLIENT] sending message.\n");

        // exit condition
        if(strcmp(message_send, "exit") == 0) break;
    }

    if(DEBUG) printf("canceling receiver thread\n");
    pthread_cancel(tid);

    // closing the socket
    close(socket_descriptor);

    return 0;
}