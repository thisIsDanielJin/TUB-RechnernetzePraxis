#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helper.h"
#include <unistd.h>
#include <string.h>

const char *HTTP_400_RESPONSE =
    "HTTP/1.1 400 Bad Request\r\n"
    "Content-Length: 0\r\n"
    "Connection: close\r\n"
    "\r\n";

const char *HTTP_404_RESPONSE =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Length: 0\r\n"
    "Connection: close\r\n"
    "\r\n";

const char *HTTP_501_RESPONSE =
    "HTTP/1.1 501 Not Implemented\r\n"
    "Content-Length: 0\r\n"
    "Connection: close\r\n"
    "\r\n";

struct sockaddr_in derive_sockaddr(const char *host, const char *port)
{
    struct addrinfo hints = {
        .ai_family = AF_INET,
    };
    struct addrinfo *result_info;

    int returncode = getaddrinfo(host, port, &hints, &result_info);
    if (returncode)
    {
        fprintf(stderr, "Error␣parsing␣host/port");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in result = *((struct sockaddr_in *)result_info->ai_addr);

    freeaddrinfo(result_info);

    return result;
}

int send_http_bad_request_response(int client_fd)
{
    ssize_t bytes_sent = send(client_fd, HTTP_400_RESPONSE, strlen(HTTP_400_RESPONSE), 0);
    if (bytes_sent < 0)
    {
        perror("send");
    }

    return 0;
}

int find_http_request(const char *buffer, size_t buffer_length, size_t *request_length)
{

    const char *headers_end = strstr(buffer, "\r\n\r\n");
    if (headers_end == NULL)
    {
        // No end of headers found yet
        return 0;
    }

    size_t headers_length = headers_end - buffer + 4;

    int content_length = 0;

    char headers[headers_length + 1];
    memcpy(headers, buffer, headers_length);
    headers[headers_length] = '\0';

    char *cl_header = strstr(headers, "Content-Length:");
    if (cl_header != NULL)
    {
        cl_header += strlen("Content-Length:");
        while (*cl_header == ' ' || *cl_header == '\t')
            cl_header++;

        content_length = atoi(cl_header);
    }

    size_t total_length = headers_length + content_length;

    if (buffer_length >= total_length)
    {

        *request_length = total_length;
        return 1;
    }
    else
    {

        return 0;
    }
}

int process_http_request(const char *request, size_t request_length, int client_fd)
{

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

    if (strcmp(method, "GET") == 0)
    {
        // HTTP 404 Not Found
        send(client_fd, HTTP_404_RESPONSE, strlen(HTTP_404_RESPONSE), 0);
    }
    else
    {
        // HTTP 501 Not Implemented
        send(client_fd, HTTP_501_RESPONSE, strlen(HTTP_501_RESPONSE), 0);
    }

    return 0;
}