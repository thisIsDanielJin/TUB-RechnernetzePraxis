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

const char* addressbook [] = {"/static/foo","/static/bar","/static/baz"};
const char* static_answers [] = {"Foo", "Bar", "Baz"};


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

// 0 = undefined URI, 1 = static URI, 2 = dynamic URI
size_t check_URI(char uri[], int *answer){
    for (size_t i = 0; i < sizeof(addressbook) / sizeof(addressbook[0]); i++) {
        if (strcmp(uri, addressbook[i]) == 0) {
            *answer = (int)i;
            return 1;
        }
    }
    if(strncmp("/dynamic/", uri,9) == 0){
        return 2;
    }
    return 0;
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

int find_http_request(const char *buffer, size_t buffer_length, size_t *request_length, int *content_length)
{

    const char *headers_end = strstr(buffer, "\r\n\r\n");
    if (headers_end == NULL)
    {
        // No end of headers found yet
        return 0;
    }

    size_t headers_length = headers_end - buffer + 4;

    char headers[headers_length + 1];
    memcpy(headers, buffer, headers_length);
    headers[headers_length] = '\0';

    char *cl_header = strstr(headers, "Content-Length:");
    if (cl_header != NULL)
    {
        cl_header += strlen("Content-Length:");
        while (*cl_header == ' ' || *cl_header == '\t')
            cl_header++;

        *content_length = atoi(cl_header);
    }

    size_t total_length = headers_length + *content_length;

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

int process_http_request(const char *request, size_t request_length, int client_fd, Resource storage[], int content_length)
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
        int num_matched2 = sscanf(header_line, "%255[^:]: %255[^\r\n]", key, value);
        if (num_matched2 != 2)
        {
            send_http_bad_request_response(client_fd);
            bad_request = 1;
            break;
        }

        current_pos = next_line_end + 2;
        headers_parsed++;
    }

    char content[8192];
    if(content_length > 0) {
        int loading_content = sscanf(current_pos, "%8191[^\r\n]", content);
        if (loading_content != 1) {
            send_http_bad_request_response(client_fd);
            bad_request = 1;
        }
    }

    if (bad_request)
    {
        return -1;
    }

    int length = 0;
    int x = -1;
    int* answer = &x;

    if(strcmp(method, "GET") == 0 && check_URI(uri,answer) == 1){
        if(x != -1) {
            length = strlen(static_answers[x]);
        }
        char response[8192];
        sprintf(response,
                "HTTP/1.1 200 Ok\r\n"
                "Content-Length: %d\r\n"
                "\r\n"
                "%s",length,static_answers[x]
        );
        send(client_fd, response, strlen(response), 0);
    }
    else if(strcmp(method, "GET") == 0 && check_URI(uri,answer) == 2){
        int exist = 0;
        for(int i = 0; i<100; i++){
            if(strcmp(storage[i].address,uri) == 0) {
                const char *content_to_send = storage[i].content;
                exist = 1;
                char response[8192];
                sprintf(response,
                        "HTTP/1.1 200 Ok\r\n"
                        "Content-Length: %d\r\n"
                        "\r\n"
                        "%s", (int)strlen(content_to_send),content_to_send
                );
                send(client_fd, response, strlen(response), 0);
            }
        }
        if(exist == 0){
            send(client_fd, HTTP_404_RESPONSE, strlen(HTTP_404_RESPONSE), 0);
        }
    }
    else if (strcmp(method, "GET") == 0)
    {
        // HTTP 404 Not Found
        send(client_fd, HTTP_404_RESPONSE, strlen(HTTP_404_RESPONSE), 0);
    }
    else if (strcmp(method, "PUT") == 0 && check_URI(uri,answer) == 2){
        int exist = 0;
        int next_free = -1;
        for(int i = 0; i<100; i++){
            if(next_free == -1 && storage[i].address[0] == '\0'){
                next_free = i;
            }
            if(strcmp(storage[i].address,uri) == 0) {
                strncpy(storage[i].content, content, content_length);
                exist = 1;
                char *response =
                        "HTTP/1.1 204 No Content\r\n"
                        "Content-Length: 0\r\n"
                        "Connection: close\r\n"
                        "\r\n";
                send(client_fd, response, strlen(response), 0);
            }
        }
        if(exist == 0){
            strncpy(storage[next_free].address, uri, 1024);
            strncpy(storage[next_free].content, content, content_length);
            char *response =
                    "HTTP/1.1 201 Created\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n"
                    "\r\n";
            send(client_fd, response, strlen(response), 0);
        }
    }
    else if((strcmp(method, "PUT") == 0 || strcmp(method, "DELETE") == 0) && check_URI(uri,answer) != 2){
        char *response =
                "HTTP/1.1 403 Forbidden\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n"
                "\r\n";
        send(client_fd, response, strlen(response), 0);
    }
    else if(strcmp(method, "DELETE") == 0 && check_URI(uri,answer) == 2){
        int exist = 0;
        for(int i = 0; i<100; i++){
            if(strcmp(storage[i].address,uri) == 0) {
                exist = 1;
                for(int j = 0; j<8192; j++){
                    if(j<1024){
                        storage[i].address[j] = '\0';
                    }
                    storage[i].content[j] = '\0';
                }
                char *response =
                        "HTTP/1.1 204 No Content\r\n"
                        "Content-Length: 0\r\n"
                        "Connection: close\r\n"
                        "\r\n";
                send(client_fd, response, strlen(response), 0);
            }
        }
        if(exist == 0){
            send(client_fd, HTTP_404_RESPONSE, strlen(HTTP_404_RESPONSE), 0);
        }
    }
    else
    {
        // HTTP 501 Not Implemented
        send(client_fd, HTTP_501_RESPONSE, strlen(HTTP_501_RESPONSE), 0);
    }

    return 0;
}