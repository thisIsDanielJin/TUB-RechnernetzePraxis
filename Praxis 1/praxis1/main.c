#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include "helper.h"
#include <unistd.h>
#include <arpa/inet.h>


int recognize_packets(int *client, char *buffer, int *start, int *end){
    ssize_t bytes_read = recv(*client,buffer + *end,256,0);
    if(bytes_read == 0){
        return 0;
    }
    *end += bytes_read;
    int diff = *end - *start;
    for(int i = 3; i<diff; i++){
        if(buffer[i-3 + *start] == '\r' && buffer[i-2+ *start] == '\n' && buffer[i-1 + *start] == '\r' && buffer[i + *start] == '\n'){
            *end = *start;
            char msg[] = "Reply\r\n\r\n";
            int len = sizeof(msg);
            int bytes_send = send(*client,msg,len-1,0);
        }
    }
    return 1;
}


int main(int argc, char *argv[])
{
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char* buffer = calloc(1024,sizeof(char));

    if (argc != 3){
        printf("Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    struct sockaddr_in server_addr = derive_sockaddr(argv[1], argv[2]);

    // create socket and check for errors
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // set socket options
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // bind socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listen
    if (listen(sockfd, 5) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Server läuft auf %s:%s\n", argv[1], argv[2]);
    
    // accept connection
    client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0){
        perror("accept");
    }

    int N = 1;
    int start = 0;
    int end = 0;
    while(N){
        N = recognize_packets(&client_fd,buffer, &start, &end);
    }

    // print client address
    printf("Client verbunden von %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    // close connection to client
    close(client_fd);
    return 0;
}