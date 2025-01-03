#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "console.h"

// Global variables
int server_sock;
Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function pointer structure for commands
typedef struct {
    const char *name;
    void (*handler)(const char *args);
} Command;

// Function prototypes
void parse_command(const char *command);
void format_time_diff(time_t start, time_t end, char *buffer, size_t size);

// Array of commands
Command commands[] = {
    {"help", con_help},
    {"stop", con_stop},
    {"kick", con_kick},
    {"status", con_status},
    {NULL, NULL}  // End marker
};

// Run the interactive console
void run_console() {
    char command[256];

    while (1) {
        printf("[server]> ");
        if (fgets(command, sizeof(command), stdin) == NULL) continue;

        command[strcspn(command, "\n")] = 0;  // Remove newline
        parse_command(command);
    }
}

// Command implementations
void con_help(const char *args) {
    printf("Available commands:\n");
    printf(" help                         - Display this help message\n");
    printf(" stop                         - Shutdown the server\n");
    printf(" kick <ip> / [OPTIONS]        - Disconnect a specific client by IP / all clients\n");
    printf(" status <ip> / [OPTIONS]      - Show info about a specific client / online clients / all\n");
}

void con_stop(const char *args) {
    printf("Stopping server...\n");
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].conn_status == 1) {
            close(clients[i].socket_id);
            clients[i].conn_status = 0;
        }
    }
    pthread_mutex_unlock(&client_mutex);
    close(server_sock);
    exit(0);
}

void con_kick(const char *args) {
    if (strcmp(args, "-a") == 0) {
        pthread_mutex_lock(&client_mutex);
        for (int i = 0; i < client_count; i++) {
            if (clients[i].conn_status == 1) {
                close(clients[i].socket_id);
                clients[i].conn_status = 0;
            }
        }
        pthread_mutex_unlock(&client_mutex);
        printf("Kicked all clients.\n");
    } else {
        pthread_mutex_lock(&client_mutex);
        for (int i = 0; i < client_count; i++) {
            if (strcmp(clients[i].client_ip, args) == 0 && clients[i].conn_status == 1) {
                close(clients[i].socket_id);
                clients[i].conn_status = 0;
                printf("Kicked client %s\n", args);
                pthread_mutex_unlock(&client_mutex);
                return;
            }
        }
        pthread_mutex_unlock(&client_mutex);
        printf("Client %s not online or doesn't exist.\n", args);
    }
}

void con_status(const char *args) {
    if (strcmp(args, "-o") == 0) {
        int online_count = 0;
        pthread_mutex_lock(&client_mutex);
        printf("Online clients:\n");
        for (int i = 0; i < client_count; i++) {
            if (clients[i].conn_status == 1) {
                online_count++;
                char time_connected[64];
                format_time_diff(clients[i].time_connected, time(NULL), time_connected, sizeof(time_connected));
                printf("%s - %s\n", clients[i].client_ip, time_connected);
            }
        }
        pthread_mutex_unlock(&client_mutex);
        printf("%d/%d online\n", online_count, client_count);
    } else if (strcmp(args, "-a") == 0) {
        pthread_mutex_lock(&client_mutex);
        printf("Showing %d entries:\n", client_count);
        for (int i = 0; i < client_count; i++) {
            char time_connected[64];
            format_time_diff(clients[i].time_connected, time(NULL), time_connected, sizeof(time_connected));
            printf("%s - %s - %s\n", clients[i].client_ip,
                   clients[i].conn_status ? "Online" : "Offline", time_connected);
        }
        pthread_mutex_unlock(&client_mutex);
    } else {
        pthread_mutex_lock(&client_mutex);
        for (int i = 0; i < client_count; i++) {
            if (strcmp(clients[i].client_ip, args) == 0) {
                printf("Client IP: %s\n", clients[i].client_ip);
                printf("Status: %s\n", clients[i].conn_status ? "Online" : "Offline");
                char time_connected[64];
                format_time_diff(clients[i].time_connected, time(NULL), time_connected, sizeof(time_connected));
                printf("Time connected: %s\n", time_connected);
                pthread_mutex_unlock(&client_mutex);
                return;
            }
        }
        pthread_mutex_unlock(&client_mutex);
        printf("IP %s invalid.\n", args);
    }
}

// Command parser
void parse_command(const char *command) {
    char base[16], args[240];
    sscanf(command, "%15s %239[^\n]", base, args);

    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(base, commands[i].name) == 0) {
            commands[i].handler(args);
            return;
        }
    }
    printf("Invalid command. Type 'help' for a list of commands.\n");
}

// Utility function to format time difference
void format_time_diff(time_t start, time_t end, char *buffer, size_t size) {
    int seconds = (int)difftime(end, start);
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    seconds %= 60;
    snprintf(buffer, size, "%02d:%02d:%02d", hours, minutes, seconds);
}
