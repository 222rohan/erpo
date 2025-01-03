#ifndef CONSOLE_H
#define CONSOLE_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <arpa/inet.h>

// Maximum number of clients
#define MAX_CLIENTS 1000

// Client structure
typedef struct {
    int socket_id;
    char client_ip[INET_ADDRSTRLEN];
    int conn_status;  // 0: disconnected, 1: connected
    time_t time_connected;
    time_t last_conn;
} Client;

// Extern variables for global access
extern int server_sock;
extern Client clients[MAX_CLIENTS];
extern int client_count;
extern pthread_mutex_t client_mutex;

// Function prototypes
void run_console();

// Command function prototypes
void con_help(const char *args);
void con_stop(const char *args);
void con_kick(const char *args);
void con_status(const char *args);

#endif  // CONSOLE_H
