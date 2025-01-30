#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>

#define MAX_MSG_SIZE 1500
#define CHUNK_SIZE 1496 // 1500 - 3 (type) - 1 (null terminator)

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <input.txt> <worker_port1> [<worker_port2> ...]\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    int num_workers = argc - 2;
    char **worker_ports = argv + 2;

    // Read input file
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open input file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = malloc(file_size + 1);
    if (!file_content)
    {
        perror("Failed to allocate memory");
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(file_content, 1, file_size, file);
    file_content[bytes_read] = '\0';
    fclose(file);

    // Prepare ZMQ context and sockets
    void *context = zmq_ctx_new();
    void *sockets[num_workers];
    for (int i = 0; i < num_workers; i++)
    {
        sockets[i] = zmq_socket(context, ZMQ_REQ);
        char addr[256];
        snprintf(addr, sizeof(addr), "tcp://localhost:%s", worker_ports[i]);
        zmq_connect(sockets[i], addr);
    }

    // Split file into chunks and send
    int num_chunks = (bytes_read + CHUNK_SIZE - 1) / CHUNK_SIZE;
    for (int i = 0; i < num_chunks; i++)
    {
        int offset = i * CHUNK_SIZE;
        int chunk_len = (i == num_chunks - 1) ? (bytes_read - offset) : CHUNK_SIZE;

        char message[MAX_MSG_SIZE];
        memcpy(message, "map", 3);
        memcpy(message + 3, file_content + offset, chunk_len);
        message[3 + chunk_len] = '\0'; // Null-terminate

        // Round-robin distribution
        int worker_idx = i % num_workers;
        zmq_send(sockets[worker_idx], message, 3 + chunk_len + 1, 0);

        // Wait for reply (REQ-REP pattern)
        char reply[MAX_MSG_SIZE];
        zmq_recv(sockets[worker_idx], reply, MAX_MSG_SIZE, 0);
    }

    // Cleanup
    free(file_content);
    for (int i = 0; i < num_workers; i++)
    {
        zmq_close(sockets[i]);
    }
    zmq_ctx_destroy(context);

    return 0;
}
