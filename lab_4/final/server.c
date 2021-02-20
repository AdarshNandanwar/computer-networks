// SERVER

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define N 1024
#define DEBUG 0

int main(int argc, char * argv[]){

    printf("[SERVER TERMINAL]\n");

    // fifo_descriptor[0]: server -> client
    // fifo_descriptor[1]: client -> server
    int fifo_descriptor[2], len = 0, read_ret;

    // named pipe / FIFO file path 
    char * fifo_path[2];
    fifo_path[0] = (char *) malloc(N*sizeof(char));
    fifo_path[1] = (char *) malloc(N*sizeof(char));
    strcat(fifo_path[0], "fifo_pipe_0");
    strcat(fifo_path[1], "fifo_pipe_1");
    
    char * msg_send = (char *) malloc(N*sizeof(char));
    char * msg_recv = (char *) malloc(N*sizeof(char));

    // creating named pipes
    if(mkfifo(fifo_path[0], 0666) == -1){
        // error
        if(errno != EEXIST){
            printf("Error creating FIFO file 0\n");
            return 1;
        } else {
            // FIFO file already exits
        }
    }
    if(mkfifo(fifo_path[1], 0666) == -1){
        // error
        if(errno != EEXIST){
            printf("Error creating FIFO file 1\n");
            return 1;
        } else {
            // FIFO file already exits
        }
    }
    
    int pid = fork();
    if(pid < 0){
        printf("Error creating child process\n");
        return -1;
    }

    if(pid != 0){
        // parent process - SENDER

        if(DEBUG) printf("[SENDER] connected!\n");

        while(1){

            if(DEBUG) printf("[SENDER] taking input from user\n");
            // take input from the user
            fgets(msg_send, N, stdin); 
            len = strlen(msg_send);
            if(msg_send[len-1] == '\n'){
                msg_send[len-1] = '\0';
            }

            if(strcmp(msg_send, "exit")){
                // opening FIFO pipe for writing. returns a file descriptor
                if(DEBUG) printf("[SENDER] opening FIFO file in write only mode\n");
                // fifo_descriptor[0] = open(fifo_path[0], O_WRONLY); 
                fifo_descriptor[0] = open(fifo_path[0], O_RDWR); 
                if(fifo_descriptor[0] == -1){
                    printf("Error opening FIFO file in write only mode\n");
                    kill(pid, SIGKILL);
                    return 2;
                }
                // write the input in FIFO pipe
                if(DEBUG) printf("[SENDER] trying to send message\n");
                if(write(fifo_descriptor[0], msg_send, strlen(msg_send)+1) == -1){
                    printf("Error writing into FIFO file\n");
                    close(fifo_descriptor[0]); 
                    // kill(pid, SIGKILL);
                    return 3;
                }
                if(DEBUG) printf("[SENDER] message sent\n");
                // closing the pipe
                if(DEBUG) printf("[SENDER] closing FIFO file\n");
                close(fifo_descriptor[0]); 
            } else {
                // server exiting

                // sending the message to the child to close its pipe

                // opening FIFO pipe for writing. returns a file descriptor
                int fifo_descriptor_self = open(fifo_path[1], O_WRONLY); 
                if(fifo_descriptor_self == -1){
                    if(DEBUG) printf("[SENDER] Error opening FIFO file in wrote only mode\n");
                    return 2;
                }
                // write the input in FIFO pipe
                if(write(fifo_descriptor_self, "exit", strlen("exit")+1) == -1){
                    if(DEBUG) printf("[SENDER] Error writing into FIFO file\n");
                    close(fifo_descriptor_self);
                    return 3;
                }
                close(fifo_descriptor_self);

                // waiting for the receiver child to end
                waitpid(pid, NULL, 0);

                break;
            }
        }

    } else {
        // child process - RECEIVER

        // opening FIFO pipe for reading. returns a file descriptor
        if(DEBUG) printf("[RECEIVER] opening FIFO file in read only mode\n");
        fifo_descriptor[1] = open(fifo_path[1], O_RDONLY); 
        if(fifo_descriptor[1] == -1){
            if(DEBUG) printf("[RECEIVER] Error opening FIFO file in read only mode\n");
            exit(2);
        }

        if(DEBUG) printf("[RECEIVER] connected!\n");

        while(1){
            // read from FIFO pipe
            read_ret = read(fifo_descriptor[1], msg_recv, N);
            if(read_ret == -1){
                if(DEBUG) printf("[RECEIVER] Error reading from FIFO file\n");
                close(fifo_descriptor[1]); 
                exit(3);
            }
            // check if client exited
            if(read_ret == 0){
                if(DEBUG) printf("[RECEIVER] disconnected!\n");
                close(fifo_descriptor[1]); 
                fifo_descriptor[1] = open(fifo_path[1], O_RDONLY); 
                if(fifo_descriptor[1] == -1){
                    if(DEBUG) printf("[RECEIVER] Error opening FIFO file in read only mode\n");
                    exit(2);
                }
                if(DEBUG) printf("[RECEIVER] connected!\n");
                continue;
            }
            if(strcmp(msg_recv, "exit")){
                // print the read message 
                printf("Client: %s\n", msg_recv);
            } else {
                // sender exited
                break;
            }
        }
        // closing the pipe
        if(DEBUG) printf("[RECEIVER] closing FIFO file\n");
        close(fifo_descriptor[1]); 

        if(DEBUG) printf("child exiting\n");
        exit(0);
    }
    
    if(DEBUG) printf("parent exiting\n");
    return 0;
}