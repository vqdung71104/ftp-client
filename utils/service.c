#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "service.h"
#include "account.h"
#include "utils.h"
#include "logging.h"

#define BUFF_SIZE 16384

int user_validation(char *username)
{
    int valid_user = 0;

    for (int i = 0; i < count; i++)
    {
        int cmp = strcmp(users[i].username, username);
        if (cmp == 0)
        {
            if (users[i].status == 0)
            {
                return -1;
            }
            else
            {
                valid_user = 1;
                break;
            }
        }
    }
    return valid_user;
}

int password_validation(char *username, char *password)
{
    for (int i = 0; i < count; i++)
    {
        int cmp = strcmp(users[i].username, username);
        if (cmp == 0)
        {
            int pass_cmp = strcmp(users[i].password, password);
            if (pass_cmp == 0)
            {
                return 1; // correct password
            }
            else
            {
                return 0; // incorrect password
            }
        }
    }
    return 0; // user not found
}

void login(char *command_value, int client_socket)
{
    int user_validation_result = user_validation(command_value);
    char *return_msg;
    
    if (is_logged_in == 1)
    {
        return_msg = "213: You have already logged in\r\n";
    }
    else if (user_validation_result == 1) // User exist and active
    {
        return_msg = "111: Username OK, need password\r\n";
        strcpy(pending_username, command_value);
    }
    else if (user_validation_result == 0) // User does not exist
    {
        return_msg = "212: Account does NOT exist\r\n";
        memset(pending_username, 0, sizeof(pending_username));
    }
    else if (user_validation_result == -1) // User exist but banned
    {
        return_msg = "211: Account IS banned\r\n";
        memset(pending_username, 0, sizeof(pending_username));
    }
    
    send_all(client_socket, return_msg, strlen(return_msg));
    usleep(1000);
}

void verify_password(char *password, int client_socket)
{
    char *return_msg;
    
    if (is_logged_in == 1)
    {
        return_msg = "213: You have already logged in\r\n";
    }
    else if (strlen(pending_username) == 0)
    {
        return_msg = "220: Please send username first (USER command)\r\n";
    }
    else
    {
        int password_validation_result = password_validation(pending_username, password);
        if (password_validation_result == 1)
        {
            return_msg = "110: Login successful\r\n";
            is_logged_in = 1;
            // Save current user info
            strcpy(current_username, pending_username);
            for (int i = 0; i < count; i++)
            {
                if (strcmp(users[i].username, pending_username) == 0)
                {
                    strcpy(current_root_dir, users[i].root_dir);
                    break;
                }
            }
            memset(pending_username, 0, sizeof(pending_username));
        }
        else
        {
            return_msg = "214: Incorrect password\r\n";
            memset(pending_username, 0, sizeof(pending_username));
        }
    }
    
    send_all(client_socket, return_msg, strlen(return_msg));
    usleep(1000);
}

void handle_upload_file(int conn_sock, const char *client_addr_str, char *command_value, const char *request_log)
{
    char filename[512];
    unsigned long file_size;
    const char *response_log;
    char file_buffer[16384];

    // Check if user is logged in
    if (is_logged_in == 0)
    {
        const char *err_msg = "221: You have NOT logged in\r\n";
        send_all(conn_sock, err_msg, strlen(err_msg));
        response_log = "-ERR Not logged in";
        log_message(client_addr_str, request_log, response_log);
        return;
    }

    /*
     * Expected format from command_value: <filename> <filesize>
     * We find the last space to separate filename and filesize
     * so the filename may contain spaces.
     */
    char *last_space = strrchr(command_value, ' ');
    if (last_space == NULL)
    {
        const char *err_msg = "ERR Invalid command\r\n";
        send_all(conn_sock, err_msg, strlen(err_msg));
        response_log = "-ERR Invalid command";
        log_message(client_addr_str, request_log, response_log);
        return;
    }

    *last_space = '\0';
    char *size_str = last_space + 1;
    
    // Trim any whitespace from size_str
    while (*size_str == ' ' || *size_str == '\t') {
        size_str++;
    }
    
    char *endptr = NULL;
    unsigned long fs = strtoul(size_str, &endptr, 10);
    if (endptr == size_str || fs == 0)
    {
        const char *err_msg = "ERR Invalid filesize\r\n";
        send_all(conn_sock, err_msg, strlen(err_msg));
        response_log = "-ERR Invalid filesize";
        log_message(client_addr_str, request_log, response_log);
        return;
    }

    file_size = fs;
    strncpy(filename, command_value, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    // Use current user's root directory instead of storage_dir
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", current_root_dir, filename);
    
    // Create user directory if not exists
    mkdir(current_root_dir, 0755);

    FILE *fp = fopen(filepath, "wb");

    if (fp == NULL)
    {
        perror("fopen() error");
        const char *err_msg = "ERR Cannot create file\r\n";
        send_all(conn_sock, err_msg, strlen(err_msg));
        response_log = "-ERR Cannot create file";
        log_message(client_addr_str, request_log, response_log);
        return;
    }

    const char *ok_msg = "+OK Please send file\r\n";
    send_all(conn_sock, ok_msg, strlen(ok_msg));

    /* Step 4: receive file in chunks until we get file_size bytes */
    unsigned long total_received = 0;
    int bytes_in_chunk;

    while (total_received < file_size)
    {
        bytes_in_chunk = recv(conn_sock, file_buffer, 16384, 0);
        if (bytes_in_chunk <= 0)
        {
            printf("Client %s disconnected while transfering file.\n", client_addr_str);
            break;
        }
        fwrite(file_buffer, 1, bytes_in_chunk, fp);
        total_received += bytes_in_chunk;
    }
    fclose(fp);

    /* Step 5: send the final result */
    if (total_received == file_size)
    {
        const char *success_msg = "OK Successful upload\r\n";
        send_all(conn_sock, success_msg, strlen(success_msg));
        response_log = "+OK Successful upload";
        printf("Client %s has uploaded file %s successfully.\n", client_addr_str, filename);
    }
    else
    {
        const char *fail_msg = "ERR Upload failed\r\n";
        send_all(conn_sock, fail_msg, strlen(fail_msg));
        response_log = "-ERR Upload failed";
        remove(filepath); /* remove incomplete file */
        printf("Client %s upload file %s failed.\n", client_addr_str, filename);
    }

    log_message(client_addr_str, request_log, response_log);
}

void get_current_directory(int client_socket)
{
    char return_msg[1100];
    
    if (is_logged_in == 0)
    {
        strcpy(return_msg, "221: You have NOT logged in\r\n");
    }
    else
    {
        snprintf(return_msg, sizeof(return_msg), "150: Current directory: %s\r\n", current_root_dir);
    }
    
    send_all(client_socket, return_msg, strlen(return_msg));
    usleep(1000);
}

void change_directory(char *new_dir, int client_socket)
{
    char return_msg[512];
    
    if (is_logged_in == 0)
    {
        strcpy(return_msg, "221: You have NOT logged in\r\n");
        send_all(client_socket, return_msg, strlen(return_msg));
        usleep(1000);
        return;
    }
    
    // Update in-memory user data
    strcpy(current_root_dir, new_dir);
    for (int i = 0; i < count; i++)
    {
        if (strcmp(users[i].username, current_username) == 0)
        {
            strcpy(users[i].root_dir, new_dir);
            break;
        }
    }
    
    // Update account.txt file
    FILE *fp = fopen("account.txt", "w");
    if (fp == NULL)
    {
        strcpy(return_msg, "240: Failed to update account file\r\n");
        send_all(client_socket, return_msg, strlen(return_msg));
        usleep(1000);
        return;
    }
    
    for (int i = 0; i < count; i++)
    {
        fprintf(fp, "%s %s %s %d\n", users[i].username, users[i].password, users[i].root_dir, users[i].status);
    }
    fclose(fp);
    
    strcpy(return_msg, "140: Directory changed successfully\r\n");
    send_all(client_socket, return_msg, strlen(return_msg));
    usleep(1000);
}

void list_files(int client_socket)
{
    char return_msg[BUFF_SIZE];
    
    if (is_logged_in == 0)
    {
        strcpy(return_msg, "221: You have NOT logged in\r\n");
        send_all(client_socket, return_msg, strlen(return_msg));
        usleep(1000);
        return;
    }
    
    // Open directory
    DIR *dir = opendir(current_root_dir);
    if (dir == NULL)
    {
        snprintf(return_msg, sizeof(return_msg), "ERR: Cannot open directory %s\r\n", current_root_dir);
        send_all(client_socket, return_msg, strlen(return_msg));
        usleep(1000);
        return;
    }
    
    // Send success message with directory path
    snprintf(return_msg, sizeof(return_msg), "150: Listing directory: %s\r\n", current_root_dir);
    send_all(client_socket, return_msg, strlen(return_msg));
    usleep(1000);
    
    // Read and send each file entry
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }
        
        // Get file stats
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", current_root_dir, entry->d_name);
        
        struct stat st;
        if (stat(fullpath, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                snprintf(return_msg, sizeof(return_msg), "[DIR]  %s\r\n", entry->d_name);
            }
            else
            {
                snprintf(return_msg, sizeof(return_msg), "[FILE] %s (%ld bytes)\r\n", entry->d_name, st.st_size);
            }
        }
        else
        {
            snprintf(return_msg, sizeof(return_msg), "[????] %s\r\n", entry->d_name);
        }
        
        send_all(client_socket, return_msg, strlen(return_msg));
        usleep(1000);
    }
    
    closedir(dir);
    
    // Send end marker
    strcpy(return_msg, "END\r\n");
    send_all(client_socket, return_msg, strlen(return_msg));
    usleep(1000);
}

void handle_download_file(int conn_sock, const char *client_addr_str, char *filename, const char *request_log)
{
    char return_msg[BUFF_SIZE];
    const char *response_log;
    char file_buffer[16384];
    
    // Check if user is logged in
    if (is_logged_in == 0)
    {
        const char *err_msg = "221: You have NOT logged in\r\n";
        send_all(conn_sock, err_msg, strlen(err_msg));
        response_log = "-ERR Not logged in";
        log_message(client_addr_str, request_log, response_log);
        return;
    }
    
    // Construct full file path
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", current_root_dir, filename);
    
    // Open file for reading
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL)
    {
        const char *err_msg = "ERR: File not found\r\n";
        send_all(conn_sock, err_msg, strlen(err_msg));
        response_log = "-ERR File not found";
        log_message(client_addr_str, request_log, response_log);
        return;
    }
    
    // Get file size
    long file_size = get_file_size(fp);
    
    // Send OK response with file size
    snprintf(return_msg, sizeof(return_msg), "+OK %ld\r\n", file_size);
    send_all(conn_sock, return_msg, strlen(return_msg));
    usleep(1000);
    
    // Send file data
    size_t bytes_read;
    long total_sent = 0;
    
    while ((bytes_read = fread(file_buffer, 1, 16384, fp)) > 0)
    {
        if (send_all(conn_sock, file_buffer, bytes_read) == -1)
        {
            fprintf(stderr, "send() error while downloading file\n");
            response_log = "-ERR Send failed";
            log_message(client_addr_str, request_log, response_log);
            fclose(fp);
            return;
        }
        total_sent += bytes_read;
    }
    
    fclose(fp);
    
    if (total_sent == file_size)
    {
        response_log = "+OK Successful download";
        printf("Client %s has downloaded file %s successfully.\n", client_addr_str, filename);
    }
    else
    {
        response_log = "-ERR Download failed";
        printf("Client %s download file %s failed.\n", client_addr_str, filename);
    }
    
    log_message(client_addr_str, request_log, response_log);
}
