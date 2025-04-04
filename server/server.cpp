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
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace std;

// --------------------- Global Definitions ---------------------
constexpr int PORT = 8080;          // Server listening port
constexpr int BACKLOG = 10;         // Maximum pending connections
constexpr int BUFFER_SIZE = 1024;   // Buffer size for communication

struct Client {
    string client_ip;
    bool conn_status;            // true if online, false if offline
    time_t time_connected;
    int socket_id;
};

vector<Client> clients;
mutex client_mutex;

int server_sock = -1;
atomic<bool> running(true);

// --------------------- Utility Functions ---------------------
string format_time_diff(time_t start, time_t end) {
    int seconds = static_cast<int>(difftime(end, start));
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, seconds);
    return string(buffer);
}

mutex log_mutex;
void server_log(const string &message) {
    lock_guard<mutex> lock(log_mutex);
    //get formatted timestamp in hh:mm:ss format
    time_t now = time(0);
    tm *ltm = localtime(&now);
    string timestamp = to_string(0 + ltm->tm_hour) + ":" + to_string(1 + ltm->tm_min) + ":" + to_string(1 + ltm->tm_sec);

    timestamp = "["+timestamp + "]";
    cout << timestamp << " " << message << endl;
    
    ofstream log_file("server.log", ios::app);
    if (log_file.is_open()) {
        log_file << timestamp + message << endl;
        log_file.close();
    }
}

// --------------------- Client Handler ---------------------
void handle_client(int client_sock, const string &client_ip) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = 0;
    
    {
        lock_guard<mutex> lock(client_mutex);
        // add client to list, but if already exists, update status
        bool found = false;
        for (auto &client : clients) {
            if (client.client_ip == client_ip) {
                client.conn_status = true;
                client.socket_id = client_sock;
                client.time_connected = time(nullptr);
                found = true;
                break;
            }
        }
        if (!found) {
            clients.push_back({client_ip, true, time(nullptr), client_sock});
        }
    }
    server_log("Client connected: " + client_ip);
    
    while ((bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        string received(buffer);
        if (received == "heartbeat") {
            //server_log("Received heartbeat from " + client_ip);
            string reply = "OK";
            send(client_sock, reply.c_str(), reply.size(), 0);
        } else {
            string log_message = "[" + client_ip + "] " + received;
            server_log(log_message);
        }
    }
    
    server_log("Client disconnected: " + client_ip);
    close(client_sock);
    {
        lock_guard<mutex> lock(client_mutex);
        for (auto &client : clients) {
            if (client.socket_id == client_sock) {
                client.conn_status = false;
                break;
            }
        }
    }
}

// --------------------- Accept Connections ---------------------
void accept_connections() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    while (running) {
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            if (running) perror("accept");
            break;
        }
        string client_ip = inet_ntoa(client_addr.sin_addr);
        {
            lock_guard<mutex> lock(client_mutex);
            bool duplicate = false;
            for (const auto &client : clients) {
                if (client.client_ip == client_ip && client.conn_status) {
                    server_log("Client with IP " + client_ip + " already connected. Rejecting.");
                    close(client_sock);
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;
        }
        thread(handle_client, client_sock, client_ip).detach();
    }
}

// --------------------- Console Command Handlers ---------------------
void con_help(const string &/*args*/) {
    cout << "Available commands:\n"
         << "  help                        - Display this help message\n"
         << "  stop                        - Shutdown the server\n"
         << "  kick <ip> / -a              - Disconnect a specific client by IP or all clients (-a)\n"
         << "  status <ip> / -o / -a       - Show info about a specific client, online clients (-o), or all (-a)\n";
}

void con_stop(const string &/*args*/) {
    cout << "Stopping server...\n";
    {
        lock_guard<mutex> lock(client_mutex);
        for (auto &client : clients) {
            if (client.conn_status) {
                close(client.socket_id);
                client.conn_status = false;
            }
        }
    }
    running = false;
    if (server_sock != -1) close(server_sock);
    exit(0);
}

void con_kick(const string &args) {
    if (args == "-a") {
        lock_guard<mutex> lock(client_mutex);
        for (auto &client : clients) {
            if (client.conn_status) {
                close(client.socket_id);
                client.conn_status = false;
            }
        }
        cout << "Kicked all clients.\n";
    } else {
        bool found = false;
        lock_guard<mutex> lock(client_mutex);
        for (auto &client : clients) {
            if (client.client_ip == args && client.conn_status) {
                close(client.socket_id);
                client.conn_status = false;
                cout << "Kicked client " << args << "\n";
                found = true;
                break;
            }
        }
        if (!found)
            cout << "Client " << args << " not online or doesn't exist.\n";
    }
}

void con_status(const string &args) {
    if (args == "-o") {
        int online_count = 0;
        lock_guard<mutex> lock(client_mutex);
        cout << "Online clients:\n";
        for (const auto &client : clients) {
            if (client.conn_status) {
                online_count++;
                cout << client.client_ip << " - " 
                     << format_time_diff(client.time_connected, time(nullptr)) << "\n";
            }
        }
        cout << online_count << " client(s) online out of " << clients.size() << "\n";
    } else if (args == "-a") {
        lock_guard<mutex> lock(client_mutex);
        cout << "Showing " << clients.size() << " client(s):\n";
        for (const auto &client : clients) {
            cout << client.client_ip << " - " 
                 << (client.conn_status ? "Online" : "Offline") << " - " 
                 << format_time_diff(client.time_connected, time(nullptr)) << "\n";
        }
    } else {
        bool found = false;
        lock_guard<mutex> lock(client_mutex);
        for (const auto &client : clients) {
            if (client.client_ip == args) {
                cout << "Client IP: " << client.client_ip << "\n"
                     << "Status: " << (client.conn_status ? "Online" : "Offline") << "\n"
                     << "Time connected: " << format_time_diff(client.time_connected, time(nullptr)) << "\n";
                found = true;
                break;
            }
        }
        if (!found)
            cout << "Client with IP " << args << " not found.\n";
    }
}

// --------------------- Command Parsing and Console ---------------------
struct Command {
    string name;
    void (*handler)(const string &args);
};

Command commands[] = {
    {"help",   con_help},
    {"stop",   con_stop},
    {"kick",   con_kick},
    {"status", con_status},
    {"", nullptr}  // End marker
};

void parse_command(const string &line) {
    istringstream iss(line);
    string cmd, args;
    iss >> cmd;
    getline(iss, args);
    size_t pos = args.find_first_not_of(" ");
    if (pos != string::npos) args = args.substr(pos); else args = "";
    bool valid = false;
    for (const auto &command : commands) {
        if (command.name == cmd) {
            command.handler(args);
            valid = true;
            break;
        }
    }
    if (!valid)
        cout << "Invalid command. Type 'help' for a list of commands.\n";
}

void run_console() {
    string line;
    while (true) {
        cout << "[server]> ";
        if (!getline(cin, line)) continue;
        if (line.empty()) continue;
        parse_command(line);
    }
}

// --------------------- Main Function ---------------------
int main() {
    // Create server socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }
    
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        return EXIT_FAILURE;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return EXIT_FAILURE;
    }
    
    if (listen(server_sock, BACKLOG) < 0) {
        perror("listen");
        return EXIT_FAILURE;
    }
    
    cout << "Server listening on port " << PORT << "...\n";
    
    thread accept_thread(accept_connections);
    run_console();  // Run console commands in main thread
    
    running = false;
    if (accept_thread.joinable())
        accept_thread.join();
    
    return EXIT_SUCCESS;
}
