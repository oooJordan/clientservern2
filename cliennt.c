#include "headerclient.h"

int main(int argc, char *argv[]) {
    int opt;
    char *server_address = NULL;
    char *f_path = NULL;
    char *o_path = NULL;
    int server_port = 0;
    char command_management = 0;

    // Analisi degli argomenti della linea di comando con getopt
    while ((opt = getopt(argc, argv, "a:p:f:o:wrl")) != -1) {
        switch (opt) {
            case 'a':
                server_address = optarg;
                break;
            case 'p':
                server_port = atoi(optarg);
                break;
            case 'f':
                f_path = optarg;
                break;
            case 'o':
                o_path = optarg;
                break;
            case 'w':
            case 'r':
            case 'l':
                if (command_management != 0) {
                    fprintf(stderr, "Error: use one of the commands -w, -r, or -l\n");
                    exit(EXIT_FAILURE);
                }
                command_management = opt;
                break;
            default:
                fprintf(stderr, "Usage: %s -w|-r|-l -a server_address -p server_port -f f_path [-o o_path]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Verifica che tutte le informazioni principali siano presenti
    if (server_address == NULL || server_port == 0 || command_management == 0) {
        fprintf(stderr, "Error: missing arguments\n");
        exit(EXIT_FAILURE);
    }

    if(!is_valid_port(server_port))
    {
        fprintf(stderr, "Error: porta non valida\n");
        exit(EXIT_FAILURE);
    }

    if(!is_valid_ip_address(server_address))
    {
        fprintf(stderr, "Error: indirizzo inserito non valido\n");
        exit(EXIT_FAILURE);
    }

    bool memory_allocated = false;
    if (f_path == NULL){
        if(command_management != 'l'){
            fprintf(stderr, "Error: missing arguments\n");
            exit(EXIT_FAILURE);
        } else {
            memory_allocated = true;
            f_path = malloc(sizeof(char)*PATH_MAX);
            strcpy(f_path, "");
        }
    }

    bool o_path_not_exist=false;
    // Se il percorso remoto non è fornito dall'utente, usa basename del percorso locale
    if (o_path == NULL) {
        o_path = malloc(sizeof(char)*250);
        strcpy(o_path, f_path);
        o_path_not_exist=true;
    }



    // Creazione della connessione TCP/IP al server
    int socket_file_descriptor = create_connection(server_address, server_port);

    switch (command_management) {
        case 'w':
            // Carica il file dal client al server
            upload_file_on_server(socket_file_descriptor, o_path, f_path);
            break;
        case 'r':
            // Scarica un file dal server al client
            download_file_from_server(socket_file_descriptor, o_path, f_path);
            break;
        case 'l':
            // Ottiene l'elenco dei file sul server
            file_list_on_stout(socket_file_descriptor, f_path);
            break;
        default:
            fprintf(stderr, "Error: invalid command\n");
            exit(EXIT_FAILURE);
    
    }

    if(o_path_not_exist)
    {
        free(o_path);
    }
    if(memory_allocated)
    {
        free(f_path);
    }
    close(socket_file_descriptor); // Chiude il descrittore del socket
    return 0;
}

// Funzione per validare il numero di porta
bool is_valid_port(int port) {
    return (port > 0 && port <= 65535);
}

// Funzione per validare l'indirizzo IP
bool is_valid_ip_address(const char *ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0;
}

// Crea una connessione TCP/IP al server
int create_connection(const char *server_address, int server_port) {
    int socket_file_descriptor;
    struct sockaddr_in server_addr;
    struct hostent *server;

    // Crea una socket di tipo SOCK_STREAM (TCP/IP) nel dominio AF_INET (IPv4)
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor < 0) {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    // Ottiene l'indirizzo IP del server a partire dal nome del server
    server = gethostbyname(server_address);
    if (server == NULL) {
        fprintf(stderr, "Error: no such host\n");
        exit(EXIT_FAILURE);
    }

    // Configurazione della struttura sockaddr_in per la connessione al server
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy((char *)&server_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    server_addr.sin_port = htons(server_port);

    // Connessione al server
    if (connect(socket_file_descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting");
        exit(EXIT_FAILURE);
    }

    return socket_file_descriptor;
}

// Invia un comando specifico al server tramite il socket identificato da socket_file_descriptor
void send_command(int client_socket, char command, char* path_file_remoto) {
    char send_path[PATH_MAX];
    snprintf(send_path, sizeof(send_path), "%c %s\n", command, path_file_remoto);
    printf("send path = '%s'\n", send_path);
    if (send(client_socket, &send_path, sizeof(send_path), 0) < 0) {
        perror("Error sending command");
        exit(EXIT_FAILURE);
    }
}

// Apre un file con il percorso specificato e la modalità specificata
FILE* open_file(const char *path, const char *mode) {
    FILE *file = fopen(path, mode);
    if (file == NULL) {
        perror("Error opening the client's file");
        exit(EXIT_FAILURE);
    }
    return file;
}

void create_dir(const char *path) {
    char buffer[256];
    char *ptr = buffer;
    
    // Copia il percorso nella variabile buffer
    strncpy(buffer, path, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // Controlla se c'è almeno una '/'
    if (strchr(buffer, '/') == NULL) {
        return;
    }

    // Scorri il buffer e crea le directory man mano che incontri '/'
    while ((ptr = strchr(ptr, '/')) != NULL) {
        *ptr = '\0';
        mkdir(buffer, S_IRWXU);
        *ptr = '/';
        ptr++;
    }

    // Crea l'ultima directory se necessario
    mkdir(buffer, S_IRWXU);
}


// Carica un file dal client al server
void upload_file_on_server(int socket_client_w, char *remote_path, char *local_path) {
    // Apro il file locale in modo da controllare se esiste prima di mandare la richiesta al server
    FILE *file = open_file(local_path, "rb");

    char command = 'w';
    send_command(socket_client_w, command, remote_path);


    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(socket_client_w, buffer, sizeof(buffer), 0);
    if(bytes_received<0) {
        perror("errore richiesta non inviata");
        close(socket_client_w);
        exit(EXIT_FAILURE);
    }

    if((strncmp(buffer, "OK", 2) == 0)) {
            printf("ok ricevuto dal server\n");
        }
    else{
        if (strncmp(buffer, "PATH_NON_VALIDO", 15) == 0){
                fprintf(stderr, "Il path '%s' non è valido\n", remote_path);
            if(strncmp(buffer, "IL_PATH_È_UNA_DIRECTORY", 23) == 0){
                fprintf(stderr, "Il path '%s' è una directory\n", remote_path);
            }else{
                perror("errore nella richiesta ");
            }
            fclose(file);
            close(socket_client_w);
            exit(EXIT_FAILURE);
        }
    }
   

    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(socket_client_w, buffer, bytes_read, 0) < 0) {
            perror("Errore nell'invio dei dati della PUT\n");
            fclose(file);
            close(socket_client_w);
        }
    }


    if(ferror(file)){
        perror("errore nella lettura del file\n");
    }else{
        printf("File %s arrivato come %s \n", local_path, remote_path);
    }

    fclose(file);
    printf("Upload complete.\n");
}


void download_file_from_server(int client_socket_r, char *local_path, char *remote_path) {
    char command = 'r';
    send_command(client_socket_r, command, remote_path);

    // Buffer per ricevere i dati
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Ricezione della risposta dal server
    bytes_received = recv(client_socket_r, buffer, sizeof(buffer), 0);

    // Gestione della risposta ricevuta
    if (bytes_received < 0) {
        perror("Errore durante la ricezione della risposta dal server");
        close(client_socket_r);
        exit(EXIT_FAILURE);
    }

    // Controllo del tipo di risposta
    if (strncmp(buffer, "PATH_NON_VALIDO", 15) == 0) {
        fprintf(stderr, "Il path '%s' non è valido\n", remote_path);
        close(client_socket_r);
        exit(EXIT_FAILURE);
    } else if (strncmp(buffer, "IL_PATH_È_UNA_DIRECTORY", 23) == 0) {
        fprintf(stderr, "Il path '%s' è una directory\n", remote_path);
        close(client_socket_r);
        exit(EXIT_FAILURE);
    }

    // Se non si è verificato un errore, si procede con la creazione del file locale
    create_dir(local_path);
    FILE *file = open_file(local_path, "wb");

    // Scrittura dei dati ricevuti nel file locale
    fwrite(buffer, 1, bytes_received, file);

    while ((bytes_received = recv(client_socket_r, buffer, sizeof(buffer), 0)) > 0) {
        if (strlen(buffer) > 0){
            fwrite(buffer, 1, bytes_received, file);
        }
    }

    if (ferror(file)) {
        perror("Errore durante la scrittura del file locale");
        fclose(file);
        exit(EXIT_FAILURE);
    } else {
        printf("File %s scaricato correttamente come %s.\n", remote_path, local_path);
    }

    fclose(file);
}

// Ottiene l'elenco dei file sul server e lo stampa sullo standard output
void file_list_on_stout(int socket_file_descriptor, char *f_path) {
    char command = 'l';
    send_command(socket_file_descriptor, command, f_path);

    char buffer[2048];
    int bytes_received;

    while ((bytes_received = recv(socket_file_descriptor, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';

        if (buffer[0] == '\0' && bytes_received == 1) {
            break;
        }

        printf("%s", buffer);
    }

    if (bytes_received < 0) {
        perror("Error receiving file list");
    } else {
        printf("File list received successfully.\n");
    }
}

