#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ncurses.h>

#define PORT 1234
#define BUFFER_SIZE 1024
#define SERVER "192.168.1.105"

int client_socket; // Global variable for the thread
WINDOW *chat_win, *input_win;

// Thread to receive messages from the server
void *receive_messages() {
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE);

        if (bytes_read <= 0) {
            //printf("Server disconnected\n");
            wprintw(chat_win, "Server disconnected\n");
            wrefresh(chat_win);
            close(client_socket);
            endwin(); // Close ncurses
            exit(0);
        }

        //printf("%s\n", buffer);
        wprintw(chat_win, "%s\n", buffer);
        wrefresh(chat_win);
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];

    // Initialize ncurses
    initscr();
    cbreak();
    //noecho();
    curs_set(1);

    // Create windows for chat and input
    int height = LINES - 3;
    int width = COLS;
    chat_win = newwin(height, width, 0, 0);
    input_win = newwin(3, width, height, 0);

    // Draw borders
    box(chat_win, 0, 0);
    box(input_win, 0, 0);
    wrefresh(chat_win);
    wrefresh(input_win);

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure the server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    
    // Replace with the server's IP address
    if (inet_pton(AF_INET, SERVER, &server_address.sin_addr) <= 0) {
        perror("Invalid or unsupported address");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection error");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server. Type 'exit' to terminate.\n");

    // Create a thread to receive messages from the server
    pthread_t reception_thread;
    pthread_create(&reception_thread, NULL, receive_messages, NULL);

    // Loop to send messages
    while (1) {
        werase(input_win);
        box(input_win, 0, 0);
        //fgets(buffer, BUFFER_SIZE, stdin);
        wgetnstr(input_win, buffer, BUFFER_SIZE);
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character

        send(client_socket, buffer, strlen(buffer), 0);

        if (strncmp(buffer, "exit", 4) == 0) {
            //printf("Closing connection...\n");
            wprintw(chat_win, "Closing connection...\n");
            wrefresh(chat_win);
            close(client_socket);
            endwin(); // Close ncurses
            exit(0);
        }
    }

    endwin(); // Close ncurses
    return 0;
}
