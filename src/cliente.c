///////////////////////////////////////////////////////////////////////////////
//
// Client Module
//
///////////////////////////////////////////////////////////////////////////////

#include "server_elements.h"

///////////////////////////////////////////////////////////////////////////////
// print_in_chatwin-> 
///////////////////////////////////////////////////////////////////////////////
int printInChatWin(WINDOW *win, void *arg) {
    char *message = (char *)arg;
    char temp_buffer[BUFFER_SEND_SIZE];
    int lines_writed = 1;

    while (*message) {
        if (line >= LINES - 5) {
            box(chat_win, ' ', ' ');
            scroll(chat_win);
            box(chat_win, 0, 0);
            line--;
        }

        if (lines_writed == 1) {
            memset(temp_buffer, '\0', BUFFER_SEND_SIZE);
            char *newline_pos = strchr(message, '\n');
            if (newline_pos && (newline_pos - message) < (max_width - 20)) {
                strncpy(temp_buffer, message, newline_pos - message);
                mvwprintw(chat_win, line, 1, "%s", temp_buffer);
                message = newline_pos + 1;
            } else {
                strncpy(temp_buffer, message, max_width - 20);
                mvwprintw(chat_win, line, 1, "%s", temp_buffer);
                message += strlen(temp_buffer);
            }
        } else if (lines_writed > 1) {
            memset(temp_buffer, '\0', BUFFER_SEND_SIZE);
            char *newline_pos = strchr(message, '\n');
            if (newline_pos && (newline_pos - message) < max_width) {
                strncpy(temp_buffer, message, newline_pos - message);
                mvwprintw(chat_win, line, 1, "%s", temp_buffer);
                message = newline_pos + 1;
            } else {
                strncpy(temp_buffer, message, max_width);
                mvwprintw(chat_win, line, 1, "%s", temp_buffer);
                message += strlen(temp_buffer);
            }
        }

        box(chat_win, 0, 0);
        wrefresh(chat_win);
        lines_writed++;
        line++;
    }

    wmove(input_win, sizeof("Message: "), 1);
    wrefresh(input_win);
    return OK;
}

///////////////////////////////////////////////////////////////////////////////
// clear_chat_win-> 
///////////////////////////////////////////////////////////////////////////////
int clearChatWin(WINDOW *win, void *arg) {   
    werase(chat_win);
    box(chat_win, 0, 0);
    wrefresh(chat_win);
    line = 1;
    return OK;
}

///////////////////////////////////////////////////////////////////////////////
// clear_input_win-> 
///////////////////////////////////////////////////////////////////////////////
int clearInputWin(WINDOW *win, void *arg) {   
    werase(input_win);
    wborder(input_win, ' ', ' ', '-', '-', '-', '-', '-', '-');
    mvwprintw(input_win, 2, 0, "Linked to: %s", linkedTo);
    mvwprintw(input_win, 1, 0, "Message: ");
    wrefresh(input_win);
    return OK;
}

///////////////////////////////////////////////////////////////////////////////
// sendGif-> Sends a .gif file to a client
///////////////////////////////////////////////////////////////////////////////
void sendGif(char *buffer, CLIENT client) {
    size_t bytes_read, bytes_send, total_bytes_send = 0;
    char file_path[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    char gif_name[BUFFER_SIZE];
    strncpy(gif_name, buffer + 4, strlen(buffer) - 4);

    snprintf(file_path, sizeof(file_path), "./media/gifs/%.100s.gif", gif_name);
    struct stat file_info;
    if (!stat(file_path, &file_info)) {
        memset(temp_buffer, '\0', sizeof(temp_buffer));
        snprintf(temp_buffer, BUFFER_SIZE, ".m|%s:%s+%lu", linkedTo, buffer, file_info.st_size);
        send(client.socket, temp_buffer, strlen(temp_buffer), 0);

        FILE *file = fopen(file_path, "rb");
        if (!file) {
            memset(temp_buffer, '\0', sizeof(temp_buffer));
            snprintf(temp_buffer, BUFFER_SIZE, "Error opening file");
            use_window(chat_win, printInChatWin, temp_buffer);
        }

        sleep(1);
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            send(client.socket, buffer, bytes_read, 0);
            if (bytes_send < 0) {
                memset(temp_buffer, '\0', BUFFER_SIZE);
                snprintf(temp_buffer, BUFFER_SIZE, "Error sending file");
                use_window(chat_win, printInChatWin, temp_buffer);
                break;
            }
            total_bytes_send += bytes_read;
            memset(temp_buffer, '\0', BUFFER_SIZE);
        }
        if (total_bytes_send == file_info.st_size) {
            memset(temp_buffer, '\0', BUFFER_SIZE);
            snprintf(temp_buffer, BUFFER_SIZE, "Me: gif %.100s sent. Total bytes sent: %zu", gif_name, total_bytes_send);
            use_window(chat_win, printInChatWin, temp_buffer);
        } else {
            memset(temp_buffer, '\0', BUFFER_SIZE);
            snprintf(temp_buffer, BUFFER_SIZE, "Error: Failed to send gif.");
            use_window(chat_win, printInChatWin, temp_buffer);
        }
        fclose(file);
    } else {
        memset(temp_buffer, '\0', BUFFER_SIZE);
        snprintf(temp_buffer, BUFFER_SIZE, "Error: Gif not found.");
        use_window(chat_win, printInChatWin, temp_buffer);
    }
}

///////////////////////////////////////////////////////////////////////////////
// receiveGif-> Receives a .gif file
///////////////////////////////////////////////////////////////////////////////
void receiveGif(char *buffer, CLIENT client) {
    ssize_t bytes_received, total_bytes_received = 0;
    char temp_buffer[BUFFER_SIZE];
    char gif_name[BUFFER_SIZE];
    char file_path[BUFFER_SIZE];
    strtok(buffer, "+"); char *value = strtok(NULL, "+");
    strtok(buffer, " "); char *name = strtok(NULL, " ");
    snprintf(gif_name, strlen(name) + 1, "%s", name);
    int gif_size = atoi(value);
    snprintf(file_path, sizeof(file_path), "./media/gifs/%s.gif", name);
    
    FILE *file = fopen(file_path, "wb");
    if (!file) {
        memset(temp_buffer, '\0', BUFFER_SIZE);
        snprintf(temp_buffer, BUFFER_SIZE, "Error creating file");
        use_window(chat_win, printInChatWin, buffer);
    }    

    while (gif_size > total_bytes_received) {
        memset(buffer, '\0', BUFFER_SIZE);
        bytes_received = recv(client.socket, buffer, sizeof(buffer), 0);
        fwrite(buffer, 1, bytes_received, file);
        if (bytes_received < 0) {
            memset(temp_buffer, '\0', BUFFER_SIZE);
            snprintf(temp_buffer, BUFFER_SIZE, "Error receiving file");
            use_window(chat_win, printInChatWin, buffer);
            break;
        }
        total_bytes_received += bytes_received;
    }
    fclose(file);
    memset(temp_buffer, '\0', BUFFER_SIZE);
    snprintf(temp_buffer, BUFFER_SIZE, "Me: gif %.100s received. Total bytes transmited: %zu", gif_name, total_bytes_received);
    use_window(chat_win, printInChatWin, temp_buffer);
}

///////////////////////////////////////////////////////////////////////////////
// *receive_messages-> 
///////////////////////////////////////////////////////////////////////////////
void *receive_messages() {
    while (1) {
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE);
        int bytes_read = read(client, buffer, BUFFER_SIZE);
        use_window(chat_win, printInChatWin, buffer);
        strtok(buffer, ":"); char *message = strtok(NULL, ":");
        if (message != NULL && strncmp(message, " gif", 4) == 0) receiveGif(message, client_i);
        if (bytes_read <= 0) {
            memset(buffer, '\0', BUFFER_SIZE);
            use_window(chat_win, clearChatWin, buffer);
            use_window(chat_win, clearInputWin, buffer);
            snprintf(buffer, BUFFER_SIZE, "Server disconnected, closing in 3 seconds...\n");
            use_window(chat_win, printInChatWin, buffer);
            sleep(3); close(client); free(temp); endwin(); exit(0);
        }
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// discoverServer-> 
///////////////////////////////////////////////////////////////////////////////
void discoverServer() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_addr);

    // Create UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creating UDP socket");
        return;
    }

    // Configure broadcast address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    server_addr.sin_port = htons(DISCOVERY_PORT);

    // Enable broadcast on the socket
    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("Error enabling broadcast");
        close(sock);
        return;
    }

    // Send discovery message
    if (sendto(sock, "DISCOVERY_MESSAGE", strlen("DISCOVERY_MESSAGE"), 0, (struct sockaddr *)&server_addr, addr_len) < 0) {
        perror("Error sending discovery message");
        close(sock);
        return;
    }

    // Wait for server response
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&server_addr, &addr_len);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the received string
    } else {
        perror("Error receiving server response");
        close(sock);
        return;
    }
    close(sock);

    // Modify the server's IP
    strncpy(SERVER, buffer, BUFFER_SIZE);
    SERVER[BUFFER_SIZE - 1] = '\0';
}

///////////////////////////////////////////////////////////////////////////////
// help-> Print the help menu
///////////////////////////////////////////////////////////////////////////////
char *help(char *buffer) {
    memset(buffer, '\0', BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "Commands:\n"
        ".close: Close the server\n"
        ".clear: Clear the chat window\n"
        ".listc: List connected clients\n"
        ".listdb: List users in the database\n"
        ".deletedb: Delete a user from the database\n"
        ".updatedb: Update a user in the database\n"
        ".link <user>: Link to a user\n"
        ".unlink: Unlink from a user\n"
        ".help: Show this help menu\n"
        "@<user>: Send a message to a specific user\n"
        "@B:<message>: Broadcast a message to all users");
    return buffer;
}


///////////////////////////////////////////////////////////////////////////////
// setup-> 
///////////////////////////////////////////////////////////////////////////////
void setup() {
    discoverServer();

    // Create socket
    client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    // Replace with the server's IP address
    if (inet_pton(AF_INET, SERVER, &server_addr.sin_addr) <= 0) {
        perror("Invalid or unsupported address");
        close(client);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection error");
        close(client);
        exit(EXIT_FAILURE);
    }

    client_i.socket = client;
    inet_ntop(AF_INET, &client_addr.sin_addr, client_i.ip, INET_ADDRSTRLEN);
    client_i.name[0] = '\0';

    // Initialize ncurses
    initscr();
    cbreak();
    curs_set(1);
    max_width = COLS - 2;
    BUFFER_SEND_SIZE = COLS - 2 - sizeof("Message: ");
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

    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    snprintf(buffer, sizeof(buffer), "Connected to the server. Type 'close' to terminate.");
    use_window(chat_win, printInChatWin, buffer);

    // Create a thread to receive messages from the server
    pthread_create(&reception_thread, NULL, receive_messages, NULL);
    pthread_detach(reception_thread);
}

int main() {
    setup();

    char buffer[BUFFER_SEND_SIZE];
    char temp_buffer[BUFFER_SIZE];

    // Loop to send messages
    while (1) {
        memset(buffer, '\0', BUFFER_SEND_SIZE);
        memset(temp_buffer, '\0', BUFFER_SIZE);
        use_window(input_win, clearInputWin, 0);
        wgetnstr(input_win, buffer, BUFFER_SEND_SIZE);
        snprintf(temp_buffer, sizeof(temp_buffer), "Me: %s", buffer);
        use_window(chat_win, printInChatWin, temp_buffer);
        
        if (strncmp(buffer, ".", 1) != 0) {
            memset(temp_buffer, '\0', BUFFER_SIZE);
            snprintf(temp_buffer, BUFFER_SIZE, ".m|%s:%s", linkedTo, buffer);
            send(client_i.socket, temp_buffer, strlen(temp_buffer), 0);
            if (strncmp(buffer, "gif", 3) == 0) sendGif(temp_buffer, client_i);
        }
        else if (strncmp(buffer, ".close", 6) == 0) {
            close(client); free(temp); endwin(); exit(0);
        }
        else if (strncmp(buffer, ".clear", 6) == 0) {
            use_window(chat_win, clearChatWin, 0);
            use_window(input_win, clearInputWin, 0);
        }
        else if (strncmp(buffer, ".help", 5) == 0) {
            memset(temp_buffer, '\0', BUFFER_SIZE);
            use_window(chat_win, printInChatWin, help(temp_buffer));
        }
        else if (strncmp(buffer, ".unlink", 7) == 0) {
            memset(linkedTo, '\0', sizeof(linkedTo));
            snprintf(linkedTo, sizeof(linkedTo), "Server");
            memset(temp_buffer, '\0', sizeof(temp_buffer));
            snprintf(temp_buffer, sizeof(temp_buffer), "*** Linked to Server ***");
            use_window(chat_win, printInChatWin, temp_buffer);
        }
        else if (strncmp(buffer, ".rename", 7) == 0 || strncmp(buffer, ".recharge", 9) == 0 || strncmp(buffer, ".userinfo", 9) == 0 
        || strncmp(buffer, ".deleteuser", 9) == 0 || strncmp(buffer, ".listc", 6) == 0 || strncmp(buffer, ".link", 5) == 0) {
            pthread_kill(reception_thread, SIGSTOP);
            send(client_i.socket, buffer, strlen(buffer), 0);
            memset(temp_buffer, '\0', sizeof(temp_buffer));
            int bytes_read = read(client, temp_buffer, BUFFER_SIZE);
            pthread_kill(reception_thread, SIGCONT);
            use_window(chat_win, printInChatWin, temp_buffer);
            if (strncmp(temp_buffer, "User deleted from database", 26) == 0){
                memset(temp_buffer, '\0', BUFFER_SIZE);
                snprintf(temp_buffer, BUFFER_SIZE, "Closing in 5 seconds...\n");
                use_window(chat_win, printInChatWin, buffer);
                use_window(chat_win, clearInputWin, buffer);
                sleep(5); endwin(); close(client); free(temp); exit(0);
            } else if (strncmp(temp_buffer, "Authorized link", 15) == 0) {
                strtok(buffer, " "); char *user_name = strtok(NULL, " ");
                memset(linkedTo, '\0', sizeof(linkedTo));
                snprintf(linkedTo, sizeof(linkedTo), "%s", user_name);
                memset(temp_buffer, '\0', sizeof(temp_buffer));
                snprintf(temp_buffer, sizeof(temp_buffer), "*** Linked to %s ***", user_name);
                use_window(chat_win, printInChatWin, temp_buffer);
            }
        }
        else {
            memset(temp_buffer, '\0', BUFFER_SIZE);
            snprintf(temp_buffer, BUFFER_SIZE, ".m|%s:%s", linkedTo, buffer);
            send(client_i.socket, temp_buffer, strlen(temp_buffer), 0);
        }
    }

    if (temp != NULL) free(temp);
    if (client >= 0) close(client);
    endwin();    
    return 0;
}
