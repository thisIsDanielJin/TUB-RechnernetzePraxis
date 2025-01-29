#include <zmq.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#define MAX_MSG_SIZE 1500

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <worker port 1> [worker port 2] ...\n", argv[0]);
        return 1;
    }

    // 1. Erstelle ZMQ Context
    void *context = zmq_ctx_new();

    // 2. Erstelle Sockets für jeden Port
    // 3. Warte auf Nachrichten und verarbeite sie:
    //    - "map": Zähle Wörter
    //    - "red": Reduziere die Ergebnisse
    //    - "rip": Beende den Worker

    zmq_ctx_destroy(context);
    return 0;
}