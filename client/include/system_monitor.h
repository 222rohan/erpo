#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

/**
 * @brief Monitors system activity such as USB insertions and file modifications.
 * 
 * @param server_socket The server socket to send logs.
 */
int monitor_system(int server_socket);

#endif // SYSTEM_MONITOR_H
