#ifndef FTP_SERVER_H
#define FTP_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#define BUFFER_SIZE 8192
#define MAX_CLIENTS 100
#define BACKLOG 10

// FTP Response Codes
#define FTP_DATA_CONNECTION_OPEN "150 File status okay; about to open data connection.\r\n"
#define FTP_SERVICE_READY "220 FTP Server Ready.\r\n"
#define FTP_GOODBYE "221 Goodbye.\r\n"
#define FTP_DATA_OPEN "225 Data connection open.\r\n"
#define FTP_TRANSFER_COMPLETE "226 Transfer complete.\r\n"
#define FTP_PASSIVE_MODE "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\r\n"
#define FTP_USER_LOGGED_IN "230 User logged in.\r\n"
#define FTP_FILE_OK "250 File action okay, completed.\r\n"
#define FTP_PATHNAME_CREATED "257 \"%s\" created.\r\n"
#define FTP_USER_OK "331 Username okay, need password.\r\n"
#define FTP_NEED_ACCOUNT "332 Need account for login.\r\n"
#define FTP_FILE_PENDING "350 File action pending.\r\n"
#define FTP_SERVICE_NOT_AVAILABLE "421 Service not available.\r\n"
#define FTP_CANNOT_OPEN_DATA "425 Can't open data connection.\r\n"
#define FTP_TRANSFER_ABORTED "426 Transfer aborted.\r\n"
#define FTP_FILE_ACTION_NOT_TAKEN "450 File action not taken.\r\n"
#define FTP_ACTION_ABORTED "451 Action aborted: local error.\r\n"
#define FTP_INSUFFICIENT_STORAGE "452 Insufficient storage.\r\n"
#define FTP_SYNTAX_ERROR "500 Syntax error.\r\n"
#define FTP_SYNTAX_ERROR_PARAM "501 Syntax error in parameters.\r\n"
#define FTP_COMMAND_NOT_IMPLEMENTED "502 Command not implemented.\r\n"
#define FTP_BAD_SEQUENCE "503 Bad sequence of commands.\r\n"
#define FTP_NOT_LOGGED_IN "530 Not logged in.\r\n"
#define FTP_NEED_ACCOUNT_FOR_FILE "532 Need account for storing files.\r\n"
#define FTP_FILE_UNAVAILABLE "550 File unavailable.\r\n"
#define FTP_PAGE_TYPE_UNKNOWN "551 Page type unknown.\r\n"
#define FTP_EXCEED_STORAGE "552 Exceed storage allocation.\r\n"
#define FTP_FILE_NAME_NOT_ALLOWED "553 File name not allowed.\r\n"

// Client session structure
typedef struct {
    int control_sock;           // Control connection socket
    int data_sock;              // Data connection socket
    int passive_sock;           // Passive mode listening socket
    struct sockaddr_in data_addr;
    
    char username[256];
    char password[256];
    char current_dir[1024];
    char root_dir[1024];
    
    int logged_in;
    int binary_mode;            // 0 = ASCII, 1 = Binary
    int passive_mode;           // 0 = Active, 1 = Passive
    
    pthread_t thread_id;
    char client_ip[INET_ADDRSTRLEN];
    int client_port;
} ftp_session_t;

// Function prototypes
void* handle_client(void* arg);
void send_response(int sock, const char* response);
int create_data_connection(ftp_session_t* session);
void close_data_connection(ftp_session_t* session);

// Command handlers
void handle_user(ftp_session_t* session, const char* username);
void handle_pass(ftp_session_t* session, const char* password);
void handle_syst(ftp_session_t* session);
void handle_feat(ftp_session_t* session);
void handle_pwd(ftp_session_t* session);
void handle_cwd(ftp_session_t* session, const char* path);
void handle_cdup(ftp_session_t* session);
void handle_type(ftp_session_t* session, const char* type);
void handle_pasv(ftp_session_t* session);
void handle_port(ftp_session_t* session, const char* params);
void handle_list(ftp_session_t* session, const char* path);
void handle_nlst(ftp_session_t* session, const char* path);
void handle_retr(ftp_session_t* session, const char* filename);
void handle_stor(ftp_session_t* session, const char* filename);
void handle_dele(ftp_session_t* session, const char* filename);
void handle_mkd(ftp_session_t* session, const char* dirname);
void handle_rmd(ftp_session_t* session, const char* dirname);
void handle_size(ftp_session_t* session, const char* filename);
void handle_quit(ftp_session_t* session);
void handle_noop(ftp_session_t* session);

// Utility functions
int authenticate_user(const char* username, const char* password, char* root_dir);
void format_list_line(char* output, const char* path, const char* filename);
void get_absolute_path(const char* root, const char* current, const char* requested, char* result);
int is_path_allowed(const char* root, const char* path);

#endif // FTP_SERVER_H
