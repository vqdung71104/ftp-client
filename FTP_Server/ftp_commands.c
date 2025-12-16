#include "ftp_server.h"

// USER command
void handle_user(ftp_session_t* session, const char* username) {
    strncpy(session->username, username, sizeof(session->username) - 1);
    send_response(session->control_sock, FTP_USER_OK);
    
    char log[256];
    snprintf(log, sizeof(log), "USER %s", username);
    printf("[%s:%d] %s\n", session->client_ip, session->client_port, log);
}

// PASS command
void handle_pass(ftp_session_t* session, const char* password) {
    int result = authenticate_user(session->username, password, session->root_dir);
    
    if (result == 1) {
        session->logged_in = 1;
        strcpy(session->current_dir, "/");
        send_response(session->control_sock, FTP_USER_LOGGED_IN);
        printf("[%s:%d] User %s logged in successfully\n", 
               session->client_ip, session->client_port, session->username);
    } else if (result == -1) {
        send_response(session->control_sock, "530 Account banned.\r\n");
    } else {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
    }
}

// SYST command
void handle_syst(ftp_session_t* session) {
    send_response(session->control_sock, "215 UNIX Type: L8\r\n");
}

// FEAT command
void handle_feat(ftp_session_t* session) {
    char response[512];
    snprintf(response, sizeof(response),
             "211-Features:\r\n"
             " PASV\r\n"
             " SIZE\r\n"
             " MDTM\r\n"
             " UTF8\r\n"
             "211 End\r\n");
    send_response(session->control_sock, response);
}

// PWD command
void handle_pwd(ftp_session_t* session) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    // Show full absolute path instead of relative path
    char full_path[1124];
    if (strcmp(session->current_dir, "/") == 0) {
        snprintf(full_path, sizeof(full_path), "%s", session->root_dir);
    } else {
        snprintf(full_path, sizeof(full_path), "%s%s", session->root_dir, session->current_dir);
    }
    
    char response[1124];
    snprintf(response, sizeof(response), "257 \"%s\" is current directory.\r\n", 
             full_path);
    send_response(session->control_sock, response);
}

// CWD command
void handle_cwd(ftp_session_t* session, const char* path) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    char absolute_path[1024];
    
    // Check if this is an absolute system path (starts with /mnt, /home, etc.)
    // If yes, use it directly instead of combining with root_dir
    if (path[0] == '/' && strlen(path) > 1 && strstr(path, "/mnt") == path) {
        // This is an absolute system path, use it directly
        strcpy(absolute_path, path);
        printf("[DEBUG] CWD: Using absolute system path: %s\n", absolute_path);
    } else {
        // Normal FTP path processing
        get_absolute_path(session->root_dir, session->current_dir, path, absolute_path);
        printf("[DEBUG] CWD: root=%s, current=%s, path=%s, absolute=%s\n", 
               session->root_dir, session->current_dir, path, absolute_path);
    }
    
    // Check if directory exists first
    struct stat st;
    if (stat(absolute_path, &st) == -1) {
        printf("[DEBUG] stat() failed for %s: %s\n", absolute_path, strerror(errno));
        send_response(session->control_sock, "550 Directory not found.\r\n");
        return;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        printf("[DEBUG] %s is not a directory\n", absolute_path);
        send_response(session->control_sock, "550 Not a directory.\r\n");
        return;
    }
    
    // Check if we have access permission
    if (access(absolute_path, R_OK | X_OK) != 0) {
        printf("[DEBUG] access() failed for %s: %s\n", absolute_path, strerror(errno));
        send_response(session->control_sock, "550 Permission denied.\r\n");
        return;
    }
    
    // For absolute system paths, allow access anywhere (remove root restriction)
    if (!(path[0] == '/' && strlen(path) > 1 && strstr(path, "/mnt") == path)) {
        if (!is_path_allowed(session->root_dir, absolute_path)) {
            printf("[DEBUG] is_path_allowed() failed for %s\n", absolute_path);
            send_response(session->control_sock, "550 Access outside root directory denied.\r\n");
            return;
        }
    }
    
    // Update current_dir relative to root
    // If using absolute system path, update root_dir too
    if (path[0] == '/' && strlen(path) > 1 && strstr(path, "/mnt") == path) {
        strcpy(session->root_dir, absolute_path);
        strcpy(session->current_dir, "/");
        
        // Update account.txt with new root directory
        if (update_user_root_dir(session->username, absolute_path)) {
            printf("[INFO] Updated root_dir for user '%s' to '%s'\n", 
                   session->username, absolute_path);
        } else {
            printf("[WARNING] Failed to update account.txt for user '%s'\n", 
                   session->username);
        }
    } else {
        strcpy(session->current_dir, absolute_path + strlen(session->root_dir));
        if (strlen(session->current_dir) == 0) {
            strcpy(session->current_dir, "/");
        }
    }
    
    send_response(session->control_sock, FTP_FILE_OK);
}

// CDUP command
void handle_cdup(ftp_session_t* session) {
    handle_cwd(session, "..");
}

// TYPE command
void handle_type(ftp_session_t* session, const char* type) {
    if (strcmp(type, "I") == 0 || strcmp(type, "A") == 0) {
        session->binary_mode = (strcmp(type, "I") == 0) ? 1 : 0;
        send_response(session->control_sock, "200 Type set.\r\n");
    } else {
        send_response(session->control_sock, FTP_SYNTAX_ERROR_PARAM);
    }
}

// PASV command
void handle_pasv(ftp_session_t* session) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    // Create passive mode socket
    session->passive_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (session->passive_sock < 0) {
        send_response(session->control_sock, FTP_CANNOT_OPEN_DATA);
        return;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0; // Let system choose port
    
    if (bind(session->passive_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(session->passive_sock);
        session->passive_sock = -1;
        send_response(session->control_sock, FTP_CANNOT_OPEN_DATA);
        return;
    }
    
    if (listen(session->passive_sock, 1) < 0) {
        close(session->passive_sock);
        session->passive_sock = -1;
        send_response(session->control_sock, FTP_CANNOT_OPEN_DATA);
        return;
    }
    
    // Get the port number
    socklen_t addr_len = sizeof(addr);
    getsockname(session->passive_sock, (struct sockaddr*)&addr, &addr_len);
    int port = ntohs(addr.sin_port);
    
    // Get server IP (same as control connection)
    struct sockaddr_in server_addr;
    addr_len = sizeof(server_addr);
    getsockname(session->control_sock, (struct sockaddr*)&server_addr, &addr_len);
    unsigned char* ip = (unsigned char*)&server_addr.sin_addr.s_addr;
    
    char response[256];
    snprintf(response, sizeof(response), FTP_PASSIVE_MODE,
             ip[0], ip[1], ip[2], ip[3], port / 256, port % 256);
    send_response(session->control_sock, response);
    
    session->passive_mode = 1;
}

// PORT command
void handle_port(ftp_session_t* session, const char* params) {
    if (!session->logged_in) {
        send_response(session->control_sock, FTP_NOT_LOGGED_IN);
        return;
    }
    
    int h1, h2, h3, h4, p1, p2;
    if (sscanf(params, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        send_response(session->control_sock, FTP_SYNTAX_ERROR_PARAM);
        return;
    }
    
    char ip[64];
    snprintf(ip, sizeof(ip), "%d.%d.%d.%d", h1, h2, h3, h4);
    int port = p1 * 256 + p2;
    
    memset(&session->data_addr, 0, sizeof(session->data_addr));
    session->data_addr.sin_family = AF_INET;
    session->data_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &session->data_addr.sin_addr);
    
    session->passive_mode = 0;
    send_response(session->control_sock, "200 PORT command successful.\r\n");
}

// NOOP command
void handle_noop(ftp_session_t* session) {
    send_response(session->control_sock, "200 NOOP okay.\r\n");
}

// QUIT command
void handle_quit(ftp_session_t* session) {
    send_response(session->control_sock, FTP_GOODBYE);
    close_data_connection(session);
}
