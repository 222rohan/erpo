#include "../include/client.h"
#include "../include/logger.h"

/**
 * @brief Establishes a connection to the ERPO server.
 * 
 * @param server_ip   IP address of the server.
 * @param server_port Port number of the server.
 * @return int        Returns 0 on success, -1 on failure.
 */
int connect_to_server(const char *server_ip, int server_port) {
    struct sockaddr_in server_addr;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        log_event("Socket creation failed");
        return -1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        log_event("Invalid IP address format");
        close(server_socket);
        return -1;
    }

    // Connect to the server
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        log_event("Failed to connect to server");
        close(server_socket);
        return -1;
    }

    log_event("Connected to ERPO server");
    return 0;
}

/**
 * @brief Sends a message to the ERPO server.
 * 
 * @param message The message to send.
 * @return int    Returns 0 on success, -1 on failure.
 */
int send_message_to_server(const char *message) {
    pthread_mutex_lock(&server_mutex);
    int status = send(server_socket, message, strlen(message), 0);
    pthread_mutex_unlock(&server_mutex);

    if (status == -1) {
        log_event("Failed to send message to server");
        return -1;
    }

    log_event("Message sent to server");
    return 0;
}

/**
 * @brief Receives a response from the ERPO server.
 * 
 * @param buffer  Buffer to store the received response.
 * @param size    Size of the buffer.
 * @return int    Returns the number of bytes received, or -1 on failure.
 */
int receive_message_from_server(char *buffer, size_t size) {
    memset(buffer, 0, size);
    
    pthread_mutex_lock(&server_mutex);
    int bytes_received = recv(server_socket, buffer, size - 1, 0);
    pthread_mutex_unlock(&server_mutex);

    if (bytes_received <= 0) {
        log_event("Failed to receive message from server");
        return -1;
    }

    log_event("Message received from server");
    return bytes_received;
}

/**
 * @brief Closes the connection to the ERPO server.
 */
void close_connection() {
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
        log_event("Connection to server closed");
    }
}
