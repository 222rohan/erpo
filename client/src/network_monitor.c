#include "../include/network_monitor.h"
#include "../include/logger.h"
#include "../include/client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <pcap.h>

#define SNAP_LEN 1518  // Maximum bytes to capture per packet
#define TIMEOUT_MS 100 // Timeout for packet capture
#define FILTER_EXPRESSION "port 21 or port 22 or port 80 or port 443"  // FTP, SSH, HTTP, HTTPS

/**
 * @brief Packet handler function for processing captured packets.
 */
void packet_handler(u_char *server_socket_ptr, const struct pcap_pkthdr *header, const u_char *packet) {
    int server_socket = *((int *)server_socket_ptr);
    char log_msg[256];

    struct iphdr *ip_header = (struct iphdr *)(packet + 14); // Ethernet header = 14 bytes
    struct tcphdr *tcp_header = (struct tcphdr *)(packet + 14 + (ip_header->ihl * 4));

    int src_port = ntohs(tcp_header->source);
    int dst_port = ntohs(tcp_header->dest);

    // Determine protocol and log the activity
    if (dst_port == 21 || src_port == 21) {
        snprintf(log_msg, sizeof(log_msg), "FTP activity detected (port 21) from %s", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
    } else if (dst_port == 22 || src_port == 22) {
        snprintf(log_msg, sizeof(log_msg), "SSH activity detected (port 22) from %s", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
    } else if (dst_port == 80 || src_port == 80) {
        snprintf(log_msg, sizeof(log_msg), "HTTP activity detected (port 80) from %s", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
    } else if (dst_port == 443 || src_port == 443) {
        snprintf(log_msg, sizeof(log_msg), "HTTPS activity detected (port 443) from %s", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
    } else {
        return; // Ignore other traffic
    }

    log_event(log_msg);
    send_message_to_server(log_msg);
}

/**
 * @brief Captures network traffic and detects specific activities.
 */
void monitor_network(int server_socket) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    struct bpf_program filter;
    bpf_u_int32 net, mask;

    pcap_if_t *alldevs, *dev;
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "Error finding devices: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    dev = alldevs;
    if (dev == NULL) {
        fprintf(stderr, "No network devices found\n");
        exit(EXIT_FAILURE);
    }

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Network devices found: %s", dev->name);
    log_event(log_msg);

    // Get network address and mask
    if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
        log_event("Network monitor: Failed to get netmask");
        return;
    }

    // Open the network interface for packet capture
    handle = pcap_open_live(dev, SNAP_LEN, 1, TIMEOUT_MS, errbuf);
    if (handle == NULL) {
        log_event("Network monitor: Failed to open device");
        return;
    }

    // Apply packet filter
    if (pcap_compile(handle, &filter, FILTER_EXPRESSION, 0, net) == -1 || pcap_setfilter(handle, &filter) == -1) {
        log_event("Network monitor: Failed to set filter");
        pcap_close(handle);
        return;
    }

    log_event("Network monitor started");

    // Capture packets indefinitely
    pcap_loop(handle, 0, packet_handler, (u_char *)&server_socket);

    // Cleanup
    pcap_freealldevs(alldevs);
    pcap_freecode(&filter);
    pcap_close(handle);
}
