#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "DS-Lab-Assignment/netUtil.h"
#include "DS-Lab-Assignment/dbms/dbms.h"
#include "DS-Lab-Assignment/services.h"


/***** Auxiliary functions *****/
void aux_send_init(const request_t *request, reply_t *reply, entry_t *entry);
void aux_send_msg_pass(const request_t *request, reply_t *reply, entry_t *recipient_entry, entry_t *msg_entry);
void aux_send_first_ack(int socket, reply_t *reply, unsigned int msg_id);
int aux_connect_send_pend_msgs(entry_t *pend_msg_entry);
int aux_connect_clt_listen_thread(entry_t *entry);

/***** Services Called By Server, Served By Client Listening Thread *****/
int clt_send_message(request_t *request, entry_t *entry);
int clt_send_mess_ack(request_t *request, entry_t *entry);


void aux_send_init(const request_t *const request, reply_t *reply, entry_t *entry) {
    /*** Checks that both users exist updates the recipient's last message ID,
     * and sets up server reply (first ACK);
     * called in srv_send function ***/
    /* check that both users exist */
    int sender_exists = db_user_exists(request->message.sender);
    int recipient_exists = db_user_exists(request->recipient);

    if (!sender_exists || !recipient_exists)
        reply->server_error_code = SRV_ERR_SEND_USR_NOT_EXISTS;
    else if (sender_exists < 0 || recipient_exists < 0)
        reply->server_error_code = SRV_ERR_SEND_ANY;
    else {    /* set up first ACK to be sent to sender client */
        reply->server_error_code = SRV_SUCCESS;

        /* update last msg ID in recipient user */
        entry->type = ENT_TYPE_UD;
        strcpy(entry->username, request->recipient);
        /* read recipient user entry */
        if (db_io_op_usr_ent(entry, READ) < 0)
            reply->server_error_code = SRV_ERR_SEND_ANY;
        else {
            /* update last msg ID & write it to DB */
            entry->user.last_msg_id = (entry->user.last_msg_id + 1) % MSG_ID_MAX_VALUE;
            if (db_io_op_usr_ent(entry, MODIFY) < 0)
                reply->server_error_code = SRV_ERR_SEND_ANY;
        }
    }
}


void aux_send_msg_pass(const request_t *const request, reply_t *reply, entry_t *recipient_entry, entry_t *msg_entry) {
    /*** Sends a given message to a given recipient;
     * called in srv_send function ***/
    /* set up message entry */
    msg_entry->type = ENT_TYPE_P_MSG;
    strcpy(msg_entry->username, request->recipient);
    strcpy(msg_entry->msg.sender, request->message.sender);
    strcpy(msg_entry->msg.content, request->message.content);
    msg_entry->msg.id = recipient_entry->user.last_msg_id;

    /* check whether recipient user is connected */
    if (recipient_entry->user.status == STATUS_CN) {
        /* set up a request for SEND_MESSAGE service */
        request_t send_msg_request;
        strcpy(send_msg_request.op_code, SEND_MESSAGE);
        strcpy(send_msg_request.message.sender, request->message.sender);
        send_msg_request.message.id = recipient_entry->user.last_msg_id;
        strcpy(send_msg_request.message.content, request->message.content);

        /* send message to recipient user */
        int send_result = clt_send_message(&send_msg_request, recipient_entry);

        /* server log message if SEND MESSAGE succeeds */
        if (send_result == SRV_SUCCESS) {
            printf("s> SEND MESSAGE %u FROM %s TO %s\n", msg_entry->msg.id,
                   request->message.sender, request->recipient);
            fflush(stdout);
        } else {
            /* if sending fails, change recipient user's status to disconnected */
            recipient_entry->user.status = STATUS_DCN;

            /* update recipient user entry in DB */
            if (db_io_op_usr_ent(recipient_entry, MODIFY) < 0)
                reply->server_error_code = SRV_ERR_SEND_ANY;
            /* at this point, we must try to store the message as pending */
        }
    }
}


void aux_send_first_ack(const int socket, reply_t *reply, const unsigned int msg_id) {
    /*** Sends reply to sender client (first ACK and msg ID if success, error otherwise);
     * called in srv_send function ***/
    if (send_server_reply(socket, reply) < 0) return;

    /* send msg ID if send service was successful */
    if (reply->server_error_code == SRV_SUCCESS) {
        char msg_id_str[16];
        sprintf(msg_id_str, "%u", msg_id);
        send_string(socket, msg_id_str);
    }
}


int aux_connect_send_pend_msgs(entry_t *pend_msg_entry) {
    /*** Reads pending messages of a user (given by username member in given entry),
     * sends them out and deletes them from the list;
     * called in srv_connect function ***/

    /* set up send message request */
    request_t send_msg_request;
    strcpy(send_msg_request.op_code, SEND_MESSAGE);
    /* set up a request for SEND_MESS_ACK service */
    request_t send_msg_ack_request;
    strcpy(send_msg_ack_request.op_code, SEND_MESS_ACK);

    /* set up sender user entry */
    entry_t sender_entry;
    sender_entry.type = ENT_TYPE_UD;
    /* set up recipient user entry */
    entry_t recipient_entry;
    recipient_entry.type = ENT_TYPE_UD;
    strcpy(recipient_entry.username, pend_msg_entry->username);
    if (db_io_op_usr_ent(&recipient_entry, READ) < 0) return DBMS_ERR_ANY;

    while (db_get_pend_msg(pend_msg_entry) == DBMS_SUCCESS) {
        /* set up rest of send message request */
        strcpy(send_msg_request.message.sender, pend_msg_entry->msg.sender);
        send_msg_request.message.id = pend_msg_entry->msg.id;
        strcpy(send_msg_request.message.content, pend_msg_entry->msg.content);

        /* set up rest of send message request */
        send_msg_ack_request.message.id = pend_msg_entry->msg.id;

        /* set up rest of sender user entry */
        strcpy(sender_entry.username, pend_msg_entry->msg.sender);

        /* send message */
        /* server log message if SEND MESSAGE succeeds */
        if (clt_send_message(&send_msg_request, &recipient_entry) == SRV_SUCCESS) {
            printf("s> SEND MESSAGE %u FROM %s TO %s\n", send_msg_request.message.id,
                   send_msg_request.message.sender, send_msg_request.recipient);
            fflush(stdout);

            /* pending message has been sent successfully, so delete it from the list */
            db_io_op_usr_ent(pend_msg_entry, DELETE);

            /* send second ACK to sender listening thread */
            clt_send_mess_ack(&send_msg_ack_request, &sender_entry);
        } else {
            /* if sending fails, change recipient user's status to disconnected */
            recipient_entry.user.status = STATUS_DCN;

            /* update recipient user entry in DB */
            db_io_op_usr_ent(&recipient_entry, MODIFY);
            return SRV_ERR_SEND_ANY;
        }
    } // END while

    return SRV_SUCCESS;
}


int aux_connect_clt_listen_thread(entry_t *entry) {
    /*** Connects to client listening thread of a given user;
     * called in client-side services (clt_send_message and clt_send_mess_ack functions) ***/
    struct sockaddr_in clt_listen_addr;
    int clt_listen_socket;
    int ret_val;    /* needed for error-checking macros */

    /* read IP and port from given user entry */
    if (db_io_op_usr_ent(entry, READ) < 0)
        return DBMS_ERR_ANY;

    /* create client socket */
    CHECK_FUNC_ERROR_WITH_ERRNO(clt_listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), GEN_ERR_ANY)

    /* set up client listening thread address */
    bzero((char*) &clt_listen_addr, sizeof(struct sockaddr_in));
    memcpy (&(clt_listen_addr.sin_addr), &(entry->user.ip), sizeof(struct in_addr));
    clt_listen_addr.sin_family = AF_INET;
    clt_listen_addr.sin_port = htons(entry->user.port);

    /* connect to client listening thread */
    CHECK_FUNC_ERROR_WITH_ERRNO(connect(clt_listen_socket, (struct sockaddr *) &clt_listen_addr,sizeof(clt_listen_addr)), GEN_ERR_ANY)

    return clt_listen_socket;
}


/***** Services *****/

/**** Client-side ****/
int clt_send_message(request_t *request, entry_t *entry) {
    /*** Executes SEND_MESSAGE service:
     * sends a message (contained in given request) to the
     * client's listening thread (user in given entry);
     * called in aux_connect_send_pend_msgs and aux_send_msg_pass functions ***/
    int ret_val;    /* needed for error-checking macros */
    int clt_listen_socket;
    CHECK_FUNC_ERROR(clt_listen_socket = aux_connect_clt_listen_thread(entry), GEN_ERR_ANY)

    /* set up message ID string */
    char msg_id_str[16]; sprintf(msg_id_str, "%u", request->message.id);

    /* send stuff */
    if (send_string(clt_listen_socket, request->op_code) < 0 ||
    send_string(clt_listen_socket, request->message.sender) < 0 ||
    send_string(clt_listen_socket, msg_id_str) < 0 ||
    send_string(clt_listen_socket, request->message.content) < 0)
        return GEN_ERR_ANY;

    /* if all went well */
    close(clt_listen_socket);
    return SRV_SUCCESS;
}


int clt_send_mess_ack(request_t *request, entry_t *entry) {
    /*** Executes SEND_MESS_ACK service:
     * sends second ACK to sender client listening thread
     * (meaning the message has been delivered to the recipient);
     * called in srv_send function ***/
    int ret_val;    /* needed for error-checking macros */
    int clt_listen_socket;
    CHECK_FUNC_ERROR(clt_listen_socket = aux_connect_clt_listen_thread(entry), GEN_ERR_ANY)

    /* set up message ID */
    char msg_id_str[16]; sprintf(msg_id_str, "%u", request->message.id);

    /* send stuff */
    send_string(clt_listen_socket, request->op_code);
    send_string(clt_listen_socket, msg_id_str);

    /* if all went well */
    close(clt_listen_socket);
    return SRV_SUCCESS;
}


/**** Server-side ****/
void srv_register(const int socket, request_t *request) {
    /*** Executes REGISTER service ***/
    reply_t reply;
    entry_t entry;

    /* receive stuff */
    if (recv_string(socket, request->username) < 0) return;

    /* set up user entry */
    strcpy(entry.username, request->username);
    entry.type = ENT_TYPE_UD;
    entry.user.last_msg_id = 0;

    /* create user entry in DB */
    reply.server_error_code = (db_creat_usr_tbl(&entry) == DBMS_ERR_EXISTS) ?
            SRV_ERR_REG_USR_ALREADY_REG : SRV_SUCCESS;

    /* server log message */
    if (reply.server_error_code == SRV_SUCCESS) {
        printf("s> %s %s OK\n", REGISTER, request->username); fflush(stdout);
    } else {
        printf("s> %s %s FAIL\n", REGISTER, request->username); fflush(stdout);
    }

    /* no need to error handle this call: whether it fails or not, the server
     * is just going to move on(continue in the while loop) */
    send_server_reply(socket, &reply);
}


void srv_unregister(const int socket, request_t *request) {
    /*** Executes UNREGISTER service ***/
    reply_t reply;

    /* receive stuff */
    if (recv_string(socket, request->username) < 0) return;

    /* check whether user exists */
    int user_exists = db_user_exists(request->username);
    if (user_exists == TRUE)
        reply.server_error_code = (db_del_usr_tbl(request->username) < 0) ?
                SRV_ERR_UNREG_ANY : SRV_SUCCESS;
    else if (user_exists == FALSE)
        reply.server_error_code = SRV_ERR_UNREG_USR_NOT_EXISTS;
    else reply.server_error_code = SRV_ERR_UNREG_ANY;

    /* server log message */
    if (reply.server_error_code == SRV_SUCCESS) {
        printf("s> %s %s OK\n", UNREGISTER, request->username); fflush(stdout);
    } else {
        printf("s> %s %s FAIL\n", UNREGISTER, request->username); fflush(stdout);
    }

    /* send reply to client */
    send_server_reply(socket, &reply);
}


void srv_connect(const int socket, request_t *request) {
    /*** Executes CONNECT service ***/
    reply_t reply;
    entry_t entry;
    struct sockaddr_in client_addr;

    /* receive stuff */
    if (recv_string(socket, request->username) < 0) return;
    if (recv_string(socket, request->client_port) < 0) return;

    /* set up user entry */
    strcpy(entry.username, request->username);
    entry.type = ENT_TYPE_UD;

    /* read user entry from DB */
    int io_result = db_io_op_usr_ent(&entry, READ);

    if (io_result == DBMS_ERR_NOT_EXISTS)
        reply.server_error_code = SRV_ERR_CN_USR_NOT_EXISTS;
    else if (io_result < 0)
        reply.server_error_code = SRV_ERR_CN_ANY;      /* some other error */
    else {    /* user entry was read successfully */
        if (entry.user.status == STATUS_CN)
            reply.server_error_code = SRV_ERR_CN_USR_ALREADY_CN;
        else {    /* entry.user.status == STATUS_DCN */
            /* prepare entry to write it to DB */
            /* cast the client port to short */
            int tmp_port;
            if (str_to_num(request->client_port, (void *) &tmp_port, INT) < 0)
                reply.server_error_code = SRV_ERR_CN_ANY;

            /* get client IP */
            socklen_t client_addr_size = sizeof client_addr;
            if (getpeername(socket, (struct sockaddr *) &client_addr, &client_addr_size) < 0)
                reply.server_error_code = SRV_ERR_CN_ANY;

            /* set up entry */
            if (reply.server_error_code != SRV_ERR_CN_ANY) {
                entry.user.status = STATUS_CN;
                entry.user.port = (uint16_t) tmp_port;
                entry.user.ip = client_addr.sin_addr;

                /* update user entry in DB */
                io_result = db_io_op_usr_ent(&entry, MODIFY);
                if (io_result == DBMS_ERR_NOT_EXISTS)
                    reply.server_error_code = SRV_ERR_CN_USR_NOT_EXISTS;
                else if (io_result < 0)
                    reply.server_error_code = SRV_ERR_CN_ANY;      /* some other error */
                else reply.server_error_code = SRV_SUCCESS;        /* user entry was correctly updated */
            }
        } //END inner else
    } //END outer else

    /* server log message */
    if (reply.server_error_code == SRV_SUCCESS) {
        printf("s> %s %s OK\n", CONNECT, request->username); fflush(stdout);
    } else {
        printf("s> %s %s FAIL\n", CONNECT, request->username); fflush(stdout);
    }

    /* send reply to client */
    send_server_reply(socket, &reply);

    /* set up pending message entry */
    entry_t pend_msg_entry;
    pend_msg_entry.type = ENT_TYPE_P_MSG;
    strcpy(pend_msg_entry.username, request->username);

    /* send pending messages */
    if (aux_connect_send_pend_msgs(&pend_msg_entry) != SRV_SUCCESS) {
        reply.server_error_code = SRV_ERR_SEND_ANY;
    }
}


void srv_disconnect(const int socket, request_t *request) {
    /*** Executes DISCONNECT service ***/
    reply_t reply;
    entry_t entry;

    /* receive stuff */
    if (recv_string(socket, request->username) < 0) return;

    /* set up user entry */
    strcpy(entry.username, request->username);
    entry.type = ENT_TYPE_UD;

    /* read user entry from DB */
    int io_result = db_io_op_usr_ent(&entry, READ);
    if (io_result == DBMS_ERR_NOT_EXISTS)
        reply.server_error_code = SRV_ERR_DCN_USR_NOT_EXISTS;
    else if (io_result < 0)
        reply.server_error_code = SRV_ERR_DCN_ANY;     /* some other error */
    else {    /* user entry was read successfully */
        if (entry.user.status == STATUS_DCN)
            reply.server_error_code = SRV_ERR_DCN_USR_NOT_CN;
        else {    /* entry.user.status == STATUS_CN */
            /* set up entry */
            entry.user.status = STATUS_DCN;
            bzero(&entry.user.ip, sizeof(struct in_addr));
            bzero(&entry.user.port, sizeof(uint16_t));

            /* update user entry in DB */
            io_result = db_io_op_usr_ent(&entry, MODIFY);
            if (io_result == DBMS_ERR_NOT_EXISTS)
                reply.server_error_code = SRV_ERR_DCN_USR_NOT_EXISTS;
            else if (io_result < 0)
                reply.server_error_code = SRV_ERR_DCN_ANY;     /* some other error */
            else reply.server_error_code = SRV_SUCCESS;        /* user entry was correctly updated */
        } //END inner else
    } //END outer else

    /* server log message */
    if (reply.server_error_code == SRV_SUCCESS) {
        printf("s> %s %s OK\n", DISCONNECT, request->username); fflush(stdout);
    } else {
        printf("s> %s %s FAIL\n", DISCONNECT, request->username); fflush(stdout);
    }

    /* send reply to client */
    send_server_reply(socket, &reply);
}


void srv_send(const int socket, request_t *request) {
    /*** Executes SEND service ***/
    reply_t reply;
    entry_t msg_entry;
    entry_t recipient_entry;
    reply_t send_msg_reply;     /* used for message transmission */

    /* receive stuff */
    if (recv_string(socket, request->message.sender) < 0) return;
    if (recv_string(socket, request->recipient) < 0) return;
    if (recv_string(socket, request->message.content) < 0) return;

    aux_send_init(request, &reply, &recipient_entry);

    /* if previous steps have failed, just send error code to client */
    if (reply.server_error_code != SRV_SUCCESS) {
        send_server_reply(socket, &reply);
        return;
    }

    /* message passing */
    aux_send_msg_pass(request, &reply, &recipient_entry, &msg_entry);

    /* if recipient user is disconnected or message transmission has failed */
    if (recipient_entry.user.status == STATUS_DCN || send_msg_reply.server_error_code != SRV_SUCCESS) {
        /* store message in recipient user's pending message list */
        if (db_io_op_usr_ent(&msg_entry, CREATE) < 0)
            reply.server_error_code = SRV_ERR_SEND_ANY;

        /* server log message */
        if (reply.server_error_code == SRV_SUCCESS) {
            printf("s> MESSAGE %u FROM %s TO %s STORED\n", msg_entry.msg.id,
                   request->message.sender, request->recipient);
            fflush(stdout);
        }
    }

    /* send reply to sender client (first ACK and msg ID if success, error otherwise) */
    aux_send_first_ack(socket, &reply, msg_entry.msg.id);

    /* check that recipient user is still connected (message transmission hasn't failed) */
    if (recipient_entry.user.status == STATUS_CN) {
        /* set up a request for SEND_MESS_ACK service */
        request_t send_msg_ack_request;
        strcpy(send_msg_ack_request.op_code, SEND_MESS_ACK);
        send_msg_ack_request.message.id = recipient_entry.user.last_msg_id;

        /* set up sender user entry */
        entry_t sender_entry;
        sender_entry.type = ENT_TYPE_UD;
        strcpy(sender_entry.username, request->message.sender);

        /* send second ACK to sender listening thread */
        clt_send_mess_ack(&send_msg_ack_request, &sender_entry);
    }
}
