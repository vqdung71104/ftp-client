#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "account.h"

int count = 0;
int capacity = 10;
int is_logged_in = 0;
char current_username[1000] = "";
char current_root_dir[1000] = "";
char pending_username[1000] = "";

Account *users = NULL;

char line[256]; //Buffer for a line

int read_file_data()
{
    printf("Begin reading file\n");
    FILE *fp = fopen("account.txt", "r");

    if (!fp)
    {
        perror("Can NOT open file");
        return 0;
    }

    users = realloc(users, capacity * sizeof(Account));
    while (fgets(line, sizeof(line), fp) != NULL) // đọc từng dòng đến EOF
    {
        if (count >= 1000000)
        {
            printf("Can NOT open file with more than 1.000.000 lines\n");
            fclose(fp);
            return 0;
        }

        if (count == capacity)
        {
            capacity = capacity * 2;
            users = realloc(users, capacity * sizeof(Account));
            if (users == NULL)
            {
                perror("Failed to realloc memory");
                fclose(fp);
                return 0;
            }
        }

        if (sscanf(line, "%s %s %s %d", users[count].username, users[count].password, users[count].root_dir, &users[count].status) != 4)
        {
            printf("Invalid username, password, root_dir or user's status at line: %d\n", count + 1);
            fclose(fp);
            return 0;
        }

        if (strlen(users[count].username) > 100)
        {
            printf("User name can NOT exceed 100 characters (at line %d)\n", count + 1);
            fclose(fp);
            return 0;
        }
        
        if( strlen(users[count].password) > 100)
        {
            printf("Password can NOT exceed 100 characters (at line %d)\n", count + 1);
            fclose(fp);
            return 0;
        }

        if( strlen(users[count].root_dir) > 100)
        {
            printf("Root directory can NOT exceed 100 characters (at line %d)\n", count + 1);
            fclose(fp);
            return 0;
        }

        if (users[count].status != 0 && users[count].status != 1)
        {
            printf("Invalid user's status at line: %d\n", count + 1);
            fclose(fp);
            return 0;
        }

        count++;
    }

    fclose(fp);
    printf("Done reading file\n");
    printf("Total line read: %d\n",count);
    return 1;
}