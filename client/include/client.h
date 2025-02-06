#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

// Default server connection details
#define DEFAULT_SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 8080

// Maximum buffer size for communication
#define BUFFER_SIZE 1024

// Mutex for thread safety in communication
extern pthread_mutex_t server_mutex;

// Global server socket (used across threads)
extern int server_socket;

/**
 * @brief Establishes a connection to the ERPO server.
 * 
 * @param server_ip   IP address of the server.
 * @param server_port Port number of the server.
 * @return int        Returns 0 on success, -1 on failure.
 */
int connect_to_server(const char *server_ip, int server_port);

/**
 * @brief Sends a message to the ERPO server.
 * 
 * @param message The message to send.
 * @return int    Returns 0 on success, -1 on failure.
 */
int send_message_to_server(const char *message);

/**
 * @brief Receives a response from the ERPO server.
 * 
 * @param buffer  Buffer to store the received response.
 * @param size    Size of the buffer.
 * @return int    Returns the number of bytes received, or -1 on failure.
 */
int receive_message_from_server(char *buffer, size_t size);

/**
 * @brief Closes the connection to the ERPO server.
 */
void close_connection();

#endif // CLIENT_H
