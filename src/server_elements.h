///////////////////////////////////////////////////////////////////////////////
//
// Server Elements Header
//
///////////////////////////////////////////////////////////////////////////////

//Verification ////////////////////////////////////////////////////////////////
#ifndef SERVER_ELEMENTS_H
#define SERVER_ELEMENTS_H

#define PORT 1234
#define BUFFER_SIZE 1024
#define SERVER "192.168.1.105"
#define USER "RIVERA_USER"
#define PASSWORD "8790"
#define DATABASE "RIVERACOM_DB"

//Includes ////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <mysql/mysql.h>
#include <ncurses.h>


//Variables ///////////////////////////////////////////////////////////////////

char buffer[BUFFER_SIZE], receiver_name[BUFFER_SIZE], sender_name[BUFFER_SIZE], query[512], *temp = NULL;
int client, server, line=1, max_width, BUFFER_SEND_SIZE, connected_clients;

//Structs /////////////////////////////////////////////////////////////////////

typedef struct client_information{
    int client_socket;
    char ip[INET_ADDRSTRLEN];
    char name[BUFFER_SIZE];
}CLIENT, *CLIENT_PTR;

typedef struct connected_clients{
    CLIENT client;
    struct connected_clients *next;
}CLIENT_LIST, *CLIENT_LIST_PTR;
    
//Global Vars /////////////////////////////////////////////////////////////////

MYSQL *conn;
MYSQL_RES *res;
MYSQL_ROW row;
CLIENT_LIST_PTR c_list;
WINDOW *chat_win, *input_win;
pthread_mutex_t lock;
struct sockaddr_in server_addr, client_addr;
socklen_t addr_size = sizeof(client_addr);

// Functions //////////////////////////////////////////////////////////////////

// Functions of server //
MYSQL_RES* executeQuery(const char *query);
int userExists(CLIENT client);
void registerUser(CLIENT client);
CLIENT_LIST_PTR addClientConn(CLIENT_LIST_PTR c_list, CLIENT client);
CLIENT_LIST_PTR removeClientConn(CLIENT_LIST_PTR c_list, CLIENT client);
void disconnectAllClients(CLIENT_LIST_PTR c_list);
void shutdownServer(CLIENT_LIST_PTR c_list);
int printInChatWin(WINDOW *win, void *arg);
int clearChatWin(WINDOW *win, void *arg);
int clearInputWin(WINDOW *win, void *arg);
void *handleClient(void *arg);
void *send_messages(void *arg);

// Functions of client //
void *receive_messages();

// Functions for both //
void setup();
int main();

// Fucntions of client ////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////////
#endif
///////////////////////////////////////////////////////////////////////////////