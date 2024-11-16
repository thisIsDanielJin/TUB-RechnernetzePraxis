#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include "helper.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    struct sockaddr_in server_addr = derive_sockaddr(argv[1], argv[2]);

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
    printf("Server läuft auf %s:%s\n", argv[1], argv[2]);

    while (1)
    {
        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        // accept connection
        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        // print client address
        printf("Client verbunden von %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // receive data from client
        char buffer[8192];
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0)
        {
            perror("recv");
            close(client_fd);
            continue;
        }

        buffer[bytes_received] = '\0'; // null-terminate string

        // send HTTP 400 Bad Request
        const char *http_response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";

        ssize_t bytes_sent = send(client_fd, http_response, strlen(http_response), 0);
        if (bytes_sent < 0)
        {
            perror("send");
            close(client_fd);
            continue;
        }

        // close connection to client
        close(client_fd);
    }
    return 0;
}