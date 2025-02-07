#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#define MAX_MSG_SIZE 1500
#define MAX_WORDS 10000

typedef struct
{
    char *key;
    char *value;
} KeyValue;

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
    int index = find_word_index(word_counts, word_count_size, word);
    if (index == -1)
    {
        word_counts[*word_count_size].key = strdup(word);
        word_counts[*word_count_size].value = (char*)calloc(256, sizeof(char));
        strcat(word_counts[*word_count_size].value, value);
        *word_count_size = *word_count_size + 1;
    }
    else
    {
        //realloc memory so that we can't get a buffer overflow in strcat
        if(strnlen(word_counts[index].value, 256) == 255){
            char *ptr = realloc(word_counts[index].value,256);
            if(ptr == NULL){
                fprintf(stderr, "Memory reallocation failed\n");
                return;
            }
            strcat(ptr, value); 
        }
        else{
           strcat(word_counts[index].value, value); 
        }
    }
    return;
}

// Verarbeitung von Map- und Reduce-Anfragen
void process_map(const char *text, char* output )
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

    size_t size = 0;
    size_t cpy = 0;
    for(size_t i = 0; i < word_count_size; i++){
        char buffer[MAX_MSG_SIZE-3];
        cpy = snprintf(buffer, sizeof(buffer), "%s%s", word_counts[i].key, word_counts[i].value);
        size += cpy;
        strncat(output, buffer, cpy);
    }
    output[size] = '\0';
    
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

    for (size_t i = 0; i < strnlen(data, MAX_MSG_SIZE -3) + 1; i++) {
        if (data[i] == '1') {
            count++;
        } else {
            if (count > 0) {
                char temp[5];
                snprintf(temp, 5,"%d" ,count);
                for(size_t k = 0; k < strnlen(temp,5); k++){
                    output[j++] = temp[k];
                }
                count = 0;
            }
            output[j++] = data[i];
        }
    }

    if (count > 0) {
        char temp[5];
        snprintf(temp, 5,"%d" ,count);
        for(size_t k = 0; k < strnlen(temp,5); k++){
            output[j++] = temp[k];
        }
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

    int rip = 0;
    int kill = 1;

    while (kill)
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
                char *output = calloc(MAX_MSG_SIZE -3, sizeof(char));
                process_map(payload,output);
                zmq_send(sockets[i], output, strlen(output) + 1, 0);
                free(output);
            }
            else if (strcmp(type, "red") == 0)
            {
                char *output = calloc(MAX_MSG_SIZE -3, sizeof(char));
                process_reduce(payload, output);
                zmq_send(sockets[i], output, strlen(output) + 1, 0);
                free(output);
            }
            else if (strcmp(type, "rip") == 0)
            {
                zmq_send(sockets[i], "rip", 4, 0);
                zmq_close(sockets[i]);
                rip++;
                if(rip == argc - 1){
                    kill = 0;
                }
            }
        }
    }
    zmq_ctx_destroy(context);
    return EXIT_SUCCESS;
}