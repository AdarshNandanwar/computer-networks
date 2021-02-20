// SERVER

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/stat.h>   // for mkfifo()
#include <fcntl.h>      // for open()
#include <unistd.h>
#include <errno.h>

#define N 1024

int main(int argc, char * argv[]){

    printf("SERVER\n");

    int fifo_descriptor, len = 0;

    // named pipe / FIFO file path 
    char * fifo_path = "fifo_pipe"; 
    char * msg_send = (char *) malloc(N*sizeof(char));
    char * msg_recv = (char *) malloc(N*sizeof(char));

    // creating the named pipe
    if(mkfifo(fifo_path, 0666) == -1){
        // error
        if(errno != EEXIST){
            printf("Error creating FIFO file\n");
            return 1;
        } else {
            // FIFO file already exits
        }
    }
    
    while(1){

        // SEND

        // opening FIFO pipe for writing. returns a file descriptor
        fifo_descriptor = open(fifo_path, O_WRONLY); 
        if(fifo_descriptor == -1){
            printf("Error opening FIFO file in write only mode\n");
            return 2;
        }
        // take input from the user
        printf("Server: ");
        // scanf("%[^\n]s", msg_send);
        fgets(msg_send, N, stdin); 
        len = strlen(msg_send);
        if(msg_send[len-1] == '\n'){
            msg_send[len-1] = '\0';
        }
        // write the input in FIFO pipe
        if(write(fifo_descriptor, msg_send, strlen(msg_send)+1) == -1){
            printf("Error writing into FIFO file\n");
            close(fifo_descriptor); 
            return 3;
        }
        // closing the pipe
        close(fifo_descriptor); 
        if(!strcmp(msg_send, "exit")){
            // server exited
            break;
        }

        // RECEIVE

        // opening FIFO pipe for reading. returns a file descriptor
        fifo_descriptor = open(fifo_path, O_RDONLY); 
        if(fifo_descriptor == -1){
            printf("Error opening FIFO file in read only mode\n");
            return 2;
        }
        // read from FIFO pipe
        if(read(fifo_descriptor, msg_recv, sizeof(msg_recv)) == -1){
            printf("Error reading from FIFO file\n");
            close(fifo_descriptor); 
            return 3;
        }
        // print the read message 
        printf("Client: %s\n", msg_recv); 
        // closing the pipe
        close(fifo_descriptor); 
        if(!strcmp(msg_recv, "exit")){
            // client exited
            break;
        }

    }
    
    return 0;
}