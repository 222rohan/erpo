#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "../include/client.h"
#include "../include/network_monitor.h"
#include "../include/system_monitor.h"
#include "../include/logger.h"

#define DEFAULT_SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 8080

// Global variables
int server_socket;
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void *network_monitor_thread(void *arg);
void *system_monitor_thread(void *arg);
void daemonize();
void signal_handler(int sig);

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    pthread_t net_thread, sys_thread;

    // Parse command-line arguments (server IP & port)
    const char *server_ip = (argc > 1) ? argv[1] : DEFAULT_SERVER_IP;
    int server_port = (argc > 2) ? atoi(argv[2]) : DEFAULT_SERVER_PORT;

    // Convert client process to a daemon
    daemonize();

    // Handle termination signals
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        log_event("Socket creation failed");
        return EXIT_FAILURE;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        log_event("Invalid IP address format");
        close(server_socket);
        return EXIT_FAILURE;
    }

    // Connect to the server
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        log_event("Failed to connect to server");
        close(server_socket);
        return EXIT_FAILURE;
    }

    log_event("Connected to ERPO server");

    // Start monitoring threads
    pthread_create(&net_thread, NULL, network_monitor_thread, &server_socket);
    pthread_create(&sys_thread, NULL, system_monitor_thread, &server_socket);

    // Join monitoring threads (daemon runs indefinitely)
    pthread_join(net_thread, NULL);
    pthread_join(sys_thread, NULL);

    // Cleanup
    close(server_socket);
    return EXIT_SUCCESS;
}

// ---------------------- THREAD FUNCTIONS ----------------------

// Network monitoring thread
void *network_monitor_thread(void *arg) {
    int server_socket = *((int *)arg);
    log_event("[Network Monitor] Started");
    monitor_network(server_socket);
    return NULL;
}

// System monitoring thread
void *system_monitor_thread(void *arg) {
    int server_socket = *((int *)arg);
    log_event("[System Monitor] Started");
    monitor_system(server_socket);
    return NULL;
}

// ---------------------- DAEMON FUNCTION ----------------------
void daemonize() {
    pid_t pid;

    // Fork the process
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);  // Parent process exits
    }

    // Create a new session
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    // Fork again to ensure no terminal control
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Set file permissions
    umask(0);

    // Change working directory to root
    chdir("/");

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    log_event("Client daemon started successfully");
}

// ---------------------- SIGNAL HANDLER ----------------------
void signal_handler(int sig) {
    log_event("Received termination signal, shutting down...");
    close(server_socket);
    exit(EXIT_SUCCESS);
}
