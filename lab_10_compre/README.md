# Lab 10
<div style="font-size: 1.2rem">
Name- Adarsh Nandanwar<br>
BITS ID- 2018A7PS0396G</div>
<br>
<br>

# Public Private RSA Encryption and Decryption in C
## Usage Instructions
1. Install OpenSSL library.
2. Open a terminal window in the directory containing `encrypt.c` and `decrypt.c` file.
3. Ensure the directory contains the required keys: `public.pem` and `private.pem`
4. Compile the c programs.
    ```bash
    $ gcc encrypt.c -o encrypt -lcrypto -lssl
    $ gcc decrypt.c -o decrypt -lcrypto -lssl
    ```
5. Encrypt the input file using the executable `encrypt`. Parameters: {public_key, input_file_name, encrypted_file_name}
    ```bash
    $ ./encrypt public.pem input_file.txt encrypted_file.txt
    ```
6. Decrypt the encrypted file using the executable `decrypt`. Parameters: {private_key, encrypted_file_name, decrypted_file_name}
    ```bash
    $ ./decrypt private.pem encrypted_file.txt decrypted_file.txt
    ```
## Generating Keys
- The RSA key was geenrated using;
    ```bash
    $ openssl genrsa -out private.pem 10000
    $ openssl rsa -in private.pem -pubout -out public.pem
    ```
- `RSA_PKCS1_PADDING` padding was used in the program. Using this, maximum input file size that can be encrypted using RSA is: 
    ```
    max_input_size (in bytes)
        = (key_size/8)-11 
        = (10000/8)-11 
        = 1239
    ```
## Testing
```
gcc -pthread server.c -o server && ./server 8000
gcc -pthread -lcrypto -lssl client.c -o client && ./client 127.0.0.1 8000 private.pem public.pem
```

## ToDo
- see how to identify the client id
- add mutex to all global variables
- check if "exit" command work properly
- curently when client does ^C, the other client stays
## Tests
- send messgae when no other client (server currently hangs)
- handle "exit"