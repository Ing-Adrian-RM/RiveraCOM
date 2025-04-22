///////////////////////////////////////////////////////////////////////////////
//
// Module
// Author: Adrián Rodríguez Murillo
//
///////////////////////////////////////////////////////////////////////////////

#include "server_elements.h"

///////////////////////////////////////////////////////////////////////////////
// executeDataQuery-> Execute a query that returns data
///////////////////////////////////////////////////////////////////////////////
MYSQL_RES* executeDataQuery(const char *query) {
    char temp_buffer[BUFFER_SIZE];
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, LOCAL_HOST, USER, PASSWORD, DATABASE, 0, NULL, 0)) {
        memset(temp_buffer, '\0', BUFFER_SIZE);
        snprintf(temp_buffer, BUFFER_SIZE, "Connection failed: %s", mysql_error(conn));
        use_window(chat_win, printInChatWin, temp_buffer);
        return NULL;
    }
    if (mysql_query(conn, query)) {
        memset(temp_buffer, '\0', BUFFER_SIZE);
        snprintf(temp_buffer, BUFFER_SIZE, "Error executing query: %s\n", mysql_error(conn));
        use_window(chat_win, printInChatWin, temp_buffer);
        return NULL;
    }
    res = mysql_store_result(conn);
    mysql_close(conn);
    return res;
}

///////////////////////////////////////////////////////////////////////////////
// executeDataQuery-> Execute a query that return affected rows
///////////////////////////////////////////////////////////////////////////////
unsigned long executeEnumQuery(const char *query) {
    char temp_buffer[BUFFER_SIZE];
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, LOCAL_HOST, USER, PASSWORD, DATABASE, 0, NULL, 0)) {
        memset(temp_buffer, '\0', BUFFER_SIZE);
        snprintf(temp_buffer, BUFFER_SIZE, "Connection failed: %s", mysql_error(conn));
        use_window(chat_win, printInChatWin, temp_buffer);
        return 0;
    }
    if (mysql_query(conn, query)) {
        memset(temp_buffer, '\0', BUFFER_SIZE);
        snprintf(temp_buffer, BUFFER_SIZE, "Error executing query: %s\n", mysql_error(conn));
        use_window(chat_win, printInChatWin, temp_buffer);
        return 0;
    }
    unsigned long affected = mysql_affected_rows(conn);
    mysql_close(conn);
    return affected;
}

///////////////////////////////////////////////////////////////////////////////
// userDBExists-> Use to check if a user exists in the database
///////////////////////////////////////////////////////////////////////////////
int userDBExists(char *search_by, int option) {
    int exists = 0;
    memset(query, '\0', QUERY_SIZE);
    if (option == 0) {
        snprintf(query, sizeof(query), "SELECT * FROM users WHERE name='%s'", search_by);
    } else if (option == 1) {
        snprintf(query, sizeof(query), "SELECT * FROM users WHERE ip='%s'", search_by);
    }
    res = executeDataQuery(query);
    if ((row = mysql_fetch_row(res))) exists = 1;
    mysql_free_result(res);
    return exists;
}

///////////////////////////////////////////////////////////////////////////////
// registerDBUser-> Register a new user in the database
///////////////////////////////////////////////////////////////////////////////
CLIENT registerDBUser(CLIENT client) {
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    int bytesReceived;
    char *ip = client.ip;
    if (userDBExists(ip,1)) {
        snprintf(query, QUERY_SIZE, "SELECT name FROM users WHERE ip='%s'", client.ip);
        res = executeDataQuery(query);
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
        snprintf(buffer, BUFFER_SIZE, "You are not registered.\nPlease enter a unique username between 2 to 20 characters without spaces:");
        send(client.socket, buffer, strlen(buffer), 0);
        bytesReceived = 0;
        int name_accepted = 0;
        while (!name_accepted) {
            memset(buffer, '\0', BUFFER_SIZE);
            bytesReceived = read(client.socket, buffer, sizeof(buffer));
            if (bytesReceived <= 0) {client.socket = -1; return client;}
            strtok(buffer, ":"); char *token = strtok(NULL, ":");
            strncpy(client.name, token, strlen(token));
            if (userDBExists(token, 0) || strlen(token) < 2 || strlen(token) > 20 || strpbrk(token, " ")) {
                memset(temp_buffer, '\0', BUFFER_SIZE);
                snprintf(temp_buffer, sizeof(temp_buffer), "Invalid Username.\nPlease enter a unique username between 2 to 20 characters without spaces:");
                send(client.socket, temp_buffer, strlen(temp_buffer), 0);
            } else {
                name_accepted = 1;
            }
        }
        client.name[strlen(client.name)] = '\0';
        snprintf(query, sizeof(query), "INSERT INTO users (ip, name, balance, uploaded, downloaded) VALUES ('%.255s', '%.255s', 0, 0, 0)", client.ip, client.name);
        executeDataQuery(query);
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
// printDBUsers-> Print all users data in the database
///////////////////////////////////////////////////////////////////////////////
void printDBUsers() {
    char buffer[BUFFER_SIZE];
    snprintf(query, sizeof(query), "SELECT * FROM users");
    res = executeDataQuery(query);
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
// updateDBUsers-> Update a user data in the database
///////////////////////////////////////////////////////////////////////////////
char *updateDBUser(char *command, char *output) {
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    char user_name[BUFFER_SIZE];
    strtok(command, "+"); char *value = strtok(NULL, "+");
    strtok(command, "*"); char *variable = strtok(NULL, "*");
    strtok(command, ":"); char *identificator = strtok(NULL, ":");
    strtok(command, " "); char *option = strtok(NULL, " ");
    char switcher[10] = {0};
    if (strncmp(option, "name", 4) == 0) snprintf(switcher, sizeof(switcher), "name");
    else if (strncmp(option, "ip", 2) == 0) snprintf(switcher, sizeof(switcher), "ip");
    if (userDBExists(identificator,0) || userDBExists(identificator,1)){
        memset(query, '\0', QUERY_SIZE);    
        if (strncmp(variable, "name", 4) == 0) {
            if (userDBExists(value, 0)) {
                memset(output, '\0', BUFFER_SIZE);
                snprintf(output, BUFFER_SIZE, "Username already exists. Request failed.");
                return output;
            }
            else snprintf(query, sizeof(query), "UPDATE users SET name='%s' WHERE %s='%s'", value, switcher, identificator);
        }
        else if (strncmp(variable, "balance", 7) == 0) snprintf(query, sizeof(query), "UPDATE users SET balance='%s' WHERE %s='%s'", value, switcher, identificator);
        else if (strncmp(variable, "uploaded", 8) == 0) snprintf(query, sizeof(query), "UPDATE users SET uploaded='%s' WHERE %s='%s'", value, switcher, identificator);
        else if (strncmp(variable, "downloaded", 10) == 0) snprintf(query, sizeof(query), "UPDATE users SET downloaded='%s' WHERE %s='%s'", value, switcher, identificator);
        unsigned long result = executeEnumQuery(query);
        if (result >= 0){
            memset(output, '\0', BUFFER_SIZE);
            snprintf(output, BUFFER_SIZE, "Data section '%s' of user %s updated in database.", variable, identificator);
            return output;
        } 
        else {
            memset(output, '\0', BUFFER_SIZE);
            snprintf(output, BUFFER_SIZE, "Error updating database.");
            return output;
        }    
    } else {
        memset(output, '\0', BUFFER_SIZE);
        snprintf(output, BUFFER_SIZE, "User %.100s not found in database", user_name);
        return output;
    }
}

///////////////////////////////////////////////////////////////////////////////
// deleteDBUsers-> Delete a user from the database
///////////////////////////////////////////////////////////////////////////////
char *deleteDBUser(char *command, char *output) {
    char temp_buffer[BUFFER_SIZE];
    char user_name[BUFFER_SIZE];
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
            res = executeDataQuery(query);
            row = mysql_fetch_row(res);
            strncpy(user_name, row[0], BUFFER_SIZE - 1);
            snprintf(query, sizeof(query), "DELETE FROM users WHERE ip='%s'", user);
        } 
        unsigned long result = executeEnumQuery(query);
        if (result > 0){
            memset(output, '\0', BUFFER_SIZE);
            snprintf(output, BUFFER_SIZE, "User deleted from database");
            return output;
        } else {
            memset(output, '\0', BUFFER_SIZE);
            snprintf(output, BUFFER_SIZE, "Error deleting user from database");
            return output;
        }
    } else {
        memset(output, '\0', BUFFER_SIZE);
        snprintf(output, BUFFER_SIZE, "User %.100s not found in database", user_name);
        return output;
    }
}

///////////////////////////////////////////////////////////////////////////////
// printClientConn-> Print the connected clients list
///////////////////////////////////////////////////////////////////////////////
char *printClientConn(char *buffer) {
    char temp_buffer[BUFFER_SIZE];

    if (c_list == NULL) {
        memset(buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "Connected clients list: NULL\n");
    } else {
        memset(buffer, '\0', BUFFER_SIZE);
        memset(temp_buffer, '\0', BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "Connected clients list:\n");
        for (CLIENT_LIST_PTR ptr = c_list; ptr != NULL; ptr = ptr->next) {
            snprintf(temp_buffer, sizeof(temp_buffer), " %.100s, IP: %.100s\n", ptr->client.name, ptr->client.ip);
            strncat(buffer, temp_buffer, BUFFER_SIZE - strlen(buffer) - 1);
        }
    }
    return buffer;
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
// linkedToFunction()-> Check if a user to link is available 
///////////////////////////////////////////////////////////////////////////////
char *linkedToFunction(char *command, char *output) {
    strtok(command, " "); char *user_name = strtok(NULL, " ");
    if (strncmp("Broadcast", user_name, strlen(user_name)) == 0) {
        snprintf(output, BUFFER_SIZE, "Broadcast");
        return output;
    }
    for (CLIENT_LIST_PTR ptr = c_list; ptr != NULL; ptr = ptr->next) {
        if (strncmp(ptr->client.name, user_name, strlen(user_name)) == 0) {
            snprintf(output, BUFFER_SIZE, "%s", user_name);
            return output;
        }
    }
    return NULL;
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
    endwin();
    exit(EXIT_SUCCESS);
}

///////////////////////////////////////////////////////////////////////////////
// print_in_chatwin-> Print a message in the chat window
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
// clear_chat_win-> Cleans the chat window
///////////////////////////////////////////////////////////////////////////////
int clearChatWin(WINDOW *win, void *arg) {
    
    werase(chat_win);
    box(chat_win, 0, 0);
    wrefresh(chat_win);
    line = 1;
    return OK;
}

///////////////////////////////////////////////////////////////////////////////
// clear_input_win-> Cleans the input window
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
        snprintf(temp_buffer, BUFFER_SIZE, "%s+%lu", buffer, file_info.st_size);
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
    snprintf(temp_buffer, BUFFER_SIZE, "Server: gif %.100s received. Total bytes transmited: %zu", gif_name, total_bytes_received);
    use_window(chat_win, printInChatWin, temp_buffer);
}

///////////////////////////////////////////////////////////////////////////////
// *handleClient-> Handle the client connection
///////////////////////////////////////////////////////////////////////////////
void *handleClient(void *arg) {

    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    char output[BUFFER_SIZE];
    int bytesReceived;
    CLIENT client = *(CLIENT *)arg;
    client = registerDBUser(client);
    if (client.socket < 0) {
        snprintf(buffer, BUFFER_SIZE, "Error: Failed to register user.");
        use_window(chat_win, printInChatWin, buffer);
        close(client.socket);
        free(arg);
        return NULL;
    } 
    c_list = addClientConn(c_list, client);

    // Handle further communication with the client
    while (1)
    {   
        memset(temp_buffer, '\0', BUFFER_SIZE);
        memset(buffer, '\0', BUFFER_SIZE);
        memset(output, '\0', BUFFER_SIZE);
        
        bytesReceived = read(client.socket, buffer, BUFFER_SIZE);
        snprintf(temp_buffer, BUFFER_SIZE, "%.100s: %.900s", client.name,buffer);

        if (bytesReceived <= 0) {
            snprintf(buffer, BUFFER_SIZE, "%.900s disconnected", client.name);
            use_window(chat_win, printInChatWin, buffer);
            break;
        }
        if (strncmp(buffer, ".link", 5) == 0) {
            if (linkedToFunction(buffer,output) != NULL) {
                memset(buffer, '\0', sizeof(buffer));
                snprintf(buffer, BUFFER_SIZE, "YES");
                send(client.socket, buffer, strlen(buffer), 0);
            } else {
                memset(buffer, '\0', sizeof(buffer));
                snprintf(buffer, BUFFER_SIZE, "NO");
                send(client.socket, buffer, strlen(buffer), 0);
            }
        }
        else if (strncmp(buffer, ".listc", 6) == 0) {
            memset(temp_buffer, '\0', sizeof(temp_buffer));
            send(client.socket, printClientConn(temp_buffer), strlen(temp_buffer), 0);
        }
        else if (strncmp(buffer, ".rename", 6) == 0) {
            strtok(buffer, " "); char *token = strtok(NULL, " ");
            memset(temp_buffer, '\0', sizeof(temp_buffer));
            snprintf(temp_buffer, BUFFER_SIZE, ".updatedb name:%.100s*name+%s", client.name, token);
            updateDBUser(temp_buffer,output);
            send(client.socket, output, strlen(output), 0);
        }
        else if (strncmp(buffer, ".recharge", 9) == 0) {
            strtok(buffer, " "); char *token = strtok(NULL, " ");
            memset(temp_buffer, '\0', sizeof(temp_buffer));
            snprintf(temp_buffer, BUFFER_SIZE, ".updatedb name:%.100s*balance+%s", client.name, token);
            updateDBUser(temp_buffer,output);
            send(client.socket, output, strlen(output), 0);
        }
        else if (strncmp(buffer, ".deleteuser", 9) == 0) {
            memset(temp_buffer, '\0', sizeof(temp_buffer));
            snprintf(temp_buffer, BUFFER_SIZE, ".deletedb name:%.100s", client.name);
            deleteDBUser(temp_buffer,output);
            send(client.socket, output, strlen(output), 0);
        }
        else if (strncmp(buffer, ".userinfo", 6) == 0) {
            memset(query, '\0', sizeof(query));
            snprintf(query, sizeof(query), "SELECT * FROM users WHERE name='%.100s'", client.name);
            res = executeDataQuery(query);
            row = mysql_fetch_row(res);
            if (res != NULL) {
                memset(temp_buffer, '\0', sizeof(temp_buffer));
                snprintf(temp_buffer, BUFFER_SIZE, "IP: %s, Name: %s, Balance: %s, Uploaded: %s, Downloaded: %s\n", row[0], row[1], row[2], row[3], row[4]);
                send(client.socket, temp_buffer, strlen(temp_buffer), 0);
            } else {
                memset(temp_buffer, '\0', sizeof(temp_buffer));
                snprintf(temp_buffer, BUFFER_SIZE, "Error executing query");
                send(client.socket, temp_buffer, strlen(temp_buffer), 0);
            }
        } else if ((strncmp(buffer, ".m", 2) == 0)) {
            strtok(buffer, ":"); char *message = strtok(NULL, ":");
            strtok(buffer, "|"); char *link = strtok(NULL, "|");
            memset(temp_buffer, '\0', sizeof(temp_buffer));
            snprintf(temp_buffer, BUFFER_SIZE, "%.100s: %.900s", client.name, message);
            int gif = 0;
            if (strncmp(message, " gif", 4) == 0) {receiveGif(message, client); gif = 1;}
            else if (strncmp(linkedTo, "Broadcast", 9) == 0) {
                for (CLIENT_LIST_PTR ptr = c_list; ptr != NULL; ptr = ptr->next) {
                    send(ptr->client.socket, temp_buffer, strlen(temp_buffer), 0);
                    if (gif) sendGif(message, ptr->client);
                }
            } else if (strncmp(link, "Server", 6) == 0) {
                use_window(chat_win, printInChatWin, temp_buffer);  
            } else {
                int found = 0;
                for (CLIENT_LIST_PTR ptr = c_list; ptr != NULL; ptr = ptr->next) {
                    if (strncmp(ptr->client.name, link, strlen(link)) == 0) {
                        send(ptr->client.socket, temp_buffer, strlen(temp_buffer), 0);
                        if (gif) sendGif(message, ptr->client);
                        found = 1;
                    }
                }
                if (!found) {
                    memset(temp_buffer, '\0', sizeof(temp_buffer));
                    snprintf(temp_buffer, BUFFER_SIZE, "Server: The user is no longer available.");
                    send(client.socket, temp_buffer, strlen(temp_buffer), 0);
                }
            }
        }
    }

    c_list = removeClientConn(c_list, client);
    free(arg);
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// help-> Print the help menu
///////////////////////////////////////////////////////////////////////////////
char *help(char *buffer) {
    memset(buffer, '\0', BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "Commands:\n .close: Close the server\n .clear: Clear the chat window\n .listc: List connected clients\n .listdb: List users in the database\n .deletedb: Delete a user from the database\n .updatedb: Update a user in the database\n .link <user>: Link to a user\n .unlink: Unlink from a user\n .help: Show this help menu\n @<user>: Send a message to a specific user\n @B:<message>: Broadcast a message to all users");
    return buffer;
}

///////////////////////////////////////////////////////////////////////////////
// *inputWindowManagement-> Manage the input window
///////////////////////////////////////////////////////////////////////////////
void *inputWindowManagement(void *arg)
{
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    char output[BUFFER_SIZE];

    while (1)
    {   
        memset(buffer, '\0', BUFFER_SIZE);
        memset(temp_buffer, '\0', BUFFER_SIZE);
        memset(output, '\0', BUFFER_SIZE);
        use_window(input_win, clearInputWin, 0);
        wgetnstr(input_win, buffer, BUFFER_SEND_SIZE);
        snprintf(temp_buffer, BUFFER_SIZE, "Server: %.900s", buffer);
        use_window(chat_win, printInChatWin, temp_buffer);

        if (strncmp(buffer, ".close", 6) == 0) shutdownServer(c_list);
        else if (strncmp(buffer, ".clear", 6) == 0) {use_window(chat_win, clearChatWin, 0); use_window(input_win, clearInputWin, 0);}
        else if (strncmp(buffer, ".listc", 6) == 0) {
            memset(temp_buffer, '\0', BUFFER_SIZE);
            use_window(chat_win, printInChatWin, printClientConn(temp_buffer));
        }
        else if (strncmp(buffer, ".listdb", 7) == 0) printDBUsers();
        else if (strncmp(buffer, ".deletedb", 9) == 0) use_window(chat_win, printInChatWin, deleteDBUser(buffer,output));
        else if (strncmp(buffer, ".updatedb", 9) == 0) use_window(chat_win, printInChatWin, updateDBUser(buffer,output));
        else if (strncmp(buffer, ".link", 5) == 0) {
            if (linkedToFunction(buffer,output) != NULL) {
                memset(linkedTo, '\0', 100);
                snprintf(linkedTo, 100, "%.99s", output);
            } else {
                snprintf(temp_buffer, BUFFER_SIZE, "User not available.");
                use_window(chat_win, printInChatWin, temp_buffer);
            }
        }
        else if (strncmp(buffer, ".unlink", 7) == 0) {
            memset(linkedTo, '\0', 100);
            snprintf(linkedTo, 100, "Server");
        }
        else if (strncmp(buffer, ".help", 5) == 0) {
            memset(temp_buffer, '\0', BUFFER_SIZE);
            use_window(chat_win, printInChatWin, help(temp_buffer));
        }
        else if (strncmp(linkedTo, "Broadcast", 9) == 0) {
            for (CLIENT_LIST_PTR ptr = c_list; ptr != NULL; ptr = ptr->next) {
                if (strncmp(buffer, "gif", 3) == 0) sendGif(temp_buffer, ptr->client);
                else send(ptr->client.socket, temp_buffer, strlen(temp_buffer), 0);
            }
        } else if (strncmp(linkedTo, "Server", 6) != 0) {
            for (CLIENT_LIST_PTR ptr = c_list; ptr != NULL; ptr = ptr->next) {
                if (strncmp(ptr->client.name, linkedTo, strlen(linkedTo)) == 0) {
                    if (strncmp(buffer, "gif", 3) == 0) sendGif(temp_buffer, ptr->client);
                    else send(ptr->client.socket, temp_buffer, strlen(temp_buffer), 0);
                }
                else {
                    memset(temp_buffer, '\0', BUFFER_SIZE);
                    snprintf(temp_buffer, BUFFER_SIZE, "The user is no longer available.");
                    use_window(chat_win, printInChatWin, temp_buffer);
                }
            }
        }
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// discovery-> This function listens for discovery messages and responds 
// with the server's IP address
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
        memset(buffer, 0, BUFFER_SIZE);
        if (recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len) < 0) {
            perror("Error receiving message");
            continue;
        }
        if (strcmp(buffer, "DISCOVERY_MESSAGE") == 0) sendto(sock, server_ip, sizeof(server_ip), 0, (struct sockaddr *)&client_addr, addr_len);
    }

    close(sock);
}

///////////////////////////////////////////////////////////////////////////////
// setup-> Setup function to initialize the server
// and create necessary threads
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
    BUFFER_SEND_SIZE = COLS - 2 - sizeof("Message: ");
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
    snprintf(buffer, BUFFER_SIZE, "Server initalized. Type 'close' to terminate.");
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
        snprintf(buffer, BUFFER_SEND_SIZE, "Server initalized error. Press enter to close.");
        use_window(chat_win, printInChatWin, buffer);
        use_window(input_win, clearInputWin, 0);
        wgetnstr(input_win, buffer, BUFFER_SEND_SIZE);
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
            snprintf(buffer, BUFFER_SEND_SIZE, "Connection requested fail");
            use_window(chat_win, printInChatWin, buffer);
        }
    }

    shutdownServer(c_list);
    return 0;
}
