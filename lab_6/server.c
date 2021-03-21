// SERVER

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
#define MAX_CLIENTS 4
#define DEBUG 0

int active_connections;
pthread_mutex_t mutex;

typedef struct ParamStruct {
    int client_socket_descriptor;
} param_struct;

void reverse_string(char * str){
    int n = strlen(str);
    for(int i = 0; i<n/2; i++){
        char c = str[i];
        str[i] = str[n-i-1];
        str[n-i-1] = c;
    }
}

void * connection_runner (void * param) {
    param_struct * p = (param_struct *) param;
    char * response_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
    int status, client_socket_descriptor = p->client_socket_descriptor;

    while(1){

        // receiving request from the client
        if(DEBUG) printf("receiving request data from client[%d] ...\n", client_socket_descriptor);
        char * request_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
        int bytes_received = recv(client_socket_descriptor, request_data, BUFFER_SIZE, 0);
        if(bytes_received == -1){
            printf("error receiving request data from client[%d]. closing connection\n", client_socket_descriptor);
            printf("[error %d]: %s\n", errno, strerror(errno));
            break;
        }
        if(DEBUG) printf("request received from client[%d] (%d bytes)\n", client_socket_descriptor, bytes_received);

        // printing the request message
        if(bytes_received){
            request_data[bytes_received] = '\0';
            reverse_string(request_data);
            printf("[client[%d], %d bytes]: %s\n", client_socket_descriptor, bytes_received, request_data);
        } else {
            printf("* client[%d] disconnected *\n", client_socket_descriptor);
            if(DEBUG) printf("[error %d]: %s\n", errno, strerror(errno));
            break;
        }

        // taking the response input from STDIN
        do {
            printf("Enter response message for client[%d]: \n", client_socket_descriptor);
            fgets(response_data, BUFFER_SIZE, stdin);
            response_data[strlen(response_data)-1] = '\0';
        } while(strlen(response_data) == 0);
        
        // sending response data
        if(DEBUG) printf("sending response to client[%d] ... (\"%s\")\n", client_socket_descriptor, response_data);
        int bytes_sent = send(client_socket_descriptor, response_data, strlen(response_data), 0);
        if(bytes_sent != strlen(response_data)){
            printf("error sending response to client[%d]\n", client_socket_descriptor);
            printf("[error %d]: %s\n", errno, strerror(errno));
            break;
        }
        printf("* response sent to client[%d] (%d bytes) *\n", client_socket_descriptor, bytes_sent);
    }

    close(client_socket_descriptor);

    // increasing limit
    pthread_mutex_lock(&mutex);
    active_connections--;
    pthread_mutex_unlock(&mutex);

	pthread_exit(NULL);
}

int main(int argc, char * argv[]){

    if(argc != 2){
        printf("Please pass the following as parameters: {port_number}\n");
        return -1;
    }

    active_connections = 0;
    int status, server_port = atoi(argv[1]);
    char * confirmation_message = (char *) malloc(2 * sizeof(char));

    // thread configuration
    int attr_state;
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr); 
    pthread_attr_getdetachstate(&attr,&attr_state);
    if(attr_state == PTHREAD_CREATE_JOINABLE)
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
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
    socket_config->sin_addr.s_addr = htonl(INADDR_ANY);

    // bind the socket
    int bind_status = bind(socket_descriptor, (struct sockaddr *) socket_config, sizeof(struct sockaddr_in));
    if(bind_status == -1){
        printf("error binding the socket\n");
        printf("[error %d]: %s\n", errno, strerror(errno));
        return 2;
    }
    if(DEBUG) printf("binding successful\n");

    // start listening
    int listen_status = listen(socket_descriptor, MAX_CLIENTS);
    if(bind_status == -1){
        printf("error listening\n");
        printf("[error %d]: %s\n", errno, strerror(errno));
        return 3;
    }
    
    printf("listening on port %d ...\n", server_port);

    while(1){

        // accepting connections
        int client_socket_descriptor;
        int client_size = sizeof(struct sockaddr_in);
        struct sockaddr_in * client_socket_config = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
        client_socket_descriptor = accept(socket_descriptor, (struct sockaddr *) client_socket_config, &client_size);
        if(client_socket_descriptor == -1){
            printf("error connecting to client socket\n");
            printf("[error %d]: %s\n", errno, strerror(errno));
            continue;
        }

        // checking limit
        pthread_mutex_lock(&mutex);
        status = (active_connections >= MAX_CLIENTS);
        if(!status) active_connections++;
        pthread_mutex_unlock(&mutex);
        if(status){
            printf("* client[%d] rejected. client limit (%d) reached. *\n", client_socket_descriptor, MAX_CLIENTS);

            // sending confirmation
            strcpy(confirmation_message, "0");
            if(DEBUG) printf("sending confirmation to client[%d] ... (\"%s\")\n", client_socket_descriptor, confirmation_message);
            int bytes_sent = send(client_socket_descriptor, confirmation_message, strlen(confirmation_message), 0);
            if(bytes_sent != strlen(confirmation_message)){
                printf("error sending confirmation to client[%d]\n", client_socket_descriptor);
                printf("[error %d]: %s\n", errno, strerror(errno));
            } else {
                if(DEBUG) printf("* confirmation sent to client[%d] (%d bytes) *\n", client_socket_descriptor, bytes_sent);
            }

            close(client_socket_descriptor);
            continue;
        } else {
            // sending confirmation
            strcpy(confirmation_message, "1");
            if(DEBUG) printf("sending confirmation to client[%d] ... (\"%s\")\n", client_socket_descriptor, confirmation_message);
            int bytes_sent = send(client_socket_descriptor, confirmation_message, strlen(confirmation_message), 0);
            if(bytes_sent != strlen(confirmation_message)){
                printf("error sending confirmation to client[%d]\n", client_socket_descriptor);
                printf("[error %d]: %s\n", errno, strerror(errno));
                close(client_socket_descriptor);
                continue;
            } else {
                if(DEBUG) printf("* confirmation sent to client[%d] (%d bytes) *\n", client_socket_descriptor, bytes_sent);
            }
        }

        printf("* client[%d] accepted *\n", client_socket_descriptor);

        param->client_socket_descriptor = client_socket_descriptor;
        if(pthread_create(&tid, &attr, &connection_runner, param)){
            printf("error creating connection thread\n");
            printf("[error %d]: %s\n", errno, strerror(errno));
            close(client_socket_descriptor);
            continue;
        }
    }

    // closing the socket
    close(socket_descriptor);

    return 0;
}