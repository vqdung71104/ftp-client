#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "logging.h"

/**
 * @brief function for logging message
 * @param client_addr_str [IN] Client address
 * 
 * @param request [IN] Recieved request from client
 * 
 * @param response [IN] Response from server
 */
void log_message(const char *client_addr_str, const char *request, const char *response) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp == NULL) {
        perror("Không thể mở file log");
        return;
    }
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%d/%m/%Y %H:%M:%S", t);
    fprintf(fp, "[%s]$%s$%s$%s\n", time_buf, client_addr_str, request, response);
    fclose(fp);
}