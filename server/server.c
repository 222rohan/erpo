#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "console.h"  // Custom header for console

int server_sock;

#define PORT 8080
#define MAX_CLIENTS 1000

// Function prototypes
void add_or_update_client(int client_sock, const char *ip);
void *handle_client(void *arg);

// Function to set up the server and call the console
void run_server() {
    int *new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t thread_id;

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_sock, 10) == -1) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server %s running on port %d...\n", inet_ntoa(server_addr.sin_addr), PORT);

    // Launch the interactive console in a separate thread
    pthread_t console_thread;
    pthread_create(&console_thread, NULL, (void *(*)(void *))run_console, NULL);

    // Accept and handle clients
    while (1) {
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock == -1) {
            perror("Accept failed");
            continue;
        }

        // Get client IP
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        printf("New connection from %s\n", client_ip);

        pthread_mutex_lock(&client_mutex);
        add_or_update_client(client_sock, client_ip);
        pthread_mutex_unlock(&client_mutex);

        // Create a thread to handle the client
        new_sock = malloc(sizeof(int));
        *new_sock = client_sock;
        pthread_create(&thread_id, NULL, handle_client, new_sock);
    }

    close(server_sock);
}

// Function to add or update a client in the clients array
void add_or_update_client(int client_sock, const char *ip) {
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].client_ip, ip) == 0) {
            // Update existing client
            clients[i].socket_id = client_sock;
            clients[i].conn_status = 1;
            clients[i].time_connected = time(NULL);
            return;
        }
    }

    // Add new client
    if (client_count < MAX_CLIENTS) {
        clients[client_count].socket_id = client_sock;
        strcpy(clients[client_count].client_ip, ip);
        clients[client_count].conn_status = 1;
        clients[client_count].time_connected = time(NULL);
        clients[client_count].last_conn = time(NULL);  // Initialize last_conn
        client_count++;
    } else {
        printf("Maximum client limit reached. Cannot add new client.\n");
    }
}

// Function to handle individual client connections
void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);

    char buffer[1024];
    ssize_t bytes_read;

    while ((bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Client %d: %s\n", client_sock, buffer);
    }

    printf("Client %d disconnected.\n", client_sock);

    // Update client status
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket_id == client_sock) {
            clients[i].conn_status = 0;
            clients[i].last_conn = time(NULL);  // Update last connection time
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);

    close(client_sock);
    return NULL;
}

// Main function
int main() {
    run_server();
    return 0;
}
