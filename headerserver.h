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
#define THREAD_POOL_SIZE 10 // Dimensione del pool di thread
#define BUFFER_SIZE 4096 // Dimensione del buffer

// Struct che memorizza le informazioni necessarie per gestire la connessione e le operazioni del client
typedef struct {
    int client_socket; // File descriptor associato al socket del client
    char *ft_root_directory; // Puntatore a una stringa che rappresenta la root directory del server FTP
} client_data;

// Struct che rappresenta un lock associato a un file
typedef struct FileLock {
    char file[1024]; // Nome del file
    pthread_mutex_t lock; // Lock sul file
    int ref_count; // Conteggio di quanti thread utilizzano il lock
    struct FileLock *next; // Puntatore al prossimo FileLock (lista concatenata)
} FileLock;

// Variabili globali
client_data client_queue[MAX_CLIENTS]; // Coda circolare di clienti da gestire
int client_count = 0;; // Numero attuale di client nella coda
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex per garantire l'accesso sicuro alla coda
sem_t queue_sem; // Semaforo per notificare l'arrivo di nuovi client nella coda
pthread_mutex_t file_locks_mutex = PTHREAD_MUTEX_INITIALIZER;
FileLock *file_locks = NULL;


// Prototipi di funzione
void *thread_function(void *arg);
void command_client(int client_socket, const char *ft_root_directory);
FileLock *get_file_lock(const char *file);
void release_file_lock(FileLock *lock);
void lockfile(const char *file);
void unlockfile(const char *file);
int create_dir(const char *ft_root_directory, bool main_folder_server);

void rimuovi_accapo(char *stringa);
bool check_path(int client_socket, char* path);
int function_for_upload(int client_socket, char *path);
int function_for_download(int client_socket, char *path);
void function_to_send_file(int client_socket, char *path);
char *extract_directory_path(char *file_path);
#endif
