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
#include <linux/limits.h>
#include <stdbool.h>


#define MAX_CLIENTS 10 // Massimo numero di client che possono essere gestiti contemporaneamente
#define BUFFER_SIZE 4096 // Dimensione del buffer

// Struct che rappresenta un lock associato a un file
typedef struct FileLock {
    char file[1024]; // Nome del file
    pthread_mutex_t lock; // Lock sul file
    int ref_count; // Conteggio di quanti thread utilizzano il lock
    struct FileLock *next; // Puntatore al prossimo FileLock (lista concatenata)
} FileLock;

// Variabili globali
int socket_file_descriptor;
pthread_mutex_t file_locks_mutex = PTHREAD_MUTEX_INITIALIZER;
FileLock *file_locks = NULL;


// Prototipi di funzione
void *thread_function(void *arg);
void command_client(int client_socket);
FileLock *get_file_lock(const char *file);
void release_file_lock(FileLock *lock);
void lockfile(const char *file);
void unlockfile(const char *file);
int create_dir(const char *ft_root_directory);

void rimuovi_accapo(char *stringa);
bool check_path(char* path);
int function_for_upload(int client_socket, char *path);
int function_for_download(int client_socket, char *path);
void function_to_send_file(int client_socket, char *path);
char *extract_directory_path(char *file_path);
bool check_dir(char *path);
#endif
