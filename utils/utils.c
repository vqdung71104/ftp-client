#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <sys/socket.h>

/**
 * @brief Remove leading and trailing whitespace characters from a string.
 *
 * This function trims whitespace (spaces, tabs, newlines, etc.) from both the
 * beginning and the end of a given string. It directly modifies the input string
 * in place by:
 *  - Skipping over leading whitespace characters.
 *  - Replacing trailing whitespace characters with null terminators ('\0').
 *
 * @details
 *  - The function uses `isspace()` to identify whitespace characters.
 *  - It first advances the string pointer past any leading whitespace.
 *  - Then, it iterates backward from the end of the string, replacing trailing
 *    whitespace with null terminators until a non-whitespace character is reached.
 *  - The function does **not** allocate new memory — it modifies the original string.
 *
 * @warning
 *  - If you pass a string literal (which resides in read-only memory), this function
 *    will cause undefined behavior since it attempts to modify the string.
 *  - This implementation does **not** shift the trimmed string to the start of the
 *    buffer. If you need the result to start at `str[0]`, you should manually shift
 *    it after trimming.
 *
 * @param[in,out] str Pointer to a null-terminated string to be trimmed.
 *                    The string is modified in place.
 *
 * @return void
 *
 * @see isspace(), strlen()
 */
void trim(char *str)
{
    int i;
    while (isspace((unsigned char)*str))
        str++;
    for (i = strlen(str) - 1; i >= 0 && isspace((unsigned char)str[i]); i--)
        str[i] = '\0';
}

char *strdup(const char *s)
{
    char *dst = malloc(strlen(s) + 1); // Space for length plus nul
    if (dst == NULL)
        return NULL; // No memory
    strcpy(dst, s);  // Copy the characters
    return dst;      // Return the new string
}

long get_file_size(FILE *fp)
{
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    return size;
}

int recv_all(int sockfd, char *buf, int len)
{
    int total_bytes_received = 0;
    int bytes_left = len;
    int n; // Received bytes in one recv() call

    while (total_bytes_received < len)
    {
        n = recv(sockfd, buf + total_bytes_received, bytes_left, 0);

        if (n == -1)
        {
            perror("recv() error");
            return -1;
        }
        else if (n == 0)
        {
            fprintf(stderr, "Socket %d closed connection prematurely\n", sockfd);
            return 0;
        }

        total_bytes_received += n;
        bytes_left -= n;
    }

    return total_bytes_received;
}

int send_all(int sockfd, const char *buf, int len)
{
    int total_bytes_sent = 0;
    int bytes_left = len;
    int n;
    while (total_bytes_sent < len)
    {
        n = send(sockfd, buf + total_bytes_sent, bytes_left, 0);

        if (n == -1)
        {
            perror("send() error in send_all");
            return -1;
        }

        total_bytes_sent += n;
        bytes_left -= n;
    }

    return total_bytes_sent;
}

int recv_line(int sockfd, char *buf, int max_len)
{
    int i = 0;
    char c;
    int n;

    while (1)
    {
        // Step 1: read a single char at a time
        n = recv(sockfd, &c, 1, 0);

        if (n == -1)
        {
            perror("recv() error");
            return -1;
        }
        else if (n == 0)
        {
            printf("Connection closed by client \n");
            return 0;
        }

        // if (i == max_len - 1)
        // {
        //     fprintf(stderr, "Buffer overflow: line too long.\n");
        //     buf[0] = '\0';
        //     return -2;
        // }

        // Step 2: Save read char into buffer
        buf[i] = c;

        // Step 3: Check terminated logic
        if (i > 0 && buf[i] == '\n' && buf[i - 1] == '\r') //TODO: fix stream test
        {
            // Found the delimiter, terminated the msg
            buf[i - 1] = '\0';

            //return the length of cmd
            return i - 1;
        }

        // Step 4: updating index
        i++;

    }
    //safeguard: if the function works properly, this will never be returned
    return -1;
}