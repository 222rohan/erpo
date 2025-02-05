// server.cpp
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// --------------------- Global Definitions ---------------------

constexpr int PORT = 8080; // Server listening port
constexpr int BACKLOG = 10; // Maximum pending connections

// Structure to represent a connected client
struct Client {
    std::string client_ip;
    bool conn_status;          // true if online, false if offline
    std::time_t time_connected;
    int socket_id;
};

// Global container for clients and associated mutex
std::vector<Client> clients;
std::mutex client_mutex;

// Global server socket and running flag
int server_sock = -1;
std::atomic<bool> running(true);

// --------------------- Utility Functions ---------------------

// Returns a string representing the elapsed time (HH:MM:SS) between start and end
std::string format_time_diff(std::time_t start, std::time_t end) {
    int seconds = static_cast<int>(difftime(end, start));
    int hours   = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    seconds     = seconds % 60;
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, seconds);
    return std::string(buffer);
}

// --------------------- Client Handler ---------------------

// Function to handle communication with a client.
// For this example, it simply waits until the client disconnects.
void client_handler(int client_sock, std::string client_ip) {
    char buffer[256];
    while (true) {
        ssize_t bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            // Connection closed or error occurred
            break;
        }
        buffer[bytes_received] = '\0';
        // Here you could process messages from the client.
        // For this example, we simply print them.
        std::cout << "[" << client_ip << "] " << buffer << std::endl;
    }
    
    // Mark the client as offline
    {
        std::lock_guard<std::mutex> lock(client_mutex);
        for (auto &client : clients) {
            if (client.socket_id == client_sock) {
                client.conn_status = false;
                break;
            }
        }
    }
    close(client_sock);
    std::cout << "Client " << client_ip << " disconnected." << std::endl;
}

// --------------------- Accept Thread ---------------------

// Function that runs in a separate thread to accept incoming client connections.
void accept_connections() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (running) {
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            if (running) {
                std::perror("accept");
            }
            break;
        }

        std::string client_ip = inet_ntoa(client_addr.sin_addr);

        // Check if the client with the same IP already exists
        {
            std::lock_guard<std::mutex> lock(client_mutex);
            for (const auto &client : clients) {
                if (client.client_ip == client_ip && client.conn_status) {
                    std::cout << "Client with IP " << client_ip << " is already connected. Rejecting connection.\n";
                    close(client_sock); // Reject the connection
                    return;
                }
            }
        }

        std::cout << "Accepted connection from " << client_ip << std::endl;

        // Add client info to the global list
        {
            std::lock_guard<std::mutex> lock(client_mutex);
            Client newClient;
            newClient.client_ip = client_ip;
            newClient.conn_status = true;
            newClient.time_connected = std::time(nullptr);
            newClient.socket_id = client_sock;
            clients.push_back(newClient);
        }

        // Spawn a thread to handle this client
        std::thread(client_handler, client_sock, client_ip).detach();
    }
}


// --------------------- Console Command Handlers ---------------------

void con_help(const std::string &args) {
    (void)args; // unused
    std::cout << "Available commands:\n"
              << "  help                        - Display this help message\n"
              << "  stop                        - Shutdown the server\n"
              << "  kick <ip> / -a              - Disconnect a specific client by IP or all clients (-a)\n"
              << "  status <ip> / -o / -a       - Show info about a specific client, online clients (-o), or all (-a)\n";
}

void con_stop(const std::string &args) {
    (void)args; // unused
    std::cout << "Stopping server...\n";
    
    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(client_mutex);
        for (auto &client : clients) {
            if (client.conn_status) {
                close(client.socket_id);
                client.conn_status = false;
            }
        }
    }
    
    // Signal the accept thread to stop and close the server socket
    running = false;
    if (server_sock != -1) {
        close(server_sock);
    }
    std::exit(0);
}

void con_kick(const std::string &args) {
    if (args == "-a") {
        std::lock_guard<std::mutex> lock(client_mutex);
        for (auto &client : clients) {
            if (client.conn_status) {
                close(client.socket_id);
                client.conn_status = false;
            }
        }
        std::cout << "Kicked all clients.\n";
    } else {
        bool found = false;
        std::lock_guard<std::mutex> lock(client_mutex);
        for (auto &client : clients) {
            if (client.client_ip == args && client.conn_status) {
                close(client.socket_id);
                client.conn_status = false;
                std::cout << "Kicked client " << args << "\n";
                found = true;
                break;
            }
        }
        if (!found) {
            std::cout << "Client " << args << " not online or doesn't exist.\n";
        }
    }
}

void con_status(const std::string &args) {
    if (args == "-o") {
        int online_count = 0;
        std::lock_guard<std::mutex> lock(client_mutex);
        std::cout << "Online clients:\n";
        for (const auto &client : clients) {
            if (client.conn_status) {
                online_count++;
                std::cout << client.client_ip << " - " 
                          << format_time_diff(client.time_connected, std::time(nullptr)) << "\n";
            }
        }
        std::cout << online_count << " client(s) online out of " << clients.size() << "\n";
    } else if (args == "-a") {
        std::lock_guard<std::mutex> lock(client_mutex);
        std::cout << "Showing " << clients.size() << " client(s):\n";
        for (const auto &client : clients) {
            std::cout << client.client_ip << " - " 
                      << (client.conn_status ? "Online" : "Offline") << " - " 
                      << format_time_diff(client.time_connected, std::time(nullptr)) << "\n";
        }
    } else {
        bool found = false;
        std::lock_guard<std::mutex> lock(client_mutex);
        for (const auto &client : clients) {
            if (client.client_ip == args) {
                std::cout << "Client IP: " << client.client_ip << "\n"
                          << "Status: " << (client.conn_status ? "Online" : "Offline") << "\n"
                          << "Time connected: " << format_time_diff(client.time_connected, std::time(nullptr)) << "\n";
                found = true;
                break;
            }
        }
        if (!found) {
            std::cout << "Client with IP " << args << " not found.\n";
        }
    }
}

// --------------------- Command Parsing ---------------------

// Command structure for console commands
struct Command {
    std::string name;
    void (*handler)(const std::string &args);
};

// Array of available commands
Command commands[] = {
    {"help",   con_help},
    {"stop",   con_stop},
    {"kick",   con_kick},
    {"status", con_status},
    {"", nullptr} // End marker
};

// Parses a command line, splits it into command and arguments, and dispatches it.
void parse_command(const std::string &line) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;
    std::string args;
    std::getline(iss, args);
    
    // Trim any leading spaces from args.
    size_t pos = args.find_first_not_of(" ");
    if (pos != std::string::npos) {
        args = args.substr(pos);
    } else {
        args = "";
    }
    
    bool valid = false;
    for (const auto &command : commands) {
        if (command.name == cmd) {
            command.handler(args);
            valid = true;
            break;
        }
    }
    if (!valid) {
        std::cout << "Invalid command. Type 'help' for a list of commands.\n";
    }
}

// Runs the interactive console in the main thread.
void run_console() {
    std::string line;
    while (true) {
        std::cout << "[server]> ";
        if (!std::getline(std::cin, line)) {
            continue;
        }
        if (line.empty()) {
            continue;
        }
        parse_command(line);
    }
}

// --------------------- Main Function ---------------------

int main() {
    // Create the server socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        std::perror("socket");
        return EXIT_FAILURE;
    }
    
    // Allow address reuse
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::perror("setsockopt");
        return EXIT_FAILURE;
    }
    
    // Bind the socket to all interfaces on the specified port
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::perror("bind");
        return EXIT_FAILURE;
    }
    
    // Start listening
    if (listen(server_sock, BACKLOG) < 0) {
        std::perror("listen");
        return EXIT_FAILURE;
    }
    
    std::cout << "Server listening on port " << PORT << "...\n";
    
    // Spawn a thread to handle incoming connections
    std::thread accept_thread(accept_connections);
    
    // Run the console in the main thread
    run_console();
    
    // Clean up (this point is never reached because run_console() loops infinitely)
    running = false;
    if (accept_thread.joinable())
        accept_thread.join();
    
    return EXIT_SUCCESS;
}
