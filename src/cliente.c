#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PUERTO 12346
#define BUFFER_SIZE 1024

int cliente; // Variable global para el hilo

// Hilo para recibir mensajes del servidor
void *recibir_mensajes(void *arg) {
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_leidos = read(cliente, buffer, BUFFER_SIZE);

        if (bytes_leidos <= 0) {
            printf("Servidor desconectado.\n");
            close(cliente);
            exit(0);
        }

        printf("%s\n", buffer);
    }
    return NULL;
}

int main() {
    struct sockaddr_in servidor_dir;
    char buffer[BUFFER_SIZE];

    // Crear socket
    cliente = socket(AF_INET, SOCK_STREAM, 0);
    if (cliente == -1) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    servidor_dir.sin_family = AF_INET;
    servidor_dir.sin_port = htons(PUERTO);
    
    // Reemplazar con la IP del servidor
    if (inet_pton(AF_INET, "192.168.1.105", &servidor_dir.sin_addr) <= 0) {
        perror("Dirección inválida o no soportada");
        close(cliente);
        exit(EXIT_FAILURE);
    }

    // Conectar al servidor
    if (connect(cliente, (struct sockaddr *)&servidor_dir, sizeof(servidor_dir)) < 0) {
        perror("Error en la conexión");
        close(cliente);
        exit(EXIT_FAILURE);
    }

    printf("Conectado al servidor. Escribe 'salir' para terminar.\n");

    // Crear un hilo para recibir mensajes del servidor
    pthread_t hilo_recepcion;
    pthread_create(&hilo_recepcion, NULL, recibir_mensajes, NULL);

    // Bucle para enviar mensajes
    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Eliminar salto de línea

        send(cliente, buffer, strlen(buffer), 0);

        if (strncmp(buffer, "salir", 5) == 0) {
            printf("Cerrando conexión...\n");
            close(cliente);
            exit(0);
        }
    }

    return 0;
}

