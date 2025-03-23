///////////////////////////////////////////////////////////////////////////////
//
// Server Module
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
// userExists-> 
///////////////////////////////////////////////////////////////////////////////
int userExists(CLIENT client) {
    int exists = 0;
    snprintf(query, sizeof(query), "SELECT * FROM users WHERE ip='%s'", client.ip);
    res = executeQuery(query);
    if ((row = mysql_fetch_row(res))) exists = 1;
    mysql_free_result(res);
    return exists;
}

///////////////////////////////////////////////////////////////////////////////
// registerUser-> 
///////////////////////////////////////////////////////////////////////////////
void registerUser(CLIENT client) {
    snprintf(query, sizeof(query), "INSERT INTO users (ip, name, balance, uploaded, downloaded) VALUES ('%.255s', '%.255s', 0, 0, 0)", client.ip, client.name);
    executeQuery(query);
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
                return c_list;
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
                c_list = current->next;
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
        free(temp);
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
// *handleClient-> 
///////////////////////////////////////////////////////////////////////////////
void *handleClient(void *arg) {

    char buffer[BUFFER_SIZE];
    int bytesReceived;
    char name[BUFFER_SIZE];
    CLIENT client = *(CLIENT *)arg;
    if (userExists(client)) {
        snprintf(query, sizeof(query), "SELECT name FROM users WHERE ip='%s'", client.ip);
        res = executeQuery(query);
        row = mysql_fetch_row(res);
        strncpy(client.name, (const char *)row[0], sizeof(buffer) - 1);
        client.name[sizeof(client.name) - 1] = '\0';
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, sizeof(buffer) - 1, "Welcome back, %.900s!", client.name);
        buffer[sizeof(buffer) - 1] = '\0';
        send(client.socket, buffer, strlen(buffer), 0);
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "%.900s Connected, IP: %.100s\n", client.name, client.ip);
        use_window(chat_win, printInChatWin, buffer);
    }
    else {
        send(client.socket, "You are not registered. Please enter a unique username:\n", 56, 0);
        memset(buffer, '\0', BUFFER_SIZE);
        bytesReceived = 0;
        while (bytesReceived == 0) {
            bytesReceived = read(client.socket, buffer, sizeof(buffer));
        }
        strncpy(client.name, buffer, sizeof(buffer) - 1);
        client.name[sizeof(client.name) - 1] = '\0';
        registerUser(client);
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, sizeof(buffer), "%.900s Connected, IP: %.100s\n", client.name, client.ip);
        use_window(chat_win, printInChatWin, buffer);
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, sizeof(buffer), "Registration successful! Welcome, %.900s!\n", client.name);
        send(client.socket, buffer, strlen(buffer), 0);
    }

    // Handle further communication with the client
    while (1)
    {   
        char temp_buffer[2049];
        memset(buffer, '\0', BUFFER_SIZE);
        bytesReceived = read(client.socket, buffer, BUFFER_SIZE);
        snprintf(temp_buffer, sizeof(temp_buffer), "%s: %s", client.name,buffer);

        if (bytesReceived <= 0) {
            snprintf(buffer, sizeof(buffer) - 1, "Client %.900s disconnected.\n", client.name);
            buffer[sizeof(buffer) - 1] = '\0';
            use_window(chat_win, printInChatWin, temp_buffer);
            removeClientConn(c_list, client);
            break;
        }

        if (strncmp(buffer, "close", 5) == 0) {
            snprintf(buffer, sizeof(buffer), "%.900s has disconnected.\n", client.name);
            use_window(chat_win, printInChatWin, temp_buffer);
            removeClientConn(c_list, client);
            break;
        }
        use_window(chat_win, printInChatWin, temp_buffer);
    }

    removeClientConn(c_list, client);
    free(arg);
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// *send_messages-> 
///////////////////////////////////////////////////////////////////////////////
void *send_messages(void *arg)
{
    char buffer[BUFFER_SEND_SIZE];
    strncpy(sender_name, "Server", BUFFER_SIZE);
    sender_name[sizeof("Server")] = '\0';
    strncpy(receiver_name, "Broadcast", BUFFER_SIZE);
    receiver_name[sizeof("Broadcast")] = '\0';

    while (1)
    {
        char temp_buffer[2049];
        use_window(input_win, clearInputWin, 0);
        memset(buffer, '\0', BUFFER_SEND_SIZE);
        wgetnstr(input_win, buffer, BUFFER_SEND_SIZE - sizeof("Message: "));
        snprintf(temp_buffer, sizeof(temp_buffer), "Server: %s",buffer);

        if (strncmp(buffer, "close", 5) == 0) shutdownServer(c_list);
        else if (strncmp(buffer, "clear", 5) == 0) use_window(chat_win, clearChatWin, 0);
        else use_window(chat_win, printInChatWin, temp_buffer);

        if (strncmp(receiver_name, "Broadcast", 9) == 0) {
            pthread_mutex_lock(&lock);
            for (CLIENT_LIST_PTR ptr = c_list; ptr != NULL; ptr = ptr->next) {
                send(ptr->client.socket, buffer, strlen(buffer), 0);
            }
            pthread_mutex_unlock(&lock);
        }
    }
    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// setup-> 
///////////////////////////////////////////////////////////////////////////////
void setup()
{   
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
    pthread_create(&send_thread, NULL, send_messages, NULL);
    pthread_detach(send_thread);
}

int main()
{
    setup();
    char buffer[BUFFER_SEND_SIZE];

    if (listen(server, 1) < 0)
    {   
        snprintf(buffer, sizeof(buffer), "Server initalized. Type 'close' to terminate.");
        use_window(chat_win, printInChatWin, buffer);
        perror("Listen error");
        close(server);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        client = accept(server, (struct sockaddr *)&client_addr, &addr_size);
        CLIENT client_i;
        client_i.socket = client;
        inet_ntop(AF_INET, &client_addr.sin_addr, client_i.ip, INET_ADDRSTRLEN);
        client_i.name[0] = '\0';
        if (client >= 0) {
            pthread_mutex_lock(&lock);
            c_list = addClientConn(c_list, client_i);
            pthread_mutex_unlock(&lock);
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
