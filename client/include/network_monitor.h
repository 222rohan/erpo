#ifndef NETWORK_MONITOR_H
#define NETWORK_MONITOR_H

#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>


/**
 * @brief Captures network traffic and detects specific activities.
 * 
 * @param arg The server socket (for sending logs).
 */
int monitor_network(int server_socket);

#endif // NETWORK_MONITOR_H
