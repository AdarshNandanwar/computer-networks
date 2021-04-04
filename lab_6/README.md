# Lab 6
<div style="font-size: 1.2rem">
Name- Adarsh Nandanwar<br>
BITS ID- 2018A7PS0396G</div>
<br>
<br>

## Usage Instructions
Open 6 terminals in the directory containing `server.c` and `client.c` files.
### Terminal 1 (Server)
1. Compile the c program
    ```bash
    $ gcc -pthread server.c -o server
    ```
2. Execute the executable.
    ```bash
    $ ./server 8000
    ```
3. Enter the response message to send to the client when asked.
4. Press `ctrl+c` to exit
### Terminal 2,3,4,5 and 6 (Clients)
1. Compile the c program
    ```bash
    $ gcc client.c -o client
    ```
2. Execute the executable.
    ```bash
    $ ./client 127.0.0.1 8000
    ```
3. Enter the message to send to the server when asked.
4. Type "exit" to quit.