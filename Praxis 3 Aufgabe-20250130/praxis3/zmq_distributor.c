#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#define MAX_MSG_SIZE 1500
#define MAX_WORKERS 8

typedef struct
{
    char *word;
    int count;
} WordCount;

WordCount word_counts[100000];
int word_count_size = 0;

// Vergleichsfunktion für das Sortieren
int compare(const void *a, const void *b)
{
    WordCount *wc1 = (WordCount *)a;
    WordCount *wc2 = (WordCount *)b;
    if (wc1->count != wc2->count)
        return wc2->count - wc1->count;
    return strcmp(wc1->word, wc2->word);
}

// Verarbeitung der Reduce-Ergebnisse
void process_final_results(const char *data)
{
    char *copy = strdup(data);
    char *token = strtok(copy, ",");
    while (token != NULL)
    {
        char word[256];
        int count;
        if (sscanf(token, "%255[^:]:%d", word, &count) == 2)
        {
            int found = 0;
            for (int i = 0; i < word_count_size; i++)
            {
                if (strcmp(word_counts[i].word, word) == 0)
                {
                    word_counts[i].count += count;
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                word_counts[word_count_size].word = strdup(word);
                word_counts[word_count_size].count = count;
                word_count_size++;
            }
        }
        token = strtok(NULL, ",");
    }
    free(copy);
}

// Hauptfunktion des Distributors
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <file.txt> <port1> <port2> ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Datei einlesen
    FILE *file = fopen(argv[1], "r");
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    char *text = malloc(file_size + 1);
    fread(text, 1, file_size, file);
    text[file_size] = '\0';
    fclose(file);

    void *context = zmq_ctx_new();
    void *sockets[MAX_WORKERS];
    int num_workers = argc - 2;

    // Verbindung zu den Workern herstellen
    for (int i = 0; i < num_workers; i++)
    {
        sockets[i] = zmq_socket(context, ZMQ_REQ);
        char addr[256];
        snprintf(addr, sizeof(addr), "tcp://127.0.0.1:%s", argv[i + 2]);
        zmq_connect(sockets[i], addr);
    }

    // Text in Chunks aufteilen und senden
    int chunk_size = file_size / num_workers;
    for (int i = 0; i < num_workers; i++)
    {
        char *chunk = text + i * chunk_size;
        if (i == num_workers - 1)
            chunk_size = file_size - i * chunk_size;
        char message[MAX_MSG_SIZE];
        snprintf(message, sizeof(message), "map%.*s", chunk_size, chunk);
        zmq_send(sockets[i], message, strlen(message), 0);
    }

    // Antworten empfangen und verarbeiten
    for (int i = 0; i < num_workers; i++)
    {
        char buffer[MAX_MSG_SIZE];
        zmq_recv(sockets[i], buffer, MAX_MSG_SIZE, 0);
        process_final_results(buffer);
    }

    // Reduce-Phase
    char reduce_data[MAX_MSG_SIZE];
    snprintf(reduce_data, sizeof(reduce_data), "red");
    for (int i = 0; i < word_count_size; i++)
    {
        strcat(reduce_data, word_counts[i].word);
        strcat(reduce_data, ":");
        for (int j = 0; j < word_counts[i].count; j++)
            strcat(reduce_data, "1");
        strcat(reduce_data, ",");
    }

    for (int i = 0; i < num_workers; i++)
    {
        zmq_send(sockets[i], reduce_data, strlen(reduce_data), 0);
        char buffer[MAX_MSG_SIZE];
        zmq_recv(sockets[i], buffer, MAX_MSG_SIZE, 0);
        process_final_results(buffer);
    }

    // Sortieren und Ausgabe
    qsort(word_counts, word_count_size, sizeof(WordCount), compare);
    printf("word,frequency\n");
    for (int i = 0; i < word_count_size; i++)
    {
        printf("%s,%d\n", word_counts[i].word, word_counts[i].count);
    }

    // Worker herunterfahren
    for (int i = 0; i < num_workers; i++)
    {
        zmq_send(sockets[i], "rip", 3, 0);
        char buffer[MAX_MSG_SIZE];
        zmq_recv(sockets[i], buffer, MAX_MSG_SIZE, 0);
        zmq_close(sockets[i]);
    }

    zmq_ctx_destroy(context);
    free(text);
    return EXIT_SUCCESS;
}
