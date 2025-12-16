#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>

#define BUFFER_SIZE 8192
#define FILE_BUFFER_SIZE 16384

// Global control socket
int control_sock = -1;
int logged_in = 0;

// Print timestamp with message
void print_with_timestamp(const char* command, const char* response) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("%02d:%02d:%02d %s %s\n", t->tm_hour, t->tm_min, t->tm_sec, command, response);
}

// Send command to server
int send_command(const char* command) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s\r\n", command);
    
    if (send(control_sock, buffer, strlen(buffer), 0) < 0) {
        perror("Send failed");
        return -1;
    }
    
    return 0;
}

// Receive response from server
int recv_response(char* response, int max_len) {
    char buffer[BUFFER_SIZE];
    int total = 0;
    
    while (1) {
        int n = recv(control_sock, buffer + total, sizeof(buffer) - total - 1, 0);
        if (n <= 0) {
            return -1;
        }
        
        total += n;
        buffer[total] = '\0';
        
        // Check if we have complete line (ending with \r\n)
        if (strstr(buffer, "\r\n")) {
            strncpy(response, buffer, max_len - 1);
            response[max_len - 1] = '\0';
            
            // Remove \r\n
            char* crlf = strstr(response, "\r\n");
            if (crlf) *crlf = '\0';
            
            return 0;
        }
    }
}

// Parse PASV response to get IP and port
int parse_pasv_response(const char* response, char* ip, int* port) {
    // Response format: 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
    const char* start = strchr(response, '(');
    if (!start) return -1;
    
    int h1, h2, h3, h4, p1, p2;
    if (sscanf(start, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        return -1;
    }
    
    sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4);
    *port = p1 * 256 + p2;
    
    return 0;
}

// Connect to data channel
int connect_data_channel(const char* ip, int port) {
    int data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (data_sock < 0) {
        perror("Data socket creation failed");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    
    if (connect(data_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Data connection failed");
        close(data_sock);
        return -1;
    }
    
    // Enable TCP_NODELAY for better performance
    int flag = 1;
    setsockopt(data_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    return data_sock;
}

// Login to server
void cmd_login() {
    char username[256], password[256];
    char response[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    
    printf("Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    
    // Send USER command
    snprintf(command, sizeof(command), "USER %s", username);
    send_command(command);
    
    if (recv_response(response, sizeof(response)) < 0) {
        printf("Failed to receive response\n");
        return;
    }
    
    print_with_timestamp(command, response);
    
    // Check if need password
    if (strncmp(response, "331", 3) != 0) {
        return;
    }
    
    // Send PASS command
    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;
    
    snprintf(command, sizeof(command), "PASS %s", password);
    send_command(command);
    
    if (recv_response(response, sizeof(response)) < 0) {
        printf("Failed to receive response\n");
        return;
    }
    
    print_with_timestamp(command, response);
    
    if (strncmp(response, "230", 3) == 0) {
        logged_in = 1;
        printf("Login successful!\n");
    }
}

// Print working directory
void cmd_pwd() {
    char response[BUFFER_SIZE];
    
    if (!logged_in) {
        printf("Please login first!\n");
        return;
    }
    
    send_command("PWD");
    
    if (recv_response(response, sizeof(response)) < 0) {
        printf("Failed to receive response\n");
        return;
    }
    
    print_with_timestamp("PWD", response);
}

// Change working directory
void cmd_cwd() {
    char path[512];
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    if (!logged_in) {
        printf("Please login first!\n");
        return;
    }
    
    printf("Enter directory path: ");
    fgets(path, sizeof(path), stdin);
    path[strcspn(path, "\n")] = 0;
    
    snprintf(command, sizeof(command), "CWD %s", path);
    send_command(command);
    
    if (recv_response(response, sizeof(response)) < 0) {
        printf("Failed to receive response\n");
        return;
    }
    
    print_with_timestamp(command, response);
}

// List files
void cmd_list() {
    char response[BUFFER_SIZE];
    char data_ip[64];
    int data_port;
    
    if (!logged_in) {
        printf("Please login first!\n");
        return;
    }
    
    // Set binary mode
    send_command("TYPE I");
    recv_response(response, sizeof(response));
    print_with_timestamp("TYPE I", response);
    
    // Enter passive mode
    send_command("PASV");
    if (recv_response(response, sizeof(response)) < 0) {
        printf("Failed to receive response\n");
        return;
    }
    
    print_with_timestamp("PASV", response);
    
    if (parse_pasv_response(response, data_ip, &data_port) < 0) {
        printf("Failed to parse PASV response\n");
        return;
    }
    
    // Connect to data channel
    int data_sock = connect_data_channel(data_ip, data_port);
    if (data_sock < 0) {
        return;
    }
    
    // Send LIST command
    send_command("LIST");
    if (recv_response(response, sizeof(response)) < 0) {
        close(data_sock);
        return;
    }
    
    print_with_timestamp("LIST", response);
    
    // Receive file list from data channel
    printf("\n--- Directory Listing ---\n");
    char buffer[BUFFER_SIZE];
    int n;
    while ((n = recv(data_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    }
    printf("--- End of Listing ---\n\n");
    
    close(data_sock);
    
    // Receive final response
    if (recv_response(response, sizeof(response)) == 0) {
        print_with_timestamp("LIST", response);
    }
}

// Download file (RETR)
void cmd_retr() {
    char filename[512];
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char data_ip[64];
    int data_port;
    
    if (!logged_in) {
        printf("Please login first!\n");
        return;
    }
    
    printf("Enter filename to download: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = 0;
    
    // Set binary mode
    send_command("TYPE I");
    recv_response(response, sizeof(response));
    print_with_timestamp("TYPE I", response);
    
    // Enter passive mode
    send_command("PASV");
    if (recv_response(response, sizeof(response)) < 0) {
        printf("Failed to receive response\n");
        return;
    }
    
    print_with_timestamp("PASV", response);
    
    if (parse_pasv_response(response, data_ip, &data_port) < 0) {
        printf("Failed to parse PASV response\n");
        return;
    }
    
    // Connect to data channel
    int data_sock = connect_data_channel(data_ip, data_port);
    if (data_sock < 0) {
        return;
    }
    
    // Send RETR command
    snprintf(command, sizeof(command), "RETR %s", filename);
    send_command(command);
    
    if (recv_response(response, sizeof(response)) < 0) {
        close(data_sock);
        return;
    }
    
    print_with_timestamp(command, response);
    
    if (strncmp(response, "150", 3) != 0) {
        close(data_sock);
        return;
    }
    
    // Receive file data
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        printf("Failed to create file: %s\n", filename);
        close(data_sock);
        return;
    }
    
    char buffer[FILE_BUFFER_SIZE];
    int total_bytes = 0;
    int n;
    
    printf("Downloading...\n");
    while ((n = recv(data_sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, n, fp);
        total_bytes += n;
    }
    
    fclose(fp);
    close(data_sock);
    
    printf("Downloaded %d bytes to %s\n", total_bytes, filename);
    
    // Receive final response
    if (recv_response(response, sizeof(response)) == 0) {
        print_with_timestamp(command, response);
    }
}

// Upload file (STOR)
void cmd_stor() {
    char filepath[512];
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char data_ip[64];
    int data_port;
    
    if (!logged_in) {
        printf("Please login first!\n");
        return;
    }
    
    printf("Enter file path to upload: ");
    fgets(filepath, sizeof(filepath), stdin);
    filepath[strcspn(filepath, "\n")] = 0;
    
    // Check if file exists
    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        printf("File not found: %s\n", filepath);
        return;
    }
    
    // Get filename from path
    char* filename = strrchr(filepath, '/');
    if (!filename) {
        filename = filepath;
    } else {
        filename++;
    }
    
    // Set binary mode
    send_command("TYPE I");
    recv_response(response, sizeof(response));
    print_with_timestamp("TYPE I", response);
    
    // Enter passive mode
    send_command("PASV");
    if (recv_response(response, sizeof(response)) < 0) {
        fclose(fp);
        return;
    }
    
    print_with_timestamp("PASV", response);
    
    if (parse_pasv_response(response, data_ip, &data_port) < 0) {
        printf("Failed to parse PASV response\n");
        fclose(fp);
        return;
    }
    
    // Connect to data channel
    int data_sock = connect_data_channel(data_ip, data_port);
    if (data_sock < 0) {
        fclose(fp);
        return;
    }
    
    // Send STOR command
    snprintf(command, sizeof(command), "STOR %s", filename);
    send_command(command);
    
    if (recv_response(response, sizeof(response)) < 0) {
        fclose(fp);
        close(data_sock);
        return;
    }
    
    print_with_timestamp(command, response);
    
    if (strncmp(response, "150", 3) != 0) {
        fclose(fp);
        close(data_sock);
        return;
    }
    
    // Send file data
    char buffer[FILE_BUFFER_SIZE];
    int total_bytes = 0;
    size_t n;
    
    printf("Uploading...\n");
    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (send(data_sock, buffer, n, 0) < 0) {
            printf("Send failed\n");
            break;
        }
        total_bytes += n;
    }
    
    fclose(fp);
    close(data_sock);
    
    printf("Uploaded %d bytes\n", total_bytes);
    
    // Receive final response
    if (recv_response(response, sizeof(response)) == 0) {
        print_with_timestamp(command, response);
    }
}

// Quit
void cmd_quit() {
    char response[BUFFER_SIZE];
    
    send_command("QUIT");
    
    if (recv_response(response, sizeof(response)) == 0) {
        print_with_timestamp("QUIT", response);
    }
    
    logged_in = 0;
    printf("Disconnected from server.\n");
}

// Menu
void show_menu() {
    char response[BUFFER_SIZE];
    int choice;
    
    while (1) {
        printf("\n=== FTP Client Menu ===\n");
        printf("1. Login\n");
        printf("2. Print Working Directory (PWD)\n");
        printf("3. Change Directory (CWD)\n");
        printf("4. List Files (LIST)\n");
        printf("5. Download File (RETR)\n");
        printf("6. Upload File (STOR)\n");
        printf("7. Exit\n");
        printf("Choice: ");
        
        if (scanf("%d", &choice) != 1) {
            // Clear input buffer
            while (getchar() != '\n');
            printf("Invalid input!\n");
            continue;
        }
        getchar(); // consume newline
        
        switch (choice) {
            case 1:
                cmd_login();
                break;
            case 2:
                cmd_pwd();
                break;
            case 3:
                cmd_cwd();
                break;
            case 4:
                cmd_list();
                break;
            case 5:
                cmd_retr();
                break;
            case 6:
                cmd_stor();
                break;
            case 7:
                if (logged_in) {
                    cmd_quit();
                }
                printf("Exiting...\n");
                return;
            default:
                printf("Invalid choice!\n");
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }
    
    const char* server_ip = argv[1];
    int port = atoi(argv[2]);
    
    // Create socket
    control_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (control_sock < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    // Connect to server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        close(control_sock);
        return 1;
    }
    
    printf("Connecting to %s:%d...\n", server_ip, port);
    
    if (connect(control_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(control_sock);
        return 1;
    }
    
    printf("Connected!\n\n");
    
    // Receive welcome message
    char response[BUFFER_SIZE];
    if (recv_response(response, sizeof(response)) == 0) {
        print_with_timestamp("CONNECT", response);
    }
    
    // Show menu
    show_menu();
    
    close(control_sock);
    return 0;
}
