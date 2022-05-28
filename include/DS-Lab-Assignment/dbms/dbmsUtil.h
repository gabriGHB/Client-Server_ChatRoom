#ifndef DBMS_UTILS_H
#define DBMS_UTILS_H

#include <dirent.h>
#include "DS-Lab-Assignment/util.h"

/*** Functions called internally in dbms module ***/
int open_file(const char *path, char mode);
int open_directory(const char *path, char mode, DIR **directory);
int remove_recursive(const char *path);
int read_entry(int entry_fd, entry_t *entry);
int write_entry(int entry_fd, entry_t *entry);

#endif //DBMS_UTILS_H
