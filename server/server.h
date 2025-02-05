#ifndef SERVER_H
#define SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "erpo.h"

#ifdef __cplusplus
}
#endif


class ErpoServer {
public:
    // Constructor: initializes the server to listen on the specified port
    ErpoServer(int port);

    // Starts the server loop to accept and handle client connections
    void start();

private:
    int port_;

    // Optional: helper function to process an event (can be used within thread handlers)
    void processEvent(const erpo_event_t &event);
};

#endif // SERVER_H

