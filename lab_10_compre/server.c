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
#define MAX_CLIENTS 2
#define DEBUG 1

int active_connections;
int client_active[MAX_CLIENTS];
int client_message_flag[MAX_CLIENTS];
char * client_message[MAX_CLIENTS];
pthread_mutex_t mutex;
pthread_mutex_t mutex_stdin;
pthread_mutex_t mutex_client_data;

typedef struct ParamStruct {
    int client_socket_descriptor;
    int client_id;
} param_struct;


void * sender_runner (void * param) {
    param_struct * p = (param_struct *) param;
    int client_socket_descriptor = p->client_socket_descriptor;
    int id = p->client_id;
    int status;
    char * data = (char *) malloc(BUFFER_SIZE * sizeof(char));
    int is_active = 1;
    int new_data = 0;
    if(DEBUG) printf("starting sender thread of client %d, fd %d\n", id, client_socket_descriptor);

    while(1){
        // if(DEBUG) printf("waiting for lock to send to client %d\n", id);
        pthread_mutex_lock(&mutex_client_data);
        // if(DEBUG) printf("unlocked to send to client %d\n", id);
        // checking if client is connected
        if(client_active[id]){
            // checking if there is any new message to send
            if(client_message_flag[id]){
                strcpy(data, client_message[id]);
                new_data = 1;
            }
            client_message_flag[id] = 0;
            pthread_mutex_unlock(&mutex_client_data);
            // if(DEBUG) printf("release lock to send to client %d\n", id);
        } else {
            // client not connected
            pthread_mutex_unlock(&mutex_client_data);
            // if(DEBUG) printf("release lock to send to client %d\n", id);
            is_active = 0;
            break;
        }

        if(new_data){
            if(DEBUG) printf("new data found to send to client %d, fd %d\n", id, client_socket_descriptor);
            // sending response data
            if(DEBUG) printf("sending response to client[%d] ... (\"%s\")\n", client_socket_descriptor, data);
            int bytes_sent = send(client_socket_descriptor, data, strlen(data), 0);
            if(bytes_sent != strlen(data)){
                printf("error sending response to client[%d]\n", client_socket_descriptor);
                printf("[error %d]: %s\n", errno, strerror(errno));
                continue;
            }
            printf("* data sent to client[%d] (%d bytes) *\n", client_socket_descriptor, bytes_sent);
        }
        new_data = 0;
    }

    printf("* closing sender thread of client[%d] *\n", client_socket_descriptor);

	pthread_exit(NULL);
}


void * connection_runner (void * param) {
    param_struct * p = (param_struct *) param;
    int client_socket_descriptor = p->client_socket_descriptor;
    int client_id = p->client_id;
    int status;

    if(DEBUG) printf("starting connection thread of client %d, fd %d\n", client_id, client_socket_descriptor);

    // thread configuration
    int attr_state;
    pthread_t tid_sender;
    pthread_attr_t attr;
    pthread_attr_init(&attr); 
    pthread_attr_getdetachstate(&attr,&attr_state);
    if(attr_state == PTHREAD_CREATE_JOINABLE){
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }

    // creating thread to send data
    if(pthread_create(&tid_sender, &attr, &sender_runner, p)){
        printf("error creating sender thread\n");
        printf("[error %d]: %s\n", errno, strerror(errno));
        close(client_socket_descriptor);
    } else {
        while(1){

            // receiving request from the client
            if(DEBUG) printf("receiving request data from client[%d] ...\n", client_socket_descriptor);
            char * recv_data = (char *) malloc(BUFFER_SIZE * sizeof(char));
            int bytes_received = recv(client_socket_descriptor, recv_data, BUFFER_SIZE, 0);
            if(bytes_received == -1){
                printf("error receiving request data from client[%d]. closing connection\n", client_socket_descriptor);
                printf("[error %d]: %s\n", errno, strerror(errno));
                break;
            }
            if(DEBUG) printf("request received from client[%d] (%d bytes)\n", client_socket_descriptor, bytes_received);


            // verifying message
            if(bytes_received == 0){
                printf("* client[%d] disconnected *\n", client_socket_descriptor);
                if(DEBUG) printf("[error %d]: %s\n", errno, strerror(errno));
                break;
            }
            recv_data[bytes_received] = '\0';       // might cause problems later
            if(DEBUG) printf("[client[%d], %d bytes]: %s\n", client_socket_descriptor, bytes_received, recv_data);


            // sending it to the other clients
            int client_sent = 0;
            for(int id = 0; id<MAX_CLIENTS; id++){
                if(id == client_id) continue;
                // if(DEBUG) printf("1 waiting for lock to receive client %d\n", id);
                pthread_mutex_lock(&mutex_client_data);
                // if(DEBUG) printf("1 unlocked to receive client %d\n", id);
                int is_active = client_active[id];
                pthread_mutex_unlock(&mutex_client_data);
                // if(DEBUG) printf("1 release to receive client %d\n", id);
                if(!is_active) continue;
                while(1){
                    // if(DEBUG) printf("2 waiting for lock to receive client %d\n", id);
                    pthread_mutex_lock(&mutex_client_data);
                    // if(DEBUG) printf("2 unlocked to receive client %d\n", id);
                    if(!client_message_flag[id]){
                        // if(DEBUG) printf("client %d flag 0\n", id);
                        client_message_flag[id] = 1;
                        strcpy(client_message[id], recv_data);
                        pthread_mutex_unlock(&mutex_client_data);
                        // if(DEBUG) printf("2 release to receive client %d\n", id);
                        client_sent++;
                        break;
                    }
                    // if(DEBUG) printf("client %d flag 1\n", id);
                    pthread_mutex_unlock(&mutex_client_data);
                    // if(DEBUG) printf("2 release to receive client %d\n", id);
                }
            }
            if(DEBUG) printf("message sent to %d clients.\n", client_sent);

        }
        
    }


    close(client_socket_descriptor);

    // increasing limit
    pthread_mutex_lock(&mutex);
    active_connections--;
    if(DEBUG) printf("active connections: %d\n", active_connections);
    pthread_mutex_unlock(&mutex);

    // cleaning client data
    // printf("waiting for lock to clost client %d\n", client_id);
    pthread_mutex_lock(&mutex_client_data);
    // printf("unlocked to close client %d\n", client_id);
    client_active[client_id] = 0;
    client_message_flag[client_id] = 0;
    pthread_mutex_unlock(&mutex_client_data);
    // printf("release lock to close client %d\n", client_id);
    if(DEBUG){
        for(int id = 0; id<MAX_CLIENTS; id++){
            printf("%d ", client_active[id]);
        }
        printf("\n");
    }

    if(DEBUG) printf("closing connection thread of client %d, fd %d\n", client_id, client_socket_descriptor);
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
    pthread_t tid_receiver;
    pthread_attr_t attr;
    pthread_attr_init(&attr); 
    pthread_attr_getdetachstate(&attr,&attr_state);
    if(attr_state == PTHREAD_CREATE_JOINABLE){
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }
    param_struct * param = (param_struct *) malloc(sizeof(param_struct));

    // client IPC configuration
    pthread_mutex_lock(&mutex_client_data);
    for(int i = 0; i<MAX_CLIENTS; i++){
        client_active[i] = 0;
        client_message_flag[i] = 0;
        client_message[i] = (char *) malloc(BUFFER_SIZE * sizeof(char));
    }
    pthread_mutex_unlock(&mutex_client_data);

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

        // assigning the client_id to the client
        int id;
        // printf("waiting for lock to create client\n");
        pthread_mutex_lock(&mutex_client_data);
        // printf("unlocked to create client\n");
        for(id = 0; id<MAX_CLIENTS; id++) if(!client_active[id]) break;
        if(id == MAX_CLIENTS){
            pthread_mutex_unlock(&mutex_client_data);
            // printf("release lock to create client\n");
            if(DEBUG) printf("id not available to allot to the client\n");
            continue;
        } else {
            client_active[id] = 1;
            pthread_mutex_unlock(&mutex_client_data);
            // printf("release lock to create client\n");
            if(DEBUG) printf("client id allocated: %d\n", id);
        }
        if(DEBUG){
            for(int id = 0; id<MAX_CLIENTS; id++){
                printf("%d ", client_active[id]);
            }
            printf("\n");
        }

        param->client_socket_descriptor = client_socket_descriptor;
        param->client_id = id;
        if(pthread_create(&tid_receiver, &attr, &connection_runner, param)){
            printf("error creating receiver thread\n");
            printf("[error %d]: %s\n", errno, strerror(errno));
            close(client_socket_descriptor);
            continue;
        }
    }

    // closing the socket
    close(socket_descriptor);

    return 0;
}