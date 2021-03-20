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

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 4
#define DEBUG 1

void reverse_string(char * str){
    int n = strlen(str);
    for(int i = 0; i<n/2; i++){
        char c = str[i];
        str[i] = str[n-i-1];
        str[n-i-1] = c;
    }
}

int main(int argc, char * argv[]){

    if(argc != 2){
        printf("Please pass the following as parameters: {port_number}\n");
        return -1;
    }

    int server_port = atoi(argv[1]);
    char * response_data = (char *) malloc(BUFFER_SIZE * sizeof(char));

    // creating the socket
    int socket_descriptor;
    socket_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(socket_descriptor == -1){
        printf("Error creating socket\n");
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
        printf("Error binding the socket\n");
        printf("[error %d]: %s\n", errno, strerror(errno));
        return 2;
    }
    if(DEBUG) printf("binding successful\n");

    // start listening
    int listen_status = listen(socket_descriptor, MAX_CLIENTS);
    if(bind_status == -1){
        printf("Error listening\n");
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
            printf("Error connecting to client socket\n");
            printf("[error %d]: %s\n", errno, strerror(errno));
            return 4;
        }
        if(DEBUG) printf("client accepted\n");

        // receiving request from the client
        if(DEBUG) printf("receiving request data from client ...\n");
        char * request_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
        int bytes_received = recv(client_socket_descriptor, request_data, BUFFER_SIZE, 0);
        if(bytes_received == -1){
            printf("Error receiving request data\n");
            printf("[error %d]: %s\n", errno, strerror(errno));
            return 4;
        }
        if(DEBUG) printf("request received (%d bytes)\n", bytes_received);

        // printing the request message
        if(bytes_received){
            request_data[bytes_received] = '\0';
            reverse_string(request_data);
            printf("[client, %d bytes]: %s\n", bytes_received, request_data);
        } else {
            printf("client disconnected\n");
            printf("[error %d]: %s\n", errno, strerror(errno));
            close(client_socket_descriptor);
            continue;
        }

        // taking the response input from STDIN
        do {
            printf("Enter response message: ");
            fgets(response_data, BUFFER_SIZE, stdin);
            response_data[strlen(response_data)-1] = '\0';
        } while(strlen(response_data) == 0);
        
        // sending response data
        if(DEBUG) printf("sending response ... (%s)\n", response_data);
        int bytes_sent = send(client_socket_descriptor, response_data, strlen(response_data), 0);
        if(bytes_sent != strlen(response_data)){
            printf("Error sending response\n");
            printf("[error %d]: %s\n", errno, strerror(errno));
            return 4;
        }
        printf("* response sent (%d bytes) *\n", bytes_sent);
    
        close(client_socket_descriptor);

    }

    // closing the socket
    close(socket_descriptor);

    return 0;
}