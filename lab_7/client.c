// CLIENT
// http://example.com/files/text.txt

// References:
// https://stackoverflow.com/questions/41229601/openssl-in-c-socket-connection-https-client

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
// #include <openssl/applink.c>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// #define BUFFER_SIZE 65536
#define BUFFER_SIZE 1024
#define DEBUG 1

void reverse_string(char * str){
    int n = strlen(str);
    for(int i = 0; i<n/2; i++){
        char c = str[i];
        str[i] = str[n-i-1];
        str[n-i-1] = c;
    }
}

int parse_url(char * file_url, char * file_host, char * file_protocol, char * file_path, char * file_name){
    int i, index = 0, len = strlen(file_url);

    // extracting file name
    for(i = len-1; i>=0; i--){
        if(i == len-1 && file_url[i] == '/') continue;
        if(file_url[i] == '/') break;
        file_name[index++] = file_url[i];
    }
    file_name[index] = '\0';

    reverse_string(file_name);

    // removing protocol
    if(len > 7 && file_url[0] == 'h' && file_url[1] == 't' && file_url[2] == 't' && file_url[3] == 'p'){
        if(len > 8 && file_url[4] == 's'){
            file_url = file_url+8;
            len -= 8;
            strcpy(file_protocol, "https");
        } else {
            file_url = file_url+7;
            len -= 7;
            strcpy(file_protocol, "http");
        }
    } else {
        if(DEBUG) printf("\"http://\" or \"https://\" not found in the URL\n");
        return 1;
    }
    

    // removing www.
    if(len > 4 && file_url[0] == 'w' && file_url[1] == 'w' && file_url[2] == 'w'){
        file_url = file_url+4;
        len -= 4;
    }

    // extracting host
    index = 0;
    for(i = 0; i<len; i++){
        if(file_url[i] == '/') {
            i++;
            break;
        } 
        file_host[index++] = file_url[i];
    }
    file_host[index++] = '\0';

    // extracting file path
    index = 0;
    for(; i<len; i++){
        file_path[index++] = file_url[i];
    }
    file_path[index++] = '\0';

    return 0;
}

void log_ssl()
{
    int err;
    while (err = ERR_get_error()) {
        char *str = ERR_error_string(err, 0);
        if(!str) return;
        printf("%s\n", str);
        fflush(stdout);
    }
}

// Beej's get_in_addr function
// get port, IPv4 or IPv6:
in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return (((struct sockaddr_in*)sa)->sin_port);

    return (((struct sockaddr_in6*)sa)->sin6_port);
}

int main(int argc, char * argv[]){

    if(argc != 2){
        printf("Please pass the file URL as parameter. Format: https://filesamples.com/samples/document/txt/sample1.txt/\n");
        return -1;
    }

    int status;
    char * file_url = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char * file_host = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char * file_protocol = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char * file_path = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char * file_name = (char *) malloc(BUFFER_SIZE * sizeof(char));


    // extracting information from the given URL
    strcpy(file_url, argv[1]);
    printf("Parsing URL ...\n");
    status = parse_url(file_url, file_host, file_protocol, file_path, file_name);
    if(status != 0){
        printf("Invalid URL. Please ensure the URL is valid and is in the format https://filesamples.com/samples/document/txt/sample1.txt/\n");
        return 0;
    }
    printf("Protocol: %s\nHost: %s\nPath: %s\nFilename: %s\n\n", file_protocol, file_host, file_path, file_name);


    // configuring socket
    struct addrinfo hints, *socket_config, *itr;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(file_host, file_protocol, &hints, &socket_config);
    if(status != 0){
        printf("error getting addrinfo.\n");
        printf("[error %d]: %s\n", errno, strerror(errno));
        return 2;
    }


    // getting the IP address and connecting to the server
    int socket_descriptor, connection;
    for(itr = socket_config; itr != NULL; itr = itr->ai_next){
        // creating the socket
        socket_descriptor = socket(itr->ai_family, itr->ai_socktype, itr->ai_protocol);
        if(socket_descriptor == -1){
            if(DEBUG) printf("error creating socket\n");
            if(DEBUG) printf("[error %d]: %s\n", errno, strerror(errno));
            continue;
        }
        if(DEBUG) printf("socket created successfully\n");

        // connecting the socket
        connection = connect(socket_descriptor, (struct sockaddr *) itr->ai_addr,  itr->ai_addrlen);
        if(connection == -1){
            if(DEBUG) printf("error connecting the server. server is not reachable.\n");
            if(DEBUG) printf("[error %d]: %s\n", errno, strerror(errno));
            continue;
        } else {
            if(DEBUG) printf("connection successful\n");
            break;
        }
        close(socket_descriptor);
    }
    freeaddrinfo(socket_config);


    // checking if the connection was established
    if(itr == NULL){
        printf("error connecting the server. server is not reachable.\n");
        printf("[error %d]: %s\n", errno, strerror(errno));
        return 3;
    }


    // printing the server IP address and the port number
    char * ip_address = (char *) malloc(BUFFER_SIZE * sizeof(char));
    inet_ntop(itr->ai_addr->sa_family, &((struct sockaddr_in*) itr->ai_addr)->sin_addr, ip_address, INET_ADDRSTRLEN);
    if(DEBUG) printf("Server IP address: %s\n", ip_address);
    if(DEBUG) printf("Server Port: %d\n", ntohs(get_in_port((struct sockaddr *)itr->ai_addr)));


    // generating the GET request message
    char * message_send = (char *) malloc(BUFFER_SIZE * sizeof(char));
    sprintf(message_send, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\n", file_path, file_host);


    if(strcmp(file_protocol, "http") == 0){

        /* ########## HTTP ########## */


        // send GET request data
        if(DEBUG) printf("* sending the message: *\n%s\n", message_send);
        char * request_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
        strcpy(request_data, message_send);
        int bytes_sent = send(socket_descriptor, request_data, strlen(request_data), 0);
        if(bytes_sent != strlen(request_data)){
            printf("error sending request\n");
            printf("[error %d]: %s\n", errno, strerror(errno));
            return 3;
        }


        // receive response data
        if(DEBUG) printf("waiting for response from server ...\n");
        char * response_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
        // storing the data received in a file
        FILE * fd_out = fopen(file_name, "w");
        if(fd_out == NULL){
            printf("Error opening \"%s\"\n", file_name);
            return 6;
        }
        while(1){
            int bytes_received = recv(socket_descriptor, response_data, BUFFER_SIZE-1, 0);
            if(bytes_received == -1){
                printf("error receiving response data\n");
                printf("[error %d]: %s\n", errno, strerror(errno));
                return 4;
            } else if(bytes_received == 0){
                // server disconnected
                break;
            }
            if(DEBUG) printf("data received (%d bytes)\n", bytes_received);
            response_data[bytes_received] = '\0';
            fprintf(fd_out, "%s", response_data);
            // printf("%s", response_data);
        }
        fclose(fd_out);

    } else {

        /* ########## HTTPS ########## */


        // initializing SSL
        SSL_library_init();
        SSLeay_add_ssl_algorithms();
        SSL_load_error_strings();
        // const SSL_METHOD * method = TLSv1_2_client_method();
        const SSL_METHOD * method = SSLv23_client_method();
        SSL_CTX * ctx = SSL_CTX_new(method);
        if(ctx == NULL){
            printf("error creating ctx.\n");
            return -1;
        }
        SSL * ssl = SSL_new(ctx);
        if(!ssl){
            printf("error creating SSL.\n");
            log_ssl();
            return -1;
        }
        int sock = SSL_get_fd(ssl);
        SSL_set_fd(ssl, socket_descriptor);
        status = SSL_connect(ssl);
        if(status <= 0){
            printf("error creating SSL connection. err=%x\n", status);
            log_ssl();
            fflush(stdout);
            return -1;
        }
        printf("SSL connection using %s\n", SSL_get_cipher(ssl));


        // send GET request data
        if(DEBUG) printf("\nsending the message to server: \n%s\n", message_send);
        int bytes_sent = SSL_write(ssl, message_send, strlen(message_send));
        if (bytes_sent < 0) {
            int err = SSL_get_error(ssl, bytes_sent);
            printf("[%d] %s\n", err, ERR_error_string(err, NULL));
            return -1;
        }


        // receive response data
        if(DEBUG) printf("waiting for response from server ...\n");
        char * response_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
        // storing the data received in a file
        FILE * fd_out = fopen(file_name, "w");
        if(fd_out == NULL){
            printf("error opening \"%s\"\n", file_name);
            return 6;
        }
        while(1){
            int bytes_received = SSL_read(ssl, response_data, BUFFER_SIZE-1);
            if (bytes_received < 0) {
                int err = SSL_get_error(ssl, bytes_received);
                printf("[%d] %s\n", err, ERR_error_string(err, NULL));
                return 0;
            } else if(bytes_received == 0){
                // server disconnected
                break;
            }
            if(DEBUG) printf("data received (%d bytes)\n", bytes_received);
            response_data[bytes_received] = '\0';
            fprintf(fd_out, "%s", response_data);
            // printf("%s", response_data);
        }
        fclose(fd_out);

    }

    printf("\nFile downloaded!\n");

    // closing the socket
    close(socket_descriptor);

    return 0;
}