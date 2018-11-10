#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "http.h"

#define BUF_SIZE 2048
#define REQ_LEN 53

/*
 * Create a new buffer to store our file
 */
Buffer *create_buffer(size_t size) {
    Buffer *buffer = (Buffer *) malloc(sizeof(Buffer));
    buffer->data = (char *) calloc(size, sizeof(char));
    buffer->length = size;
    return buffer;
}

/*
 * Clear the contents of the buffer, then clear the buffer.
 */
void clear_buffer(Buffer *buff) {
    free(buff->data);
    free(buff);
}

/*
 * Frees all allocated memory.
 */
void free_memory(int net_socket, char *request, struct addrinfo *server_address) {
    close(net_socket);
    free(request);
    freeaddrinfo(server_address);
}

/*
 * Get the data back from the server
 */
void web_read(Buffer *http_file, int sockfd) {
    int size = 0;
    int file_size = 0;
    while ((size = recv(sockfd, http_file->data + file_size, BUF_SIZE, 0)) != 0) {
        file_size += size;

        if (file_size + BUF_SIZE > http_file->length) {
            http_file->length = http_file->length + BUF_SIZE;
            http_file->data = realloc(http_file->data, http_file->length);
        }
//        printf("Size: %d, Length: %d\n", file_size, http_file->length);
    }
    memset(http_file->data + file_size, 0, http_file->length - file_size);
    http_file->length = strlen(http_file->data);
    http_file->data = realloc(http_file->data, file_size);
}

/*
 * Allocates variables, sets up our socket, and then sends and receives base from the server.
 */
Buffer *http_query(char *host, char *page, int port) {
    struct addrinfo server_info, *server_address = 0; //server info struct
    char port_no[6];
    int request_length = 0, status_code = 0;
    int net_socket = socket(AF_INET, SOCK_STREAM, 0);
    char *request = NULL;

    //Create our buffer for the final file
    Buffer *result_file = create_buffer(BUF_SIZE);
    sprintf(port_no, "%d", port);
    memset(&server_info, 0, sizeof(struct addrinfo));
    server_info.ai_family = AF_UNSPEC, server_info.ai_socktype = SOCK_STREAM;

    //Check if we can get the address info
    if ((status_code = getaddrinfo(host, port_no, &server_info, &server_address)) != 0) {
        fprintf(stderr, "Getting address info failed: %s\n", gai_strerror(status_code));
        free_memory(net_socket, request, server_address);
        clear_buffer(result_file);
        return NULL;
    }
    if(connect(net_socket, server_address->ai_addr, server_address->ai_addrlen) == -1){
        fprintf(stderr, "Status code error\n");
    }

    //Setting up the headers for our request
    request_length = (int) (REQ_LEN + strlen(host) + strlen(page));
    request = (char *) calloc(request_length, sizeof(char) * request_length);
    sprintf(request, "GET %s%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: "
                     "getter\r\n\r\n", page[0] == '/' ? "" : "/", page, host);

    //Reading and writing from the net socket
    if (write(net_socket, request, request_length) == -1) {
        fprintf(stderr, "Error sending.\n");
    }
    web_read(result_file, net_socket);
    //Cleaning up our variables
    close(net_socket);
    free(request);
    freeaddrinfo(server_address);
    return result_file;
}


/*
 * Split up the http response from the headers.
 */
char *http_get_content(Buffer *response) {
    char *header_end = strstr(response->data, "\r\n\r\n");
    if (header_end) {
        return header_end + 4;
    } else {
        return response->data;
    }
}

/*
 * Split the host and query, and then make the query to the default web port (80).
 */
Buffer *http_url(const char *url) {
    char host[BUF_SIZE];
    strncpy(host, url, BUF_SIZE);

    //strstr finds the first occurrence of a substring in a string.
    char *page = strstr(host, "/");
    if (page) {
        page[0] = '\0';
        ++page;
        return http_query(host, page, 80);
    } else { //There's no slash in the url
        fprintf(stderr, "could not split url into host/page %s\n", url);
        return NULL;
    }
}