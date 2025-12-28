#include "ftp_server.h"
#include "../utils/account.h"
#include "../utils/logging.h"
#include "../utils/utils.h"
#include <strings.h>
#include <ctype.h>
#include <time.h>

// Global variables from account.c
extern Account *users;  // Dynamic array, not static array
extern int count;

// Load users from account.txt
void load_users() {
    read_file_data();
}

// Get current time string in HH:MM:SS
static void get_current_time_str(char *buf, size_t len) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t) {
        strftime(buf, len, "%d/%m/%Y %H:%M:%S", t);
    } else {
        snprintf(buf, len, "--/--/---- --:--:--");
    }
}

// Send response to client
void send_response(int sock, const char* response) {
    send(sock, response, strlen(response), 0);
}

// Authenticate user
int authenticate_user(const char* username, const char* password, char* root_dir) {
    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if (users[i].status == 0) {
                return -1; // Banned
            }
            if (strcmp(users[i].password, password) == 0) {
                strcpy(root_dir, users[i].root_dir);
                return 1; // Success
            }
            return 0; // Wrong password
        }
    }
    return 0; // User not found
}

// Format list line (Unix ls -l format)
void format_list_line(char* output, const char* path, const char* filename) {
    struct stat st;
    char fullpath[2048];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);
    
    if (stat(fullpath, &st) == -1) {
        return;
    }
    
    // Permission string
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
    
    // Get time
    struct tm* timeinfo = localtime(&st.st_mtime);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", timeinfo);
    
    // Format: perms links owner group size date filename
    snprintf(output, 1024, "%s   1 owner group %10ld %s %s\r\n",
             perms, (long)st.st_size, timebuf, filename);
}

// Create data connection
int create_data_connection(ftp_session_t* session) {
    if (session->passive_mode) {
        // Passive mode: accept connection
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        session->data_sock = accept(session->passive_sock, 
                                    (struct sockaddr*)&client_addr, 
                                    &addr_len);
        
        if (session->data_sock < 0) {
            return -1;
        }
        
        // Enable TCP optimization
        int flag = 1;
        setsockopt(session->data_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
        
        int sndbuf = 262144;
        int rcvbuf = 262144;
        setsockopt(session->data_sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
        setsockopt(session->data_sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
        
        close(session->passive_sock);
        session->passive_sock = -1;
    } else {
        // Active mode: connect to client
        session->data_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (session->data_sock < 0) {
            return -1;
        }
        
        if (connect(session->data_sock, 
                   (struct sockaddr*)&session->data_addr, 
                   sizeof(session->data_addr)) < 0) {
            close(session->data_sock);
            session->data_sock = -1;
            return -1;
        }
        
        // Enable TCP optimization
        int flag = 1;
        setsockopt(session->data_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
        
        int sndbuf = 262144;
        int rcvbuf = 262144;
        setsockopt(session->data_sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
        setsockopt(session->data_sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
    }
    
    return 0;
}

// Close data connection
void close_data_connection(ftp_session_t* session) {
    if (session->data_sock > 0) {
        close(session->data_sock);
        session->data_sock = -1;
    }
    if (session->passive_sock > 0) {
        close(session->passive_sock);
        session->passive_sock = -1;
    }
}

// Thread function to handle each client
void* handle_client(void* arg) {
    ftp_session_t* session = (ftp_session_t*)arg;
    char buffer[BUFFER_SIZE];
    char command[64], params[1024];
    char timestr[32];
    
    // Send welcome message
    send_response(session->control_sock, FTP_SERVICE_READY);
    get_current_time_str(timestr, sizeof(timestr));
    printf("[%s:%d][%s] New client connected\n", session->client_ip, session->client_port, timestr);
    
    // Main command loop
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int n = recv_line(session->control_sock, buffer, sizeof(buffer) - 1);
        
        if (n <= 0) {
            break; // Client disconnected
        }
        
        // Parse command and parameters
        memset(command, 0, sizeof(command));
        memset(params, 0, sizeof(params));
        
        char* space = strchr(buffer, ' ');
        if (space) {
            *space = '\0';
            strncpy(command, buffer, sizeof(command) - 1);
            strncpy(params, space + 1, sizeof(params) - 1);
            
            // Remove trailing \r\n
            char* crlf = strstr(params, "\r\n");
            if (crlf) *crlf = '\0';
        } else {
            strncpy(command, buffer, sizeof(command) - 1);
            // Remove trailing \r\n from command
            char* crlf = strstr(command, "\r\n");
            if (crlf) *crlf = '\0';
        }
        
        // Convert command to uppercase
        for (int i = 0; command[i]; i++) {
            command[i] = toupper(command[i]);
        }
        
         get_current_time_str(timestr, sizeof(timestr));
         printf("[%s:%d][%s] Command: %s %s\n", session->client_ip, session->client_port,
             timestr, command, params);
        
        // Dispatch commands
        if (strcmp(command, "USER") == 0) {
            handle_user(session, params);
        } else if (strcmp(command, "PASS") == 0) {
            handle_pass(session, params);
        } else if (strcmp(command, "SYST") == 0) {
            handle_syst(session);
        } else if (strcmp(command, "FEAT") == 0) {
            handle_feat(session);
        } else if (strcmp(command, "PWD") == 0 || strcmp(command, "XPWD") == 0) {
            handle_pwd(session);
        } else if (strcmp(command, "CWD") == 0 || strcmp(command, "XCWD") == 0) {
            handle_cwd(session, params);
        } else if (strcmp(command, "CDUP") == 0 || strcmp(command, "XCUP") == 0) {
            handle_cdup(session);
        } else if (strcmp(command, "TYPE") == 0) {
            handle_type(session, params);
        } else if (strcmp(command, "PASV") == 0) {
            handle_pasv(session);
        } else if (strcmp(command, "PORT") == 0) {
            handle_port(session, params);
        } else if (strcmp(command, "LIST") == 0) {
            handle_list(session, params);
        } else if (strcmp(command, "NLST") == 0) {
            handle_nlst(session, params);
        } else if (strcmp(command, "RETR") == 0) {
            handle_retr(session, params);
        } else if (strcmp(command, "STOR") == 0) {
            handle_stor(session, params);
        } else if (strcmp(command, "DELE") == 0) {
            handle_dele(session, params);
        } else if (strcmp(command, "MKD") == 0 || strcmp(command, "XMKD") == 0) {
            handle_mkd(session, params);
        } else if (strcmp(command, "RMD") == 0 || strcmp(command, "XRMD") == 0) {
            handle_rmd(session, params);
        } else if (strcmp(command, "SIZE") == 0) {
            handle_size(session, params);
        } else if (strcmp(command, "QUIT") == 0) {
            handle_quit(session);
            break;
        } else if (strcmp(command, "NOOP") == 0) {
            handle_noop(session);
        } else if (strcmp(command, "OPTS") == 0) {
            // Handle UTF8 option
            if (strncasecmp(params, "UTF8", 4) == 0) {
                send_response(session->control_sock, "200 UTF8 mode enabled.\r\n");
            } else {
                send_response(session->control_sock, "200 OPTS okay.\r\n");
            }
        } else {
            send_response(session->control_sock, FTP_COMMAND_NOT_IMPLEMENTED);
        }
    }
    
    // Cleanup
    get_current_time_str(timestr, sizeof(timestr));
    printf("[%s:%d][%s] Client disconnected\n", session->client_ip, session->client_port, timestr);
    close_data_connection(session);
    close(session->control_sock);
    free(session);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    int port = 2121; // Default FTP control port (using 2121 for non-root)
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    // Load account data
    load_users();
    
    printf("Starting FTP Server on port %d...\n", port);
    printf("Loaded %d user account(s)\n", count);
    
    // Create socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//tránh lỗi cổng bận
    
    // Bind to address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        return 1;
    }
    
    // Listen for connections
    if (listen(server_sock, BACKLOG) < 0) {
        perror("Listen failed");
        close(server_sock);
        return 1;
    }
    
    printf("FTP Server listening on port %d\n", port);
    printf("Waiting for connections...\n\n");
    
    // Accept clients
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }
        
        // Create session structure
        ftp_session_t* session = (ftp_session_t*)malloc(sizeof(ftp_session_t));
        if (!session) {
            close(client_sock);
            continue;
        }
        
        memset(session, 0, sizeof(ftp_session_t));
        session->control_sock = client_sock;
        session->passive_sock = -1;
        session->data_sock = -1;
        session->logged_in = 0;
        session->binary_mode = 1; // Default to binary
        session->passive_mode = 0;
        strcpy(session->current_dir, "/");
        strcpy(session->root_dir, "/");
        
        // Get client info
        inet_ntop(AF_INET, &client_addr.sin_addr, session->client_ip, sizeof(session->client_ip));
        session->client_port = ntohs(client_addr.sin_port);
        
        // Create thread to handle client
        if (pthread_create(&session->thread_id, NULL, handle_client, session) != 0) {
            perror("Thread creation failed");
            close(client_sock);
            free(session);
            continue;
        }
        
        // Detach thread so it cleans up automatically
        pthread_detach(session->thread_id);
    }
    
    close(server_sock);
    return 0;
}
