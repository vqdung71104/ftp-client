# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lpthread

# Directories
BUILD_DIR = build
SERVER_DIR = FTP_Server
CLIENT_DIR = FTP_Client
UTILS_DIR = utils

# Output binaries
SERVER_BIN = $(BUILD_DIR)/ftp_server
CLIENT_BIN = $(BUILD_DIR)/ftp_client

# Source files
SERVER_SRCS = $(SERVER_DIR)/ftp_server.c \
              $(SERVER_DIR)/ftp_commands.c \
              $(SERVER_DIR)/ftp_data.c \
              $(SERVER_DIR)/ftp_utils.c \
              $(UTILS_DIR)/account.c \
              $(UTILS_DIR)/logging.c \
              $(UTILS_DIR)/utils.c

CLIENT_SRCS = $(CLIENT_DIR)/ftp_client.c

# Object files
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

# Default target
all: $(SERVER_BIN) $(CLIENT_BIN)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build server
$(SERVER_BIN): $(SERVER_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJS) $(LDFLAGS)
	@echo "✓ FTP Server built successfully!"

# Build client
$(CLIENT_BIN): $(CLIENT_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJS)
	@echo "✓ FTP Client built successfully!"

# Compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run server (default port 2121)
run-server: $(SERVER_BIN)
	@echo "Starting FTP Server on port 2121..."
	./$(SERVER_BIN) 2121

# Run client
run-client: $(CLIENT_BIN)
	@echo "Starting FTP Client..."
	./$(CLIENT_BIN)

# Run both (server in background, then client)
run-both: $(SERVER_BIN) $(CLIENT_BIN)
	@echo "Starting FTP Server in background..."
	./$(SERVER_BIN) 2121 &
	@sleep 1
	@echo "Starting FTP Client..."
	./$(CLIENT_BIN)

# Clean build files
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS)
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
	@echo "✓ Cleaned build files"

# Clean and rebuild
rebuild: clean all

# Help target
help:
	@echo "FTP Server/Client Makefile"
	@echo "=========================="
	@echo "Targets:"
	@echo "  all         - Build both server and client (default)"
	@echo "  run-server  - Build and run FTP server on port 2121"
	@echo "  run-client  - Build and run FTP client"
	@echo "  run-both    - Run server in background, then client"
	@echo "  clean       - Remove all build files"
	@echo "  rebuild     - Clean and rebuild everything"
	@echo "  help        - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build everything"
	@echo "  make run-server         # Run server"
	@echo "  make run-client         # Run client"
	@echo "  make run-both           # Run both"

.PHONY: all clean rebuild run-server run-client run-both help
