#ifndef NETUTILS_H
#define NETUTILS_H

#define MAX_CONN_BACKLOG 10     /* max number of open client connections waiting to get processed */
#define LISTEN_BACKLOG 10       /* max number of waiting clients */

#include "DS-Lab-Assignment/util.h"

/*** Sending functions ***/
int send_server_reply(int socket, const reply_t *reply);
int send_string(int socket, const char *string);

/*** Receiving functions ***/
int recv_string(int socket, char *string);

#endif //NETUTILS_H
