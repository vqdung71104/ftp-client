#ifndef LOGGING_H
#define LOGGING_H

#define LOG_FILE "log_20225818.txt"

void log_message(const char *client_addr_str, const char *request, const char *response);

#endif