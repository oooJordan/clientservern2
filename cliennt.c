#include "headerclient.h"
#include "funzioni_condivise.h"

int main(int argc, char *argv[]) {
    int opt; //variabile per memorizzare il carattere dell'opzione corrente restituito da getopt
    char *server_address = NULL; //indirizzo del server
    char *path_opt_f = NULL; //percorso del file path_opt_f
    char *path_opt_o = NULL; //percorso del file path_opt_o
    int server_port = 0; //porta del server
    char command_management = 0; // variabile per gestire il comando -w, -r, -l

    // analisi degli argomenti della linea di comando con getopt
    while ((opt = getopt(argc, argv, "a:p:f:o:wrl")) != -1) {
        switch (opt) {
            case 'a':
                server_address = optarg; // imposta l'indirizzo del server
                break;
            case 'p':
                server_port = atoi(optarg); // imposta la porta del server
                break;
            case 'f':
                path_opt_f = optarg; // imposta il percorso del file path_opt_f
                break;
            case 'o':
                path_opt_o = optarg; // imposta il percorso del file path_opt_o
                break;
            case 'w':
            case 'r':
            case 'l':
                if (command_management != 0) {
                    fprintf(stderr, "Error: use one of the commands -w, -r, or -l\n");
                    exit(EXIT_FAILURE);
                }
                command_management = opt; // imposta il comando -w, -r, o -l corrente
                break;
            default:
                //in caso di opzione non riconosciuta
                fprintf(stderr, "Usage: %s -w|-r|-l -a server_address -p server_port -f path_opt_f [-o path_opt_o]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // verifica che tutte le informazioni principali siano presenti
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
    if (path_opt_f == NULL){ // se il percorso del path_opt_f non è stato fornito dall'utente
        if(command_management != 'l'){ // verifica se il comando non è di tipo 'l'
            fprintf(stderr, "Error: missing arguments\n");
            exit(EXIT_FAILURE);
        } else {
            // altrimenti, se il comando è di tipo 'l'
            memory_allocated = true;
            path_opt_f = malloc(sizeof(char) * 1024); //alloca memoria per path_opt_f
            strcpy(path_opt_f, ""); //inizializza path_opt_f con una stringa vuota
        }
    }

    bool path_opt_o_not_exists = false;
    // Se path_opt_o non è fornito dall'utente
    if (path_opt_o == NULL) {
        path_opt_o = malloc(sizeof(char) * 250); //alloco memoria per path_opt_o
        strcpy(path_opt_o, path_opt_f); //copio il path di path_opt_f in path_opt_o
        path_opt_o_not_exists=true;
    }

    // creazione della connessione TCP/IP al server
    int socket_file_descriptor = create_connection(server_address, server_port);

    switch (command_management) {
        case 'w':
            // carica il file dal client al server
            upload_file_on_server(socket_file_descriptor, path_opt_o, path_opt_f);
            break;
        case 'r':
            // scarica un file dal server al client
            download_file_from_server(socket_file_descriptor, path_opt_o, path_opt_f);
            break;
        case 'l':
            // ottiene l'elenco dei file sul server
            file_list_on_stout(socket_file_descriptor, path_opt_f);
            break;
        default:
            fprintf(stderr, "Error: invalid command\n");
            exit(EXIT_FAILURE);
    
    }


    //se è stata allocata memoria la libero
    if(path_opt_o_not_exists){
        free(path_opt_o);
    }
    if(memory_allocated){
        free(path_opt_f);
    }
    close(socket_file_descriptor); // chiude il descrittore del socket
    return 0;
}

/**
 * stabilisce una connessione TCP/IP con il server
 * @param server_address
 * @param server_port
 * @return file descriptor del client
 */
int create_connection(const char *server_address, int server_port) {
    int socket_file_descriptor;
    struct sockaddr_in server_addr; //struct per l'indirizzo IPv4 del server
    struct hostent *server; //struct per informazioni su server (nome e indirizzi)

    // Crea una socket di tipo SOCK_STREAM (TCP/IP) nel dominio AF_INET (IPv4)
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor < 0) {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    // Ottiene l'indirizzo IP del server a partire dal nome del server
    server = gethostbyname(server_address); //restituisce un puntatore a struct hostent
    if (server == NULL) {
        fprintf(stderr, "Error: no such host\n");
        exit(EXIT_FAILURE);
    }

    // inizializzazione della struttura sockaddr_in per la connessione al server
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; //imposta famiglia di indirizzi IPv4
    memcpy((char *)&server_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length); // copia l'indirizzo IP del server da server->h_addr a server_addr.sin_addr.s_addr
    server_addr.sin_port = htons(server_port); //imposta la porta del server

    // connessione al server utilizzando la socket creata
    if (connect(socket_file_descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting");
        exit(EXIT_FAILURE);
    }
    // restituisce il descrittore della socket per la connessione stabilita
    return socket_file_descriptor;
}

/**
 * Invia un comando specifico al server tramite il socket identificato da client_socket
 * @param client_socket
 * @param command
 * @param path_file_remoto
 */
void send_command(int client_socket, char command, char* path_file_remoto) {
    char send_path[1024]; //buffer che contiene la stringa da inviare al server
    // costruisce la stringa da inviare al server con il comando e il percorso del file remoto
    snprintf(send_path, sizeof(send_path), "%c %s\n", command, path_file_remoto);
    //invia al serverla stringa costruita
    if (send(client_socket, &send_path, sizeof(send_path), 0) < 0) {
        perror("Error sending command");
        exit(EXIT_FAILURE);
    }
}

/**
 * la funzione crea le directory necessarie, se non esistono
 * @param path
 */
void create_dir(const char *path) {
    if(path == NULL){
        return;
    }
    char buffer[256]; // buffer per memorizzare il percorso
    char *ptr = buffer; // puntatore per scorrere il buffer
    
    // copia il percorso nella variabile buffer
    strncpy(buffer, path, sizeof(buffer) - 1);


    buffer[sizeof(buffer) - 1] = '\0'; //terminatore nullo

    // scorre il buffer e crea le directory man mano che incontri '/'
    while ((ptr = strchr(ptr, '/')) != NULL) {
        *ptr = '\0'; // termina la stringa temporaneamente
        mkdir(buffer, S_IRWXU); // crea la directory con i permessi dell'utente (lettura, scrittura, esecuzione)
        *ptr = '/'; // ripristina la '/'
        ptr++; // avanza al prossimo nome situato dopo '/'
    }
    // crea l'ultima directory, se necessario
    mkdir(buffer, S_IRWXU);
}

/**
 * L'opzione -w viene utilizzata per caricare un file dal client al server
 * @param socket_client_w
 * @param remote_path
 * @param path_opt_f
 */
void upload_file_on_server(int socket_client_w, char *remote_path, char *path_opt_f) {
    // apre il file locale in modo da controllare se esiste prima di mandare la richiesta al server
    FILE *file = open_file(path_opt_f, "rb");

    char command = 'w'; // Invio il comando 'w' e il percorso remoto al server
    send_command(socket_client_w, command, remote_path);


    char buffer[BUFFER_SIZE]; //ricevo la risposta dal server per la richiesta
    ssize_t bytes_received = recv(socket_client_w, buffer, sizeof(buffer), 0);
    if(bytes_received<0) {
        perror("errore richiesta non inviata");
        close(socket_client_w);
        exit(EXIT_FAILURE);
    }
    //se ricevo OK vuol dire che il server ha ricevuto correttamente il percorso
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
    size_t bytes_read; //dopo aver ricevuto l'OK, invio il contenuto del file al server
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(socket_client_w, buffer, bytes_read, 0) < 0) {
            perror("Errore nell'invio dei dati\n");
            fclose(file);
            close(socket_client_w);
        }
    }
    // verifica errori nella lettura del file
    if(ferror(file)){
        perror("errore nella lettura del file\n");
    }else{
        printf("File %s arrivato come %s \n", path_opt_f, remote_path);
    }
    fclose(file);
    
}
/**
 * L'opzione -r viene utilizzata per richiedere un file dal server e salvarlo localmente nel client
 * @param client_socket_r
 * @param local_path
 * @param path_opt_f
 */
void download_file_from_server(int client_socket_r, char *local_path, char *path_opt_f) {
    char command = 'r'; // Invio il comando 'r' e il percorso remoto al server
    send_command(client_socket_r, command, path_opt_f);


    // buffer per ricevere i dati
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    // ricezione della risposta dal server
    bytes_received = recv(client_socket_r, buffer, sizeof(buffer), 0);

    // gestione della risposta ricevuta
    if (bytes_received < 0) {
        perror("Errore durante la ricezione della risposta dal server");
        close(client_socket_r);
        exit(EXIT_FAILURE);
    }

    // controllo del tipo di risposta
    if (strncmp(buffer, "PATH_NON_VALIDO", 15) == 0) {
        fprintf(stderr, "Il path '%s' non è valido\n", path_opt_f);
        close(client_socket_r);
        exit(EXIT_FAILURE);
    } else if (strncmp(buffer, "IL_PATH_È_UNA_DIRECTORY", 23) == 0) {
        fprintf(stderr, "Il path '%s' è una directory\n", path_opt_f);
        close(client_socket_r);
        exit(EXIT_FAILURE);
    }
    // Se non si è verificato un errore controllo che non sia una directory
    if(!check_dir(local_path)){
        close(client_socket_r);
        exit(EXIT_FAILURE);
    }
    // creo la directory base se non esiste già
    char* path_no_name = extract_directory_path(local_path);
    
    create_dir(path_no_name); //creo eventuali cartelle
    FILE *file = open_file(local_path, "wb"); //apro il file in modalità scrittura

    // scrittura dei dati ricevuti nel file locale
    fwrite(buffer, 1, bytes_received, file);

    //scrivo fin quando mi arrivano byte
    while ((bytes_received = recv(client_socket_r, buffer, sizeof(buffer), 0)) > 0) {
        if (strlen(buffer) > 0){
            fwrite(buffer, 1, bytes_received, file);
        }
    }

    if (ferror(file)) { //controllo degli errori
        perror("Errore durante la scrittura del file locale");
        fclose(file);
        exit(EXIT_FAILURE);
    } else {
        printf("File %s scaricato correttamente come %s.\n", path_opt_f, local_path);
    }

    fclose(file);
}

/**
 * L'opzione -l ottiene l'elenco dei file che si trovano in path_opt_f sul server e li stampa sullo standard output
 * @param socket_file_descriptor
 * @param path_opt_f
 */
void file_list_on_stout(int socket_file_descriptor, char *path_opt_f) {
    char command = 'l'; // Invio il comando 'l' e il percorso remoto al server
    send_command(socket_file_descriptor, command, path_opt_f);

    char buffer[2048];
    ssize_t bytes_received;
    //ricezione dei nomi dei file
    while ((bytes_received = recv(socket_file_descriptor, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        //se abbiamo ricevuto esattamente un byte ed è proprio un carattere di terminazione siamo arrivati al termine
        if (buffer[0] == '\0' && bytes_received == 1) {
            break; //usciamo dal while
        }
        printf("%s", buffer); //stampo nomi dei file
    }
    if ((strncmp(buffer, "PATH_NON_VALIDO", 15) == 0) || (bytes_received < 0)) {
        fprintf(stderr, "Il path '%s' non è valido\n", path_opt_f);
    } else {
        printf("File list received successfully.\n");
    }
}
