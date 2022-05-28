#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "DS-Lab-Assignment/util.h"


/**** Number Casting Stuff ****/

int str_to_num(const char *const string, void *number, const char type) {
    /*** Casts a given string value_str to a number;
     * writes cast value to value pointer; this value pointer
     * should be a pointer to the desired number type.
     * Currently supported types: int, float ***/
    errno = 0;
    char *endptr;                   /* used for castings */
    char error[MAX_STR_SIZE];       /* used to display errors */
    void *cast_number;              /* this points to cast value */
    size_t result_size;             /* used to determine the size of the cast value in memcpy() call */

    /* auxiliary variables to cast value */
    int str_to_int;
    unsigned int str_to_uint;
    float str_to_float;

    /* check arguments */
    CHECK_ARGS(type != INT && type != FLOAT && type != UINT, "Invalid Casting Type")
    CHECK_ARGS(!strlen(string), "Empty String")

    /* cast value */
        if (type == INT) {
            str_to_int = (int) strtol(string, &endptr, 10);
            cast_number = (void *) &str_to_int;
            strcpy(error, "strtol");
            result_size = sizeof(int);
        } else if (type == UINT) {
            str_to_uint = (unsigned int) strtol(string, &endptr, 10);
            cast_number = (void *) &str_to_uint;
            strcpy(error, "strtol");
            result_size = sizeof(unsigned int);
        } else {    /* type == FLOAT */
            str_to_float = strtof(string, &endptr);
            cast_number = (void *) &str_to_float;
            strcpy(error, "strtof");
            result_size = sizeof(float);
        }

    /* check casting errors */
    if (errno) {
        perror(error);
        return GEN_ERR_ANY;
    }
    if (endptr == string) {
        fprintf(stderr, "No digits were found\n");
        return GEN_ERR_ANY;
    }

    /* cast succeeded: set value */
    memcpy(number, cast_number, result_size);
    return 0;
}


/**** File/Socket Descriptor I/O Functions ****/

int write_bytes(const int d, const char *buffer, const int len) {
    /*** Writes len bytes to d (socket/file descriptor) ***/
    ssize_t bytes_written;          /* number of bytes written by last write() */
    ssize_t bytes_left = len;       /* number of bytes left to be received */
    int bytes_written_total = 0;    /* total bytes written so far */

    CHECK_ARGS(len <= 0 || buffer == NULL, "Invalid Buffer or Length")

    do {
        bytes_written = write(d, buffer, bytes_left);
        bytes_left -= bytes_written;
        bytes_written_total += (int) bytes_written;
        buffer += bytes_written;
    } while ((bytes_left > 0) && (bytes_written >= 0));

    if (bytes_written < 0) return GEN_ERR_ANY;  /* write() error */
    return bytes_written_total;
}


int read_bytes(const int d, char *buffer, const int len) {
    /*** Reads len bytes from d (socket/file descriptor) ***/
    ssize_t bytes_read;             /* number of bytes fetched by last read() */
    ssize_t bytes_left = len;       /* number of bytes left to be received */
    int bytes_read_total = 0;       /* total bytes read so far */

    CHECK_ARGS(len <= 0 || buffer == NULL, "Invalid Buffer or Length")

    do {
        bytes_read = read(d, buffer, bytes_left);
        bytes_left -= bytes_read;
        bytes_read_total += (int) bytes_read;
        buffer += bytes_read;
    } while ((bytes_left > 0) && (bytes_read > 0));

    if (bytes_read < 0) return GEN_ERR_ANY;  /* read() error */
    return bytes_read_total;
}


int read_line(const int d, char *buffer, const int buf_space) {
    /*** Reads a 1-line string of at most (buf_space -1) chars, excluding terminating byte,
     * from d (socket/file descriptor); stops reading when a '\0' or '\n' is found ***/
    errno = 0;
    ssize_t bytes_read;             /* number of bytes fetched by last read() */
    int bytes_read_total = 0;       /* total bytes read so far */
    char ch;

    CHECK_ARGS(buf_space <= 0 || buffer == NULL, "Invalid Buffer or Buffer Space")

    /* read from fd */
    while (TRUE) {
        bytes_read = read(d, &ch, 1);	/* read a byte */

        /* check what's been read */
        if (bytes_read == -1) {
            if (errno == EINTR)	continue;       /* interrupted -> restart read() */
            /* some other error */
            return GEN_ERR_ANY;
        } else if (!bytes_read) {           /* EOF */
            if (!bytes_read_total) {        /* no bytes read */
                return 0;
            }
            else break;
        }
        /* bytes_read must be 1 if we get here */
        if (ch == '\n' || ch == '\0') break;        /* break line or string end found, so line ends */
        if (bytes_read_total < buf_space - 1) {	    /* discard > (n-1) bytes */
            bytes_read_total++;
            *buffer++ = ch;
        }
    } // end while

    *buffer = '\0';
    return bytes_read_total;
}
