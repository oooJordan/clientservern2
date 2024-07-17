#include "headerserver.h"
#include "funzioni_condivise.h"


int main(int argc, char *argv[]) {
    int opt; //variabile per memorizzare il carattere dell'opzione corrente restituito da getopt
    char *server_address = NULL; //indirizzo del server
    char *ft_root_directory = NULL; //directory radice del server
    int server_port = 0; //porta del server

    // analisi degli argomenti della linea di comando con getopt
    while ((opt = getopt(argc, argv, "a:p:d:")) != -1) {
        switch (opt) {
            case 'a':
                server_address = optarg; //imposta l'indirizzo del server
                break;
            case 'p':
                server_port = atoi(optarg); // converte la porta del server da stringa a intero
                break;
            case 'd':
                ft_root_directory = optarg; // imposta la directory radice del file transfer
                break;
            default:
                // stampa un messaggio sullo standard error se viene incontrata un'opzione non valida
                fprintf(stderr, "Usage: %s -a server_address -p server_port -d ft_root_directory\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    //argomenti essenziali
    if (server_address == NULL || server_port == 0) {
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

    //crea la directory radice se non esiste
    if(create_dir(ft_root_directory) != 0) {
        perror("Errore nella creazione delle directory");
        exit(EXIT_FAILURE);
    }

    //imposta la directory radice come current working directory
    if(chdir(ft_root_directory)!=0) {
        perror("Errore dirante il cambio della directory");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr; // struttura per memorizzare l'indirizzo del server
    //int optval = 1; // opzione per consentire il riutilizzo immediato dell'indirizzo e della porta

    printf("Creazione del socket...\n");
    // crea un socket TCP(SOCK_STREAM) IPv4 (AF_INET) con il protocollo predefinito (0)
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor < 0) {
        perror("Error during creation socket");
        exit(EXIT_FAILURE); // stampo un errore e esco con EXIT_FAILURE se non è possibile creare il socket
    }

    /*
    - l'opzione SO_REUSEADDR | SO_REUSEPORT permette al socket di riutilizzare l'indirizzo e la porta
                assegnati più rapidamente, riducendo il tempo di inattività e migliorando l'efficienza
                dell'applicazione nella gestione delle connessioni di rete
    - SOL_SOCKET ->  livello di protocollo del socket
    - &optval -> puntatore al valore da impostare per l'opzione

    setsockopt -> permette di impostare delle opzioni su un socket già creato


    if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval))) {
        perror("Error during reuse");
        exit(EXIT_FAILURE);
    }
*/
    printf("Binding del socket...\n");
    memset(&server_addr, 0, sizeof(server_addr)); // inizializzo la struttura server_addr per il bind del socket
    server_addr.sin_family = AF_INET; // famiglia degli indirizzi: IPv4
    server_addr.sin_addr.s_addr = inet_addr(server_address); // indirizzo IP del server
    server_addr.sin_port = htons(server_port); // porta del server
    //server_addr contiene quindi indirizzo IP e numero di porta
    //BIND -> associazione di un indirizzo locale (IP e numero di porta) a un socket
    if (bind(socket_file_descriptor, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { //socket_file_descriptor rappresenta il socket stesso all'interno del programma
        perror("Error during binding");
        exit(EXIT_FAILURE);
    }

    printf("Avvio dell'ascolto sul socket...\n");
    // metto il socket in ascolto per le connessioni in entrata
    if (listen(socket_file_descriptor, MAX_CLIENTS) < 0) { //max 4 connessioni simultanee
        perror("Error listening for connection");
        exit(EXIT_FAILURE);// stampo un errore e esco con EXIT_FAILURE se non è possibile mettere il socket in ascolto
    }

    printf("Server in ascolto su %s:%d\n", server_address, server_port);

    while (1) { //ciclo while rimane in ascolto per nuove connessioni
        int *client_socket = malloc(sizeof(int)); // alloca dinamicamente memoria per il file descriptor del client
        struct sockaddr_in client_addr; // struttura per memorizzare l'indirizzo del client e lunghezza
        socklen_t client_addr_len = sizeof(client_addr); //inizializzo la lunghezza dell'indirizzo del client

        // accetta una nuova connessione
        if((*client_socket = accept(socket_file_descriptor, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            perror("Error accepting connection"); // Stampa un messaggio di errore se accept() restituisce -1
            close(socket_file_descriptor);
            free(client_socket);
            exit(EXIT_FAILURE);
        }

        printf("Client connesso da %s\n", inet_ntoa(client_addr.sin_addr));

        //creazione di un thread per la gestione della connessione con il client
        pthread_t thread; //dichiara un thread
        pthread_create(&thread, NULL, thread_function, client_socket); // crea il thread passando il file descriptor del client
        pthread_detach(thread); // il thread creato terminerà autonomamente
    }

    return 0;
}
/**
 * ottiene il file descriptor del client che viene utilizzato per ricevere
 * un comando specifico dal client e agire di conseguenza
 * @param c_socket (file descriptor del client)
 */
//funzione che ritorna un void* e accetta un void*
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
        function_for_upload_w(client_socket, path);
        printf("buffer = '%s'\n", path);
    } else if(strncmp(buffer,"r ",2) == 0){
        //funzione per scrivere un file sul server
        function_for_download_r(client_socket, path);
        //printf("buffer = '%s'\n", path);
    } else if(strncmp(buffer,"l ",2) == 0){
        //funzione per scrivere un file sul server
        function_to_send_file_l(client_socket, path);
        //printf("buffer = '%s'\n", path);
    }

    close(client_socket);// chiudo la connessione del socket del client
    return;
}

/**
 * La funzione create_dir è progettata per creare una gerarchia di directory in un percorso specificato (percorso_directory)
 * @param percorso_directory
 * @return -1 errore, 0  successo
 */
int create_dir(const char *percorso_directory) {
    if (!percorso_directory){
        return -1; //verifica se il percorso_directory è nullo
    }
    char *percorso_duplicato = strdup(percorso_directory);//duplico il percorso_directory per lavorare su una copia
    char *punt_a_directory = percorso_duplicato;//puntatore per iterare atraverso il percorso_directory

    while (*punt_a_directory) { // itera fino alla fine del percorso_directory
        if (*punt_a_directory == '/') { // se il puntatore trova uno slash
            *punt_a_directory = '\0'; // termina temporaneamente la stringa per creare la directory
            if (mkdir(percorso_duplicato, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) { // crea la directory con permessi rwx per l'utente, il gruppo e altri
                struct stat st; // verifica se la creazione della directory ha fallito per un motivo diverso da EEXIST
                if (stat(percorso_duplicato, &st) != 0 || !S_ISDIR(st.st_mode)) {
                    fprintf(stderr, "Impossibile creare la directory %s\n", percorso_duplicato);
                    free(percorso_duplicato);
                    return -1;
                }
            }// ripristina il carattere slash nel percorso
            *punt_a_directory = '/';
        }// avanza nel percorso_directory
        punt_a_directory++;
    }
    // crea l'ultima directory nel percorso, se non esiste già
    if (mkdir(percorso_duplicato, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
        // verifica se la creazione della directory ha fallito per un motivo diverso da EEXIST
        struct stat st;
        if (stat(percorso_duplicato, &st) != 0 || !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Impossibile creare la directory %s\n", percorso_duplicato);
            free(percorso_duplicato);
            return -1;
        }
    }
    // libera la memoria allocata per il percorso_duplicato
    free(percorso_duplicato);
    printf("Percorso %s creato con successo\n", percorso_directory);
    return 0;
}

/**
 * funzione per rimuovere i caratteri di nuova linea da una stringa
 * @param stringa
 */
void rimuovi_accapo(char *stringa) {
    char *puntatore_a_stringa = stringa; // puntatore per la nuova stringa
    while (*stringa) {
        if (*stringa != '\r' && *stringa != '\n') {
            *puntatore_a_stringa++ = *stringa; // copia il carattere nella nuova stringa
        }
        stringa++;
    }
    *puntatore_a_stringa = '\0'; // termina la nuova stringa con il carattere nullo
}
/**
 * verifica se un il path è valido
 * @param path
 * @return true se il percorso è valido, false altrimenti
 */
bool check_path(char* path) {
    // verifica se il percorso remoto è assoluto (inizia con '/')
    if (path[0] == '/') {
        // è percorso assoluto
        fprintf(stderr, "Errore: è stato inserito un percorso assoluto\n");
        return false;
    }
    // il percorso è stato costruito correttamente
    return true;
}
/**
 * funzione che gestisce l'upload di un file sul server, dal client
 * @param client_socket
 * @param path
 * @return 0 successo, -1 insuccesso
 */
int function_for_upload_w(int client_socket, char *path) {
    if (!check_path(path)) { //verifica che sia un path valido
        send(client_socket, "PATH_NON_VALIDO",15, 0);
        close(client_socket);
        return -1;
    }

    //verifica che il percorso non sia una directory
    if(!check_dir(path)){
        send(client_socket, "IL_PATH_È_UNA_DIRECTORY",23, 0);
        close(client_socket);
        return -1;
    }

    // estrae il percorso della directory base (senza il nome del file)
    char* path_no_name = extract_directory_path(path);

    create_dir(path_no_name); // crea la/le directory se non esiste/esistono già
    printf("Ricevuto nome file remoto: %s\n", path);

    lockfile(path); // blocca il file per evitare accessi concorrenti
    printf("File locked\n");

    FILE *file = open_file(path, "wb"); //apre il file in modalità scrittura binaria
    send(client_socket, "OK", 2, 0); // invia conferma al client che è possibile iniziare il trasferimento dai dati

    char buffer_in[BUFFER_SIZE];
    ssize_t bytes_received;
    // Continua a ricevere i dati rimanenti, se ce ne sono
    while ((bytes_received = recv(client_socket, buffer_in, sizeof(buffer_in), 0)) > 0) {
        if (strlen(buffer_in) > 0){
            //scrivo i dati contenuti nel file
            fwrite(buffer_in, 1, bytes_received, file);
        }
    }
    
    // controllo degli errori di recv
    if (bytes_received < 0) {
        perror("Error receiving file data");
        fclose(file);
        unlockfile(path);
        close(client_socket);
        return -1;
    }
    // chiude il file dopo aver ricevuto tutti i dati
    fclose(file);
    close(client_socket);
    unlockfile(path); //sblocca il file
    printf("File unlocked\n");

    printf("File %s salvato correttamente.\n", path);

    return 0; //ritorna 0 per indicare successo
}

/**
 * La funzione function_for_download_r gestisce la richiesta di download di un file dal server al client
 * @param client_socket
 * @param path
 * @return 0 successo, -1 insuccesso
 */
int function_for_download_r(int client_socket, char *path) {
    if (!check_path(path)) { //verifica che sia un path valido
        send(client_socket, "PATH_NON_VALIDO",15, 0);
        close(client_socket);
        return -1;
    }
    if(!check_dir(path)) //verifica che il percorso non sia una directory
    {
        send(client_socket, "IL_PATH_È_UNA_DIRECTORY",23, 0);
        close(client_socket);
        return -1;
    }
    // blocca il file per evitare accessi concorrenti
    lockfile(path);
    printf("File locked\n");

    // apertura del file per la lettura in modalità binaria
    FILE *file = open_file(path, "rb");

    // invio dei dati del file
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (strlen(buffer) > 0){
            if (send(client_socket, buffer, bytes_read, 0) < 0) {
                perror("Error sending file data\n");
                fclose(file);
                close(client_socket);
                unlockfile(path);
                return -1; //errore
            }
        }
    }

    if (ferror(file)) {
        perror("Error reading file");
        fclose(file);
        unlockfile(path);
        return -1;
    } else {
        printf("File %s inviato correttamente.\n", path);
    }

    fclose(file);  // chiude il file alla fine della lettura
    unlockfile(path); // sblocca il file
    printf("File unlocked\n");

    return 0; // successo
}

/**
 * La funzione function_to_send_file_l invia al client l'elenco dei file presenti nella directory specificata
 * @param client_socket -> socket del client a cui inviare i file
 * @param path -> percorso della directory da cui ottenere l'elenco dei file
 */
void function_to_send_file_l(int client_socket, char *path) {
    if (!check_path(path)) { //verifica che sia un path valido
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

/**
 * get_file_lock cerca un lock associato a un file specifico.
 * Se non trova un lock esistente, ne crea uno nuovo
 * @param file
 * @return puntatore al lock trovato
 */
FileLock *get_file_lock(const char *file) {
    FileLock *lock;

    pthread_mutex_lock(&file_locks_mutex); // acquisisco il mutex file_locks_mutex e blocco tutti i thread che tentano di acquisire lo stesso
    //scorro lista dei FileLock
    for (lock = file_locks; lock != NULL; lock = lock->next) {
        if (strcmp(lock->file, file) == 0) { //se trovo un lock associato al file specificato
            lock->count_lock++; // incrementa il conteggio dei riferimenti al lock
            pthread_mutex_unlock(&file_locks_mutex); //rilascia il mutex
            return lock; //ritorna il puntatore al lock trovato
        }
    }
    // se il lock non è stato trovato, crea un nuovo FileLock
    lock = (FileLock *)malloc(sizeof(FileLock)); // alloca memoria per il nuovo lock
    if (lock == NULL) {
        pthread_mutex_unlock(&file_locks_mutex);
        return NULL;
    }
    // copia il percorso del file nel campo file del nuovo lock
    strncpy(lock->file, file, sizeof(lock->file) - 1);
    lock->file[sizeof(lock->file) - 1] = '\0'; //aggiungo terminatore di stringa per terminare correttamente il campo file
    pthread_mutex_init(&lock->lock, NULL); // inizializza il mutex associato al lock
    lock->count_lock = 1; // imposta il conteggio dei riferimenti a 1 (primo riferimento)
    lock->next = file_locks; // collega il nuovo lock alla lista dei lock esistenti
    file_locks = lock; // aggiorna il puntatore alla testa della lista di lock

    pthread_mutex_unlock(&file_locks_mutex); //rilascia il mutex
    return lock; //ritorna il puntatore al lock trovato
}

/**
 * La funzione release_file_lock gestisce il rilascio di un lock associato a un file
 * @param lock
 */
void release_file_lock(FileLock *lock) {
    pthread_mutex_lock(&file_locks_mutex);  // acquisisce il mutex globale per proteggere l'accesso alla lista dei lock dei file
    lock->count_lock--;  // decrementa il contatore di riferimento del lock
    if (lock->count_lock == 0) {  // se il contatore di riferimento è zero, il lock può essere rimosso dalla lista dei FileLock
        FileLock **p_a_p = &file_locks;  // inizializza un puntatore a puntatore al primo elemento della lista
        while (*p_a_p != lock) {  // scorre la lista finché non trova il lock
            p_a_p = &(*p_a_p)->next;  // aggiorna il puntatore per puntare al prossimo elemento della lista
        }
        /*
         * rimuove il lock dalla lista collegando il puntatore precedente (p_a_p) al
         * successivo (lock->next) per saltare il lock da rimuovere.
         */
        *p_a_p = lock->next;

        pthread_mutex_destroy(&lock->lock);  // distrugge il mutex del lock
        free(lock);
    }
    pthread_mutex_unlock(&file_locks_mutex);  // rilascia il mutex globale
}

/**
 * La funzione lockfile prende in input un file (file) e tenta di ottenere un lock su di esso
 * @param file
 */
void lockfile(const char *file) {
    FileLock *lock = get_file_lock(file); //richiede lock per il file
    if (lock != NULL) { //per assicurarsi che il lock sia stato ottenuto correttamente
        //passa l'indirizzo del mutex (&lock->lock) a pthread_mutex_lock che acquisisce il lock sul file
        pthread_mutex_lock(&lock->lock);
    }
}
/**
 * La funzione unlockfile prende in input un file (file) e tenta di rilasciare il lock su di esso
 * @param file
 */
void unlockfile(const char *file) {
    //chiama get_file_lock per ottenere il puntatore alla struttura FileLock associata al file
    FileLock *lock = get_file_lock(file);
    if (lock != NULL) {
        //chiama pthread_mutex_unlock passando l'indirizzo del mutex (&lock->lock) per rilasciare il lock sul file
        pthread_mutex_unlock(&lock->lock);
        release_file_lock(lock); //chiama release_file_lock(lock) per indicare che il lock può essere liberato
    }
}
