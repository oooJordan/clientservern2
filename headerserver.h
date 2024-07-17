#ifndef SERVER_HEADER_H
#define SERVER_HEADER_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/statvfs.h>
#include <libgen.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdbool.h>


#define MAX_CLIENTS 4 // massimo numero di client che possono essere gestiti contemporaneamente
#define BUFFER_SIZE 4096 // dimensione del buffer

// struct che rappresenta un lock associato a un file
typedef struct FileLock {
    char file[1024]; // nome del file
    pthread_mutex_t lock; // lock sul file
    int count_lock; // conteggio di quanti thread utilizzano il lock su un determinato file
    struct FileLock *next; // puntatore al prossimo FileLock (lista concatenata)
} FileLock;

// Variabili globali
int socket_file_descriptor;
pthread_mutex_t file_locks_mutex = PTHREAD_MUTEX_INITIALIZER;
FileLock *file_locks = NULL;

// prototipi di funzione
void *thread_function(void *arg);
void command_client(int client_socket);

int create_dir(const char *percorso_directory);

void rimuovi_accapo(char *stringa);
bool check_path(char* path);
int function_for_upload_w(int client_socket, char *path);
int function_for_download_r(int client_socket, char *path);
void function_to_send_file_l(int client_socket, char *path);

//lock
FileLock *get_file_lock(const char *file);
void release_file_lock(FileLock *lock);
void lockfile(const char *file);
void unlockfile(const char *file);
#endif
