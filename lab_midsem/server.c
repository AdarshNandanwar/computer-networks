// SERVER

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h> 

#define BUFFER_SIZE 1024
#define DEBUG 0

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

    // creating the socket
    int socket_descriptor;
    socket_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(socket_descriptor == -1){
        printf("Error creating socket\n");
        return 1;
    }
    if(DEBUG) printf("socket created successfully\n");

    // configuring socket
    struct sockaddr_in * socket_config = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
    socket_config->sin_family = AF_INET;
    socket_config->sin_port = htons(server_port);
    socket_config->sin_addr.s_addr = htonl(INADDR_ANY);

    // bind the socket
    int bind_status = bind(socket_descriptor, (struct sockaddr *) socket_config,  sizeof(struct sockaddr_in));
    if(bind_status == -1){
        printf("Error binding the socket\n");
        return 2;
    }
    if(DEBUG) printf("binding successful\n");

    // start listening
    int listen_status = listen(socket_descriptor, 10);
    if(bind_status == -1){
        printf("Error listening\n");
        return 3;
    }

    while(1){
        printf("listening on port %d ...\n", server_port);

        // accepting connections
        int client_socket_descriptor;
        int client_size = sizeof(struct sockaddr_in);
        struct sockaddr_in * client_socket_config = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
        client_socket_descriptor = accept(socket_descriptor, (struct sockaddr *) client_socket_config, &client_size);
        if(client_socket_descriptor == -1){
            printf("Error connecting to client socket\n");
            return 4;
        }
        if(DEBUG) printf("client accepted\n");

        // receiving request from the client
        if(DEBUG) printf("receiving request data from client ...\n");
        char * request_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
        int bytes_received = recv(client_socket_descriptor, request_data, BUFFER_SIZE, 0);
        if(bytes_received == -1){
            printf("Error receiving request data\n");
            return 4;
        }
        request_data[bytes_received] = '\0';
        reverse_string(request_data);
        if(DEBUG) printf("request received (%d bytes)\n", bytes_received);
        printf("[client, %d bytes]: %s\n", bytes_received, request_data);

        // taking the response input from STDIN
        char * response_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
        printf("Enter response message: ");
        fgets(response_data, BUFFER_SIZE, stdin);
        response_data[strlen(response_data)-1] = '\0';
        
        // sending response data
        if(DEBUG) printf("sending response ... (%s)\n", response_data);
        int bytes_sent = send(client_socket_descriptor, response_data, strlen(response_data), 0);
        if(bytes_sent != strlen(response_data)){
            printf("Error sending response\n");
            return 4;
        }
        printf("* response sent (%d bytes) *\n", bytes_sent);
    
        close(client_socket_descriptor);

    }

    // closing the socket
    close(socket_descriptor);

    return 0;
}