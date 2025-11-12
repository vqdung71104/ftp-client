#ifndef SERVICE_H
#define SERVICE_H


/**
 * @brief Function for posting message
 */
void post_message();

/**
 * @brief Function for logging out
 */
void logout();

/**
 * @brief Function for handling login request
 */
/**
 * @brief Function for handling login request (username validation)
 * 
 * @param command_value username to validate
 * @param client_socket socket descriptor
 */
void login(char *command_value, int client_socket);

/**
 * @brief Function for handling password verification
 * 
 * @param password password to verify
 * @param client_socket socket descriptor
 */
void verify_password(char *password, int client_socket);

/**
 * @brief Validate a given username against the loaded account list.
 *
 * This function checks whether the provided username exists in the
 * global `users` array and returns a code indicating its validity
 * and status.
 *
 * @details
 *  - Iterate through all loaded accounts (0 → count-1).
 *  - Compare each stored username with the given input.
 *      - If a match is found:
 *          - If account status == 0 → return -1 (account is banned).
 *          - If account status == 1 → set valid_user = 1 and stop search.
 *  - If no match is found → valid_user remains 0.
 *
 * @param username The username string to validate.
 *
 * @note Uses global variables:
 *
 *  - `users` (array of Account)
 *
 *  - `count` (number of loaded accounts)
 *
 * @return int
 *
 *  -  1 : Valid user (exists and active)
 *
 *  -  0 : User does not exist
 *
 *  - -1 : User exists but is banned
 *
 */
int user_validation(char *username);

/**
 * @brief Function for handling posting article 
 * 
 * @param client_socket socket descriptor for client socket
 * 
 * @param command_value msg for posting article
 */
void post_article(int client_socket, char *command_value);

/**
 * @brief Function for handling file upload
 * 
 * @param conn_sock client socket descriptor
 * @param client_addr_str client address string
 * @param command_value command value containing filename and filesize
 * @param request_log request log string
 */
void handle_upload_file(int conn_sock, const char *client_addr_str, char *command_value, const char *request_log);

/**
 * @brief Get current working directory for logged in user
 * 
 * @param client_socket socket descriptor
 */
void get_current_directory(int client_socket);

/**
 * @brief Function for changing working directory
 * 
 * @param new_dir new directory path
 * @param client_socket socket descriptor
 */
void change_directory(char *new_dir, int client_socket);

/**
 * @brief Function for listing files in current working directory
 * 
 * @param client_socket socket descriptor
 */
void list_files(int client_socket);

#endif