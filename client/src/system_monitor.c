#include "../include/system_monitor.h"
#include "../include/logger.h"
#include "../include/client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libaudit.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

/**
 * @brief Monitors USB insertions using inotify.
 */
void *usb_monitor_thread(void *arg) {
    int server_socket = *((int *)arg);
    int fd, wd;
    char buffer[BUF_LEN];

    fd = inotify_init();
    if (fd < 0) {
        log_event("Failed to initialize inotify");
        return NULL;
    }

    // Watch the /media and /mnt directories for USB insertions
    wd = inotify_add_watch(fd, "/media", IN_CREATE | IN_ATTRIB);
    inotify_add_watch(fd, "/mnt", IN_CREATE | IN_ATTRIB);

    log_event("USB monitor started");

    while (1) {
        int length = read(fd, buffer, BUF_LEN);
        if (length < 0) {
            continue;
        }

        for (int i = 0; i < length; i += EVENT_SIZE + ((struct inotify_event *)&buffer[i])->len) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];

            if (event->mask & IN_CREATE || event->mask & IN_ATTRIB) {
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "USB device detected: %s", event->name);
                log_event(log_msg);
                send_message_to_server(log_msg);
            }
        }
    }

    inotify_rm_watch(fd, wd);
    close(fd);
    return NULL;
}

/**
 * @brief Monitors system modifications using auditd.
 */
void *audit_monitor_thread(void *arg) {
    int server_socket = *((int *)arg);
    int audit_fd = audit_open();
    if (audit_fd < 0) {
        log_event("Failed to open auditd connection");
        return NULL;
    }

    log_event("System audit monitor started");

    while (1) {
        char buffer[1024];
        int length = audit_get_reply(audit_fd, (struct audit_reply *)buffer, sizeof(buffer), 0);
        if (length > 0) {
            log_event("System modification detected");
            send_message_to_server("System modification detected");
        }
    }

    audit_close(audit_fd);
    return NULL;
}

/**
 * @brief Monitors system activity (USB insertions, file modifications).
 */
void monitor_system(int server_socket) {
    pthread_t usb_thread, audit_thread;

    // Start USB monitor thread
    pthread_create(&usb_thread, NULL, usb_monitor_thread, &server_socket);
    pthread_create(&audit_thread, NULL, audit_monitor_thread, &server_socket);

    pthread_join(usb_thread, NULL);
    pthread_join(audit_thread, NULL);
}
