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
    int *server_socket = (int *)server_socket_ptr;
    struct ip *ip_header;
    struct tcphdr *tcp_header;
    char source_ip[INET_ADDRSTRLEN];
    char dest_ip[INET_ADDRSTRLEN];
    char source_port[6];
    char dest_port[6];
    char *payload;
    int payload_length;
    int i;

    // get IP header
    ip_header = (struct ip *)(packet + 14); // skip Ethernet header

    // get TCP header
    tcp_header = (struct tcphdr *)(packet + 14 + ip_header->ip_hl * 4); // skip Ethernet and IP headers

    // get source and destination IP addresses
    inet_ntop(AF_INET, &(ip_header->ip_src), source_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(ip_header->ip_dst), dest_ip, INET_ADDRSTRLEN);

    // get source and destination port numbers
    sprintf(source_port, "%d", ntohs(tcp_header->th_sport));
    sprintf(dest_port, "%d", ntohs(tcp_header->th_dport));

    char message[1024];
    sprintf(message, "Packet captured: %s:%s -> %s:%s", source_ip, source_port, dest_ip, dest_port);

    // check for FTP, SSH, HTTP, HTTPS traffic
    if (ntohs(tcp_header->th_dport) == 21) {
        strcat(message, " [FTP]");
    } else if (ntohs(tcp_header->th_dport) == 22) {
        strcat(message, " [SSH]");
    } else if (ntohs(tcp_header->th_dport) == 80) {
        strcat(message, " [HTTP]");
    } else if (ntohs(tcp_header->th_dport) == 443) {
        strcat(message, " [HTTPS]");
    }   

    log_eventf("[Network Monitor]: %s", message);

    /* experimental : print payload  */
    int CONFIG_print_payload = 0;

    if(CONFIG_print_payload) {
        payload = (char *)(packet + 14 + ip_header->ip_hl * 4 + tcp_header->th_off * 4);
        payload_length = ntohs(ip_header->ip_len) - (ip_header->ip_hl * 4 + tcp_header->th_off * 4);
        if (payload_length > 0) {
            log_eventf("[Network Monitor]: Payload (%d bytes):", payload_length);
            //copy payload to message

            for (i = 0; i < payload_length; i++) {
                if (payload[i] >= 32 && payload[i] <= 126) {
                    message[i] = payload[i];
                } else {
                    message[i] = '.';
                }
            }

            log_eventf("[Network Monitor]: Payload: %s", message);
        }
    }
    send_message_to_server("Suspicious network activity detected");
}

/**
 * @brief Captures network traffic and detects specific activities.
 */
int monitor_network(int server_socket) {

    char *dev = "eth0"; // for linux exam pcs, its typically eth0

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    struct bpf_program fp;
    bpf_u_int32 mask, net;

    // get network ip and mask
    if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
        log_eventf("Could not get netmask for device %s: %s", dev, errbuf);
        return -1;
    }

    // open network device for packet capture
    handle = pcap_open_live(dev, SNAP_LEN, 1, TIMEOUT_MS, errbuf);
    if(handle == NULL) {
        log_eventf("Could not open device %s: %s", dev, errbuf);
        perror("pcap_open_live");
        return -1;
    }

    // compile the filter expression
    if (pcap_compile(handle, &fp, FILTER_EXPRESSION, 0, net) == -1) {
        log_eventf("Could not parse filter %s: %s", FILTER_EXPRESSION, pcap_geterr(handle));
        return -1;
    }

    // apply the filter
    if (pcap_setfilter(handle, &fp) == -1) {
        log_eventf("Could not install filter %s: %s", FILTER_EXPRESSION, pcap_geterr(handle));
        return -1;
    }

    log_eventf("Network monitor started successfully on device %s", dev);
    // start capturing packets
    
    pcap_loop(handle, 0, packet_handler, (u_char *)&server_socket);

    log_eventf("test");
    // cleanup
    pcap_freecode(&fp);
    pcap_close(handle);

    return 0;
}
