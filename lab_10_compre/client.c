// CLIENT

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define DEBUG 0

int is_connected = 1;
pthread_mutex_t mutex;

typedef struct ParamStruct {
    int server_socket_descriptor;
} param_struct;


void * receiver_runner(void * param){
    param_struct * p = (param_struct *) param;
    char * recv_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
    int server_socket_descriptor = p->server_socket_descriptor;
    int status;

    if(DEBUG) printf("starting receiver thread fd %d\n", server_socket_descriptor);

    while(1){
        // receive response data
        if(DEBUG) printf("waiting for response from server ...\n");
        char * response_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
        int bytes_received = recv(server_socket_descriptor, response_data, BUFFER_SIZE, 0);
        if(bytes_received == -1){
            printf("error receiving response data\n");
            printf("[error %d]: %s\n", errno, strerror(errno));
            continue;
        }
        if(DEBUG) printf("data received (%d bytes)\n", bytes_received);

        // printing the response message
        response_data[bytes_received] = '\0';
        if(bytes_received && strcmp(response_data, "exit")){

            // decrypt






            printf("[CLIENT, %d bytes]: %s\n", bytes_received, response_data);
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
    printf("[CLIENT] press <enter> to exit.\n");
    close(server_socket_descriptor);
	pthread_exit(NULL);
}

int main(int argc, char * argv[]){

    if(argc != 5){
        printf("Please pass the following as parameters: {server_IP_address, port_number, private_key_file, public_key_file}\n");
        return -1;
    }

    int server_port = atoi(argv[2]);
    char * server_ip_address = argv[1];
    char * private_key_file = argv[3];
    char * public_key_file = argv[4];

    char * message_send = (char *) malloc(BUFFER_SIZE * sizeof(char));
    
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
            printf(">>> ");
            fgets(message_send, BUFFER_SIZE, stdin);
            message_send[strlen(message_send)-1] = '\0';

            // checking the connection
            pthread_mutex_lock(&mutex);
            disconnected = !is_connected;
            pthread_mutex_unlock(&mutex);
            if(disconnected) break;
        } while(strlen(message_send) == 0);

        if(disconnected) break;

        // encrypt







        // send request data
        if(DEBUG) printf("sending the message to server ... (\"%s\")\n", message_send);
        char * request_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
        strcpy(request_data, message_send);
        int bytes_sent = send(socket_descriptor, request_data, strlen(request_data), 0);
        if(bytes_sent != strlen(request_data)){
            printf("error sending request\n");
            printf("[error %d]: %s\n", errno, strerror(errno));
            return 4;
        }
        printf("[CLIENT] sending message (%d bytes).\n", bytes_sent);

        // exit condition
        if(strcmp(message_send, "exit") == 0) break;

    }

    if(DEBUG) printf("canceling receiver thread\n");
    pthread_cancel(tid);

    // closing the socket
    close(socket_descriptor);

    return 0;
}