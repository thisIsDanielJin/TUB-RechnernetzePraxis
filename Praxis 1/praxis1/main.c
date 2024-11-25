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

// Function declarations
int find_http_request(const char *buffer, size_t buffer_length, size_t *request_length);
int process_http_request(const char *request, size_t request_length, int client_fd);

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
                int complete = find_http_request(buffer + bytes_consumed, buffer_length - bytes_consumed, &request_length);
                if (!complete)
                {
                    break;
                }

                // Process the request
                int res = process_http_request(buffer + bytes_consumed, request_length, client_fd);
                if (res < 0)
                {
                    // Bad request or error, close connection
                    break;
                }

                // Move the bytes_consumed forward
                bytes_consumed += request_length;
            }

            if (bytes_consumed > 0)
            {
                memmove(buffer, buffer + bytes_consumed, buffer_length - bytes_consumed);
                buffer_length -= bytes_consumed;
            }

            // If buffer is full and no complete request found, send 400 Bad Request and close connection
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

int find_http_request(const char *buffer, size_t buffer_length, size_t *request_length)
{
    // Search for "\r\n\r\n" to find the end of headers
    const char *headers_end = strstr(buffer, "\r\n\r\n");
    if (headers_end == NULL)
    {
        // No end of headers found yet
        return 0; // Not complete
    }

    size_t headers_length = headers_end - buffer + 4; // Include the "\r\n\r\n"

    // Now parse the headers to find Content-Length
    int content_length = 0;

    // Extract headers
    char headers[headers_length + 1];
    memcpy(headers, buffer, headers_length);
    headers[headers_length] = '\0';

    // Now parse headers to find Content-Length
    // For simplicity, we can search for "Content-Length:" in headers
    char *cl_header = strstr(headers, "Content-Length:");
    if (cl_header != NULL)
    {
        cl_header += strlen("Content-Length:");
        // Skip any whitespace
        while (*cl_header == ' ' || *cl_header == '\t')
            cl_header++;
        // Read the content length value
        content_length = atoi(cl_header);
    }

    // Calculate total request length
    size_t total_length = headers_length + content_length;

    if (buffer_length >= total_length)
    {
        // We have received the complete request
        *request_length = total_length;
        return 1; // Complete
    }
    else
    {
        // We have not received the complete payload yet
        return 0; // Not complete
    }
}

int process_http_request(const char *request, size_t request_length, int client_fd)
{
    // Remove or comment out this block for tasks 2.4 and 2.5
    /*
    const char *response = "Reply\r\n\r\n";
    send(client_fd, response, strlen(response), 0);
    return 0;
    */

    // Parse the start line
    const char *line_end = strstr(request, "\r\n");
    if (line_end == NULL)
    {
        send_http_bad_request_response(client_fd);
        return -1;
    }

    size_t line_length = line_end - request;
    char start_line[1024];
    if (line_length >= sizeof(start_line))
    {
        send_http_bad_request_response(client_fd);
        return -1;
    }
    strncpy(start_line, request, line_length);
    start_line[line_length] = '\0';

    char method[16];
    char uri[1024];
    char http_version[16];

    int num_matched = sscanf(start_line, "%15s %1023s %15s", method, uri, http_version);
    if (num_matched != 3)
    {
        send_http_bad_request_response(client_fd);
        return -1;
    }

    // Parse headers
    const char *header_start = line_end + 2;
    const char *current_pos = header_start;
    int headers_parsed = 0;
    int bad_request = 0;

    while (headers_parsed < 40)
    {
        const char *next_line_end = strstr(current_pos, "\r\n");
        if (next_line_end == NULL)
        {
            send_http_bad_request_response(client_fd);
            bad_request = 1;
            break;
        }
        if (next_line_end == current_pos)
        {
            // Empty line indicates end of headers
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
        return -1;
    }

    // Now, handle the request
    if (strcmp(method, "GET") == 0)
    {
        // HTTP 404 Not Found
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

    return 0;
}