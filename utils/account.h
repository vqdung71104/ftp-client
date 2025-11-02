
#ifndef ACCOUNT_H
#define ACCOUNT_H

/**
 * @struct Account
 * @brief  A structure for storing account information.
 *
 * @param username A character array storing the username of the account.
 *                 Maximum length is 1000 characters (including null terminator).
 * @param password A character array storing the password of the account.
 *                Maximum length is 1000 characters (including null terminator).
 * @param root_dir A character array storing the root directory associated with the account.
 *                Maximum length is 1000 characters (including null terminator).
 *
 * @param status   An integer representing the account status.
 *
 *                 - 0: Account is banned.
 *
 *                 - 1: Account is active.
 */
typedef struct
{
    char username[1000];
    char password[1000];
    char root_dir[1000];
    int status; // 0 is banned, 1 is active
} Account;

extern Account *users;

extern int count;
extern int capacity;
extern int is_logged_in; // 0 is not logged in, 1 is logged in
extern char current_username[1000]; // Current logged in username
extern char current_root_dir[1000]; // Current logged in user's root directory
extern char pending_username[1000]; // Username waiting for password verification

/**
 * @brief Read account data from "account.txt" into memory.
 *
 * This function opens the file "account.txt" in read mode and loads each line
 * into a dynamically allocated array of `Account`. Each line in the file must
 * have the format:
 *
 *     <username> <status>
 *
 * @param username: a string (≤ 100 characters, no whitespace allowed).
 * 
 * @param password: a string (≤ 100 characters, no whitespace allowed).
 *
 * @param root_dir: a string (≤ 100 characters, no whitespace allowed).
 *
 * @param status  : must be either 0 or 1.
 *
 * @details
 *  - If the file cannot be opened:
 *      - Print an error message with perror().
 *      - Return 0 immediately.
 *  - While reading lines with fgets():
 *      - If the number of accounts reaches 1,000,000:
 *          - Print error message and stop.
 *      - If `count == capacity`:
 *          - Expand capacity (start with 10, then double).
 *          - Reallocate memory for `users` array.
 *      - Parse each line with sscanf("%s %d", ...).
 *          - If parsing fails, print an error and stop.
 *      - Validate input:
 *          - `username` must not exceed 100 characters.
 *          - `status` must be 0 or 1.
 *      - If valid, store data in `users[count]` and increment `count`.
 *  - Stop reading when EOF is reached.
 *  - Close the file before returning.
 *
 * @note
 *  - Global variables used: `users`, `count`, `capacity`.
 * 
 *  - Each `Account` contains two fields: `username` and `status`.
 *
 * @return
 *  - 1: success, file read completely and data loaded into memory.
 * 
 *  - 0: error occurred (invalid file, invalid input, memory error, etc.).
 */
int read_file_data();
#endif