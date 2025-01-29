#include <zmq.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#define MAX_MSG_SIZE 1500

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <file.txt> <worker port 1> [worker port 2] ...\n", argv[0]);
        return 1;
    }

    // 1. Erstelle ZMQ Context
    void *context = zmq_ctx_new();

    // 2. Lese die Datei
    // 3. Verteile die Arbeit auf die Worker
    // 4. Sammle die Ergebnisse
    // 5. Sortiere und gebe das Ergebnis aus
    // 6. Sende "rip" an alle Worker

    zmq_ctx_destroy(context);
    return 0;
}