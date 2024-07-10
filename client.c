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
    if (server_address == NULL || server_port == 0 || f_path == NULL || command_management == 0) {
        fprintf(stderr, "Error: missing arguments\n");
        exit(EXIT_FAILURE);
    }

    bool o_path_not_exist=false;
    // Se il percorso remoto non è fornito dall'utente, usa basename del percorso locale
    if (o_path == NULL) {
        char *o_path = malloc(sizeof(char)*250);
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

    close(socket_file_descriptor); // Chiude il descrittore del socket
    return 0;
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
void send_command(int socket_file_descriptor, char command, char* path_file_remoto) {
    char send_path[PATH_MAX];
    snprintf(send_path, sizeof(send_path), "%c %s\n", command, path_file_remoto);
    if (send(socket_file_descriptor, &send_path, sizeof(send_path), 0) < 0) {
        perror("Error sending command");
        exit(EXIT_FAILURE);
    }
}

// Apre un file con il percorso specificato e la modalità specificata
FILE* open_file(const char *path, const char *mode) {
    FILE *file = fopen(path, mode);
    if (file == NULL) {
        perror("Error opening file");
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
void upload_file_on_server(int socket_file_descriptor, char *o_path, char *f_path) {
    FILE *file = open_file(f_path, "rb");
    if (file == NULL) {
        perror("Error opening local file");
        exit(EXIT_FAILURE);
    }
    char command = 'w';
    send_command(socket_file_descriptor, command, o_path);
    printf("f_path -> %s\n", f_path);
    printf("o_path -> %s\n", o_path);


    char buffer_ricezione[BUFFER_SIZE];
    ssize_t bytes_received = recv(socket_file_descriptor, buffer_ricezione, sizeof(buffer_ricezione), 0);
    if(bytes_received<0)
    {
        perror("errore richiesta non inviata");
        close(socket_file_descriptor);
        exit(EXIT_FAILURE);
    }else if(strncmp(buffer_ricezione, "OK", 2)==0)
    {
        printf("ok ricevuto");
    }else{
        perror("errore nella richiesta ");
        close(socket_file_descriptor);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(socket_file_descriptor, buffer, bytes_read, 0) < 0) {
            perror("Error sending file data");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }
    if(ferror(file)){
        perror("errore nella lettura del file\n");
    }else{
        printf("File %s arrivato come %s \n", f_path, o_path);
    }

    fclose(file);
    printf("Upload complete.\n");
}
void download_file_from_server(int socket_file_descriptor, char *o_path, char *f_path) {
    // Invio del comando al server (opzionale se già implementato)
    char command = 'r';
    send_command(socket_file_descriptor, command, f_path);


    create_dir(o_path);
    // Apertura del file locale per la scrittura in modalità binaria
    FILE *file = fopen(o_path, "wb");
    if (file == NULL) {
        perror("Error opening local file");
        exit(EXIT_FAILURE);
    }

    // Ricezione dei dati del file dal server e scrittura nel file locale
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv(socket_file_descriptor, buffer, sizeof(buffer), 0)) > 0) {
        size_t bytes_written = fwrite(buffer, 1, bytes_received, file);
        if (bytes_written < bytes_received) {
            perror("Error writing to local file");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_received < 0) {
        perror("Error receiving file data");
        exit(EXIT_FAILURE);
    } else {
        printf("File %s scaricato correttamente come %s.\n", f_path, o_path);
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

