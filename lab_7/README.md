# Lab 7
<div style="font-size: 1.2rem">
Name- Adarsh Nandanwar<br>
BITS ID- 2018A7PS0396G</div>
<br>
<br>

## Usage Instructions
1. Compile the c program
    ```bash
    $ gcc client.c -o client -lcrypto -lssl
    ```
2. Execute the executable.
    ```bash
    $ ./client http://example.com/files/text.txt
    ```
3. File will be downloaded in the current working directory.