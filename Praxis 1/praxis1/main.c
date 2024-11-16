#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include "helper.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

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

        char *line_end = strstr(buffer, "\r\n");
        if (line_end == NULL)
        {
            send_http_bad_request_response(client_fd);
            close(client_fd);
            continue;
        }

        size_t line_length = line_end - buffer;
        char start_line[1024];
        if (line_length >= sizeof(start_line))
        {
            send_http_bad_request_response(client_fd);
            close(client_fd);
            continue;
        }
        strncpy(start_line, buffer, line_length);
        start_line[line_length] = '\0';

        char method[16];
        char uri[1024];
        char http_version[16];

        int num_matched = sscanf(start_line, "%15s %1023s %15s", method, uri, http_version);
        if (num_matched != 3)
        {
            send_http_bad_request_response(client_fd);
            close(client_fd);
            continue;
        }

        char *header_start = line_end + 2;
        char *current_pos = header_start;
        int headers_parsed = 0;
        int bad_request = 0;

        while (headers_parsed < 40)
        {
            char *next_line_end = strstr(current_pos, "\r\n");
            if (next_line_end == NULL)
            {
                send_http_bad_request_response(client_fd);
                bad_request = 1;
                break;
            }
            if (next_line_end == current_pos)
            {
                current_pos += 2;
                break;
            }
            size_t header_line_length = next_line_end - current_pos;
            char header_line[1024];
            if (header_line_length >= sizeof(header_line))
            {
                send_http_bad_request_response(client_fd);
                bad_request = 1;

                break;
            }
            strncpy(header_line, current_pos, header_line_length);
            header_line[header_line_length] = '\0';

            char key[256];
            char value[256];
            int num_matched = sscanf(header_line, "%255[^:]: %255[^\r\n]", key, value);
            if (num_matched != 2)
            {
                send_http_bad_request_response(client_fd);
                bad_request = 1;

                break;
            }
            current_pos = next_line_end + 2;
            headers_parsed++;
        }

        if (bad_request)
        {
            send_http_bad_request_response(client_fd);
            close(client_fd);
            continue;
        }

        if (strcmp(method, "GET") == 0)
        {
            // HTTP 404 Not found
            const char *http_response =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n"
                "\r\n";
            send(client_fd, http_response, strlen(http_response), 0);
        }
        else
        {
            // HTTP 501 Not Implemented
            const char *http_response =
                "HTTP/1.1 501 Not Implemented\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n"
                "\r\n";
            send(client_fd, http_response, strlen(http_response), 0);
        }

        close(client_fd);
    }
    return 0;
}
