#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <math.h>
//  gcc -o zmq_distributor zmq_distributor.c -pthread -lzmq
// ./zmq_distributor test_simple_text.txt 5555
#define MAX_MSG_SIZE 1500
#define MAX_WORKERS 8

typedef struct
{
    char *word;
    int count;
} WordCount;

typedef struct{
    char* message;
    void* socket;
    int chunk_size;
} Threaddata;

WordCount word_counts[100000];
int word_count_size = 0;

//pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

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
void* process_final_results(const char* data, int len){
    int i = 0;
    //int len = strlen(data);
    while (i < len) {
        if(!(isalpha(data[i])) && !(isdigit(data[i]))){
            return NULL;
        }
        int start = i;
        while (i < len && isalpha(data[i])) {
            i++;
        }
        int wordLen = i - start;
        char tempWord[wordLen +1];
        if (wordLen > 0) {
            strncpy(tempWord, data + start, wordLen); //strncpy not memory safe
            tempWord[wordLen] = '\0';
        }
        int count = 0;
        while (i < len && isdigit(data[i])) {
            count = count * 10 + (data[i] - '0');
            i++;
        }
        //pthread_mutex_lock(&lock);
        int found = 0;
        for (int j = 0; j < word_count_size; j++) {
            if (strcmp(word_counts[j].word, tempWord) == 0) {
                word_counts[j].count += count;
                found = 1;
                break;
            }
        }
        if (!found) {
            word_counts[word_count_size].word = (char *)malloc(wordLen + 1);
            strcpy(word_counts[word_count_size].word, tempWord);
            word_counts[word_count_size].count = count;
            word_count_size++;
        }
        //pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void *worker_simultaneous_task(void* Worker){
    Threaddata *temp = (Threaddata *) Worker;
    void *socket = temp->socket;
    char * message = temp->message;
    int size = temp->chunk_size;

    // Antworten empfangen und verarbeiten
    int message_beginning = 0;
    while (message_beginning < size) {
        int message_end = message_beginning + MAX_MSG_SIZE - 4;
        if (message_end >= size){
            message_end = size;
        }
        while (message_end > message_beginning && isalpha(message[message_end])) {
            message_end--;
        }

        // MAP-Phase
        char buffer_send[MAX_MSG_SIZE];
        //printf("%c", message[message_end]);
        int copy_map = snprintf(buffer_send, sizeof(buffer_send), "map%.*s", message_end-message_beginning, message+message_beginning);
        //printf("Worker sending chunk: [%d]\n", message_end-message_beginning);
        zmq_send(socket, buffer_send, copy_map + 1, 0);
        message_beginning = message_end + 1;

        //MAP recv
        char buffer[MAX_MSG_SIZE];
        int recv_bytes_map = zmq_recv(socket, buffer, MAX_MSG_SIZE, 0);
        //buffer[recv_bytes_map] = '\0';

        // Reduce-Phase
        char reduce_data[recv_bytes_map + 4];
        int copy_red = snprintf(reduce_data, sizeof(reduce_data), "red%s", buffer);
        zmq_send(socket, reduce_data, copy_red + 1, 0);

        //Reduce recv
        char new_buffer[MAX_MSG_SIZE];
        int recv_bytes_reduce = zmq_recv(socket, new_buffer, MAX_MSG_SIZE, 0);
        //new_buffer[recv_bytes_reduce] = '\0';

        process_final_results(new_buffer, recv_bytes_reduce);
    }
    return NULL;
    //pthread_exit(NULL);
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
    if(file == NULL){
        fprintf(stderr, "Failed to open File");
        return EXIT_FAILURE;
    }
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

    //pthread_t threads[MAX_WORKERS];
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
    int total_chunck_size = 0;
    Threaddata Workers[num_workers];
    for (int i = 0; i < num_workers; i++)
    {
        char *chunk = text + total_chunck_size;

        if (i == num_workers - 1) {
            chunk_size = file_size - total_chunck_size;
        }

        while (chunk_size > 0 && isalpha(chunk[chunk_size])) {
            chunk_size--;
        }

        total_chunck_size += chunk_size;

        char *message = malloc(chunk_size + 1);
        snprintf(message, chunk_size + 1, "%.*s", chunk_size, chunk);
        Workers[i].message = strdup(message);
        Workers[i].socket = sockets[i];
        Workers[i].chunk_size = chunk_size;
        worker_simultaneous_task((void*)&Workers[i]);

        free(message);
        free(Workers[i].message);
        //pthread_create(&threads[i], NULL, worker_simultaneous_task,(void*)&Workers[i]);
    }

    //Warten auf alle Workers
    //for (int i = 0; i < num_workers; i++)
    //{
    //    pthread_join(threads[i], NULL);
    //}


    // Sortieren und Ausgabe
    qsort(word_counts, word_count_size, sizeof(WordCount), compare);
    fprintf(stdout,"word,frequency\n");
    for (int i = 0; i < word_count_size; i++)
    {
        fprintf(stdout, "%s,%d\n", word_counts[i].word, word_counts[i].count);
    }

    //Worker herunterfahren
    for (int i = 0; i < num_workers; i++)
    {
        zmq_send(sockets[i], "rip", 4, 0);
        char buffer[MAX_MSG_SIZE];
        zmq_recv(sockets[i], buffer, MAX_MSG_SIZE, 0);
        zmq_close(sockets[i]);
    }

    zmq_ctx_destroy(context);
    free(text);
    return EXIT_SUCCESS;
}