# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -Wextra -std=c11 -O2

# Include Path
INCLUDES = -Iinclude

# Library Flags (Required for pthread, pcap, auditd)
LIBS = -lpthread -lpcap -laudit

# Source Files
SRCS = src/main.c \
       src/client.c \
       src/network_monitor.c \
       src/system_monitor.c \
       src/logger.c

# Object Files (Automatically replaces .c with .o)
OBJS = $(SRCS:.c=.o)

# Output Binary Name
TARGET = client

# Default rule: Build the client
all: $(TARGET)

# Link Object Files into Executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

# Compile C files into object files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)

# Run the client with default IP and port
run: all
	./client 127.0.0.1 8080 &
