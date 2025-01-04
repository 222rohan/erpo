#ifndef LOGGER_H
#define LOGGER_H

//c file to recv logs from clients and write to file
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define LOG_NET_FTP 1 //indicates ftp was used
#define LOG_NET_SSH 2 //indicates ssh was used
#define LOG_NET_HT 3 //indicates http/https was used
#define LOG_NET_LOCAL 4 //indicates some local packet transfer occurred
#define LOG_NET_OTHER 5 //indicates some other network activity occurred

#define LOG_SYS_EXT 6 //indicates external hard drive was used
#define LOG_SYS_MOD 7 //indicates system modification (remote system modification)
#define LOG_SYS_OTHER 8 //indicates some other system activity occurred

//log bytes' structure to store log type and message
typedef struct {
    int log_type;
    char log_msg[1024];
} logb;

extern pthread_mutex_t log_mutex;
extern int server_sock;

// Function to receive logs from clients
//receive from client, write to file
void recv_fc(logb *logb);

//prints and logs msg to file
void log_file(char *logfile, char *logmsg);

//function to classify log type
void id_log(char *logmsg);


#endif  // LOGGER_H