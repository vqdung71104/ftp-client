#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <libgen.h>

#include "utils.h"

#define BUFF_SIZE 16384

/**
 * @brief A simple menu function
 */
void simple_menu(int sockfd, char *recv_buf);

/**
 * @brief Function for sending login command to server
 *
 * @param sockfd socket descriptor
 * @param recv_buf buffer to save the result
 */
void send_login_command(int sockfd, char *recv_buf);

/**
 * @brief function for sending cmd and wait for server response
 *
 * @param sockfd socket descriptor communication
 * @param msg sending message
 * @param recv_buf buffer to save the result
 */
void send_and_response(int sockfd, char *msg, char *recv_buf);

/**
 * @brief Function for sending bye command to server
 *
 * @param sockfd socket descriptor
 * @param recv_buf buffer to save the result
 */
void send_bye_command(int sockfd, char *recv_buf);

/**
 * @brief Funcion for sending article to post
 *
 * @param sockfd socket descriptor
 * @param recv_buf buffer to save the result
 */
void send_article_command(int sockfd, char *recv_buf);

/**
 * @brief Function for uploading file
 *
 * @param sockfd socket descriptor
 * @param recv_buf buffer to save the result
 */
void send_upload_command(int sockfd, char *recv_buf);

/**
 * @brief Function for changing working directory
 *
 * @param sockfd socket descriptor
 * @param recv_buf buffer to save the result
 */
void send_chdir_command(int sockfd, char *recv_buf);

/**
 * @brief Function for listing files in working directory
 *
 * @param sockfd socket descriptor
 * @param recv_buf buffer to save the result
 */
void send_list_command(int sockfd, char *recv_buf);

/**
 * @brief Function for downloading file from server
 *
 * @param sockfd socket descriptor
 * @param recv_buf buffer to save the result
 */
void send_download_command(int sockfd, char *recv_buf);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        perror("Usage: ./client <IP_Address> <Port_number>\n");
        exit(EXIT_FAILURE);
    }

    int client_sock;                // client sock
    struct sockaddr_in server_addr; // server address
    char buffer[BUFF_SIZE + 1];

    char *server_ip = argv[1]; // server IP
    int server_port = atoi(argv[2]); // server port

    // Step 1:Create socket
    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }

    // Step 2: config server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Step 3: Connect to server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
    {
        perror("connect() error");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at %s:%d\n", server_ip, server_port);

    // Welcome msg
    int received_bytes = recv(client_sock, buffer, BUFF_SIZE, 0);
    if (received_bytes <= 0)
    {
        printf("Server has closed connection.\n");
        close(client_sock);
        return 1;
    }

    buffer[received_bytes] = '\0';
    printf("Server: %s\n", buffer);

    // 2. Communication loop
    simple_menu(client_sock, buffer);

    // 3. Đóng kết nối
    close(client_sock);
    return 0;
}

void send_and_response(int sockfd, char *msg, char *recv_buf)
{
    char buffer[512];

    // 1. Read command from user
    if (fgets(buffer, sizeof(buffer) - 2, stdin) == NULL)
    {
        return; // reading error or EOF
    }

    // 2. Concat the last '\n' by fgets
    buffer[strcspn(buffer, "\n")] = 0;

    // 3. Delimiter for bytes stream problem
    strcat(buffer, "\r\n");
    strcat(msg, buffer);
    // 4. Sending command to server
    if (send_all(sockfd, msg, strlen(msg)) == -1)
    {
        fprintf(stderr, "Failed to send message.\n");
        return;
    }

    // 5. Waiting for response
    int len = recv_line(sockfd, recv_buf, BUFF_SIZE);
    if (len <= 0)
    {
        printf("Server disconnected or error.\n");
        return;
    }

    printf("Server response: %s\n", recv_buf);
}

void simple_menu(int sockfd, char *recv_buf)
{
    int choice;
    do
    {
        choice = 0;
        // Clear the console for a cleaner menu display
        // system("cls"); // For Windows
        //system("clear"); // For Linux/macOS
        printf("\n--- Simple Menu ---\n");
        printf("1. Login\n");
        printf("2. Upload file\n");
        printf("3. Download file\n");
        printf("4. List files in working directory\n");
        printf("5. Change working directory\n");
        printf("6. Logout\n");
        printf("7. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        // Consume the newline character left in the buffer by scanf
        while (getchar() != '\n')
            ;

        switch (choice)
        {
        case 1:
            send_login_command(sockfd, recv_buf);
            break;
        case 2:
            send_upload_command(sockfd, recv_buf);
            break;
        case 3:
            send_download_command(sockfd, recv_buf);
            break;
        case 4:
            send_list_command(sockfd, recv_buf);
            break;
        case 5:
            send_chdir_command(sockfd, recv_buf);
            break;
        case 6:
            send_bye_command(sockfd, recv_buf);
            break;
        case 7:
            // exit_program();
            break;
        default:
            printf("Invalid choice. Please try again.\n");
            break;
        }
        printf("\nPress Enter to continue...");
        while (getchar() != '\n')
            ; // Wait for user to press Enter

    } while (choice != 8);
}

void send_login_command(int sockfd, char *recv_buf)
{
    char username[80];
    char password_msg[80];
    
    // Step 1: Send username
    strcpy(username, "USER ");
    printf("Please enter a username: ");
    send_and_response(sockfd, username, recv_buf);
    
    // Check if server asks for password
    if (strncmp(recv_buf, "111:", 4) == 0) // Username OK, need password
    {
        // Step 2: Send password
        strcpy(password_msg, "PASS ");
        printf("Please enter password: ");
        send_and_response(sockfd, password_msg, recv_buf);
    }
}

void send_bye_command(int sockfd, char *recv_buf)
{
    char *msg = "BYE\r\n";
     // 4. Sending command to server
    if (send_all(sockfd, msg, strlen(msg)) == -1)
    {
        fprintf(stderr, "Failed to send message.\n");
        return;
    }

    // 5. Waiting for response
    int len = recv_line(sockfd, recv_buf, BUFF_SIZE);
    if (len <= 0)
    {
        printf("Server disconnected or error.\n");
        return;
    }

    printf("Server response: %s\n", recv_buf);
}

void send_article_command(int sockfd, char *recv_buf)
{
    char article[4096];
    strcpy(article, "POST ");
    printf("Please enter an article to post: ");

    // 4. Send command to server and wait for response
    send_and_response(sockfd, article, recv_buf);
}

void send_upload_command(int sockfd, char *recv_buf)
{
    char filepath[1024];
    char buffer[BUFF_SIZE + 1];

    printf("Enter file path: ");
    if (fgets(filepath, sizeof(filepath), stdin) == NULL)
    {
        return;
    }

    // Trim the last '\n'
    filepath[strcspn(filepath, "\r\n")] = 0;

    if (strlen(filepath) == 0)
    {
        printf("No file path entered.\n");
        return;
    }

    // Open file for sending
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL)
    {
        perror("Error: Cannot open file");
        return;
    }

    long file_size = get_file_size(fp);

    char *filename_ptr = basename(filepath);

    char filename[256];
    strncpy(filename, filename_ptr, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    // Craft payload
    char command[1280];
    snprintf(command, sizeof(command), "UPLD %s %ld\r\n", filename, file_size);

    if (send(sockfd, command, strlen(command), 0) < 0)
    {
        perror("Send() error when sending UPLD command");
        fclose(fp);
        return;
    }

    //Wait for "+OK Please send file"
    int received_bytes = recv(sockfd, buffer, BUFF_SIZE, 0);
    if (received_bytes <= 0)
    {
        printf("Server has closed connection.\n");
        fclose(fp);
        return;
    }
    buffer[received_bytes] = '\0';

    //Check server response
    if (strncmp(buffer, "+OK Please send file", strlen("+OK Please send file")) != 0) {
        printf("Server is not available for uploading file: %s\n", buffer);
        fclose(fp);
        return; 
    }

    printf("Sending file %s (%ld bytes)... \n",filename, file_size);
    size_t bytes_read;
    long total_sent = 0;

    while((bytes_read = fread(buffer, 1, BUFF_SIZE, fp)) > 0){
        if(send(sockfd, buffer, bytes_read, 0) < 0){
            perror("send() error while uploading file");
            break;
        }
        total_sent += bytes_read;
    }
    fclose(fp);

    if(total_sent != file_size){
        printf("Error: Sent %ld bytes but file size is %ld. \n", total_sent,file_size);
    }

    received_bytes = recv(sockfd, buffer, BUFF_SIZE, 0);
    if (received_bytes <= 0) {
        printf("Server has closed connection.\n");
        return;
    }
    buffer[received_bytes] = '\0';
    printf("Result: %s\n", buffer);
}

void send_chdir_command(int sockfd, char *recv_buf)
{
    char buffer[BUFF_SIZE];
    
    // Step 1: Get current directory from server
    char *get_dir_cmd = "GETDIR\r\n";
    if (send_all(sockfd, get_dir_cmd, strlen(get_dir_cmd)) == -1)
    {
        fprintf(stderr, "Failed to send message.\n");
        return;
    }
    
    int len = recv_line(sockfd, recv_buf, BUFF_SIZE);
    if (len <= 0)
    {
        printf("Server disconnected or error.\n");
        return;
    }
    
    printf("Thư mục làm việc hiện tại: %s\n", recv_buf);
    
    // Step 2: Ask user if they want to change
    char choice;
    while (1)
    {
        printf("Bạn có muốn đổi thư mục làm việc không? y/n: ");
        scanf(" %c", &choice);
        while (getchar() != '\n'); // Clear buffer
        
        if (choice == 'y' || choice == 'Y')
        {
            // Get new directory
            char new_dir[512];
            printf("Vui lòng nhập thư mục làm việc mới: ");
            if (fgets(new_dir, sizeof(new_dir), stdin) == NULL)
            {
                return;
            }
            new_dir[strcspn(new_dir, "\n")] = 0; // Remove newline
            
            // Send CHDIR command
            char chdir_cmd[600];
            snprintf(chdir_cmd, sizeof(chdir_cmd), "CHDIR %s\r\n", new_dir);
            
            if (send_all(sockfd, chdir_cmd, strlen(chdir_cmd)) == -1)
            {
                fprintf(stderr, "Failed to send message.\n");
                return;
            }
            
            len = recv_line(sockfd, recv_buf, BUFF_SIZE);
            if (len <= 0)
            {
                printf("Server disconnected or error.\n");
                return;
            }
            
            printf("Server response: %s\n", recv_buf);
            break;
        }
        else if (choice == 'n' || choice == 'N')
        {
            printf("Không thay đổi thư mục làm việc.\n");
            break;
        }
        else
        {
            printf("Vui lòng nhập \"y\" hoặc \"n\".\n");
        }
    }
}

void send_list_command(int sockfd, char *recv_buf)
{
    char buffer[BUFF_SIZE];
    
    // Send LIST command to server
    char *list_cmd = "LIST\r\n";
    if (send_all(sockfd, list_cmd, strlen(list_cmd)) == -1)
    {
        fprintf(stderr, "Failed to send message.\n");
        return;
    }
    
    // Receive response
    int len = recv_line(sockfd, recv_buf, BUFF_SIZE);
    if (len <= 0)
    {
        printf("Server disconnected or error.\n");
        return;
    }
    
    // Check if error or success
    if (strncmp(recv_buf, "221:", 4) == 0 || strncmp(recv_buf, "ERR", 3) == 0)
    {
        printf("Server response: %s\n", recv_buf);
        return;
    }
    
    // Print header
    printf("\n=== Files in working directory ===\n");
    printf("%s\n", recv_buf);
    
    // Receive file list (multiple lines)
    while (1)
    {
        len = recv_line(sockfd, buffer, BUFF_SIZE);
        if (len <= 0)
        {
            break;
        }
        
        // Check for end marker
        if (strcmp(buffer, "END") == 0)
        {
            break;
        }
        
        printf("%s\n", buffer);
    }
    printf("==================================\n");
}

void send_download_command(int sockfd, char *recv_buf)
{
    char filename[512];
    char buffer[BUFF_SIZE + 1];
    char command[600];

    printf("Enter filename to download: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL)
    {
        return;
    }

    // Trim the last '\n'
    filename[strcspn(filename, "\r\n")] = 0;

    if (strlen(filename) == 0)
    {
        printf("No filename entered.\n");
        return;
    }

    // Send DNLD command
    snprintf(command, sizeof(command), "DNLD %s\r\n", filename);
    if (send_all(sockfd, command, strlen(command)) == -1)
    {
        fprintf(stderr, "Failed to send message.\n");
        return;
    }

    // Wait for server response
    int received_bytes = recv_line(sockfd, recv_buf, BUFF_SIZE);
    if (received_bytes <= 0)
    {
        printf("Server has closed connection.\n");
        return;
    }

    // Check for error responses
    if (strncmp(recv_buf, "221:", 4) == 0 || strncmp(recv_buf, "ERR", 3) == 0)
    {
        printf("Server response: %s\n", recv_buf);
        return;
    }

    // Parse response: "+OK <filesize>"
    if (strncmp(recv_buf, "+OK", 3) != 0)
    {
        printf("Unexpected server response: %s\n", recv_buf);
        return;
    }

    // Extract file size
    char *size_str = strchr(recv_buf, ' ');
    if (size_str == NULL)
    {
        printf("Invalid response format: %s\n", recv_buf);
        return;
    }
    size_str++; // Skip the space

    long file_size = atol(size_str);
    if (file_size <= 0)
    {
        printf("Invalid file size: %ld\n", file_size);
        return;
    }

    printf("Downloading file %s (%ld bytes)...\n", filename, file_size);

    // Open file for writing
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        perror("Error: Cannot create file");
        return;
    }

    // Receive file data
    long total_received = 0;
    while (total_received < file_size)
    {
        int to_receive = (file_size - total_received > BUFF_SIZE) ? BUFF_SIZE : (file_size - total_received);
        received_bytes = recv(sockfd, buffer, to_receive, 0);
        if (received_bytes <= 0)
        {
            printf("Connection error while downloading file.\n");
            fclose(fp);
            remove(filename);
            return;
        }
        fwrite(buffer, 1, received_bytes, fp);
        total_received += received_bytes;
    }

    fclose(fp);

    if (total_received == file_size)
    {
        printf("Download successful! File saved as: %s\n", filename);
    }
    else
    {
        printf("Error: Expected %ld bytes but received %ld bytes.\n", file_size, total_received);
        remove(filename);
    }
}
