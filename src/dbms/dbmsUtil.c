#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "DS-Lab-Assignment/dbms/dbmsUtil.h"


int open_file(const char *const path, const char mode) {
    /*** Open given path file with given mode ***/
    int fd;

    switch (mode) {
        case READ: fd = open(path, O_RDONLY); break;
        case CREATE: fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600); break;
        case MODIFY: fd = open(path, O_WRONLY | O_TRUNC); break;
        default:
            fprintf(stderr, "Invalid open mode");
            return GEN_ERR_INV_ARGS;
    }

    return fd;
}


int open_directory(const char *const path, const char mode, DIR **directory) {
    /*** Open given path as a directory with given mode and map it to given DIR **directory ***/
    errno = 0;
    int ret_val;    /* needed for error-checking macros */
    char error[MAX_STR_SIZE];   /* message displayed in perror */

    CHECK_ARGS(mode != CREATE && mode != READ && mode != OVERWRITE, "Invalid Open Mode")

    /* try to open directory */
    *directory = opendir(path);

    if (!*directory) {
        if (errno == ENOENT) {      /* directory doesn't exist */
            if (mode == CREATE || mode == OVERWRITE) {   /* so create it */
                CHECK_FUNC_ERROR_WITH_ERRNO(mkdir(path, S_IRWXU), DBMS_ERR_ANY)
                /* and try to open it */
                *directory = opendir(path);
                CHECK_ERROR_WITH_ERRNO(!*directory, "Could not open directory", DBMS_ERR_ANY)
                return DBMS_SUCCESS;
            } else return DBMS_ERR_NOT_EXISTS; /* mode == READ */
        } else {    /* some other opendir() error */
            sprintf(error, "Could not open %s directory", path); perror(error);
            return DBMS_ERR_ANY;
        }
    }

    /* directory exists, depending on given mode, this is success or error */
    return ((mode == CREATE) ? DBMS_ERR_EXISTS : DBMS_SUCCESS);
}


/* adapted from:
 * https://codereview.stackexchange.com/questions/263536/c-delete-files-and-directories-recursively-if-directory */
int remove_recursive(const char *const path) {
    /*** Deletes the given path, whether it be a file or a directory;
     * it deletes inner contents in case it's a directory ***/
    errno = 0;
    int ret_val;    /* needed for error-checking macros */
    char error[MAX_STR_SIZE];   /* message displayed in perror */

    /* first try to open it as a directory */
    DIR *const directory = opendir(path);

    if (directory) {
        /* at this point, we know it is a directory */
        struct dirent *entry;
        /* go through directory contents */
        while ((entry = readdir(directory))) {
            if (!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name)) continue;

            /* set up the path to the current directory entry */
            char entry_path[strlen(path) + strlen(entry->d_name) + 2];
            sprintf(entry_path, "%s/%s", path, entry->d_name);

            /* set up a function pointer to call the appropriate function:
             * remove if the current entry is a file, or a recursive call if it is a directory */
            int (*const rm)(const char *) = entry->d_type == DT_DIR ? remove_recursive : remove;
            CHECK_RM_ERROR(rm(entry_path), DBMS_ERR_ANY)
        }
        closedir(directory);
    }
    /* error opening path as a directory */

    /* check whether it exists */
    if (errno == ENOENT) {
        sprintf(error, "%s does not exist", path); perror(error);
        return DBMS_ERR_NOT_EXISTS;
    }

    /* it's not a directory, but it exists, so it must be just a file */
    return remove(path);
}


int read_entry(const int entry_fd, entry_t *entry) {
    /*** Reads an entry from a given open entry fd ***/
    ssize_t bytes_read;     /* used for error handling of read_bytes call */

    /* read entry */
    bytes_read = read_bytes(entry_fd, (char *) entry, sizeof(entry_t));
    if (bytes_read == -1) {
        perror("Error reading entry");
        close(entry_fd);
        return DBMS_ERR_ANY;
    } else if (!bytes_read) {
        fprintf(stderr, "No bytes were read\n");
        close(entry_fd);
        return DBMS_ERR_ANY;
    }

    return DBMS_SUCCESS;
}


int write_entry(const int entry_fd, entry_t *entry) {
    /*** Writes an entry to a given open entry fd ***/
    ssize_t bytes_written;     /* used for error handling of write_bytes call */

    /* write entry */
    bytes_written = write_bytes(entry_fd, (char *) entry, sizeof(entry_t));
    if (bytes_written == -1) {
        perror("Error writing entry");
        close(entry_fd);
        return DBMS_ERR_ANY;
    }

    return DBMS_SUCCESS;
}
