#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
//  python3 -m pytest -s test/test_praxis3.py -k test_simple_text
#define MAX_MSG_SIZE 1500
#define MAX_WORDS 10000
#define SEPARATORS " \t\n\r.,;:!?(){}[]<>\"'`~@#$%^&*-_=+/\\|1234567890"

typedef struct
{
    char *key;
    int value;
} KeyValue;

// Hashmap zur Speicherung der Wortfrequenzen
KeyValue word_counts[MAX_WORDS];
int word_count_size = 0;

// Hilfsfunktionen
void to_lower(char *str)
{
    for (; *str; ++str)
        *str = tolower(*str);
}

int find_word_index(const char *word)
{
    for (int i = 0; i < word_count_size; i++)
    {
        if (strcmp(word_counts[i].key, word) == 0)
        {
            return i;
        }
    }
    return -1;
}

void add_word(const char *word, const char *value)
{
    int index = find_word_index(word);
    if (index == -1)
    {
        word_counts[word_count_size].key = strdup(word);
        word_counts[word_count_size].value = strlen(value);
        word_count_size++;
    }
    else
    {
        word_counts[index].value += strlen(value);
    }
}

// Verarbeitung von Map- und Reduce-Anfragen
void process_map(const char *text)
{
    char *copy = strdup(text);
    char *token = strtok(copy, SEPARATORS);
    while (token != NULL)
    {
        to_lower(token);
        add_word(token, "1");
        token = strtok(NULL, SEPARATORS);
    }
    free(copy);
}

char *process_reduce(const char *data)
{
    char *copy = strdup(data);
    char *token = strtok(copy, ",");
    char *output = malloc(MAX_MSG_SIZE+1);
    output[0] = '\0';

    while (token != NULL)
    {
        char word[256];
        char value_str[256];
        if (sscanf(token, "%255[^:]:%255s", word, value_str) == 2)
        {
            int value = strlen(value_str);
            int index = find_word_index(word);
            if (index != -1)
            {
                word_counts[index].value += value;
            }
            else
            {
                add_word(word, value_str);
            }
        }
        token = strtok(NULL, ",");
    }

    for (int i = 0; i < word_count_size; i++)
    {
        char entry[512];
        snprintf(entry, sizeof(entry), "%s:%d,", word_counts[i].key, word_counts[i].value);
        strcat(output, entry);
    }

    free(copy);
    return output;
}

// Worker-Hauptfunktion
int main(int argc, char *argv[])
{
    //printf("[DEBUG] Worker started on port %s\n", argv[1]);
    if (argc < 2)
    {
        //fprintf(stderr, "Usage: %s <port1> <port2> ...\n", argv[0]);
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
                word_count_size = 0;
                memset(word_counts, 0, sizeof(word_counts));
                process_map(payload);
                char map_result[MAX_MSG_SIZE] = "";

                for (int k = 0; k < word_count_size; k++)
                {
                    char word[512];
                    word[511] = '\0';
                    snprintf(word, sizeof(word), "%s:", word_counts[k].key);
                    for (int j = 0; j < word_counts[k].value; j++) {
                        strcat(word, "1");
                    }
                    strcat(map_result, word);
                    strcat(map_result, ",");
                }
                map_result[MAX_MSG_SIZE-1] = '\0';
                //printf("SENDING MAP: %s\n\n\n\n\n\n",map_result);
                zmq_send(sockets[i], map_result, strlen(map_result), 0);
            }
            else if (strcmp(type, "red") == 0)
            {
                //printf("RECEIVED FIRST WORDS: %s\n\n\n\n\n\n",payload);
                word_count_size = 0;
                memset(word_counts, 0, sizeof(word_counts));
                char *result = process_reduce(payload);
                result[MAX_MSG_SIZE-1] = '\0';
                //printf("RESULT: %s\n\n\n\n\n\n",result);
                zmq_send(sockets[i], result, strlen(result), 0);
                free(result);
            }
            else if (strcmp(type, "rip") == 0)
            {
                zmq_send(sockets[i], "rip", 3, 0);
                zmq_close(sockets[i]);
                zmq_ctx_destroy(context);
                return EXIT_SUCCESS;
            }
        }
    }

    return EXIT_SUCCESS;
}
