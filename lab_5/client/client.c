// CLIENT

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

int main(int argc, char * argv[]){

    if(argc != 4){
        printf("Please provide server's IP address, server's port number, and the requested filename as the three input parameters\n");
        return -1;
    }

    char * server_ip_address = argv[1];
    int server_port = atoi(argv[2]);
    char * requested_file_name = argv[3];

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
    socket_config->sin_addr.s_addr = inet_addr(server_ip_address);

    // connecting the socket
    int connection = connect(socket_descriptor, (struct sockaddr *) socket_config,  sizeof(struct sockaddr_in));
    if(connection == -1){
        printf("Error connecting the socket. Server is not reachable.\n");
        return 2;
    }
    if(DEBUG) printf("connection successful\n");

    // send request data
    printf("requesting the file \"%s\" from server ...\n", requested_file_name);
    char * request_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
    strcpy(request_data, requested_file_name);
    int bytes_sent = send(socket_descriptor, request_data, strlen(request_data), 0);
    if(bytes_sent != strlen(request_data)){
        printf("Error sending request\n");
        return 3;
    }
    if(DEBUG) printf("request sent (%d bytes)\n", bytes_sent);

    // receive response data
    if(DEBUG) printf("waiting for response from server ...\n");
    char * response_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
    int bytes_received = recv(socket_descriptor, response_data, BUFFER_SIZE, 0);
    if(bytes_received == -1){
        printf("Error receiving response data\n");
        return 4;
    }
    response_data[bytes_received] = '\0';
    printf("data received (%d bytes)\n", bytes_received);
    if(DEBUG) printf("data: \"%s\"\n", response_data);

    // storing the data received in a file
    FILE * fd_out = fopen(requested_file_name, "w");
    if(fd_out == NULL){
        printf("Error opening \"%s\"\n", requested_file_name);
    } else {
        fprintf(fd_out, "%s", response_data);
        fclose(fd_out);
    }

    // closing the socket
    close(socket_descriptor);

    return 0;
}