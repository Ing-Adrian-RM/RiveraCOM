///////////////////////////////////////////////////////////////////////////////
//
// Module
// Author: Adrián Rodríguez Murillo
//
///////////////////////////////////////////////////////////////////////////////

#include "server_elements.h"

///////////////////////////////////////////////////////////////////////////////
// executeQuery-> 
///////////////////////////////////////////////////////////////////////////////
MYSQL_RES* executeQuery(const char *query) {
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, LOCAL_HOST, USER, PASSWORD, DATABASE, 0, NULL, 0)) {
        fprintf(stderr, "Connection failed: %s\n", mysql_error(conn));
        return NULL;
    }
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Query error: %s\n", mysql_error(conn));
        return NULL;
    }
    res = mysql_store_result(conn);
    mysql_close(conn);
    return res;
}

///////////////////////////////////////////////////////////////////////////////
// userDBExists-> 
///////////////////////////////////////////////////////////////////////////////
int userDBExists(char *search_by, int option) {
    int exists = 0;
    memset(query, '\0', QUERY_SIZE);
    if (option == 0) {
        snprintf(query, sizeof(query), "SELECT * FROM users WHERE name='%s'", search_by);
    } else if (option == 1) {
        snprintf(query, sizeof(query), "SELECT * FROM users WHERE ip='%s'", search_by);
    }
    res = executeQuery(query);
    if ((row = mysql_fetch_row(res))) exists = 1;
    mysql_free_result(res);
    return exists;
}

///////////////////////////////////////////////////////////////////////////////
// registerDBUser-> 
///////////////////////////////////////////////////////////////////////////////
CLIENT registerDBUser(CLIENT client) {
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    int bytesReceived;
    char *ip = client.ip;
    if (userDBExists(ip,1)) {
        snprintf(query, QUERY_SIZE, "SELECT name FROM users WHERE ip='%s'", client.ip);
        res = executeQuery(query);
        row = mysql_fetch_row(res);
        strncpy(client.name, (const char *)row[0], sizeof(buffer) - 1);
        client.name[sizeof(client.name) - 1] = '\0';
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, sizeof(buffer) - 1, "Welcome back, %.900s!", client.name);
        send(client.socket, buffer, strlen(buffer), 0);
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "%.900s Connected, IP: %.100s\n", client.name, client.ip);
        use_window(chat_win, printInChatWin, buffer);
    }
    else {
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "You are not registered. Please enter a unique username:");
        send(client.socket, buffer, strlen(buffer), 0);
        bytesReceived = 0;
        int name_accepted = 0;
        while (!name_accepted) {
            memset(buffer, '\0', BUFFER_SIZE);
            bytesReceived = read(client.socket, buffer, sizeof(buffer));
            if (userDBExists(buffer, 0)) {
                memset(temp_buffer, '\0', BUFFER_SIZE);
                snprintf(temp_buffer, sizeof(temp_buffer), "Username already exists. Please enter a unique username:");
                send(client.socket, temp_buffer, strlen(temp_buffer), 0);
            } else {
                name_accepted = 1;
            }
        }
        strncpy(client.name, buffer, strlen(buffer));
        client.name[strlen(client.name)] = '\0';
        snprintf(query, sizeof(query), "INSERT INTO users (ip, name, balance, uploaded, downloaded) VALUES ('%.255s', '%.255s', 0, 0, 0)", client.ip, client.name);
        executeQuery(query);
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "%.900s Connected, IP: %.100s\n", client.name, client.ip);
        use_window(chat_win, printInChatWin, buffer);
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "Registration successful! Welcome, %.900s!\n", client.name);
        send(client.socket, buffer, strlen(buffer), 0);
    }
    return client;
}

///////////////////////////////////////////////////////////////////////////////
// printDBUsers-> 
///////////////////////////////////////////////////////////////////////////////
void printDBUsers() {
    char buffer[BUFFER_SIZE];
    snprintf(query, sizeof(query), "SELECT * FROM users");
    res = executeQuery(query);
    if (res == NULL) {
        fprintf(stderr, "Error executing query: %s\n", mysql_error(conn));
        return;
    }
    while ((row = mysql_fetch_row(res))) {
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "IP: %s, Name: %s, Balance: %s, Uploaded: %s, Downloaded: %s\n", row[0], row[1], row[2], row[3], row[4]);
        use_window(chat_win, printInChatWin, buffer);
    }
    mysql_free_result(res);
}

///////////////////////////////////////////////////////////////////////////////
// updateDBUsers-> 
///////////////////////////////////////////////////////////////////////////////
void updateDBUser(CLIENT client, int field, int value) {
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    //snprintf(query, sizeof(query), "UPDATE users SET %s = %d WHERE name='%s'", field == 0 ? "balance" : field == 1 ? "uploaded" : "downloaded", value, client.name);
    executeQuery(query);
    memset(buffer, '\0', BUFFER_SIZE);
    snprintf(buffer, sizeof(buffer), "User %.100s updated in database", client.name);
    use_window(chat_win, printInChatWin, buffer);
}

///////////////////////////////////////////////////////////////////////////////
// deleteDBUsers-> 
///////////////////////////////////////////////////////////////////////////////
void deleteDBUser(char *command){
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    char user_name[BUFFER_SIZE];
    SND_RCV sr;
    strtok(command, ":"); char *user = strtok(NULL, ":");
    strtok(command, " "); char *option = strtok(NULL, " ");
    if (userDBExists(user,0) || userDBExists(user,1)){
        memset(query, '\0', QUERY_SIZE);    
        if (strncmp(option, "name", 4) == 0){
            snprintf(query, sizeof(query), "DELETE FROM users WHERE name='%s'", user);
            strncpy(user_name, user, strlen(user));
        }
        else if (strncmp(option, "ip", 2) == 0){
            snprintf(query, sizeof(query), "SELECT name FROM users WHERE ip='%s'", user);
            res = executeQuery(query);
            row = mysql_fetch_row(res);
            strncpy(user_name, row[0], BUFFER_SIZE - 1);
            snprintf(query, sizeof(query), "DELETE FROM users WHERE ip='%s'", user);
        } 
        executeQuery(query);
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, sizeof(buffer), "User %s deleted from database", user);
        use_window(chat_win, printInChatWin, buffer);
        for (CLIENT_LIST_PTR ptr = c_list; ptr != NULL; ptr = ptr->next) {
            if (strncmp(ptr->client.name, user_name, strlen(ptr->client.name)) == 0) {
                memset(buffer, '\0', BUFFER_SIZE);
                snprintf(temp_buffer, sizeof(temp_buffer), "Your user has been deleted from the server database, you must register again.\n Type any command to proceed with disconnection and please log in again.");
                send_messages(sr, ptr, buffer, temp_buffer);
                removeClientConn(c_list, ptr->client);
            }
        }
    } else {
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, sizeof(buffer), "User %.100s not found in database", user_name);
        use_window(chat_win, printInChatWin, buffer);
    }
}

///////////////////////////////////////////////////////////////////////////////
// printClientConn-> Print the connected clients list
///////////////////////////////////////////////////////////////////////////////
void printClientConn(CLIENT_LIST_PTR c_list){
    char buffer[BUFFER_SIZE];
    if (c_list == NULL) {
        snprintf(buffer, sizeof(buffer), "Connected clients list: NULL");
        use_window(chat_win, printInChatWin, buffer);
    } else {
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, sizeof(buffer), "Connected clients list:");
        for (CLIENT_LIST_PTR ptr = c_list; ptr != NULL; ptr = ptr->next){    
            use_window(chat_win, printInChatWin, buffer);
            memset(buffer, '\0', BUFFER_SIZE);
            snprintf(buffer, sizeof(buffer), "Client: %.100s, IP: %.100s, Client_socket: %d", ptr->client.name, ptr->client.ip, ptr->client.socket);
            use_window(chat_win, printInChatWin, buffer);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// addClientConn-> Insert an actual connected client in a connected clients list
///////////////////////////////////////////////////////////////////////////////
CLIENT_LIST_PTR addClientConn(CLIENT_LIST_PTR c_list, CLIENT client){
    CLIENT_LIST_PTR new_node = (CLIENT_LIST_PTR) malloc(sizeof(CLIENT_LIST));
    new_node->client = client;
    new_node->next = NULL;
    if (c_list == NULL) {
        return new_node;
    } else {
        for (CLIENT_LIST_PTR ptr = c_list; ptr != NULL; ptr = ptr->next){
            if (ptr->next == NULL) {
                ptr->next = new_node;
                return ptr;
            }
        }
    }
    return c_list;
}

///////////////////////////////////////////////////////////////////////////////
// removeClientConn-> Remove a client from the connected clients list
///////////////////////////////////////////////////////////////////////////////
CLIENT_LIST_PTR removeClientConn(CLIENT_LIST_PTR c_list, CLIENT client) {
    CLIENT_LIST_PTR prev = NULL;
    CLIENT_LIST_PTR current = c_list;
    
    while (current != NULL) {
        if (strcmp(current->client.ip, client.ip) == 0) {
            if (prev == NULL) {
                CLIENT_LIST_PTR temp = current;
                c_list = NULL;
                close(temp->client.socket);
                free(temp);
            } else {
                prev->next = current->next;
                close(current->client.socket);
                free(current);
            }
            return c_list;
        }
        prev = current;
        current = current->next;
    }
    return c_list;
}

///////////////////////////////////////////////////////////////////////////////
// disconnectAllClients-> Disconnect all connected clients
///////////////////////////////////////////////////////////////////////////////
void disconnectAllClients(CLIENT_LIST_PTR c_list) {
    CLIENT_LIST_PTR current = c_list;
    while (current != NULL) {
        close(current->client.socket);
        CLIENT_LIST_PTR temp = current;
        current = current->next;
    }
}

///////////////////////////////////////////////////////////////////////////////
// shutdownServer-> Safely shuts down the server, releasing resources
///////////////////////////////////////////////////////////////////////////////
void shutdownServer(CLIENT_LIST_PTR c_list) {

    
    if (temp != NULL) {
        free(temp);
        temp = NULL;
    }


    if (server >= 0) {
        close(server);
    }

    disconnectAllClients(c_list);
    pthread_mutex_destroy(&lock);
    endwin();
    exit(EXIT_SUCCESS);
}

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
            //pthread_mutex_lock(&lock);
            line--;
            //pthread_mutex_unlock(&lock);
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

        //pthread_mutex_lock(&lock);
        line++;
        //pthread_mutex_unlock(&lock);
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
void sendGif(char *buffer, CLIENT client) {
    size_t bytes_read, bytes_send, total_bytes_send = 0;
    char file_path[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    char gif_name[BUFFER_SIZE];
    strncpy(gif_name, buffer + 4, strlen(buffer) - 4);

    snprintf(file_path, sizeof(file_path), "./media/gifs/%s.gif", gif_name);
    struct stat file_info;
    if (!stat(file_path, &file_info)) {
        memset(temp_buffer, '\0', sizeof(temp_buffer));
        snprintf(temp_buffer, BUFFER_SIZE, "%s:%lu", buffer, file_info.st_size);
        send(client.socket, temp_buffer, strlen(temp_buffer), 0);

        FILE *file = fopen(file_path, "rb");
        if (!file) {
            memset(temp_buffer, '\0', sizeof(temp_buffer));
            snprintf(temp_buffer, BUFFER_SIZE, "Error opening file");
            use_window(chat_win, printInChatWin, temp_buffer);
        }

        sleep(1);
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
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
            snprintf(temp_buffer, BUFFER_SIZE, "gif %.100s sent. Total bytes sent: %zu", gif_name, total_bytes_send);
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
    strtok(buffer, ":"); char *value = strtok(NULL, ":");
    strtok(buffer, " "); char *name = strtok(NULL, " ");
    strncpy(gif_name, name, strlen(name));
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
    snprintf(temp_buffer, BUFFER_SIZE, "%.100s: gif %.100s received. Total bytes transmited: %zu", client.name, gif_name, total_bytes_received);
    use_window(chat_win, printInChatWin, temp_buffer);
}

///////////////////////////////////////////////////////////////////////////////
// *handleClient-> 
///////////////////////////////////////////////////////////////////////////////
void *handleClient(void *arg) {

    char buffer[BUFFER_SIZE], temp_buffer[BUFFER_SIZE];
    int bytesReceived;
    CLIENT client = *(CLIENT *)arg;
    client = registerDBUser(client); 
    c_list = addClientConn(c_list, client);

    // Handle further communication with the client
    while (1)
    {   
        memset(temp_buffer, '\0', BUFFER_SIZE);
        memset(buffer, '\0', BUFFER_SIZE);
        
        bytesReceived = read(client.socket, buffer, BUFFER_SIZE);
        snprintf(temp_buffer, sizeof(temp_buffer), "%.100s: %.900s", client.name,buffer);

        if (bytesReceived <= 0) {
            snprintf(buffer, BUFFER_SIZE, "%.900s disconnected", client.name);
            use_window(chat_win, printInChatWin, buffer);
            break;
        }
        if (strncmp(buffer, "gif", 3) == 0) receiveGif(buffer, client);   
        else use_window(chat_win, printInChatWin, temp_buffer);
    }

    c_list = removeClientConn(c_list, client);
    free(arg);
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// *send_messages-> 
///////////////////////////////////////////////////////////////////////////////
void send_messages(SND_RCV sr, CLIENT_LIST_PTR ptr, char *buffer, char *temp_buffer)
{
    if (strncmp(buffer, "gif", 3) == 0) sendGif(buffer, ptr->client);
    else {
        use_window(chat_win, printInChatWin, temp_buffer);
        send(ptr->client.socket, temp_buffer, strlen(temp_buffer), 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
// *inputWindowManagement-> 
///////////////////////////////////////////////////////////////////////////////
void *inputWindowManagement(void *arg)
{
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    SND_RCV sr;

    while (1)
    {   
        memset(buffer, '\0', BUFFER_SIZE);
        memset(temp_buffer, '\0', BUFFER_SIZE);
        use_window(input_win, clearInputWin, 0);
        wgetnstr(input_win, buffer, BUFFER_SEND_SIZE - sizeof("Message: "));
        snprintf(temp_buffer, sizeof(temp_buffer), "Server: %.900s", buffer);

        if (strncmp(buffer, ".close", 6) == 0) shutdownServer(c_list);
        else if (strncmp(buffer, ".clear", 6) == 0) {use_window(chat_win, clearChatWin, 0);}
        else if (strncmp(buffer, ".listc", 6) == 0) printClientConn(c_list);
        else if (strncmp(buffer, ".listdb", 7) == 0) printDBUsers();
        else if (strncmp(buffer, ".deletedb", 9) == 0) deleteDBUser(buffer);
        else if (strncmp(buffer, ".help", 5) == 0) {
            snprintf(temp_buffer, sizeof(temp_buffer), "Commands: @close, @clear, @list");
            use_window(chat_win, printInChatWin, temp_buffer);
        }
        else if (strncmp(buffer, "@B:", 3) == 0) {
            char *token = strtok(buffer, ":");
            char *message = strtok(NULL, ":");
            snprintf(temp_buffer, sizeof(temp_buffer), "Server: %s", message);
            for (CLIENT_LIST_PTR ptr = c_list; ptr != NULL; ptr = ptr->next) {
                send_messages(sr, ptr, message, temp_buffer);
            }
        } else if (strncmp(buffer, "@", 1) == 0) {
            char *token = strtok(buffer + 1, ":");
            char *message = strtok(NULL, ":");
            snprintf(temp_buffer, sizeof(temp_buffer), "Server: %s", message);
            for (CLIENT_LIST_PTR ptr = c_list; ptr != NULL; ptr = ptr->next) {
                if (strncmp(ptr->client.name, token, strlen(token)) == 0) {
                    send_messages(sr, ptr, message, temp_buffer);
                    break;
                }
            }
        }
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// discovery-> 
///////////////////////////////////////////////////////////////////////////////

void *discovery() {
    int sock;
    struct sockaddr_in server_addr, client_addr, local_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);
    socklen_t local_addr_len = sizeof(local_addr);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creating UDP socket");
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DISCOVERY_PORT);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding UDP socket");
        close(sock);
        return NULL;
    }

    char server_ip[BUFFER_SIZE] = "0.0.0.0";
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == 0) {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == NULL) continue;
            if (ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
                if (strcmp(ifa->ifa_name, "lo") != 0) {
                    inet_ntop(AF_INET, &addr->sin_addr, server_ip, INET_ADDRSTRLEN);
                    break;
                }
            }
        }
        freeifaddrs(ifaddr);
    }   

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        if (recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len) < 0) {
            perror("Error receiving message");
            continue;
        }
        if (strcmp(buffer, "DISCOVERY_MESSAGE") == 0) sendto(sock, server_ip, sizeof(server_ip), 0, (struct sockaddr *)&client_addr, addr_len);
    }

    close(sock);
}

///////////////////////////////////////////////////////////////////////////////
// setup-> 
///////////////////////////////////////////////////////////////////////////////
void setup()
{   
    pthread_t discovery_thread;
    pthread_create(&discovery_thread, NULL, discovery, NULL);
    pthread_detach(discovery_thread);

    char buffer[BUFFER_SIZE];
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

    box(chat_win, 0, 0);
    box(input_win, 0, 0);
    wrefresh(chat_win);
    wrefresh(input_win);

    memset(buffer, '\0', BUFFER_SIZE);
    snprintf(buffer, sizeof(buffer), "Server initalized. Type 'close' to terminate.");
    use_window(chat_win, printInChatWin, buffer);

    pthread_t send_thread;
    pthread_create(&send_thread, NULL, inputWindowManagement, NULL);
    pthread_detach(send_thread);
}

int main()
{
    setup();
    char buffer[BUFFER_SEND_SIZE];

    if (listen(server, 1) < 0)
    {   
        snprintf(buffer, sizeof(buffer), "Server initalized error. Press enter to close.");
        use_window(chat_win, printInChatWin, buffer);
        use_window(input_win, clearInputWin, 0);
        wgetnstr(input_win, buffer, BUFFER_SEND_SIZE - sizeof("Message: "));
        close(server);
        exit(EXIT_FAILURE);
    }

    while (1)
    {   
        client = accept(server, (struct sockaddr *)&client_addr, &addr_size);
        client_i.socket = client;
        inet_ntop(AF_INET, &client_addr.sin_addr, client_i.ip, INET_ADDRSTRLEN);
        if (client >= 0) {
            pthread_t client_thread;
            CLIENT_PTR client_socket = (CLIENT_PTR)malloc(sizeof(CLIENT));
            *client_socket = client_i;
            pthread_create(&client_thread, NULL, handleClient, (void *)client_socket);
            pthread_detach(client_thread);
        } else {
            snprintf(buffer, sizeof(buffer), "Connection requested fail");
            use_window(chat_win, printInChatWin, buffer);
        }
    }

    shutdownServer(c_list);
    return 0;
}
