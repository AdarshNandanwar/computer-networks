# Lab 5
<div style="font-size: 1.2rem">
Name- Adarsh Nandanwar<br>
BITS ID- 2018A7PS0396G</div>
<br>
<br>

## Usage Instructions
### Terminal 1 (Server)
1. Navigate to the server directory
    ```bash
    $ cd server
    ```
2. Compile the c program
    ```bash
    $ gcc server.c -o server
    ```
2. Execute the executable. Parameters: `{port number}`
    ```bash
    $ ./server 8000
    ```
### Terminal 2 (Client)
1. Navigate to the client directory
    ```bash
    $ cd client
    ```
2. Compile the c program
    ```bash
    $ gcc client.c -o client
    ```
3. Execute the executable. Parameters: `{IP address, port number, file name}`
    ```bash
    $ ./client 127.0.0.1 8000 Networkingtrends.txt
    ```