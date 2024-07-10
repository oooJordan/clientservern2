#include "headerserver.h"

int socket_file_descriptor;

int main(int argc, char *argv[]) {
    int opt;
    char *server_address = NULL; // puntatore a carattere, inizialmente NULL, memorizza l'indirizzo del server
    char *ft_root_directory = NULL; // puntatore a carattere, inizialmente NULL, memorizza la directory radice del server FTP
    int server_port = 0; // numero di porta del server, inizialmente 0

    // getopt analizza gli argomenti della linea di comando
    while ((opt = getopt(argc, argv, "a:p:d:")) != -1) { // ':' indica che l'opzione richiede un argomento
        switch (opt) {
            case 'a':
                server_address = optarg; // optarg punta alla stringa dell'argomento dell'opzione corrente
                break;
            case 'p':
                server_port = atoi(optarg); // atoi converte la stringa in un intero (numero di porta)
                break;
            case 'd':
                ft_root_directory = optarg; // imposto la directory radice del server FTP 
                break;
            default:
                fprintf(stderr, "Usage: %s -a server_address -p server_port -d ft_root_directory\n", argv[0]);
                exit(EXIT_FAILURE); // stampo l'uso corretto e esco con EXIT_FAILURE se le opzioni sono errate
        }
    }

    // controllo se sono stati forniti tutti gli argomenti obbligatori
    if (server_address == NULL || server_port == 0 ) {
        fprintf(stderr, "Error: missing arguments\n");
        exit(EXIT_FAILURE); // stampo un errore e esco con EXIT_FAILURE in mancanza di informazioni obbligatorie
    }

    if(create_dir(ft_root_directory) != 0)
    {
        perror("Errore nella creazione delle directory");
        exit(EXIT_FAILURE);
    }

    if(chdir(ft_root_directory) != 0)
    {
        perror("errore durate il cambio di directory");
        //printf("non conosco %s", ft_root_directory);
        exit(EXIT_FAILURE);
    }

 
    struct sockaddr_in server_addr, client_addr;
    int optval = 1;

    printf("Creazione del socket...\n");
    // Crea un socket TCP(SOCK_STREAM) IPv4 (AF_INET)
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor < 0) {
        perror("Error during creation socket");
        exit(EXIT_FAILURE); // stampo un errore e esco con EXIT_FAILURE se non è possibile creare il socket
    }

    //BIND -> associazione di un indirizzo locale (IP e numero di porta) a un socket
    printf("Binding del socket...\n");
    // inizializzo la struttura server_addr per il bind del socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // famiglia degli indirizzi: IPv4
    server_addr.sin_addr.s_addr = inet_addr(server_address); // indirizzo IP del server
    server_addr.sin_port = htons(server_port); // porta del server

    /*
    - l'opzione SO_REUSEADDR | SO_REUSEPORT permette al socket di riutilizzare l'indirizzo e la porta 
                assegnati più rapidamente, riducendo il tempo di inattività e migliorando l'efficienza 
                dell'applicazione nella gestione delle connessioni di rete
    - SOL_SOCKET ->  livello di protocollo del socket
    - &optval -> puntatore al valore da impostare per l'opzione

    setsockopt -> permette di impostare delle opzioni su un socket già creato
    */

    if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval))) {
        perror("Error during reuse");
        exit(EXIT_FAILURE); // stampo un errore e esco con EXIT_FAILURE se non è possibile inserire opzioni al socket
    }


    //server_addr contiene quindi indirizzo IP e numero di porta

    // eseguo il bind
    if (bind(socket_file_descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { //socket_file_descriptor rappresenta il socket stesso all'interno del programma
        perror("Error during binding");
        exit(EXIT_FAILURE); // stampo un errore e esco con EXIT_FAILURE se non è possibile eseguire il bind del socket
    }

    printf("Avvio dell'ascolto sul socket...\n");
    // metto il socket in ascolto per le connessioni in entrata
    if (listen(socket_file_descriptor, 10) < 0) { //max 10 connessioni simultanee
        perror("Error listening for connection");
        exit(EXIT_FAILURE); // stampo un errore e esco con EXIT_FAILURE se non è possibile mettere il socket in ascolto
    }


    printf("Server in ascolto su %s:%d\n", server_address, server_port);

    // ciclo infinito: accetta le connessioni e gestisce i client
    while (1) {
        socklen_t client_addr_len = sizeof(client_addr); //inizializzo la lunghezza dell'indirizzo del client con sizeof(client_addr)
        int *client_socket=malloc(sizeof(int));
        //struct sockaddr_in ; //struttura che contiene le informazioni sull'indirizzo del client che si connette
        printf("In attesa di connessione...\n");

        // accetta una connessione in entrata
        //client_addr viene riempita con le informazioni sull'indirizzo del client
        *client_socket = accept(socket_file_descriptor, (struct sockaddr *)&client_addr, &client_addr_len);
        if (*client_socket < 0) { //se non riesce a connettersi stampa un messaggio di errore
            perror("Error accepting connection");
            exit(EXIT_FAILURE); // stampo un errore e esco con EXIT_FAILURE se non è possibile accettare la connessione
        }
        printf("lol\n");
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client_socket);
    }

    return 0;
}


void *handle_client(void *socket) {
    
    int client_socket = *(int *)socket;
    printf("dioscristo\n");

    free(socket);

    //char path_file[1024]; // Buffer per il nome del file
    char buffer[1024]; // Buffer per leggere i dati dal socket
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer)); // Numero di byte letti

    printf("%s\n", buffer);

    if (bytes_read < 0) {
        perror("Errore durante la lettura dal socket");
        close(client_socket);
        return NULL;
    }
    buffer[bytes_read] = '\0'; // Aggiungi terminatore di stringa
    if (strncmp(buffer, "w", 1) == 0) {
        // Funzione per scrivere un file sul server
        //function_w(client_socket, path_file);
    } else if (strncmp(buffer, "r ", 2) == 0) {
        // Funzione per leggere un file dal server
            printf("suca\n");
        function_r(client_socket, buffer+2);
        printf("comando r\n");
    } else if (strncmp(buffer, "l", 1) == 0) {
        // Funzione per visualizzare il contenuto di una cartella
        // function_to_send_file(client_socket, path_file);
    }
    
    close(client_socket);
    return NULL;
}

int create_dir(const char *ft_root_directory)
{
    char tmp[1024];
    char *p=NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", ft_root_directory);
    len=strlen(tmp);

    if(tmp[len -1] == '/')
    {
        tmp[len-1]=0;
    }

    for(p=tmp+1; *p;p++)
    {
        if(*p == '/'){
            *p=0;

            if(access(tmp, F_OK) != 0)
            {
                if(mkdir(tmp, 0777) != 0)
                {
                    perror("Errore");
                    return 1;
                }
            }
            *p='/';
        }
    }

    if(access(tmp, F_OK)!=0)
    {
        if(mkdir(tmp, 0777) != 0)
        {
            perror("Errore");
            return 1;
        }
    }
    printf("cartella %s creata", ft_root_directory);
    return 0;
}


char* rimuovi_accapo(char *str) {
    int i, j = 0;
    int len = strlen(str);

    // Rimuovi '\r' e '\n'
    for (i = 0; i < len; i++) {
        if (str[i] != '\r' && str[i] != '\n') {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';

    // Trova l'ultimo separatore di percorso '/'
    char *nomefile = strrchr(str, '/');
    if (nomefile != NULL) {
        return nomefile + 1;  // Restituisce la sottostringa dopo l'ultimo '/'
    } else {
        return str;  // Se non ci sono '/', restituisce l'intera stringa
    }
}

char* rimuovi_ultima_componente(char *path) {
    char *last_slash = strrchr(path, '/'); // Trova l'ultima occorrenza di '/'
    if (last_slash != NULL) {
        *last_slash = '\0'; // Imposta il carattere '/' a '\0' per terminare la stringa
    }
    return path;
}


// Funzione per dividere una stringa in token utilizzando il separatore specificato
void divide_per_slash(const char *str, char *tokens[], int *num_tokens) {
    char *str_copy = strdup(str); // Duplica la stringa originale poiché strsep modifica la stringa
    char *token;
    *num_tokens = 0;
    
    while ((token = strsep(&str_copy, "/")) != NULL) {
        if (*token == '\0') continue; // Ignora token vuoti
        tokens[(*num_tokens)++] = strdup(token); // Duplica il token per evitare che venga sovrascritto
    }
    free(str_copy); // Libera la memoria allocata da strdup
}

// Funzione per verificare se ogni token di path è presente nei token di base_dir
int is_subdirectory(const char *base_dir, const char *path) {
    char *base_tokens[100];
    int base_num_tokens = 0;
    divide_per_slash(base_dir, base_tokens, &base_num_tokens);

    char *path_tokens[100];
    int path_num_tokens = 0;
    divide_per_slash(path, path_tokens, &path_num_tokens);

    printf("base_dir: %s\n", base_dir);
    printf("path: %s\n", path);

    printf("Tokens in base_dir:\n");
    for (int i = 0; i < base_num_tokens; i++) {
        printf("%d: %s\n", i, base_tokens[i]);
    }

    printf("Tokens in path:\n");
    for (int i = 0; i < path_num_tokens; i++) {
        printf("%d: %s\n", i, path_tokens[i]);
    }

    // Verifica se ogni token di path è presente nei token di base_dir
    for (int i = 0; i < path_num_tokens; i++) {
        int found = 0;
        for (int j = 0; j < base_num_tokens; j++) {
            if (strcmp(path_tokens[i], base_tokens[j]) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            // Libera la memoria allocata per i token
            for (int k = 0; k < base_num_tokens; k++) {
                free(base_tokens[k]);
            }
            for (int k = 0; k < path_num_tokens; k++) {
                free(path_tokens[k]);
            }
            return 0;
        }
    }

    // Libera la memoria allocata per i token
    for (int i = 0; i < base_num_tokens; i++) {
        free(base_tokens[i]);
    }
    for (int i = 0; i < path_num_tokens; i++) {
        free(path_tokens[i]);
    }

    return 1; // Ogni token di path è presente nei token di base_dir
}

void function_r(int client_socket, char *ft_root_directory) {
    char cwd[PATH_MAX]; // Buffer per la directory corrente di lavoro
    FILE *file;

    // Rimuovere accapo dal percorso inviato dal client
    char* nomefile = rimuovi_accapo(ft_root_directory);
    char* path_noname = rimuovi_ultima_componente(ft_root_directory);
    
    // Ottenere la directory corrente di lavoro
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error getting current working directory");
        exit(EXIT_FAILURE);
    }
    
    printf("cwd -> %s\n", cwd);
    printf("ft_root_directory -> %s\n", ft_root_directory);
    printf("nomefile -> %s\n", nomefile);
    printf("path -> %s\n", path_noname);

    // Verifica se è una subdirectory
    if (is_subdirectory(cwd, path_noname)==0) {
        fprintf(stderr, "Accesso non consentito, %s non è una sottodirectory di %s\n", ft_root_directory, cwd);
        return;
    }

    // Risolvi il percorso completo
    char path_completo[PATH_MAX];
    if (realpath(nomefile, path_completo) == NULL) {
        perror("Error resolving real path");
        exit(EXIT_FAILURE);
    }

    file = fopen(path_completo, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    // Invio dei dati del file
    printf("Sending file: %s\n", path_completo);

    // Invio dei dati del file
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        ssize_t bytes_sent = send(client_socket, buffer, bytes_read, 0);
        if (bytes_sent < 0) {
            perror("Error sending file data");
            fclose(file);
            return;
        }
        if (bytes_sent != bytes_read) {
            fprintf(stderr, "Error: sent fewer bytes than read\n");
            fclose(file);
            return;
        }
    }

    if (ferror(file)) {
        perror("Error reading file");
    } else {
        printf("File %s inviato correttamente.\n", path_completo);
    }

    fclose(file);  // Chiudo il file alla fine della lettura
}



int fetch_and_send_file(int client_socket, const char *cwd, const char *remote_path) {
    char resolved_path[4096];

    // Risolvi il path assoluto del file richiesto
    if (realpath(remote_path, resolved_path) == NULL) {
        perror("realpath");
        send(client_socket, "FILE_NOT_FOUND\n", 15, 0);
        return 1;
    }

    printf("Percorso assoluto del file richiesto: %s\n", resolved_path);

    // Controlla se il path risolto è sotto la CWD
    if (is_subdirectory(cwd, resolved_path) == 0) {
        fprintf(stderr, "Access denied: '%s' is outside the server directory\n", resolved_path);
        send(client_socket, "ACCESS_DENIED\n", 14, 0);
        return 1;
    }

    printf("Il percorso %s è sotto la directory %s\n", resolved_path, cwd);

    printf("Apro il file: '%s'\n", resolved_path);

    char bufferOut[BUFFER_SIZE];
    FILE *fp = fopen(resolved_path, "rb");
    if (fp == NULL) {
        perror("fopen");
        send(client_socket, "FILE_NOT_FOUND\n", 15, 0);
        return 1;
    }

    while (!feof(fp)) {
        size_t bytes_read = fread(bufferOut, 1, sizeof(bufferOut), fp);
        if (bytes_read > 0) {
            if (send(client_socket, bufferOut, bytes_read, 0) == -1) {
                perror("send");
                fclose(fp);
                return 1;
            }
        }
    }
    fclose(fp);
    return 0;
}
