#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>

#include "logging.h"
#include "utils.h"
#include "account.h"
#include "service.h"

#define BUFF_SIZE 16384
#define BACK_LOG 2
/**
 * @brief Handle client connection for uploading files.
 *
 * This function receives commands from a client, processes the "UPLD" command,
 * and saves the uploaded file to the given storage directory. It also sends
 * responses to the client and logs all requests and results.
 *
 * @param conn_sock       Client socket descriptor.
 * @param client_addr_str Client IP address as a string.
 * @param storage_dir     Directory path to store uploaded files.
 */
void handle_client(int conn_sock, const char *client_addr_str, const char *storage_dir);

/**
 * @brief Send a welcome message to a newly connected client.
 *
 * This function extracts the client's IP address and port, formats them into a string,
 * sends a welcome message to the client, and logs the connection event.
 *
 * @param conn_sock          Client socket descriptor.
 * @param client_addr        Client address structure.
 * @param client_addr_str_out Output buffer to store the formatted client address (IP:port).
 * @param str_len            Length of the output buffer.
 *
 * @return 1 on success, 0 on failure.
 */
int welcome_to_server(int conn_sock, struct sockaddr_in client_addr, char *client_addr_str_out, int str_len);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        perror("Usage: ./server <Port_number> <Directory_name>\n");
        exit(EXIT_FAILURE);
    }

    int listen_sock, conn_sock;
    struct sockaddr_in server_addr, client_addr;
    int sin_size;

    int port = atoi(argv[1]);
    char *storage_dir = argv[2];

    // Step 5: Reading account file
    read_file_data();

    // Create storage dir
    mkdir(storage_dir, 0755); // Owner: 7(RWX), Group: 5(RW), Other: 5(RW)

    // Step 1: Create a listening socket
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }

    // Enable address reuse to avoid "Address already in use" error
    int opt = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt() error");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    // Step 2: Bind socket address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind() error");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    // Step 3: Listen
    if (listen(listen_sock, BACK_LOG) == -1)
    {
        perror("listen() error");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    // Step 4: Welcome message
    printf("Server started at %d, storage dir is '%s'\n", port, storage_dir);
    printf("Waiting for client (iterative mode)...\n");

    while (1)
    {
        sin_size = sizeof(struct sockaddr_in);

        // accept request
        if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &sin_size)) == -1)
        {
            perror("accept() error");
            continue;
        }

        char client_addr_str[INET_ADDRSTRLEN + 6];

        int welcome_ok = welcome_to_server(conn_sock, client_addr, client_addr_str, sizeof(client_addr_str));
        if (welcome_ok)
        {
            handle_client(conn_sock, client_addr_str, storage_dir);
        }

        printf("Closing connection with %s\n", client_addr_str);
        close(conn_sock);
    }

    close(listen_sock);
    return 0;
}

void handle_client(int conn_sock, const char *client_addr_str, const char *storage_dir)
{

    // Step 2: Receive command
    char buffer[BUFF_SIZE];
    int received_bytes;
    while (1)
    {

        received_bytes = recv_line(conn_sock, buffer, BUFF_SIZE);
        if (received_bytes <= 0)
        {
            printf("Client %s has disconnected.\n", client_addr_str);
            close(conn_sock);
            return;
        }

        if (strlen(buffer) == 0)
        {
            printf("Client %s has sent empty string. Closing session\n", client_addr_str);
            return;
        }
        
        printf("Client %s sent command: %s\n", client_addr_str, buffer);
        
        char request_log[BUFF_SIZE];
        strncpy(request_log, buffer, BUFF_SIZE);
        strncat(request_log, "\\r\\n", 6);
        
        // Step 3: parse the command for handling
        char command[50];
        char command_value[512];

        memset(command, 0, sizeof(command));
        memset(command_value, 0, sizeof(command_value));

        char *ptr = strchr(buffer, ' '); // For seeking the index of the first space in buffer

        if (ptr != NULL)
        {
            // Other command such as "USER tungbt" OR "POST Hello World" OR "UPLD filename filesize"
            // Step 3: Getting command
            int index = ptr - buffer; // index of the first space
            strncpy(command, buffer, index);
            command[index] = '\0';

            // Step 4: Getting command value
            strcpy(command_value, ptr + 1);

            if (strcmp(command, "USER") == 0)
            {
                login(command_value, conn_sock);
                const char *response_log = "Login attempt - username";
                log_message(client_addr_str, request_log, response_log);
            }
            else if (strcmp(command, "PASS") == 0)
            {
                verify_password(command_value, conn_sock);
                const char *response_log = "Login attempt - password";
                log_message(client_addr_str, request_log, response_log);
            }
            else if (strcmp(command, "POST") == 0)
            {
                post_article(conn_sock, command_value);
                const char *response_log = "Post attempt";
                log_message(client_addr_str, request_log, response_log);
            }
            else if (strcmp(command, "UPLD") == 0)
            {
                handle_upload_file(conn_sock, client_addr_str, command_value, request_log);
            }
            else if (strcmp(command, "CHDIR") == 0)
            {
                change_directory(command_value, conn_sock);
                const char *response_log = "Change directory attempt";
                log_message(client_addr_str, request_log, response_log);
            }
            else if (strcmp(command, "DNLD") == 0)
            {
                handle_download_file(conn_sock, client_addr_str, command_value, request_log);
            }
            else
            {
                char *return_msg = "300: Unknown command\r\n";
                send_all(conn_sock, return_msg, strlen(return_msg));
                const char *response_log = "300: Unknown command";
                log_message(client_addr_str, request_log, response_log);
            }
        }
        else
        {
            // Command "BYE" or "GETDIR" or unknown command
            strcpy(command, buffer);
            if (strcmp(command, "BYE") == 0)
            { // BYE command
                char *return_msg;
                if (is_logged_in == 0)
                { // have not logged in yet
                    return_msg = "221: You have NOT logged in\r\n";
                }
                else // Logged in
                {
                    return_msg = "130: Logout successfully\r\n";
                    is_logged_in = 0;
                    // Clear current user info
                    memset(current_username, 0, sizeof(current_username));
                    memset(current_root_dir, 0, sizeof(current_root_dir));
                }
                send_all(conn_sock, return_msg, strlen(return_msg));
                const char *response_log = return_msg;
                log_message(client_addr_str, request_log, response_log);
            }
            else if (strcmp(command, "GETDIR") == 0)
            { // GETDIR command
                get_current_directory(conn_sock);
                const char *response_log = "Get current directory";
                log_message(client_addr_str, request_log, response_log);
            }
            else if (strcmp(command, "LIST") == 0)
            { // LIST command
                list_files(conn_sock);
                const char *response_log = "List files";
                log_message(client_addr_str, request_log, response_log);
            }
            else
            { // Unkwon command
                char *return_msg = "300: Unknown command\r\n";
                send_all(conn_sock, return_msg, strlen(return_msg));
                const char *response_log = "300: Unknown command";
                log_message(client_addr_str, request_log, response_log);
            }
        }
    }
}


int welcome_to_server(int conn_sock, struct sockaddr_in client_addr, char *client_addr_str_out, int str_len)
{
    char client_ip[INET_ADDRSTRLEN];
    int client_port;
    // đổi địa chỉ ip sang xâu nhị phân
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    //ntohs: chuyển đổi số cổng từ định dạng mạng sang định dạng máy chủ
    client_port = ntohs(client_addr.sin_port);
    snprintf(client_addr_str_out, str_len, "%s:%d", client_ip, client_port);

    printf("You got connection from client: %s\n", client_addr_str_out);

    // Step 1: Send welcome message
    const char *welcome_msg = "+OK Welcome to file server";
    if (send(conn_sock, welcome_msg, strlen(welcome_msg), 0) < 0)
    {
        perror("send() error");
        return 0; //Fail
    }

    log_message(client_addr_str_out, "CONNECT", "+OK Welcome to file server");

    return 1; //Success
}
