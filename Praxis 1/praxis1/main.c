#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include "helper.h"
#include <unistd.h>
#include <arpa/inet.h>

int recognize_packets(int* client, char* buffer){
    int bytes_read = recv(*client,buffer+3,1024-3,0);
    if(bytes_read == 0){
        return 0;
    }
    for(int i = 0; i<bytes_read; i++){
        if(buffer[i] == '\r' && buffer[i+1] == '\n' && buffer[i+2] == '\r' && buffer[i+3] == '\n'){
            char msg[] = "Reply\r\n\r\n";
            int len = sizeof(msg);
            int bytes_send = send(*client,msg,len-1,0);
        }
    }
    if(bytes_read > 2){
        buffer[0] = buffer[bytes_read];
        buffer[1] = buffer[bytes_read+1];
        buffer[2] = buffer[bytes_read+2];
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
    if (listen(sockfd, 5) < 0)
    {
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
    while (N){
        N = recognize_packets(&client_fd,buffer);
    }
    // print client address
    printf("Client verbunden von %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    // close connection to client
    close(client_fd);
    return 0;
}
}