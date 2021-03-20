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

void reverse_string(char * str){
    int n = strlen(str);
    for(int i = 0; i<n/2; i++){
        char c = str[i];
        str[i] = str[n-i-1];
        str[n-i-1] = c;
    }
}

int main(int argc, char * argv[]){

    if(argc != 3){
        printf("Please pass the following as parameters: {server_IP_address, port_number}\n");
        return -1;
    }

    int server_port = atoi(argv[2]);
    char * server_ip_address = (char *) malloc(16 * sizeof(char));
    strcpy(server_ip_address, argv[1]);

    char * message_send = (char *) malloc(BUFFER_SIZE * sizeof(char));
        

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
        printf("Error connecting the server. Server is not reachable.\n");
        return 2;
    }
    if(DEBUG) printf("connection successful\n");


    printf("Enter the message: ");
    fgets(message_send, BUFFER_SIZE, stdin);
    message_send[strlen(message_send)-1] = '\0';

    // send request data
    if(DEBUG) printf("sending the message \"%s\" to server ...\n", message_send);
    char * request_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
    strcpy(request_data, message_send);
    int bytes_sent = send(socket_descriptor, request_data, strlen(request_data), 0);
    if(bytes_sent != strlen(request_data)){
        printf("Error sending request\n");
        return 3;
    }
    printf("* message sent (%d bytes) *\n", bytes_sent);

    // receive response data
    if(DEBUG) printf("waiting for response from server ...\n");
    char * response_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
    int bytes_received = recv(socket_descriptor, response_data, BUFFER_SIZE, 0);
    if(bytes_received == -1){
        printf("Error receiving response data\n");
        return 4;
    }
    response_data[bytes_received] = '\0';
    reverse_string(response_data);
    // if(DEBUG) printf("data received (%d bytes)\n", bytes_received);
    printf("[server, %d bytes]: %s\n", bytes_received, response_data);

    // closing the socket
    close(socket_descriptor);


    return 0;
}