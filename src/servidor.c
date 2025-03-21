#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <mysql/mysql.h>

#define PORT 1234
#define BUFFER_SIZE 1024
#define SERVER "192.168.8.100"
#define USER "RIVERA_USER"
#define PASSWORD "8790"
#define DATABASE "RIVERACOM_DB"

char buffer[BUFFER_SIZE];
int client, server;
struct sockaddr_in server_addr, client_addr;
socklen_t addr_size = sizeof(client_addr);


// Hilo para enviar mensajes al cliente
void *send_messages(void *arg) {
    char buffer[BUFFER_SIZE];

    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Eliminar el salto de línea
        send(client, buffer, strlen(buffer), 0);
        if (strncmp(buffer, "exit", 4) == 0) {
            printf("Close connection...\n");
            close(client);
            exit(0);
        }
    }
    return NULL;
}

void executeQuery(const char *query) {
    MYSQL *conn = mysql_init(NULL);

    if (!mysql_real_connect(conn, SERVER, USER, PASSWORD, DATABASE, 0, NULL, 0)) {
        fprintf(stderr, "Connection fail: %s\n", mysql_error(conn));
        exit(1);
    }

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Query error: %s\n", mysql_error(conn));
    }
    mysql_close(conn);
}

void handleClient(int client_socket) {
    char buffer[256];
    int bytesReceived;

    while ((bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytesReceived] = '\0';
        executeQuery(buffer);
        send(client_socket, "OK\n", 3, 0);
    }
    close(client_socket);
}

void setup() {
    // Crear socket
    server = socket(AF_INET, SOCK_STREAM, 0);

    // Habilitar la opción SO_REUSEADDR para reutilizar la dirección
    int opt = 1;
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt error");
        close(server);
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Enlazar el socket
    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind error");
        close(server);
        exit(EXIT_FAILURE);
    }
}

int main() {
    setup();

    // Escuchar conexiones
    if (listen(server, 1) < 0) {
        perror("Listen error");
        close(server);
        exit(EXIT_FAILURE);
    }

    printf("Server open in port %d...\n", PORT);
    client = accept(server, (struct sockaddr *)&client_addr, &addr_size);	
    if (client < 0) {
        perror("Connection requested fail");
        close(server);
        exit(EXIT_FAILURE);
    } else {
        printf("Client descriptor: %d\n", client);
        printf("Connection success with  %s\n", inet_ntoa(client_addr.sin_addr));
    }

    pthread_t send_thread;
    pthread_create(&send_thread, NULL, send_messages, NULL);

    // Bucle de recepción de mensajes del cliente
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client, buffer, BUFFER_SIZE);

        if (bytes_read <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        printf("%s\n", buffer);
        if (strncmp(buffer, "exit", 4) == 0) {
            printf("Connection close by client.\n");
            break;
        }
    }

    // Cerrar conexiones
    close(client);
    close(server);

    return 0;
}

