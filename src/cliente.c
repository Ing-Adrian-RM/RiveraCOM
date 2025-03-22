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

int client_socket, line=1; // Global variable for the thread
int max_width;
int BUFFER_SEND_SIZE;
char *temp = NULL;
WINDOW *chat_win, *input_win;
pthread_mutex_t lock;

// Thread to receive messages from the server
void *receive_messages() {
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE);

        if (bytes_read <= 0) {
            werase(chat_win);
            box(chat_win, 0, 0);
            wrefresh(chat_win);
            line = 1;
            mvwprintw(chat_win, line, 1, "Server disconnected, press enter to close\n");
            box(chat_win, 0, 0);
            wrefresh(chat_win);
            wmove(input_win, sizeof("Message: "), 1);
            wrefresh(input_win);
            wgetnstr(input_win, buffer, 0);
            close(client_socket);
            endwin();
            free(temp);
            pthread_mutex_destroy(&lock);
            exit(0);
        }

        char *message = buffer;
        int lines_writed = 1;
        while (*message) {
            if (line >= LINES - 5) { // Leave space for the input window
                box(chat_win,' ',' ');
                scroll(chat_win);
                box(chat_win, 0, 0);
                pthread_mutex_lock(&lock);
                line--;
                pthread_mutex_unlock(&lock); 
            }
            
            memset(temp, ' ', sizeof(max_width + 1));
            if (lines_writed == 1){
                strncpy(temp, message, max_width - 8);
                temp[max_width + 1] = '\0';
                mvwprintw(chat_win, line, 1, "Server: %s", temp);
                message += (max_width - 8);
            } else if (lines_writed > 1){
                strncpy(temp, message, max_width);
                temp[max_width + 1] = '\0';
                mvwprintw(chat_win, line, 1, "%s", temp);
                message += (max_width);
            }            
            
            box(chat_win, 0, 0);
            wrefresh(chat_win);
            lines_writed++;

            pthread_mutex_lock(&lock);
            line++;
            pthread_mutex_unlock(&lock);
        }

        wmove(input_win, sizeof("Message: "), 1);
        wrefresh(input_win);
    }
    return NULL;
}

void setup() {
    struct sockaddr_in server_address;
    pthread_mutex_init(&lock, NULL);

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

    // Initialize ncurses
    initscr();
    cbreak();
    curs_set(1);
    max_width = COLS - 2;
    BUFFER_SEND_SIZE = COLS * 2;
    temp = malloc((max_width + 1) * sizeof(char));

    // Create windows for chat and input
    int height = LINES - 4;
    int width = COLS;
    chat_win = newwin(height, width, 0, 0);
    input_win = newwin(4, width, height, 0);
    scrollok(chat_win, TRUE);

    // Draw borders
    box(chat_win, 0, 0);
    box(input_win, 0, 0);
    wrefresh(chat_win);
    wrefresh(input_win);

    mvwprintw(chat_win, line, 1, "Connected to the server. Type 'close' to terminate.\n");
    wrefresh(chat_win);
    wmove(input_win, 1, sizeof("Message: "));
    wrefresh(input_win);
    pthread_mutex_lock(&lock);
    line++;
    pthread_mutex_unlock(&lock);

    // Create a thread to receive messages from the server
    pthread_t reception_thread;
    pthread_create(&reception_thread, NULL, receive_messages, NULL);
}

int main() {
    setup();
    char buffer[BUFFER_SEND_SIZE];

    // Loop to send messages
    while (1) {
        werase(input_win);
        wborder(input_win, ' ', ' ', '-', '-', '-', '-', '-', '-');
        mvwprintw(input_win, 1, 0, "Message: ");
        wrefresh(input_win);
        memset(buffer, 0, BUFFER_SEND_SIZE);
        wgetnstr(input_win, buffer, BUFFER_SEND_SIZE - 9);
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
        
        send(client_socket, buffer, strlen(buffer), 0);
        
        char *message = buffer;
        int lines_writed = 1;
        while (*message) {
            if (line >= LINES - 5) {
                box(chat_win,' ',' ');
                scroll(chat_win);
                box(chat_win, 0, 0);
                pthread_mutex_lock(&lock);
                line--;
                pthread_mutex_unlock(&lock);
            }

            memset(temp, ' ', sizeof(max_width + 1));
            if (lines_writed == 1){
                strncpy(temp, message, max_width - 4);
                temp[max_width + 1] = '\0';
                mvwprintw(chat_win, line, 1, "Me: %s", temp);
                message += (max_width - 4);
            } else if (lines_writed > 1){
                strncpy(temp, message, max_width);
                temp[max_width + 1] = '\0';
                mvwprintw(chat_win, line, 1, "%s", temp);
                message += (max_width);
            }            
            
            box(chat_win, 0, 0);
            wrefresh(chat_win);
            lines_writed++;

            pthread_mutex_lock(&lock);
            line++;
            pthread_mutex_unlock(&lock);
        }

        wmove(input_win, sizeof("Message: "), 1);
        wrefresh(input_win);
        
        if (strncmp(buffer, "close", 4) == 0) {
            wprintw(chat_win, "Closing connection...\n");
            wrefresh(chat_win);
            close(client_socket);
            endwin(); // Close ncurses
            exit(0);
        } else if (strncmp(buffer, "clear", 5) == 0) {
            werase(chat_win);
            box(chat_win, 0, 0);
            wrefresh(chat_win);
            line = 1;
        }   
    }

    free(temp);
    pthread_mutex_destroy(&lock);
    endwin(); // Close ncurses
    return 0;
}
