#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>

#define SERIAL_PORT "/dev/ttyUSB0"
#define BAUDRATE B9600

int serial_fd;  // Descriptor del puerto serie

// Función para configurar el puerto serie
int configure_serial(const char *port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("Error abriendo el puerto serie");
        return -1;
    }
    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, BAUDRATE);
    cfsetospeed(&options, BAUDRATE);
    
    options.c_cflag = CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &options);
    return fd;
}

// Hilo para recibir datos
void *receive_thread(void *arg) {
    char buffer[256];
    int n;
    while (1) {
        n = read(serial_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("\n📩 Recibido: %s\n", buffer);
        }
        usleep(100000);  // Pausa para no sobrecargar
    }
}

// Función para enviar datos
void send_data() {
    char message[256];
    while (1) {
        printf("✉️ Escribe un mensaje: ");
        fgets(message, sizeof(message), stdin);
        write(serial_fd, message, strlen(message));
    }
}

int main() {
    serial_fd = configure_serial(SERIAL_PORT);
    if (serial_fd == -1) return 1;

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_thread, NULL);
    
    send_data();
    
    close(serial_fd);
    return 0;
}
