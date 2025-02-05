#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
//  python3 -m pytest -s test/test_praxis3.py -k test_simple_text
#define MAX_MSG_SIZE 1500
#define MAX_WORDS 10000

typedef struct
{
    char *key;
    char *value;
} KeyValue;

//pthread_mutex_t word_counts_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

// Hilfsfunktionen
void generate_non_alpha_ascii(char *output) {
    int j = 0;
    for (int i = 1; i < 128; i++) { // ASCII-Werte von 0 bis 127
        if (!isalpha(i) && j < 76) {
            output[j++] = (char)i;
        }
    }
    output[j] = '\0';
}

void to_lower(char *str)
{
    for (; *str; ++str)
        *str = tolower(*str);
}

int find_word_index(KeyValue *word_counts, size_t *word_count_size, const char *word)
{
    for (size_t i = 0; i < *word_count_size; i++)
    {
        if (strcmp(word_counts[i].key, word) == 0)
        {
            return i;
        }
    }
    return -1;
}

void add_word(KeyValue *word_counts, size_t *word_count_size,const char *word, const char *value)
{
    //pthread_mutex_lock(&word_counts_mutex);
    int index = find_word_index(word_counts, word_count_size, word);
    if (index == -1)
    {
        word_counts[*word_count_size].key = strdup(word);
        word_counts[*word_count_size].value = strdup(value);
        *word_count_size = *word_count_size + 1;
    }
    else
    {
        //realloc memory so that we can't get a buffer overflow in strcat
        char *ptr = realloc(word_counts[index].value,1);
        if(ptr == NULL){
            fprintf(stderr, "Memory reallocation failed\n");
            return;
        }
        strcat(ptr, value); 
    }
    //pthread_mutex_unlock(&word_counts_mutex);
    return;
}

// Verarbeitung von Map- und Reduce-Anfragen
void process_map(const char *text, char* output ) //TODO MULTI-Thread DANGER
{
    KeyValue word_counts[MAX_MSG_SIZE];         
    size_t word_count_size = 0;
    size_t *word_ptr = &word_count_size; 

    char *copy = strdup(text);
    char *saveptr = NULL;
    char none_ascii[77];
    generate_non_alpha_ascii(none_ascii);
    char *token = __strtok_r(copy, none_ascii, &saveptr);
    output[0] = '\0';

    while (token != NULL)
    {
        to_lower(token);
        add_word(word_counts, word_ptr, token, "1");
        token = __strtok_r(NULL, none_ascii, &saveptr);
    }

    //pthread_mutex_lock(&output_mutex);
    size_t size = 0;
    for(size_t i = 0; i < word_count_size; i++){
        char buffer[MAX_MSG_SIZE-3];
        size += snprintf(buffer, sizeof(buffer), "%s%s", word_counts[i].key, word_counts[i].value);
        if(strlen(buffer) > (MAX_MSG_SIZE - 3 - strlen(output) - 1)){
            fprintf(stderr, "No enough space.\n");
            return;
        }
        strncat(output, buffer, MAX_MSG_SIZE - 3 - strlen(output) - 1);
    }
    output[size] = '\0';
    //pthread_mutex_unlock(&output_mutex);
    //free allocated memory
    for(size_t k = 0; k < word_count_size; k++){
        free(word_counts[k].key);
        free(word_counts[k].value);
    }
    free(copy);
    return;
}

void process_reduce(const char *data, char* output) {
    int count = 0;
    size_t j = 0;

    for (size_t i = 0; i < strlen(data); i++) {
        if (data[i] == '1') {
            count++;
        } else {
            if (count > 0) {
                output[j++] = count + '0';
                count = 0;
            }
            output[j++] = data[i];
        }
    }

    if (count > 0) {
        output[j++] = count + '0';
    }

    output[j] = '\0';
    return;
}


// Worker-Hauptfunktion
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <port1> <port2> ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    void *context = zmq_ctx_new();
    void *sockets[argc - 1];
    for (int i = 0; i < argc - 1; i++)
    {
        sockets[i] = zmq_socket(context, ZMQ_REP);
        char addr[256];
        snprintf(addr, sizeof(addr), "tcp://*:%s", argv[i + 1]);
        zmq_bind(sockets[i], addr);
    }

    while (1)
    {
        for (int i = 0; i < argc - 1; i++)
        {
            char buffer[MAX_MSG_SIZE];
            int size = zmq_recv(sockets[i], buffer, MAX_MSG_SIZE - 1, ZMQ_DONTWAIT);
            if (size == -1)
                continue;

            buffer[size] = '\0';
            char type[4];
            strncpy(type, buffer, 3);
            type[3] = '\0';
            char *payload = buffer + 3;

            if (strcmp(type, "map") == 0)
            {
                char output[MAX_MSG_SIZE-3];
                process_map(payload,output);
                zmq_send(sockets[i], output, strlen(output) + 1, 0);
            }
            else if (strcmp(type, "red") == 0)
            {
                char output[MAX_MSG_SIZE-3];
                process_reduce(payload, output);
                zmq_send(sockets[i], output, strlen(output) + 1, 0);
            }
            else if (strcmp(type, "rip") == 0)
            {
                zmq_send(sockets[i], "rip", 4, 0);
                zmq_close(sockets[i]);
                zmq_ctx_destroy(context);
                return EXIT_SUCCESS;
            }
        }
    }

    return EXIT_SUCCESS;
}