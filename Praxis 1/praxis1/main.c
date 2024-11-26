#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include "helper.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#define MAX_BUFFER_SIZE 8192

int setUpServer(char *ip, char *port)
{
    struct sockaddr_in server_addr = derive_sockaddr(ip, port);

    // create socket and check for errors
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // set socket options
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // bind socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listen
    if (listen(sockfd, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Server läuft auf %s:%s\n", ip, port);
    return sockfd;
}

int acceptClientConnection(int sockfd, struct sockaddr_in *client_addr)
{
    socklen_t client_addr_len = sizeof(*client_addr);
    int client_fd = accept(sockfd, (struct sockaddr *)client_addr, &client_addr_len);
    if (client_fd < 0)
    {
        perror("accept");
    }
    return client_fd;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }
    char *IP = argv[1];
    char *PORT = argv[2];

    int sockfd = setUpServer(IP, PORT); // 2.1
    Resource storage[100];
    for(int i = 0; i < 100; i++){
        storage[i].address[0] = '\0';
    }
    while (1)
    {
        struct sockaddr_in client_addr;
        int client_fd = acceptClientConnection(sockfd, &client_addr);

        if (client_fd < 0)
        {
            continue;
        }

        printf("Client verbunden von %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Initialize buffer
        char buffer[MAX_BUFFER_SIZE];
        size_t buffer_length = 0;

        // Process client connection
        while (1)
        {
            // Receive data from client
            ssize_t bytes_received = recv(client_fd, buffer + buffer_length, sizeof(buffer) - buffer_length - 1, 0);
            if (bytes_received < 0)
            {
                perror("recv");
                break;
            }
            else if (bytes_received == 0)
            {
                // Client closed connection
                break;
            }

            buffer_length += bytes_received;
            buffer[buffer_length] = '\0';

            size_t bytes_consumed = 0;
            while (1)
            {
                size_t request_length = 0;
                int content_length = 0;
                int complete = find_http_request(buffer + bytes_consumed, buffer_length - bytes_consumed, &request_length, &content_length);
                if (!complete)
                {
                    break;
                }

                int res = process_http_request(buffer + bytes_consumed, request_length, client_fd, storage, content_length);
                if (res < 0)
                {
                    break;
                }

                bytes_consumed += request_length;
            }

            if (bytes_consumed > 0)
            {
                memmove(buffer, buffer + bytes_consumed, buffer_length - bytes_consumed);
                buffer_length -= bytes_consumed;
            }

            if (buffer_length == sizeof(buffer) - 1)
            {
                send_http_bad_request_response(client_fd);
                break;
            }
        }

        close(client_fd);
    }
    return 0;
}
