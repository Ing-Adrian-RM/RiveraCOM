#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <mysql/mysql.h>
#include <ncurses.h>

#define PORT 1234
#define BUFFER_SIZE 1024
#define SERVER "192.168.1.105"
#define USER "RIVERA_USER"
#define PASSWORD "8790"
#define DATABASE "RIVERACOM_DB"
#define MAX_CLIENTS 100

int client_count, client_socketasd, line=1, max_width, BUFFER_SEND_SIZE;
char *temp = NULL, clients[MAX_CLIENTS];
WINDOW *chat_win, *input_win;
pthread_mutex_t lock;

char buffer[BUFFER_SIZE];
int client, server;
struct sockaddr_in server_addr, client_addr;
socklen_t addr_size = sizeof(client_addr);

void executeQuery(const char *query) {
    MYSQL *conn = mysql_init(NULL);
    
    if (!mysql_real_connect(conn, LOCAL_HOST, USER, PASSWORD, DATABASE, 0, NULL, 0)) {
        fprintf(stderr, "Connection failed: %s\n", mysql_error(conn));
        exit(1);
    }

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Query error: %s\n", mysql_error(conn));
    }
    mysql_close(conn);
}

int userExists(const char *ip) {
    MYSQL *conn = mysql_init(NULL);
    MYSQL_RES *res;
    MYSQL_ROW row;
    int exists = 0;

    if (!mysql_real_connect(conn, LOCAL_HOST, USER, PASSWORD, DATABASE, 0, NULL, 0)) {
        fprintf(stderr, "Connection failed: %s\n", mysql_error(conn));
        exit(1);
    }

    char query[512];
    snprintf(query, sizeof(query), "SELECT * FROM users WHERE ip='%s'", ip);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Query error: %s\n", mysql_error(conn));
    } else {
        res = mysql_store_result(conn);
        if ((row = mysql_fetch_row(res))) {
            exists = 1;
        }
        mysql_free_result(res);
    }

    mysql_close(conn);
    return exists;
}

void registerUser(const char *ip, const char *name) {
    char query[512];
    
    snprintf(query, sizeof(query), 
             "INSERT INTO users (ip, name, balance, uploaded, downloaded) VALUES ('%s', '%s', 0, 0, 0)", 
             ip, name);
    executeQuery(query);
}

void *handleClient(void *arg) {
    int client_socket = *(int *)arg;
    char ip[INET_ADDRSTRLEN];
    char name[BUFFER_SIZE];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
    if (userExists(ip)) {
        char query[512];
        snprintf(query, sizeof(query), "SELECT name FROM users WHERE ip='%s'", ip);

        MYSQL *conn = mysql_init(NULL);
        MYSQL_RES *res;
        MYSQL_ROW row;

        if (!mysql_real_connect(conn, LOCAL_HOST, USER, PASSWORD, DATABASE, 0, NULL, 0)) {
            fprintf(stderr, "Connection failed: %s\n", mysql_error(conn));
            exit(1);
        }

        if (mysql_query(conn, query)) {
            fprintf(stderr, "Query error: %s\n", mysql_error(conn));
        } else {
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);
            char welcome_message[BUFFER_SIZE];
            snprintf(welcome_message, sizeof(welcome_message), "Welcome back, %s!\n", row[0]);
            strncpy(name, (const char *)row[0], sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';
            printf("%s Connected, IP: %s\n", name,ip);
            send(client_socket, welcome_message, strlen(welcome_message), 0);
            mysql_free_result(res);
        }

        mysql_close(conn);
    } else {
        send(client_socket, "You are not registered. Please enter a unique username:\n", 56, 0);
        int bytesReceived = 0;
        while (bytesReceived == 0) {
            bytesReceived = read(client_socket, name, sizeof(name));
        }
        name[bytesReceived] = '\0';
    
        // Remove newline character if present
        name[strcspn(name, "\n")] = 0;

        registerUser(ip, name);

        char success_message[BUFFER_SIZE + 100]; // Increase buffer size to handle long names
        snprintf(success_message, sizeof(success_message), "Registration successful! Welcome, %.900s!\n", name);
        printf("%s Connected, IP: %s\n", name,ip);
        send(client_socket, success_message, strlen(success_message), 0);
    }

    // Handle further communication with the client
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client, buffer, BUFFER_SIZE);

        if (bytes_read <= 0) {
            printf("Client %s disconnected.\n", name);
            close(client_socket);
            break;
        }

        printf("%s: %s\n", name, buffer);
        if (strncmp(buffer, "exit", 4) == 0) {
            printf("%s has disconnected.\n",name);
            close(client_socket);
            break;
        }

        // Simple chatbot responses
        char response[BUFFER_SIZE];
        if (strstr(buffer, "hello") || strstr(buffer, "hi")) {
            snprintf(response, sizeof(response), "Hello, %.900s! How can I assist you today?\n", name);
        } else if (strstr(buffer, "help")) {
            snprintf(response, sizeof(response), "Sure, %.900s! You can ask me about your account, balance, or other services.\n", name);
        } else if (strstr(buffer, "bye")) {
            snprintf(response, sizeof(response), "Goodbye, %.900s! Have a great day!\n", name);
            send(client_socket, response, strlen(response), 0);
            printf("%s has disconnected.\n", name);
            close(client_socket);
            break;
        } else {
            snprintf(response, sizeof(response), "I'm sorry, %.900s. I didn't understand that. Can you rephrase?\n", name);
        }

        send(client_socket, response, strlen(response), 0);
    }
    free(arg);
}

void *send_messages(void *arg) {
    char buffer[BUFFER_SIZE];

    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Eliminar el salto de línea
        send(client, buffer, strlen(buffer), 0);
        if (strncmp(buffer, "close", 5) == 0) {
            printf("Close connection...\n");
            close(client);
            exit(0);
        }
    }
    return NULL;
}

void add_client(char client_name){
    pthread_mutex_lock(&lock);
    if (client_count < MAX_CLIENTS) {
        clients[client_count++] = client_name;
    }
    pthread_mutex_unlock(&lock);
}

void remove_client(char client_name) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_count; i++) {
        if (clients[i] == client_name) {
            clients[i] = clients[client_count - 1];
            clients[client_count - 1] = '\0';
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
}

void broadcast_message(char client_name, const char *message) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_count; i++) {
        if (clients[i] != client_name) { 
            send(clients[i], message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&lock);
}

void direct_message(char sender, char receiver, const char *message) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_count; i++) {
        if (clients[i] != receiver) { 
            send(clients[i], message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&lock);
}

void setup() {
    server = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt error");
        close(server);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind error");
        close(server);
        exit(EXIT_FAILURE);
    }
    
}

int main() {
    setup();

    if (listen(server, 1) < 0) {
        perror("Listen error");
        close(server);
        exit(EXIT_FAILURE);
    }

    printf("Server open on port %d...\n", PORT);
    pthread_t send_thread;
    pthread_create(&send_thread, NULL, send_messages, NULL);
    pthread_detach(send_thread);

    while (1) {
        client = accept(server, (struct sockaddr *)&client_addr, &addr_size);	
        if (client < 0) {
            perror("Connection requested fail");
        } else {
            pthread_t client_thread;
            int *client_socket = malloc(sizeof(int));
            *client_socket = client;
            pthread_create(&client_thread, NULL, handleClient, client_socket);
            pthread_detach(client_thread);
        }
    }

    close(server);
    return 0;
}
