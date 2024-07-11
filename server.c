#include "headerserver.h"


int main(int argc, char *argv[]) {
    int opt;
    char *server_address = NULL; 
    char *ft_root_directory = NULL;
    int server_port = 0;

    while ((opt = getopt(argc, argv, "a:p:d:")) != -1) {
        switch (opt) {
            case 'a':
                server_address = optarg;
                break;
            case 'p':
                server_port = atoi(optarg);
                break;
            case 'd':
                ft_root_directory = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -a server_address -p server_port -d ft_root_directory\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (server_address == NULL || server_port == 0) {
        fprintf(stderr, "Error: missing arguments\n");
        exit(EXIT_FAILURE);
    }

    if(create_dir(ft_root_directory) != 0) {
        perror("Errore nella creazione delle directory");
        exit(EXIT_FAILURE);
    }

    if(chdir(ft_root_directory)!=0) {
        perror("Errore dirante il cambio della directory");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    int optval = 1;

    printf("Creazione del socket...\n");
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor < 0) {
        perror("Error during creation socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval))) {
        perror("Error during reuse");
        exit(EXIT_FAILURE);
    }

    printf("Binding del socket...\n");
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_address);
    server_addr.sin_port = htons(server_port);

    if (bind(socket_file_descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error during binding");
        exit(EXIT_FAILURE);
    }

    printf("Avvio dell'ascolto sul socket...\n");
    if (listen(socket_file_descriptor, 10) < 0) {
        perror("Error listening for connection");
        exit(EXIT_FAILURE);
    }

    printf("Server in ascolto su %s:%d\n", server_address, server_port);

    while (1) {
        int *client_socket = malloc(sizeof(int));
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        printf("In attesa di connessione...\n");

        if((*client_socket = accept(socket_file_descriptor, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
            perror("Error accepting connection");
            close(socket_file_descriptor);
            free(client_socket);
            exit(EXIT_FAILURE);
        }

        printf("Client connesso da %s\n", inet_ntoa(client_addr.sin_addr));
        
        pthread_t thread;
        pthread_create(&thread, NULL, thread_function, client_socket);
        pthread_detach(thread);
    }

    return 0;
}

void *thread_function(void *arg) {
    int client_socket = *((int *)arg);
    free(arg);
    command_client(client_socket);
    close(client_socket);
    return NULL;
}


//funzione per ricevere un comando specifico dal client eagire di conseguenza
void command_client(int client_socket) {
    char buffer[1024];
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer));
    printf("comando %s\n", buffer);

    if(bytes_read<0)
    {
        perror("Errore durante la lettura del socket");
        close(client_socket);
        return;
    }
    buffer[bytes_read]='\0';

    char* path = buffer + 2;

    rimuovi_accapo(path);

    if(strncmp(buffer,"w ",2) == 0){
        //funzione per scrivere un file sul server
        function_for_upload(client_socket, path);
        printf("buffer = '%s'\n", path);
    } else if(strncmp(buffer,"r ",2) == 0){
        //funzione per scrivere un file sul server
        function_for_download(client_socket, path);
        //printf("buffer = '%s'\n", path);
    } else if(strncmp(buffer,"l ",2) == 0){
        //funzione per scrivere un file sul server
        function_to_send_file(client_socket, path);
        //printf("buffer = '%s'\n", path);
    }

    close(client_socket);// chiudo la connessione del socket del client
    return;
}

int create_dir(const char *path)
{
    if(path == NULL){
        return 0;
    }
    char tmp[1024];
    char *p=NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len=strlen(tmp);

    if(tmp[len-1] == '/'){
        tmp[len-1]=0;
    }

    for(p=tmp+1; *p; p++){
        if(*p == '/'){
            *p=0;

            if(access(tmp, F_OK)!=0){
                if(mkdir(tmp, 0777)!=0){
                    perror("Errore nella creazione delle directory");
                    return 1;
                }
            }
            *p='/';
        }
    }

    if(access(tmp, F_OK)!=0){
        if(mkdir(tmp, 0777)!=0)
        {
            perror("Errore nella creazione delle directory");
            return 1;
        }
    }

    printf("cartella %s creata\n", path);
    return 0;
}

// Funzione per rimuovere i caratteri di nuova linea da una stringa e restituire una nuova stringa
void rimuovi_accapo(char *stringa) {
    int len = strlen(stringa);
    int j = 0;
    for (int i = 0; i < len; i++) {
        if (stringa[i] != '\r' && stringa[i] != '\n') {
            stringa[j++] = stringa[i];
        }
    }
    stringa[j] = '\0'; // Termina la nuova stringa
}

bool check_path(char* path) {

    // Verifica se il percorso remoto è assoluto (inizia con '/')
    if (path[0] == '/') {
        // Percorso assoluto
        fprintf(stderr, "Errore: è stato inserito un percorso assoluto\n");
        return false;
    }
    // Il percorso è stato costruito correttamente
    return true;
}

// Verifica se è una directory il percorso
bool check_dir(char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) { //S_ISDIR -> restituisce zero se il file è una directory
            //il percorso è una directory, non un file
            return false;
        }
    }
    return true;
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

// Funzione per estrarre il percorso della directory dal percorso completo del file
char *extract_directory_path(char *file_path) {
    char *dir_path = strdup(file_path);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';
        return dir_path;
    }

    free(dir_path);
    return NULL;
}

int function_for_upload(int client_socket, char *path) {
    if (!check_path(path)) {
        send(client_socket, "PATH_NON_VALIDO",15, 0);
        close(client_socket);
        return 1;
    }

    if(!check_dir(path))
    {
        send(client_socket, "IL_PATH_È_UNA_DIRECTORY",23, 0);
        close(client_socket);
        return 1;
    }

    // Creo la directory base se non esiste già
    char* path_no_name = extract_directory_path(path);

    create_dir(path_no_name);
    printf("Ricevuto nome file remoto: %s\n", path);

    lockfile(path);
    printf("File locked\n");

    FILE *file = open_file(path, "wb");

    send(client_socket, "OK", 2, 0);

    char buffer_in[BUFFER_SIZE];
    ssize_t bytes_received;
    // Continua a ricevere i dati rimanenti, se ce ne sono
    while ((bytes_received = recv(client_socket, buffer_in, sizeof(buffer_in), 0)) > 0) {
        if (strlen(buffer_in) > 0){
            fwrite(buffer_in, 1, bytes_received, file);
        }
    }

    // Controllo degli errori di recv
    if (bytes_received < 0) {
        perror("Error receiving file data");
        fclose(file);
        unlockfile(path);
        close(client_socket);
        return 1;
    }
    // Chiudi il file dopo aver ricevuto tutti i dati
    fclose(file);
    close(client_socket);
    unlockfile(path);

    printf("File %s salvato correttamente.\n", path);
    printf("File unlocked\n");

    return 0;
}


/*
La funzione function_for_download:
- gestisce la richiesta di download di un file dal server al client
 */

// Funzione per gestire il download dei file
int function_for_download(int client_socket, char *path) {
    if (!check_path(path)) {
        send(client_socket, "PATH_NON_VALIDO",15, 0);
        close(client_socket);
        return 1;
    }

    if(!check_dir(path))
    {
        send(client_socket, "IL_PATH_È_UNA_DIRECTORY",23, 0);
        close(client_socket);
        return 1;
    }

    // Debug: stampa il percorso completo del file che si sta cercando di aprire
    printf("Percorso remoto ricevuto dal client: %s\n", path);

    // Blocco del file
    lockfile(path);
    printf("File locked\n");

    // Apertura del file per la lettura in modalità binaria
    FILE *file = open_file(path, "rb");

    // Invio dei dati del file
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (strlen(buffer) > 0){
            printf("numeri dei byte inviati -> %ld\n", bytes_read);
            if (send(client_socket, buffer, bytes_read, 0) < 0) {
                perror("Error sending file data\n");
                fclose(file);
                close(client_socket);
                unlockfile(path); // Assicurati di sbloccare il file anche in caso di errore
                return 1; // Restituisce un valore intero in caso di errore
            }
        }
    }

    if (ferror(file)) {
        perror("Error reading file");
        fclose(file);
        unlockfile(path); // Assicurati di sbloccare il file anche in caso di errore
        return 1; // Restituisce un valore intero in caso di errore
    } else {
        printf("File %s inviato correttamente.\n", path);
    }

    fclose(file);  // Chiudo il file alla fine della lettura
    unlockfile(path);
    printf("File unlocked\n");

    return 0; // Restituisce un valore intero di successo
}


/*
La funzione function_to_send_file:
- invia al client l'elenco dei file presenti nella directory specificata 
- invia i dati al client tramite il socket specificato.
- termina l'invio con un segnale di terminazione e chiude la connessione del socket.
- gestisce errori di apertura del file/pipe e di invio dei dati
1) client_socket -> socket del client a cui inviare i dati.
2) path -> percorso della directory da cui ottenere l'elenco dei file.
 */

void function_to_send_file(int client_socket, char *path) {
    if (!check_path(path)) {
        send(client_socket, "PATH_NON_VALIDO",15, 0);
        close(client_socket);
        return ;
    }
    char command[1024]; // buffer per memorizzare il comando da eseguire
    char buffer[2048]; // buffer per memorizzare i dati letti dal comando
    int bytes_sent; // variabile per memorizzare il numero di byte inviati

    // comando per elencare tutti i file nella directory specificata, escludendo le sottodirectory
    snprintf(command, sizeof(command), "ls -p %s | grep -v /", path);
    // eseguo il comando e apro una pipe per leggere l'output
    FILE *fl = popen(command, "r");

    //controllo se la pipe è stata aperta correttamente
    if (fl == NULL) {
        // stampo un messaggio di errore se non è stato possibile aprire la pipe e termino la funzione
        fprintf(stderr, "Error: could not open the pipe for the output\n");
        return;
    }

    // leggo output del comando e lo invio al client
    while (fgets(buffer, sizeof(buffer), fl) != NULL) {
        // invio l'intero buffer 
            bytes_sent = send(client_socket, buffer, strlen(buffer), 0);
            //controllo se l'invio dei dati ha avuto successo
            if (bytes_sent < 0) {
                perror("Error sending file data");//invio messaggio  di errorein caso di fallimento
                pclose(fl); //chiudo la pipe
                return; //termino la funzione
            }
    }

    pclose(fl); //chiudo la pipe

    //invio un segnale di terminazione
    send(client_socket, "\0", 1, 0);

    // chiudo la connessione del socket del client dopo aver inviato tutti i dati
    close(client_socket);
}


//questa funzione cerca un lock associato a un file specifico. Se non trova un lock esistente, ne crea uno nuovo
FileLock *get_file_lock(const char *file) {
    FileLock *lock;

    pthread_mutex_lock(&file_locks_mutex); // acquisisco il mutex file_locks_mutex e blocco tutti i thread che tentano di acquisire lo stesso
    for (lock = file_locks; lock != NULL; lock = lock->next) {
        if (strcmp(lock->file, file) == 0) {
            lock->ref_count++;
            pthread_mutex_unlock(&file_locks_mutex);
            return lock;
        }
    }

    lock = (FileLock *)malloc(sizeof(FileLock));
    if (lock == NULL) {
        pthread_mutex_unlock(&file_locks_mutex);
        return NULL;
    }
    strncpy(lock->file, file, sizeof(lock->file) - 1);
    lock->file[sizeof(lock->file) - 1] = '\0';
    pthread_mutex_init(&lock->lock, NULL);
    lock->ref_count = 1;
    lock->next = file_locks;
    file_locks = lock;

    pthread_mutex_unlock(&file_locks_mutex);
    return lock;
}

void release_file_lock(FileLock *lock) {
    pthread_mutex_lock(&file_locks_mutex);  // Acquisisce il mutex globale per proteggere l'accesso alla lista dei lock dei file
    lock->ref_count--;  // Decrementa il contatore di riferimento del lock
    if (lock->ref_count == 0) {  // Se il contatore di riferimento è zero, il lock può essere rimosso
        FileLock **pp = &file_locks;  // Inizializza un puntatore a puntatore al primo elemento della lista
        while (*pp != lock) {  // Scorre la lista finché non trova il lock
            pp = &(*pp)->next;  // Aggiorna il puntatore per puntare al prossimo elemento della lista
        }
        *pp = lock->next;  // Rimuove il nodo lock dalla lista
        pthread_mutex_destroy(&lock->lock);  // Distrugge il mutex del lock
        free(lock);  // Libera la memoria allocata per il lock
    }
    pthread_mutex_unlock(&file_locks_mutex);  // Rilascia il mutex globale
}


void lockfile(const char *file) {
    FileLock *lock = get_file_lock(file);
    if (lock != NULL) {
        pthread_mutex_lock(&lock->lock);
    }
}

void unlockfile(const char *file) {
    FileLock *lock = get_file_lock(file);
    if (lock != NULL) {
        pthread_mutex_unlock(&lock->lock);
        release_file_lock(lock);
    }
}
/*

esempio

./muFTserver -a 127.0.0.5 -p 8080 -d /miao/bau/cip
------------------------------------------------------
getopt trova -a

-> opt impostato su a
-> optarg punta a 127.0.0.5
-> server address impostato a 127.0.0.5
-----------------------------------------------
getopt trova -p

-> opt impostato su p
-> optarg punta a 8080
->atoi(optarg) converte in intero 8080 e assegna valore a server_port
-------------------------------------------------------

getopt trova -d

-> opt impostato su d
-> optarg punta a /miao/bau/cip
-> server address impostato a /miao/bau/cip


-----------------------------------------------------

UTILI

-----------------------------------------------
ASCOLTO

funzione di ascolto, ascolta la connessione su un socket e contrassegna il 
socket a cui fa riferimento il socket_file_descriptor

backlog-> numero di connessioni simultanee che un sistema può gestire
int listen(int socket_file_descriptor, int backlog)
------------------------------------------------
ACCETTAZIONE

funzione accettazione
newsockfd= accept(socket_file_descriptor, (struct sockaddr *)&addr, &addrlen)

memorizza il file descriptor nella variabile newsockfd che è del tipo int
tutte le comunicazioni vengono effettuate sul newsockfd
la funzione accept attende la funzione di connessione che viene generata dal lato client
-------------------------------------------------
LETTURA

int read(newsockfd, buffer, buffer_size)

buffer è la stringa che viene passata cioè il messaggio
il nostro messaggio non può essere maggiore di buffer_size
------------------------------------------------
SCRITTURA

int write(newsockfd, buffer, buffer_size)

------------------------------------------------

PERMESSI

0700 -> 

Il primo numero 7 in ottale corrisponde a 111 in binario. 
Questo significa:
Il proprietario ha permessi di lettura (r), scrittura (w) e esecuzione (x).
Il secondo e il terzo numero 0 in ottale corrispondono a 000 in binario. Questo significa:
Il gruppo e tutti gli altri utenti non hanno alcun permesso (---).

--------------------------------------------------
Spiegazione Dettagliata di (struct sockaddr *)&client_addr

1) Struttura struct sockaddr_in:

- struct sockaddr_in è una struttura utilizzata per rappresentare un indirizzo IP e 
    una porta nella programmazione di rete in C. È specificamente progettata per supportare indirizzi IPv4.

2) Variabile client_addr di tipo struct sockaddr_in:

-client_addr è una variabile di tipo struct sockaddr_in che viene utilizzata per contenere le 
informazioni sull'indirizzo del client che si connette al server. Questo include l'indirizzo IP e la porta del client.


3) Cast (struct sockaddr *)&client_addr:

- Quando esegui il cast (struct sockaddr *)&client_addr, stai reinterpretando il puntatore della 
variabile client_addr in un puntatore di tipo struct sockaddr *. 
Questo è necessario quando devi passare client_addr a funzioni che accettano un parametro di tipo 
generico struct sockaddr *, anziché struct sockaddr_in *.


4) Utilizzo nei Parametri delle Funzioni di Rete:

- Le funzioni come bind(), accept(), connect(), ecc., accettano un parametro di tipo struct sockaddr * per 
rappresentare l'indirizzo del socket.
Poiché client_addr è di tipo struct sockaddr_in, per utilizzarla con queste funzioni è necessario 
eseguire il cast a struct sockaddr *.

*/
