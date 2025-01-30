#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <workerport1> [<workerport2> ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_REP);
    int linger = 0;
    zmq_setsockopt(socket, ZMQ_LINGER, &linger, sizeof(linger)); // Prevent hangs on close

    // Bind to all specified ports
    for (int i = 1; i < argc; i++)
    {
        char addr[256];
        snprintf(addr, sizeof(addr), "tcp://*:%s", argv[i]);
        if (zmq_bind(socket, addr) != 0)
        {
            perror("zmq_bind failed");
            return EXIT_FAILURE;
        }
    }

    while (1)
    {
        char buffer[1500];
        int recv_size = zmq_recv(socket, buffer, sizeof(buffer), 0);
        if (recv_size <= 0)
            continue;

        // Extract message type (bytes 3-5)
        char type[4] = {0};
        if (recv_size >= 6)
            memcpy(type, buffer + 3, 3);

        if (strcmp(type, "map") == 0)
        {
            // Send empty response (6 bytes: 00 00 06 res\0)
            char reply[7] = {0x00, 0x00, 0x07, 'r', 'e', 's', '\0'};
            zmq_send(socket, reply, sizeof(reply), 0);
        }
        else if (strcmp(type, "rip") == 0)
        {
            char rip_reply[7] = {0x00, 0x00, 0x07, 'r', 'i', 'p', '\0'};
            zmq_send(socket, rip_reply, sizeof(rip_reply), 0);
            break;
        }
    }

    zmq_close(socket);
    zmq_ctx_destroy(context);
    return EXIT_SUCCESS;
}
