#include "funzioni_condivise.h"

/**
 * verifica se il percorso è una directory
 * @param path
 * @return true se non è una directory, false altrimenti
 */
bool check_dir(char *path)
{
    struct stat st; // struttura per contenere le informazioni sul file/directory
    if (stat(path, &st) == 0) { //riempie la struttura st con le informazioni sul file/directory(zero=successo)
        if (S_ISDIR(st.st_mode)) { //S_ISDIR -> restituisce zero se il file è una directory
            //il percorso è una directory, non un file
            return false;
        }
    }
    return true;
}

/**
 * funzione per estrarre il percorso della directory dal percorso completo del file
 * @param file_path
 * @return puntatore alla stringa contenente il percorso della directory
 */
char *extract_directory_path(char *file_path) {
    char *dir_path = strdup(file_path); // duplica la stringa file_path in dir_path
    char *last_slash = strrchr(dir_path, '/'); //trova l'ultima occorrenza del carattere '/' in dir_path
    if (last_slash != NULL) { // se è stata trovata almeno una occorrenza di '/'
        *last_slash = '\0'; //viene sostituito il carattere '/' con '\0' per terminare la stringa
        return dir_path;
    }
    //se non è stata trovata alcuna occorrenza di '/', dealloca la memoria di dir_path e ritorna NULL
    free(dir_path);
    return NULL;
}

/**
 * funzione che verifica che il numero di porta inserito è valido
 * @param port
 * @return true = valido, false=non valido
 */
bool is_valid_port(int port) {
    if(port > 0 && port <= 65535){
        return true;
    }
    return false;
}

/**
 * funzione che verifica che l'indirizzo IP inserito è valido
 * @param ip
 * @return true = valido, false=non valido
 */
bool is_valid_ip_address(const char *ip) {
    struct sockaddr_in sa;
    // converte l'indirizzo IP in formato stringa in formato binario
    if (inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0) {
        return true; // Restituisce true se l'indirizzo IP è valido
    } else {
        return false; // Restituisce false se l'indirizzo IP non è valido
    }
}

/**
 * Apre un file con il percorso specificato e la modalità specificata
 * @param path
 * @param mode
 * @return puntatore al file aperto
 */
FILE* open_file(const char *path, const char *mode) {
    // apre il file specificato dal percorso 'path' con la modalità 'mode'
    FILE *file = fopen(path, mode);
    if (file == NULL) { //verifica se l'apertura del file è avvenuta con successo
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    return file;
}
