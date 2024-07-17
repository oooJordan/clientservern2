#ifndef CLIENT_SERVER_FUNZIONI_CONDIVISE_H
#define CLIENT_SERVER_FUNZIONI_CONDIVISE_H

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdio.h>

// prototipi di funzione
bool check_dir(char *path);
bool is_valid_port(int server_port);
bool is_valid_ip_address(const char *server_address);
char *extract_directory_path(char *file_path);
FILE* open_file(const char *path, const char *mode);

#endif
