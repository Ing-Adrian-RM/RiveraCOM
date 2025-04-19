///////////////////////////////////////////////////////////////////////////////
//
// Server Elements Header
//
///////////////////////////////////////////////////////////////////////////////

//Verification ////////////////////////////////////////////////////////////////
#ifndef SERVER_ELEMENTS_H
#define SERVER_ELEMENTS_H

#define PORT 1234
#define DISCOVERY_PORT 8888
#define BUFFER_SIZE 1024
#define QUERY_SIZE 512
//#define SERVER "172.24.102.88"
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
#include <ifaddrs.h>
#include <sys/stat.h>


//Variables ///////////////////////////////////////////////////////////////////

extern char SERVER[BUFFER_SIZE];
char query[QUERY_SIZE], *temp = NULL, linkedTo[100] = "Server";
int client, server, line=1, max_width, BUFFER_SEND_SIZE, connected_clients;

//Structs /////////////////////////////////////////////////////////////////////

typedef struct client_information{
    int socket;
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
CLIENT client_i;
CLIENT_LIST_PTR c_list;
WINDOW *chat_win, *input_win;
pthread_mutex_t lock;
struct sockaddr_in server_addr, client_addr;
socklen_t addr_size = sizeof(client_addr);

// Functions //////////////////////////////////////////////////////////////////

// Functions of server //
MYSQL_RES* executeDataQuery(const char *query);
unsigned long executeEnumQuery(const char *query);
int userDBExists(char *search_by, int option);
CLIENT registerDBUser(CLIENT client);
void printDBUsers();
char *updateDBUser(char *command, char *output);
char *deleteDBUser(char *command, char *output);
char *printClientConn(char *buffer);
CLIENT_LIST_PTR addClientConn(CLIENT_LIST_PTR c_list, CLIENT client);
CLIENT_LIST_PTR removeClientConn(CLIENT_LIST_PTR c_list, CLIENT client);
void disconnectAllClients(CLIENT_LIST_PTR c_list);
char *linkedToFunction(char *command, char *output);
void shutdownServer(CLIENT_LIST_PTR c_list);
int printInChatWin(WINDOW *win, void *arg);
int clearChatWin(WINDOW *win, void *arg);
int clearInputWin(WINDOW *win, void *arg);
void sendGif(char *buffer, CLIENT client);
void receiveGif(char *buffer, CLIENT client);
void *handleClient(void *arg);
char *help(char *buffer);
void *inputWindowManagement(void *arg);
void *discovery();

// Functions of client //
void *receive_messages();
void discoverServer();

// Functions for both //
void setup();
int main();

// Fucntions of client ////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////////
#endif
///////////////////////////////////////////////////////////////////////////////