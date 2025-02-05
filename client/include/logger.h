#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>


// Log file path
#define LOG_FILE "./local_logs.log"  // Change if necessary

// Mutex for thread-safe logging
extern pthread_mutex_t log_mutex;

/**
 * @brief Logs an event to the log file.
 * 
 * @param message The message to log.
 */
void log_event(const char *message);

#endif // LOGGER_H
