#include "../include/logger.h"
#include "../include/client.h"
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

char log_file_name[64];

void log_init() {
    // Create logs directory if it doesn't exist
    if (access(LOG_FILE_PATH, F_OK) == -1) {
        mkdir(LOG_FILE_PATH, 0700);
    }

  // log file name must be 
  // logs/pid_date_time.log
    pid_t pid = getpid();

    sprintf(log_file_name, "%serpo_%d.log", LOG_FILE_PATH, pid);
    FILE* log_file = fopen(log_file_name, "w");
    if (log_file) {
        fclose(log_file);
    }
}

/**
 * @brief Logs an event to a file with a timestamp.
 * 
 * @param message The message to log.
 */
void log_event(const char *message) {
    pthread_mutex_lock(&log_mutex);

    FILE* log_file = fopen(log_file_name, "a");

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

/**
 * @brief Logs an event (with format strings) to a file with a timestamp.
 * 
 * @param message The message to log.
 */
void log_eventf(const char *format, ...) {
    pthread_mutex_lock(&log_mutex);

    FILE* log_file = fopen(log_file_name, "a");

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
    va_list args;
    va_start(args, format);
    fprintf(log_file, "[%s] ", timestamp);
    vfprintf(log_file, format, args);
    fprintf(log_file, "\n");
    va_end(args);

    fclose(log_file);

    pthread_mutex_unlock(&log_mutex);
}