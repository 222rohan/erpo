#include "../include/logger.h"
#include "../include/client.h"
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Logs an event to a file with a timestamp.
 * 
 * @param message The message to log.
 */
void log_event(const char *message) {
    pthread_mutex_lock(&log_mutex);

    FILE *log_file = fopen(LOG_FILE, "a+"); // Append mode

    if (!log_file) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    // Get current timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // Write log entry
    fprintf(log_file, "[%s] %s\n", timestamp, message);
    fclose(log_file);

    pthread_mutex_unlock(&log_mutex);
}
