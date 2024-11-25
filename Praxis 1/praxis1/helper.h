#ifndef HELPER_H
#define HELPER_H

#include <netinet/in.h>

struct sockaddr_in derive_sockaddr(const char *host, const char *port);
int send_http_bad_request_response(int client_fd);
int find_http_request(const char *buffer, size_t buffer_length, size_t *request_length);
int process_http_request(const char *request, size_t request_length, int client_fd);

#endif