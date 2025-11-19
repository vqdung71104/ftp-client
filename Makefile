# --- Compiler and Flags ---
CC = gcc
# Thêm LDFLAGS để chứa các cờ cho linker
LDFLAGS =
# Cờ cho trình biên dịch, không thay đổi
CFLAGS = -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200112L -Iutils

# VPATH: Chỉ định cho 'make' các thư mục cần tìm kiếm tệp nguồn (.c)
VPATH = utils:TCP_Client:TCP_Server

# --- Directories and Targets ---
BUILD_DIR = build
TARGET_CLIENT = $(BUILD_DIR)/client
TARGET_SERVER = $(BUILD_DIR)/server

# --- Source and Object Files ---
UTILS_SRC = $(wildcard utils/*.c)
CLIENT_SRC = TCP_Client/TCP_Client.c
SERVER_SRC = TCP_Server/TCP_Server.c

# SỬA LỖI TẠI ĐÂY: Sửa $(UTILS_SRCS) thành $(UTILS_SRC)
UTILS_OBJS = $(patsubst utils/%.c, $(BUILD_DIR)/%.o, $(UTILS_SRC))
CLIENT_OBJ = $(patsubst TCP_Client/%.c, $(BUILD_DIR)/%.o, $(CLIENT_SRC))
SERVER_OBJ = $(patsubst TCP_Server/%.c, $(BUILD_DIR)/%.o, $(SERVER_SRC))


.PHONY: all clean

# Quy tắc mặc định
all: $(TARGET_CLIENT) $(TARGET_SERVER)
	@mkdir -p $(BUILD_DIR)/storage
	@mkdir -p logs
	@echo "Build complete! Storage and logs directories created."

# --- Linking Rules ---
# Quy tắc này giờ sẽ hoạt động đúng vì $(UTILS_OBJS) không còn rỗng
$(TARGET_CLIENT): $(CLIENT_OBJ) $(UTILS_OBJS)
	@echo "Linking to create $(TARGET_CLIENT)..."
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_SERVER): $(SERVER_OBJ) $(UTILS_OBJS)
	@echo "Linking to create $(TARGET_SERVER)..."
	$(CC) $^ -o $@ $(LDFLAGS)

# --- Generic Compilation Rule ---
# Quy tắc mẫu này sẽ biên dịch tất cả các file .c cần thiết
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Quy tắc tạo thư mục build
$(BUILD_DIR):
	@mkdir -p $@

# --- Cleanup Rule ---
clean:
	@echo "Cleaning up build artifacts..."
	rm -rf $(BUILD_DIR)
package:
	zip VuQuangDung_20225818_prj.zip TCP_Server/* TCP_Client/* utils/* Makefile 