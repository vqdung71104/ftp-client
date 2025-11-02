#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
void trim(char *str);

/**
 * @brief Duplicate a string
 *
 * @param s A string for duplicating
 *
 * @details
 * 1. It tries to allocate enough memory to hold the old string (plus a '\0' character to mark the end of the string).
 * 2. If the allocation failed, it sets errno to ENOMEM and returns NULL immediately.
 * 3. Otherwise the allocation worked, copy the old string to the new string.
 */
char *strdup(const char *s);

/**
 * Hàm lấy kích thước file
 * @param fp Con trỏ file đã mở
 * @return Kích thước file (bytes)
 */
long get_file_size(FILE *fp);

/**
 * @brief Helper function for handling bytes stream problem using method 3: Prefetching length
 *
 * @param sockfd socket descriptor
 * @param buf buffer for storing received msg
 * @param len bytes that have to be received
 *
 * @return
 * 'len': if sucess
 *
 * -1: if there was socket error
 *
 * 0: if the connection was closed before received 'len' bytes
 */
int recv_all(int sockfd, char *buf, int len);

/**
 * @brief Send exactly 'len' bytes from 'buf' to socket through 'sockfd'
 *
 * @param sockfd socket descriptor
 * @param buf buffer for sending data
 * @param len bytes that have to be sent
 *
 * @return
 * 'len': if success
 *
 * -1: if there was an error.
 */
int send_all(int sockfd, const char *buf, int len);

/**
 * @brief Read a whole line (with ending '\r\\n') from socket
 *
 * @return
 *
 * number of read bytes (does NOT inclue '\r\\n') if success

 *
 * 0: if the connection was closed
 *
 *-1: if there was socket error
 * 
 * -2: if the command was too long and could lead to buffer overflow
 */
int recv_line(int sockfd, char *buf, int max_len);
#endif