#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE

#include "ftp_server.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Helper to normalize paths (remove ../ and ./)
static void normalize_path(char* path) {
    char temp[1024];
    char* parts[256];
    int count = 0;
    
    // Split by '/'
    char* token = strtok(path, "/");
    while (token != NULL && count < 256) {
        if (strcmp(token, "..") == 0) {
            if (count > 0) count--; // Go up one level
        } else if (strcmp(token, ".") != 0 && strlen(token) > 0) {
            parts[count++] = token;
        }
        token = strtok(NULL, "/");
    }
    
    // Rebuild path
    if (count == 0) {
        strcpy(path, "/");
    } else {
        temp[0] = '\0';
        for (int i = 0; i < count; i++) {
            strcat(temp, "/");
            strcat(temp, parts[i]);
        }
        strcpy(path, temp);
    }
}

// Get absolute path from root, current dir, and requested path
void get_absolute_path(const char* root, const char* current, const char* requested, char* result) {
    char temp[1024];
    
    if (requested == NULL || strlen(requested) == 0) {
        requested = ".";
    }
    
    if (requested[0] == '/') {
        // Absolute path from FTP root
        snprintf(temp, sizeof(temp), "%s", requested);
    } else {
        // Relative path
        if (strcmp(current, "/") == 0) {
            snprintf(temp, sizeof(temp), "/%s", requested);
        } else {
            snprintf(temp, sizeof(temp), "%s/%s", current, requested);
        }
    }
    
    // Normalize the path
    normalize_path(temp);
    
    // Combine with root
    snprintf(result, 1024, "%s%s", root, temp);
}

// Check if path is within allowed root directory
int is_path_allowed(const char* root, const char* path) {
    char real_root[PATH_MAX];
    char real_path[PATH_MAX];
    
    // Get real paths (resolves symlinks, .., etc.)
    if (realpath(root, real_root) == NULL) {
        return 0;
    }
    
    // Try to resolve the full path first
    if (realpath(path, real_path) != NULL) {
        // Path exists and was resolved
        // Check if real_path starts with real_root
        size_t root_len = strlen(real_root);
        if (strncmp(real_path, real_root, root_len) != 0) {
            return 0;
        }
        
        // Make sure it's exactly the root or a subdirectory (not just prefix match)
        if (strlen(real_path) > root_len && real_path[root_len] != '/') {
            return 0;
        }
        
        return 1;
    }
    
    // Path doesn't exist, try checking parent directory
    char* path_copy = strdup(path);
    if (!path_copy) {
        return 0;
    }
    
    char* last_slash = strrchr(path_copy, '/');
    
    if (last_slash != NULL && last_slash != path_copy) {
        // File doesn't exist, check parent directory
        *last_slash = '\0';
        if (realpath(path_copy, real_path) == NULL) {
            free(path_copy);
            return 0;
        }
        free(path_copy);
        
        // Check if parent is within root
        size_t root_len = strlen(real_root);
        if (strncmp(real_path, real_root, root_len) != 0) {
            return 0;
        }
        
        if (strlen(real_path) > root_len && real_path[root_len] != '/') {
            return 0;
        }
        
        return 1;
    }
    
    free(path_copy);
    return 0;
}
