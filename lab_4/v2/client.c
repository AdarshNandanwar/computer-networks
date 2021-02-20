// CLIENT

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/stat.h>   // for mkfifo()
#include <fcntl.h>      // for open()
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#define N 1024

int main(int argc, char * argv[]){

    printf("CLIENT TERMINAL\n");

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
        // parent process 
        // - RECEIVE

        // opening FIFO pipe for reading. returns a file descriptor
        fifo_descriptor[0] = open(fifo_path[0], O_RDONLY); 
        if(fifo_descriptor[0] == -1){
            printf("Error opening FIFO file in read only mode\n");
            exit(2);
        }

        while(1){
            // read from FIFO pipe
            read_ret = read(fifo_descriptor[0], msg_recv, N);
            if(read_ret == -1){
                printf("Error reading from FIFO file\n");
                close(fifo_descriptor[0]);
                exit(3);
            }
            // check if server exited
            if(read_ret == 0){
                break;
            }
            // print the read message 
            printf("Server: %s\n", msg_recv); 
            if(!strcmp(msg_recv, "exit")){
                // server exited
                break;
            }
        }
        // closing the pipe
        close(fifo_descriptor[0]);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);

    } else {
        // child process 
        // - SEND

        // opening FIFO pipe for writing. returns a file descriptor
        fifo_descriptor[1] = open(fifo_path[1], O_WRONLY); 
        if(fifo_descriptor[1] == -1){
            printf("Error opening FIFO file in write only mode\n");
            kill(pid, SIGKILL);
            return 2;
        }

        printf("Connected!\n");

        while(1){

            // take input from the user
            fgets(msg_send, N, stdin); 
            len = strlen(msg_send);
            if(msg_send[len-1] == '\n'){
                msg_send[len-1] = '\0';
            }

            // write the input in FIFO pipe
            if(write(fifo_descriptor[1], msg_send, strlen(msg_send)+1) == -1){
                printf("Error writing into FIFO file\n");
                close(fifo_descriptor[1]); 
                // kill(pid, SIGKILL);
                return 3;
            }

            if(!strcmp(msg_send, "exit")){
                // client exited
                break;
            }
        }
        // closing the pipe
        close(fifo_descriptor[1]); 

        // printf("child exiting\n");
        exit(0);
    }
    
    // printf("parent exiting\n");
    return 0;
}