#include "ftp_server.h"

// Establish data connection
int establish_data_connection(ftp_session_t* session) {
    int data_sock;
    
    if (session->passive_mode) {
        // Accept connection in passive mode
        if (session->passive_sock < 0) {
            return -1;
        }
        
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        data_sock = accept(session->passive_sock, (struct sockaddr*)&client_addr, &addr_len);
        
        close(session->passive_sock);
        session->passive_sock = -1;
        
        if (data_sock < 0) {
            return -1;
        }
    } else {
        // Connect to client in active mode
        data_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (data_sock < 0) {
            return -1;
        }
        
        if (connect(data_sock, (struct sockaddr*)&session->data_addr, sizeof(session->data_addr)) < 0) {
            close(data_sock);
            return -1;
        }
    }
    
    // Enable TCP_NODELAY for better performance
    int flag = 1;
    setsockopt(data_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    // Increase buffer sizes
    int buffer_size = 262144; // 256KB
    setsockopt(data_sock, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
    setsockopt(data_sock, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));
    
    return data_sock;
}

// Close data connection
//void close_data_connection(ftp_session_t* session) {
//    if (session->passive_sock >= 0) {
//        close(session->passive_sock);
//        session->passive_sock = -1;
//    }
//    session->passive_mode = 0;
//}

// LIST command
void handle_list(ftp_session_t* session, const char* path) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    // Resolve path
    char absolute_path[1024];
    if (path && strlen(path) > 0) {
        get_absolute_path(session->root_dir, session->current_dir, path, absolute_path);
    } else {
        get_absolute_path(session->root_dir, session->current_dir, ".", absolute_path);
    }
    
    if (!is_path_allowed(session->root_dir, absolute_path)) {
        send_response(session->control_sock, "550 Permission denied.\r\n");
        return;
    }
    
    DIR* dir = opendir(absolute_path);
    if (!dir) {
        send_response(session->control_sock, "550 Directory not found.\r\n");
        return;
    }
    
    send_response(session->control_sock, FTP_DATA_CONNECTION_OPEN);
    
    int data_sock = establish_data_connection(session);
    if (data_sock < 0) {
        closedir(dir);
        send_response(session->control_sock, FTP_CANNOT_OPEN_DATA);
        return;
    }
    
    // Read directory entries
    struct dirent* entry;
    char buffer[4096];
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0) continue;
        
        char full_path[2048];
        snprintf(full_path, sizeof(full_path), "%s/%s", absolute_path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == -1) continue;
        
        // Format: Unix ls -l style
        // -rw-r--r-- 1 user group size month day time filename
        char perms[11] = "----------";
        perms[0] = S_ISDIR(st.st_mode) ? 'd' : '-';
        perms[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        perms[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        perms[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
        perms[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
        perms[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
        perms[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
        perms[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
        perms[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
        perms[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
        
        struct tm* timeinfo = localtime(&st.st_mtime);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%b %d %H:%M", timeinfo);
        
        int len = snprintf(buffer, sizeof(buffer), "%s %3ld %-8s %-8s %8ld %s %s\r\n",
                          perms, st.st_nlink, "ftp", "ftp", 
                          (long)st.st_size, time_str, entry->d_name);
        
        send(data_sock, buffer, len, 0);
    }
    
    closedir(dir);
    close(data_sock);
    send_response(session->control_sock, FTP_TRANSFER_COMPLETE);
}

// RETR command (download from server)
void handle_retr(ftp_session_t* session, const char* filename) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    char absolute_path[1024];
    get_absolute_path(session->root_dir, session->current_dir, filename, absolute_path);
    
    if (!is_path_allowed(session->root_dir, absolute_path)) {
        send_response(session->control_sock, "550 Permission denied.\r\n");
        return;
    }
    
    FILE* file = fopen(absolute_path, "rb");
    if (!file) {
        send_response(session->control_sock, "550 File not found.\r\n");
        return;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    send_response(session->control_sock, FTP_DATA_CONNECTION_OPEN);
    
    int data_sock = establish_data_connection(session);
    if (data_sock < 0) {
        fclose(file);
        send_response(session->control_sock, FTP_CANNOT_OPEN_DATA);
        return;
    }
    
    // Transfer file
    char buffer[16384];
    size_t bytes_read;
    long total_sent = 0;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        ssize_t sent = send(data_sock, buffer, bytes_read, 0);
        if (sent < 0) break;
        total_sent += sent;
    }
    
    fclose(file);
    close(data_sock);
    
    printf("[%s:%d] RETR %s (%ld bytes)\n", 
           session->client_ip, session->client_port, filename, file_size);
    
    send_response(session->control_sock, FTP_TRANSFER_COMPLETE);
}

// STOR command (upload to server)
void handle_stor(ftp_session_t* session, const char* filename) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    char absolute_path[1024];
    get_absolute_path(session->root_dir, session->current_dir, filename, absolute_path);
    
    if (!is_path_allowed(session->root_dir, absolute_path)) {
        send_response(session->control_sock, "550 Permission denied.\r\n");
        return;
    }
    
    FILE* file = fopen(absolute_path, "wb");
    if (!file) {
        send_response(session->control_sock, "550 Cannot create file.\r\n");
        return;
    }
    
    send_response(session->control_sock, FTP_DATA_CONNECTION_OPEN);
    
    int data_sock = establish_data_connection(session);
    if (data_sock < 0) {
        fclose(file);
        unlink(absolute_path);
        send_response(session->control_sock, FTP_CANNOT_OPEN_DATA);
        return;
    }
    
    // Receive file
    char buffer[16384];
    ssize_t bytes_received;
    long total_received = 0;
    
    while ((bytes_received = recv(data_sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
        total_received += bytes_received;
    }
    
    fclose(file);
    close(data_sock);
    
    printf("[%s:%d] STOR %s (%ld bytes)\n", 
           session->client_ip, session->client_port, filename, total_received);
    
    send_response(session->control_sock, FTP_TRANSFER_COMPLETE);
}

// SIZE command
void handle_size(ftp_session_t* session, const char* filename) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    char absolute_path[1024];
    get_absolute_path(session->root_dir, session->current_dir, filename, absolute_path);
    
    if (!is_path_allowed(session->root_dir, absolute_path)) {
        send_response(session->control_sock, "550 Permission denied.\r\n");
        return;
    }
    
    struct stat st;
    if (stat(absolute_path, &st) == -1 || S_ISDIR(st.st_mode)) {
        send_response(session->control_sock, "550 File not found.\r\n");
        return;
    }
    
    char response[256];
    snprintf(response, sizeof(response), "213 %ld\r\n", (long)st.st_size);
    send_response(session->control_sock, response);
}

// MDTM command (modification time)
void handle_mdtm(ftp_session_t* session, const char* filename) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    char absolute_path[1024];
    get_absolute_path(session->root_dir, session->current_dir, filename, absolute_path);
    
    if (!is_path_allowed(session->root_dir, absolute_path)) {
        send_response(session->control_sock, "550 Permission denied.\r\n");
        return;
    }
    
    struct stat st;
    if (stat(absolute_path, &st) == -1) {
        send_response(session->control_sock, "550 File not found.\r\n");
        return;
    }
    
    struct tm* timeinfo = gmtime(&st.st_mtime);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y%m%d%H%M%S", timeinfo);
    
    char response[256];
    snprintf(response, sizeof(response), "213 %s\r\n", time_str);
    send_response(session->control_sock, response);
}

// NLST command (name list - simple file listing)
void handle_nlst(ftp_session_t* session, const char* path) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    // Resolve path
    char absolute_path[1024];
    if (path && strlen(path) > 0) {
        get_absolute_path(session->root_dir, session->current_dir, path, absolute_path);
    } else {
        get_absolute_path(session->root_dir, session->current_dir, ".", absolute_path);
    }
    
    if (!is_path_allowed(session->root_dir, absolute_path)) {
        send_response(session->control_sock, "550 Permission denied.\r\n");
        return;
    }
    
    DIR* dir = opendir(absolute_path);
    if (!dir) {
        send_response(session->control_sock, "550 Directory not found.\r\n");
        return;
    }
    
    send_response(session->control_sock, FTP_DATA_CONNECTION_OPEN);
    
    int data_sock = establish_data_connection(session);
    if (data_sock < 0) {
        closedir(dir);
        send_response(session->control_sock, FTP_CANNOT_OPEN_DATA);
        return;
    }
    
    // Send simple file names
    struct dirent* entry;
    char buffer[256];
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        int len = snprintf(buffer, sizeof(buffer), "%s\r\n", entry->d_name);
        send(data_sock, buffer, len, 0);
    }
    
    closedir(dir);
    close(data_sock);
    send_response(session->control_sock, FTP_TRANSFER_COMPLETE);
}

// DELE command (delete file)
void handle_dele(ftp_session_t* session, const char* filename) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    char absolute_path[1024];
    get_absolute_path(session->root_dir, session->current_dir, filename, absolute_path);
    
    if (!is_path_allowed(session->root_dir, absolute_path)) {
        send_response(session->control_sock, "550 Permission denied.\r\n");
        return;
    }
    
    if (unlink(absolute_path) == 0) {
        send_response(session->control_sock, FTP_FILE_OK);
        printf("[%s:%d] DELE %s\n", session->client_ip, session->client_port, filename);
    } else {
        send_response(session->control_sock, "550 Delete failed.\r\n");
    }
}

// MKD command (make directory)
void handle_mkd(ftp_session_t* session, const char* dirname) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    char absolute_path[1024];
    get_absolute_path(session->root_dir, session->current_dir, dirname, absolute_path);
    
    if (!is_path_allowed(session->root_dir, absolute_path)) {
        send_response(session->control_sock, "550 Permission denied.\r\n");
        return;
    }
    
    if (mkdir(absolute_path, 0755) == 0) {
        char response[1124];
        snprintf(response, sizeof(response), "257 \"%s\" directory created.\r\n", dirname);
        send_response(session->control_sock, response);
        printf("[%s:%d] MKD %s\n", session->client_ip, session->client_port, dirname);
    } else {
        send_response(session->control_sock, "550 Create directory failed.\r\n");
    }
}

// RMD command (remove directory)
void handle_rmd(ftp_session_t* session, const char* dirname) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    char absolute_path[1024];
    get_absolute_path(session->root_dir, session->current_dir, dirname, absolute_path);
    
    if (!is_path_allowed(session->root_dir, absolute_path)) {
        send_response(session->control_sock, "550 Permission denied.\r\n");
        return;
    }
    
    if (rmdir(absolute_path) == 0) {
        send_response(session->control_sock, FTP_FILE_OK);
        printf("[%s:%d] RMD %s\n", session->client_ip, session->client_port, dirname);
    } else {
        send_response(session->control_sock, "550 Remove directory failed.\r\n");
    }
}
