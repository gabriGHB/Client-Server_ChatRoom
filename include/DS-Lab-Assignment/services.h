#ifndef SERVICES_H
#define SERVICES_H

#include "DS-Lab-Assignment/util.h"

/****** Services ******/

/*** Services Called By Client, Served By Server ***/
void srv_register(int socket, request_t *request);
void srv_unregister(int socket, request_t *request);
void srv_connect(int socket, request_t *request);
void srv_disconnect(int socket, request_t *request);
void srv_send(int socket, request_t *request);

#endif //SERVICES_H
