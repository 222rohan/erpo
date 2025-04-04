# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -Wextra

# Include Path
INCLUDES = -Iinclude

# Library Flags (Required for pthread, pcap, auditd)
LIBS += -lpthread -lpcap -ludev

# Source Files
SRCS = src/main.c \
       src/client.c \
       src/network_monitor.c \
       src/system_monitor.c \
       src/logger.c

# Output Binary Name
TARGET = client

LOGS = logs/

# Default rule: Build the client
all: $(TARGET)

# Build the client
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(SRCS) $(LIBS) $(PKGCONFIG)

# Clean build files
clean:
	rm -rf $(LOGS) $(TARGET)

# Run the client with default IP and port
run: all
	./client 127.0.0.1 8080 &
