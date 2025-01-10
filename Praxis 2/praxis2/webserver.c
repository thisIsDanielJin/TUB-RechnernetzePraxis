#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "data.h"
#include "http.h"
#include "util.h"

#define MAX_RESOURCES 100

//struct that saves the redirection addr
struct responsible_addr
{
    uint16_t hash;
    uint16_t port;
    char ip[INET_ADDRSTRLEN];
};

static uint16_t self_id = 0;
static char self_ip[INET_ADDRSTRLEN] = {0};
static uint16_t self_port = 0;
static uint16_t pred_id = 0;
static char pred_ip[INET_ADDRSTRLEN] = {0};
static uint16_t pred_port = 0;
static uint16_t succ_id = 0;
static char succ_ip[INET_ADDRSTRLEN] = {0};
static uint16_t succ_port = 0;

static struct sockaddr_in succ_addr;
static struct sockaddr_in pred_addr;

static int have_pred = 0;
static int have_succ = 0;

int lookup_index = 0;
static struct responsible_addr lookuptable[10];

struct tuple resources[MAX_RESOURCES] = {
    {"/static/foo", "Foo", sizeof "Foo" - 1},
    {"/static/bar", "Bar", sizeof "Bar" - 1},
    {"/static/baz", "Baz", sizeof "Baz" - 1}};

/**
 * Sends an HTTP reply to the client based on the received request.
 *
 * @param conn      The file descriptor of the client connection socket.
 * @param request   A pointer to the struct containing the parsed request
 * information.
 */
void send_reply(int conn, struct request *request)
{

    // Create a buffer to hold the HTTP reply
    char buffer[HTTP_MAX_SIZE];
    char *reply = buffer;
    size_t offset = 0;

    fprintf(stderr, "Handling %s request for %s (%lu byte payload)\n",
            request->method, request->uri, request->payload_length);

    if (strcmp(request->method, "GET") == 0)
    {
        // Find the resource with the given URI in the 'resources' array.
        size_t resource_length;
        const char *resource =
            get(request->uri, resources, MAX_RESOURCES, &resource_length);

        if (resource)
        {
            size_t payload_offset =
                sprintf(reply, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n",
                        resource_length);
            memcpy(reply + payload_offset, resource, resource_length);
            offset = payload_offset + resource_length;
        }
        else
        {
            reply = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            offset = strlen(reply);
        }
    }
    else if (strcmp(request->method, "PUT") == 0)
    {
        // Try to set the requested resource with the given payload in the
        // 'resources' array.
        if (set(request->uri, request->payload, request->payload_length,
                resources, MAX_RESOURCES))
        {
            reply = "HTTP/1.1 204 No Content\r\n\r\n";
        }
        else
        {
            reply = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n";
        }
        offset = strlen(reply);
    }
    else if (strcmp(request->method, "DELETE") == 0)
    {
        // Try to delete the requested resource from the 'resources' array
        if (delete (request->uri, resources, MAX_RESOURCES))
        {
            reply = "HTTP/1.1 204 No Content\r\n\r\n";
        }
        else
        {
            reply = "HTTP/1.1 404 Not Found\r\n\r\n";
        }
        offset = strlen(reply);
    }
    else
    {
        reply = "HTTP/1.1 501 Method Not Supported\r\n\r\n";
        offset = strlen(reply);
    }

    // Send the reply back to the client
    if (send(conn, reply, offset, 0) == -1)
    {
        perror("send");
        close(conn);
    }
}

static int is_responsible(uint16_t key)
{
    if (!have_pred && !have_succ)
    {
        return 1;
    }
    if (pred_id < self_id)
    {
        return (pred_id < key && key <= self_id);
    }
    else if (pred_id > self_id)
    {
        return (key <= self_id || key > pred_id);
    }
    else
    {
        return 1;
    }
}

/**
 * Derives a sockaddr_in structure from the provided host and port information.
 *
 * @param host The host (IP address or hostname) to be resolved into a network
 * address.
 * @param port The port number to be converted into network byte order.
 *
 * @return A sockaddr_in structure representing the network address derived from
 * the host and port.
 */
static struct sockaddr_in derive_sockaddr(const char *host, const char *port)
{
    struct addrinfo hints = {
        .ai_family = AF_INET,
    };
    struct addrinfo *result_info;

    // Resolve the host (IP address or hostname) into a list of possible
    // addresses.
    int returncode = getaddrinfo(host, port, &hints, &result_info);
    if (returncode)
    {
        fprintf(stderr, "Error parsing host/port");
        exit(EXIT_FAILURE);
    }

    // Copy the sockaddr_in structure from the first address in the list
    struct sockaddr_in result = *((struct sockaddr_in *)result_info->ai_addr);

    // Free the allocated memory for the result_info
    freeaddrinfo(result_info);
    return result;
}
/**
 * Example IPv4 127.0.0.1 gets converted into [127][0][0][1]
 *
 * @param ipAddres IPv4 as an string
 * @param ip_dec a buffer where the INT Values are safed
 *
 * @return
 */
void getDecValueOfIP4v(char* ipAddress,int *ip_dec)
{
    size_t j = 0;
    size_t k = 0;
    for(size_t i = 0; i < strlen(ipAddress);i++){
        if(ipAddress[i] == '.'){
            char temp[INET_ADDRSTRLEN-1];
            strncpy(temp,ipAddress + k,i+1-k);
            k = i+1;
            ip_dec[j] = atoi(temp);
            j++;
        }
    }
    char temp[INET_ADDRSTRLEN-1];
    strncpy(temp,ipAddress + k,4);
    ip_dec[j] = atoi(temp);
    return;
}
/**
 * Searches if the hash_id is in the lookuptable. 
 *
 * @param dht_table lookuptable for hash_ids
 * @param hash_id the hash_id we're looking for in the lookuptable
 *
 * @return IF the hash_id is in the lookuptable it returns the index in lookuptable. Else -1.
 */
ssize_t search_hash(struct responsible_addr* dht_table,uint16_t hash_id){
    for(int i = 0; i < 10; i++){
        if(dht_table[i].hash == hash_id){
            return i;
        }
    }
    return -1;
}
/**
 * Adds hash_id into the lookuptable. 
 *
 * @param dht_table lookuptable for hash_ids
 * @param responsible_node the responsible_node for the hash_id, with the port and the IP4v 
 *
 * @return
 */
void add_hash(struct responsible_addr* dht_table, uint16_t port, char *ip){
        dht_table[lookup_index].port = port;
        strncpy(dht_table[lookup_index].ip, ip, 9);
        lookup_index++;
        if(lookup_index == 10){
            lookup_index = 0;
        }
}

/**
 * Send lookup reply or forward lookup 
 *
 * @param udp_socket fd of udp_socket
 * @param hash_id hash_id from the lookup
 *
 * @return
 */
void reply_lookup(int udp_socket,uint16_t hash_id, uint16_t request_id, uint16_t request_port){
    char lookup[BUFSIZ];
    // clear the Buffers
    memset(lookup, 0, BUFSIZ);
    //lookup reply 
    if(is_responsible(hash_id))
    {
        int blocks_of_pred_ip [4];
        getDecValueOfIP4v(self_ip,blocks_of_pred_ip);
            lookup[0] = htons(1)>> 8;
            lookup[1] = htons(pred_id) & 0xFF;
            lookup[2] = htons(pred_id) >> 8;
            lookup[3] = htons(self_id) & 0xFF;
            lookup[4] = htons(self_id) >> 8;
            lookup[5] = htons(blocks_of_pred_ip[0]) >> 8;
            lookup[6] = htons(blocks_of_pred_ip[1]) >> 8;
            lookup[7] = htons(blocks_of_pred_ip[2]) >> 8;
            lookup[8] = htons(blocks_of_pred_ip[3]) >> 8; 
            lookup[9] = htons(self_port) & 0xFF;
            lookup[10] = htons(self_port) >> 8;
        //UDP: send msg to pred addr    
        char r_port [4];
        sprintf(r_port,"%d",request_port);
        struct sockaddr_in request_addr = derive_sockaddr("127.0.0.1",r_port); 

        sendto(udp_socket, lookup, 11, 0, (struct sockaddr *)&request_addr, sizeof(request_addr));

    }
    else if(hash_id > self_id && hash_id <= succ_id)
    {
        int blocks_of_pred_ip [4];
        getDecValueOfIP4v(self_ip,blocks_of_pred_ip);
            lookup[0] = htons(1)>> 8;
            lookup[1] = htons(self_id) & 0xFF;
            lookup[2] = htons(self_id) >> 8;
            lookup[3] = htons(succ_id) & 0xFF;
            lookup[4] = htons(succ_id) >> 8;
            lookup[5] = htons(blocks_of_pred_ip[0]) >> 8;
            lookup[6] = htons(blocks_of_pred_ip[1]) >> 8;
            lookup[7] = htons(blocks_of_pred_ip[2]) >> 8;
            lookup[8] = htons(blocks_of_pred_ip[3]) >> 8; 
            lookup[9] = htons(succ_port) & 0xFF;
            lookup[10] = htons(succ_port) >> 8;
        //UDP: send msg to pred addr    
        char r_port [4];
        sprintf(r_port,"%d",request_port);
        struct sockaddr_in request_addr = derive_sockaddr("127.0.0.1",r_port); 

        sendto(udp_socket, lookup, 11, 0, (struct sockaddr *)&request_addr, sizeof(request_addr));

    }
    else // lookup forward
    {    

        char lookup[HTTP_MAX_SIZE];
        int blocks_of_self_ip [4];
        getDecValueOfIP4v(pred_ip,blocks_of_self_ip);
            lookup[0] = htons(0)>> 8;
            lookup[1] = hash_id >> 8;
            lookup[2] = hash_id & 0xFF;
            lookup[3] = htons(request_id) & 0xFF;
            lookup[4] = htons(request_id) >> 8;
            lookup[5] = htons(blocks_of_self_ip[0]) >> 8;
            lookup[6] = htons(blocks_of_self_ip[1]) >> 8;
            lookup[7] = htons(blocks_of_self_ip[2]) >> 8;
            lookup[8] = htons(blocks_of_self_ip[3]) >> 8; 
            lookup[9] = htons(request_port) & 0xFF;
            lookup[10] = htons(request_port) >> 8;
        //UDP: send msg to succ addr             
        sendto(udp_socket, lookup, 11, 0, (struct sockaddr *)&succ_addr, sizeof(succ_addr));

    }
    return;
}

/**
 * Processes an incoming packet from the client.
 *
 * @param conn The socket descriptor representing the connection to the client.
 * @param buffer A pointer to the incoming packet's buffer.
 * @param n The size of the incoming packet.
 *
 * @return Returns the number of bytes processed from the packet.
 *         If the packet is successfully processed and a reply is sent, the
 * return value indicates the number of bytes processed. If the packet is
 * malformed or an error occurs during processing, the return value is -1.
 *
 */
size_t process_packet(int udp_socket,int conn, char *buffer, size_t n)
{
    struct request request = {
        .method = NULL, .uri = NULL, .payload = NULL, .payload_length = -1};
    ssize_t bytes_processed = parse_request(buffer, n, &request);

    if (bytes_processed > 0)
    {
        uint16_t uri_hash = pseudo_hash((unsigned char *)request.uri, strlen(request.uri));

        if (strcmp(request.method, "GET") == 0)
        {
            // Not responsible -> redirect
            ssize_t index = search_hash(lookuptable,uri_hash); 
            if(index > -1)
            {
                char redirect_buf[HTTP_MAX_SIZE];
                int len = snprintf(redirect_buf, sizeof(redirect_buf),
                                "HTTP/1.1 303 See Other\r\n"
                                "Location: http://%.9s:%d%s\r\n"
                                "Content-Length: 0\r\n\r\n",
                                lookuptable[index].ip, lookuptable[index].port, request.uri);
                send(conn, redirect_buf, len, 0);
            }
            else if (!is_responsible(uri_hash) && succ_addr.sin_port == pred_addr.sin_port)
            {
                char redirect_buf[HTTP_MAX_SIZE];
                int len = snprintf(redirect_buf, sizeof(redirect_buf),
                                "HTTP/1.1 303 See Other\r\n"
                                "Location: http://%.9s:%d%s\r\n"
                                "Content-Length: 0\r\n\r\n",
                                succ_ip, succ_port, request.uri);
                send(conn, redirect_buf, len, 0);
            }
            // Check responsibility
            else if (!is_responsible(uri_hash) && have_succ)
            {
                //503: Service Unavailable-Antwort
                char no_service[HTTP_MAX_SIZE];
                int no_service_len = snprintf(no_service, sizeof(no_service),
                                   "HTTP/1.1 503 Service Unavailable\r\n"
                                   "Retry-After: 1\r\n"
                                   "Content-Length: 0\r\n\r\n");
                send(conn, no_service, no_service_len, 0);

                if(index < 0){ 
                    lookuptable[lookup_index].hash = uri_hash;

                    char lookup[BUFSIZ];
                    memset(lookup, 0, BUFSIZ);
                    int blocks_of_self_ip [4];
                    getDecValueOfIP4v(self_ip,blocks_of_self_ip);
                        lookup[0] = htons(0)>> 8;
                        lookup[1] = htons(uri_hash) & 0xFF;
                        lookup[2] = htons(uri_hash) >> 8;
                        lookup[3] = htons(self_id) & 0xFF;
                        lookup[4] = htons(self_id) >> 8;
                        lookup[5] = htons(blocks_of_self_ip[0]) >> 8;
                        lookup[6] = htons(blocks_of_self_ip[1]) >> 8;
                        lookup[7] = htons(blocks_of_self_ip[2]) >> 8;
                        lookup[8] = htons(blocks_of_self_ip[3]) >> 8; 
                        lookup[9] = htons(self_port) & 0xFF;
                        lookup[10] = htons(self_port) >> 8;

                    //UDP: send msg to succ addr      
                    int bytessend_udp;
                    bytessend_udp = sendto(udp_socket, lookup, 11, 0,(struct sockaddr*)&succ_addr,sizeof(succ_addr));    
                    if(bytessend_udp < 1){
                        perror("sendto succ");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else{
                send_reply(conn, &request);
            }
        }
        else
        {
            send_reply(conn, &request);
        }
        // Check the "Connection" header in the request to determine if the
        // connection should be kept alive or closed.
        const string connection_header = get_header(&request, "Connection");
        if (connection_header && strcmp(connection_header, "close"))
        {
            return -1;
        }
    }
    else if (bytes_processed == -1)
    {
        // If the request is malformed or an error occurs during processing,
        // send a 400 Bad Request response to the client.
        const string bad_request = "HTTP/1.1 400 Bad Request\r\n\r\n";
        send(conn, bad_request, strlen(bad_request), 0);
        printf("Received malformed request, terminating connection.\n");
        close(conn);
        return -1;
    }

    return bytes_processed;
}

/**
 * Sets up the connection state for a new socket connection.
 *
 * @param state A pointer to the connection_state structure to be initialized.
 * @param sock The socket descriptor representing the new connection.
 *
 */
static void connection_setup(struct connection_state *state, int sock)
{
    // Set the socket descriptor for the new connection in the connection_state
    // structure.
    state->sock = sock;

    // Set the 'end' pointer of the state to the beginning of the buffer.
    state->end = state->buffer;

    // Clear the buffer by filling it with zeros to avoid any stale data.
    memset(state->buffer, 0, HTTP_MAX_SIZE);
}

/**
 * Discards the front of a buffer
 *
 * @param buffer A pointer to the buffer to be modified.
 * @param discard The number of bytes to drop from the front of the buffer.
 * @param keep The number of bytes that should be kept after the discarded
 * bytes.
 *
 * @return Returns a pointer to the first unused byte in the buffer after the
 * discard.
 * @example buffer_discard(ABCDEF0000, 4, 2):
 *          ABCDEF0000 ->  EFCDEF0000 -> EF00000000, returns pointer to first 0.
 */
char *buffer_discard(char *buffer, size_t discard, size_t keep)
{
    memmove(buffer, buffer + discard, keep);
    memset(buffer + keep, 0, discard); // invalidate buffer
    return buffer + keep;
}

/**
 * Handles incoming connections and processes data received over the socket.
 *
 * @param state A pointer to the connection_state structure containing the
 * connection state.
 * @return Returns true if the connection and data processing were successful,
 * false otherwise. If an error occurs while receiving data from the socket, the
 * function exits the program.
 */
bool handle_connection(struct connection_state *state, int udp_socket)
{
    // Calculate the pointer to the end of the buffer to avoid buffer overflow
    const char *buffer_end = state->buffer + HTTP_MAX_SIZE;

    // Check if an error occurred while receiving data from the socket
    ssize_t bytes_read =
        recv(state->sock, state->end, buffer_end - state->end, 0);
    if (bytes_read == -1)
    {
        perror("recv");
        close(state->sock);
        exit(EXIT_FAILURE);
    }
    else if (bytes_read == 0)
    {
        return false;
    }

    char *window_start = state->buffer;
    char *window_end = state->end + bytes_read;

    ssize_t bytes_processed = 0;
    while ((bytes_processed = process_packet(udp_socket,state->sock, window_start,
                                             window_end - window_start)) > 0)
    {
        window_start += bytes_processed;
    }
    if (bytes_processed == -1)
    {
        return false;
    }

    state->end = buffer_discard(state->buffer, window_start - state->buffer,
                                window_end - window_start);
    return true;
}


/**
 * Sets up a TCP server socket and binds it to the provided sockaddr_in address.
 *
 * @param addr The sockaddr_in structure representing the IP address and port of
 * the server.
 *
 * @return The file descriptor of the created TCP server socket.
 */
static int setup_server_socket(struct sockaddr_in addr)
{
    const int enable = 1;
    const int backlog = 1;

    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Avoid dead lock on connections that are dropped after poll returns but
    // before accept is called
    if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    // Set the SO_REUSEADDR socket option to allow reuse of local addresses
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) ==
        -1)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind socket to the provided address
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Start listening on the socket with maximum backlog of 1 pending
    // connection
    if (listen(sock, backlog))
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return sock;
}

/**
 * Sets up a UDP server socket and binds it to the provided sockaddr_in address.
 *
 * @param addr The sockaddr_in structure representing the IP address and port of
 * the server.
 *
 * @return The file descriptor of the created UDP server socket.
 */
static int setup_udp_socket(struct sockaddr_in addr)
{
    const int enable_udp = 1;

    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1)
    {
        perror("socket(UDP)");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &enable_udp, sizeof(enable_udp)) == -1)
    {
        perror("setsockopt(UDP)");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    if (bind(udp_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind(UDP)");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    return udp_socket;
}

/**
 *  The program expects 3; otherwise, it returns EXIT_FAILURE.
 *
 *  Call as:
 *
 *  ./build/webserver self.ip self.port
 */
int main(int argc, char **argv)
{
    if (argc < 3 || argc > 4)
    {
        return EXIT_FAILURE;
    }

    if (argc == 4)
    {
        char *endptr;
        strncpy(self_ip,argv[1],INET_ADDRSTRLEN - 1);
        self_port = safe_strtoul(argv[2], NULL, 10, "Invalid self Port");
        self_id = safe_strtoul(argv[3], &endptr, 10, "Invalid self ID");
    }
    else
    {
        self_id = 0;
    }

    char *env;
    if ((env = getenv("PRED_ID")) != NULL)
    {
        pred_id = safe_strtoul(env, NULL, 10, "Invalid PRED_ID");
        have_pred = 1;
    }
    if ((env = getenv("PRED_IP")) != NULL)
    {
        strncpy(pred_ip, env, INET_ADDRSTRLEN - 1);
    }
    if ((env = getenv("PRED_PORT")) != NULL)
    {
        pred_port = safe_strtoul(env, NULL, 10, "Invalid PRED_PORT");
    }

    if ((env = getenv("SUCC_ID")) != NULL)
    {
        succ_id = safe_strtoul(env, NULL, 10, "Invalid SUCC_ID");
        have_succ = 1;
    }
    if ((env = getenv("SUCC_IP")) != NULL)
    {
        strncpy(succ_ip, env, INET_ADDRSTRLEN - 1);
    }
    if ((env = getenv("SUCC_PORT")) != NULL)
    {
        succ_port = safe_strtoul(env, NULL, 10, "Invalid SUCC_PORT");
    }

    struct sockaddr_in addr = derive_sockaddr(argv[1], argv[2]);

    // Set up a server socket.
    int server_socket = setup_server_socket(addr);
    //Set up a udp_socket.
    int udp_socket = setup_udp_socket(addr);

    if(have_succ)
    {
        char s_port [4];
        sprintf(s_port,"%d",succ_port);
        succ_addr = derive_sockaddr(succ_ip,s_port);
    }
    if(have_pred)
    {
        char p_port [4];
        sprintf(p_port,"%d",pred_port);
        pred_addr = derive_sockaddr(pred_ip,p_port);
    }

    // Create an array of pollfd structures to monitor sockets.
    struct pollfd sockets[3] = {
        {.fd = server_socket, .events = POLLIN},{.fd = -1, .events = 0},
        {.fd = udp_socket, .events = POLLIN}
    };

    struct connection_state state = {0};
    while (true)
    {

        // Use poll() to wait for events on the monitored sockets.
        int ready = poll(sockets, sizeof(sockets) / sizeof(sockets[0]), -1);
        if (ready == -1)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        // Process events on the monitored sockets.
        for (size_t i = 0; i < sizeof(sockets) / sizeof(sockets[0]); i += 1)
        {
            if (sockets[i].revents != POLLIN)
            {
                // If there are no POLLIN events on the socket, continue to the
                // next iteration.
                continue;
            }
            int s = sockets[i].fd;

            if (s == server_socket)
            {

                // If the event is on the server_socket, accept a new connection
                // from a client.
                int connection = accept(server_socket, NULL, NULL);
                if (connection == -1 && errno != EAGAIN &&
                    errno != EWOULDBLOCK)
                {
                    close(server_socket);
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    connection_setup(&state, connection);

                    // limit to one connection at a time
                    sockets[0].events = 0;
                    sockets[1].fd = connection;
                    sockets[1].events = POLLIN;
                }
            }
            else if(s == udp_socket)
            {   //handle incomming msg onto the UDP Socket
                uint8_t buffer[BUFSIZ];
                // clear the buffer
                memset(buffer, 0, BUFSIZ);
                
                //bytesread from recv() of udpsocket 
                ssize_t check = recv(s, buffer, 11, 0);
                
                if(check > 0 && buffer[0] == 0) //reply lookup
                {
                    uint8_t hash1 = buffer [1];
                    uint8_t hash2 = buffer [2];
                    uint16_t hash_id = ((uint16_t)hash1 << 8) | hash2;
                    uint8_t id1 = buffer [3];
                    uint8_t id2 = buffer [4];
                    uint16_t id_request = ((uint16_t)id1 << 8) | id2;
                    uint8_t port1 = buffer [9];
                    uint8_t port2 = buffer [10];
                    uint16_t port_request = ((uint16_t)port1 << 8) | port2;
                    reply_lookup(udp_socket,hash_id, id_request, port_request);
                }
                else if(check > 0 && buffer[0] == 1) //recv reply add the responsiple node into the lookuptable
                {
                    uint8_t port1 = buffer [9];
                    uint8_t port2 = buffer [10];
                    uint16_t port_reply = ((uint16_t)port1 << 8) | port2;
                    char *ip_reply = "127.0.0.1";
                    add_hash(lookuptable, port_reply, ip_reply);
                }
            }
            else
            {
                assert(s == state.sock);

                // Call the 'handle_connection' function to process the incoming
                // data on the socket.
                bool cont = handle_connection(&state,udp_socket);
                if (!cont)
                { // get ready for a new connection
                    sockets[0].events = POLLIN;
                    sockets[1].fd = -1;
                    sockets[1].events = 0;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
