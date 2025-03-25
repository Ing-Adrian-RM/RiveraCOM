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
    int lines_writed = 1;
    while (*message)
    {
        if (line >= LINES - 5)
        {
            box(chat_win, ' ', ' ');
            scroll(chat_win);
            box(chat_win, 0, 0);
            pthread_mutex_lock(&lock);
            line--;
            pthread_mutex_unlock(&lock);
        }
        memset(temp, ' ', sizeof(max_width + 1));
        if (lines_writed == 1)
        {
            strncpy(temp, message, max_width - 10); //No puede ser un valor fijo
            temp[max_width + 1] = '\0';
            mvwprintw(chat_win, line, 1, "%s", temp);
            message += (max_width - 4);
        }
        else if (lines_writed > 1)
        {
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
    mvwprintw(input_win, 1, 0, "Message: ");
    wrefresh(input_win);
    return OK;
}

///////////////////////////////////////////////////////////////////////////////
// sendGif-> Sends a .gif file to a client
///////////////////////////////////////////////////////////////////////////////
ssize_t sendGif(char *file_path, CLIENT client) {
    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    size_t bytes_read, total_bytes_sent = 0;
    FILE *file = fopen(file_path, "rb");

    if (!file) {
        snprintf(buffer, sizeof(buffer), "Error opening file");
        use_window(chat_win, printInChatWin, buffer);
        return total_bytes_sent;
    }

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        ssize_t bytes_sent = send(client.socket, buffer, bytes_read, 0);
        if (bytes_sent < 0) {
            memset(buffer, '\0', BUFFER_SIZE);
            snprintf(buffer, sizeof(buffer), "Error sending file");
            use_window(chat_win, printInChatWin, buffer);
            break;
        }
        total_bytes_sent += bytes_sent;
    }

    fclose(file);
    return total_bytes_sent;
}

///////////////////////////////////////////////////////////////////////////////
// receiveGif-> Receives a .gif file from a client
///////////////////////////////////////////////////////////////////////////////
ssize_t receiveGif(char *file_path, CLIENT client) {
    FILE *file = fopen(file_path, "wb");
    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    ssize_t bytes_received, total_bytes_received = 0;

    if (!file) {
        snprintf(buffer, sizeof(buffer), "Error creating file");
        use_window(chat_win, printInChatWin, buffer);
        return total_bytes_received;
    }    

    while ((bytes_received = recv(client.socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
        if (bytes_received < 0) {
            memset(buffer, '\0', BUFFER_SIZE);
            snprintf(buffer, sizeof(buffer), "Error receiving file");
            use_window(chat_win, printInChatWin, buffer);
            break;
        }
        total_bytes_received += bytes_received;
    }

    fclose(file);
    return total_bytes_received;
}

///////////////////////////////////////////////////////////////////////////////
// *receive_messages-> 
///////////////////////////////////////////////////////////////////////////////
void *receive_messages() {
    while (1) {
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE);
        int bytes_read = read(client, buffer, BUFFER_SIZE);

        if (bytes_read <= 0) {
            use_window(chat_win, clearChatWin, buffer);
            snprintf(buffer, sizeof(buffer), "Server disconnected, press enter to close\n");
            use_window(chat_win, printInChatWin, buffer);
            use_window(chat_win, clearInputWin, buffer);
            wgetnstr(input_win, buffer, BUFFER_SEND_SIZE - sizeof("Message: "));
            endwin(); close(client); free(temp);
            pthread_mutex_destroy(&lock);
            exit(0);
        }

        if (strncmp(buffer, "gif", 3) == 0){
            char file_path[BUFFER_SIZE];
            snprintf(file_path, sizeof(file_path), "./media/gifs/%s.gif", (buffer + 4));                 
            size_t bytes_received = receiveGif(file_path, client_i);
        }
        use_window(chat_win, printInChatWin, temp_buffer);
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// setup-> 
///////////////////////////////////////////////////////////////////////////////
void setup() {
    struct sockaddr_in server_address;
    pthread_mutex_init(&lock, NULL);

    // Create socket
    client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure the server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    
    // Replace with the server's IP address
    if (inet_pton(AF_INET, SERVER, &server_address.sin_addr) <= 0) {
        perror("Invalid or unsupported address");
        close(client);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
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

    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    snprintf(buffer, sizeof(buffer), "Connected to the server. Type 'close' to terminate.");
    use_window(chat_win, printInChatWin, buffer);

    // Create a thread to receive messages from the server
    pthread_t reception_thread;
    pthread_create(&reception_thread, NULL, receive_messages, NULL);
    pthread_detach(reception_thread);
}

int main() {
    setup();

    char buffer[BUFFER_SEND_SIZE];
    char temp_buffer[BUFFER_SIZE];

    SND_RCV sr;
    strncpy(sr.sender_name, "Server", BUFFER_SIZE);
    sr.sender_name[sizeof(sr.sender_name - 1)] = '\0';
    strncpy(sr.receiver_name, "Broadcast", BUFFER_SIZE);
    sr.receiver_name[sizeof(sr.receiver_name) - 1] = '\0';

    // Loop to send messages
    while (1) {
        memset(buffer, '\0', BUFFER_SEND_SIZE);
        memset(temp_buffer, '\0', BUFFER_SIZE);
        use_window(input_win, clearInputWin, 0);
        wgetnstr(input_win, buffer, BUFFER_SEND_SIZE - sizeof("Message: "));
        snprintf(temp_buffer, sizeof(temp_buffer), "Me: %s", buffer);
        
        if (strncmp(buffer, "close", 5) == 0) {
            endwin(); close(client); free(temp);
            pthread_mutex_destroy(&lock);
            exit(0);
        }
        if (strncmp(buffer, "clear", 5) == 0) {use_window(chat_win, clearChatWin, 0); continue;}

        if (strncmp(buffer, "gif", 3) == 0) {
            char file_path[BUFFER_SIZE];
            snprintf(file_path, sizeof(file_path), "./media/gifs/%s.gif", buffer + 4);        
            if (access(file_path, F_OK) == 0) {
                size_t bytes_send = sendGif(file_path, client_i);
                if (bytes_send > 0) {
                    memset(buffer, '\0', BUFFER_SEND_SIZE);
                    snprintf(buffer, BUFFER_SEND_SIZE, "%s sent. Total bytes sent: %zu", temp_buffer, bytes_send);
                    use_window(chat_win, printInChatWin, buffer);
                } else {
                    memset(temp_buffer, '\0', BUFFER_SIZE);
                    snprintf(temp_buffer, BUFFER_SIZE, "Error: Failed to send gif.");
                    use_window(chat_win, printInChatWin, temp_buffer);
                }
            } else {
                memset(temp_buffer, '\0', BUFFER_SIZE);
                snprintf(temp_buffer, BUFFER_SIZE, "Error: Gif not found.");
                use_window(chat_win, printInChatWin, temp_buffer);
            }
        } else {
            use_window(chat_win, printInChatWin, temp_buffer);
            send(client_i.socket, temp_buffer, strlen(temp_buffer), 0);
        }
    }

    if (temp != NULL) free(temp);
    if (client >= 0) close(client);
    pthread_mutex_destroy(&lock);
    endwin();    
    return 0;
}
