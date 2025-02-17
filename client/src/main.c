// main.c
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
#define HEARTBEAT_INTERVAL 10   // seconds between heartbeats
#define BUFFER_SIZE 1024

// Global variables
int server_socket;
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void *network_monitor_thread(void *arg);
void *system_monitor_thread(void *arg);
void *heartbeat_thread(void *arg);
void daemonize();
void signal_handler(int sig);

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    pthread_t net_thread, sys_thread, hb_thread;

    // Parse command-line arguments (server IP & port)
    const char *server_ip = (argc > 1) ? argv[1] : DEFAULT_SERVER_IP;
    int server_port = (argc > 2) ? atoi(argv[2]) : DEFAULT_SERVER_PORT;

    log_init();

    log_eventf("Connecting to server %s:%d", server_ip, server_port);
    //log_event("Connecting to server");
    // Convert client process to a daemon
    daemonize(); 
    log_eventf("Client daemon started successfully (PID: %d)", getpid());

    // Handle termination signals
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    // Connect to the server using function from client.c
    if (connect_to_server(server_ip, server_port) != 0) {
        log_event("Failed to connect to server, exiting.");
        return EXIT_FAILURE;
    }

    log_eventf("Connected to ERPO server at %s:%d", server_ip, server_port);

    // Start monitoring threads
    pthread_create(&net_thread, NULL, network_monitor_thread, &server_socket);
    //pthread_create(&sys_thread, NULL, system_monitor_thread, &server_socket);
    pthread_create(&hb_thread, NULL, heartbeat_thread, &server_socket);

    // Join threads (the heartbeat thread will exit if the server stops responding)
    pthread_join(net_thread, NULL);
    pthread_join(sys_thread, NULL);
    pthread_join(hb_thread, NULL);

    // Cleanup
    close(server_socket);
    return EXIT_SUCCESS;
}

// Network monitoring thread
void *network_monitor_thread(void *arg) {
    int sock = *((int *)arg);
    log_event("[Network Monitor] Starting...");
    if(monitor_network(sock) == -1){
        log_event("[Network Monitor] Failed to start");
        exit(EXIT_FAILURE);
    }
    return NULL;
}

// System monitoring thread
void *system_monitor_thread(void *arg) {
    int sock = *((int *)arg);
    log_event("[System Monitor] Started");
    monitor_system(sock);
    return NULL;
}

// Heartbeat thread: sends "heartbeat" messages and waits for "OK" reply.
// If no proper reply is received, the daemon exits.
void *heartbeat_thread(void *arg) {
    int sock = *((int *)arg);
    char response[BUFFER_SIZE];
    while (1) {
        sleep(HEARTBEAT_INTERVAL);
        if (send_message_to_server("heartbeat") < 0) {
            log_event("Heartbeat send failed; exiting client daemon");
            exit(EXIT_FAILURE);
        }
        if (receive_message_from_server(response, BUFFER_SIZE) <= 0 || strcmp(response, "OK") != 0) {
            log_event("Heartbeat response not received or invalid; exiting client daemon");
            exit(EXIT_FAILURE);
        }
        //log_event("Heartbeat OK received");
    }
    return NULL;
}

void daemonize() {
    log_event("Daemonizing client process...");

    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);  // Parent exits

    if (setsid() < 0) exit(EXIT_FAILURE);
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    // chdir("/");

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}


// Signal handler for graceful shutdown
void signal_handler(int sig) {
    log_event("Received termination signal, shutting down...");
    close(server_socket);
    exit(EXIT_SUCCESS);
}
