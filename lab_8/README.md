# Lab 8
<div style="font-size: 1.2rem">
Name- Adarsh Nandanwar<br>
BITS ID- 2018A7PS0396G</div>
<br>
<br>

## Usage Instructions
1. Install OpenSSL library.
2. Open a terminal window in the directory containing `client.c` file.
3. Compile the c program.
    ```bash
    $ gcc client.c -o client -lcrypto -lssl
    ```
4. Execute the executable.
    ```bash
    $ ./client https://raw.githubusercontent.com/torvalds/linux/master/README
    $ ./client https://raw.githubusercontent.com/git/git/master/Documentation/git.txt
    $ ./client http://www.example.com/sample.html
    ```
5. Files will be downloaded in the current working directory.