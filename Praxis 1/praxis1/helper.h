#ifndef HELPER_H
#define HELPER_H

#include <netinet/in.h>

struct sockaddr_in derive_sockaddr(const char *host, const char *port);
int send_http_bad_request_response(int client_fd);

#endif