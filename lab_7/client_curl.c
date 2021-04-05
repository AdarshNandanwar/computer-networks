// CLIENT
// http://example.com/files/text.txt

// References:
// https://stackoverflow.com/questions/9786150/save-curl-content-result-into-a-string-in-c

#include <arpa/inet.h>
#include <curl/curl.h>
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

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
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


    // curl setup
    CURL * curl;
    FILE * fp;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(file_name,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, file_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        /* always cleanup */
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    printf("\nFile downloaded!\n");

    return 0;
}