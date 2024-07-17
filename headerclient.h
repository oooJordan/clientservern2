#ifndef CLIENT_HEADER_H
#define CLIENT_HEADER_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096

// prototipi di funzione
int create_connection(const char *server_address, int server_port);
void upload_file_on_server(int socket_file_descriptor, char *path_opt_o, char *path_opt_f);
void download_file_from_server(int socket_file_descriptor, char *path_opt_o, char *path_opt_f);
void send_command(int socket_file_descriptor, char command, char* path_file_remoto);
void file_list_on_stout(int socket_file_descriptor, char *path_opt_f);
void create_dir(const char *percorso_directory);

#endif
