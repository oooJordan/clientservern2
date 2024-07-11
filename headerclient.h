#ifndef CLIENT_HEADER_H
#define CLIENT_HEADER_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096

int create_connection(const char *server_address, int server_port);
void upload_file_on_server(int socket_file_descriptor, char *o_path, char *f_path);
void download_file_from_server(int socket_file_descriptor, char *o_path, char *f_path);
FILE* open_file(const char *path, const char *mode);
void send_command(int socket_file_descriptor, char command, char* path_file_remoto);
void file_list_on_stout(int socket_file_descriptor, char *f_path);
void create_dir(const char *path);

#endif
