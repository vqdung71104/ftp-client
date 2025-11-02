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
        printf("2. Post message\n");
        printf("3. Upload file\n");
        printf("4. Logout\n");
        printf("5. Exit\n");
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
            send_article_command(sockfd, recv_buf);
            break;
        case 3:
            send_upload_command(sockfd, recv_buf);
            break;
        case 4:
            send_bye_command(sockfd, recv_buf);
            break;
        case 5:
            // exit_program();
            break;
        default:
            printf("Invalid choice. Please try again.\n");
            break;
        }
        printf("\nPress Enter to continue...");
        while (getchar() != '\n')
            ; // Wait for user to press Enter

    } while (choice != 5);
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
    snprintf(command, sizeof(command), "UPLD %s %ld \r\n", filename, file_size);

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
