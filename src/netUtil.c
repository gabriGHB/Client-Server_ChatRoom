#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "DS-Lab-Assignment/util.h"
#include "DS-Lab-Assignment/netUtil.h"


/*** Sending functions ***/
int send_server_reply(const int socket, const reply_t *reply) {
    /*** Sends server_error_code member to socket ***/
    int ret_val;    /* needed for error-checking macros */
    CHECK_SOCK_ERROR(write_bytes(socket, (const char *) &reply->server_error_code, 1), socket)
    return 0;
}


int send_string(const int socket, const char *string) {
    /*** Sends a string to socket ***/
    int ret_val;    /* needed for error-checking macros */
    CHECK_SOCK_ERROR(write_bytes(socket, string, (int) (strlen(string) + 1)), socket)
    return 0;
}


/*** Receiving functions ***/
int recv_string(const int socket, char *string) {
    /*** Receives a string from socket ***/
    int ret_val;    /* needed for error-checking macros */
    CHECK_SOCK_ERROR(read_line(socket, string, MAX_MSG_SIZE), socket)
    return 0;
}
