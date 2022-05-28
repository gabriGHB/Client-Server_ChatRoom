#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include "DS-Lab-Assignment/netUtil.h"
#include "DS-Lab-Assignment/dbms/dbms.h"
#include "DS-Lab-Assignment/services.h"

/* prototypes */
void *service_thread(void *args);
void set_server_error_code_std(reply_t *reply, int req_error_code);


/* connection queue */
int conn_q[MAX_CONN_BACKLOG];   /* array of client sockets; used as a connection queue */
int conn_q_size = 0;            /* current number of backlogged connections */
int service_th_pos = 0;         /* connection queue position used by service threads to handle connections */

#define THREAD_POOL_SIZE 5      /* max number of service threads running */

/* mutex and cond vars for conn_q access */
pthread_mutex_t mutex_conn_q;
pthread_cond_t cond_conn_q_not_empty;
pthread_cond_t cond_conn_q_not_full;

pthread_mutex_t mutex_db;                   /* mutex for atomic operations on the DB */
pthread_attr_t th_attr;                     /* service thread attributes */
pthread_t thread_pool[THREAD_POOL_SIZE];    /* array of service threads */


void set_server_error_code_std(reply_t *reply, const int req_error_code) {
    /* most services follow this error code model */
    switch (req_error_code) {
        case 0: reply->server_error_code = SRV_SUCCESS; break;
        case -1: reply->server_error_code = SRV_ERR_CN_ANY; break;
        default: break;
    }
}


void *service_thread(void *args) {
    while (TRUE) {
        int client_socket;

        /* copy client socket descriptor from connection queue */
        pthread_mutex_lock(&mutex_conn_q);

        /* there are no connections to handle, so sleep */
        while (conn_q_size == 0)
            pthread_cond_wait(&cond_conn_q_not_empty, &mutex_conn_q);

        /* deque client socket descriptor */
        client_socket = conn_q[service_th_pos];
        service_th_pos = (service_th_pos + 1) % MAX_CONN_BACKLOG;
        conn_q_size -= 1;

        /* signal that there is space for new connections */
        if (conn_q_size == MAX_CONN_BACKLOG - 1) pthread_cond_signal(&cond_conn_q_not_full);

        pthread_mutex_unlock(&mutex_conn_q);

        /* handle connection now
         * receive op_code */
        request_t request;
        if (recv_string(client_socket, request.op_code) == -1) continue;

        if (!strcmp(request.op_code, REGISTER))
            srv_register(client_socket, &request);
        else if (!strcmp(request.op_code, UNREGISTER))
            srv_unregister(client_socket, &request);
        else if (!strcmp(request.op_code, CONNECT))
            srv_connect(client_socket, &request);
        else if (!strcmp(request.op_code, DISCONNECT))
            srv_disconnect(client_socket, &request);
        else if (!strcmp(request.op_code, SEND))
            srv_send(client_socket, &request);
    } // end outer while
}


void shutdown_server() {
    /* destroy server resources before shutting it down */
    pthread_mutex_destroy(&mutex_conn_q);
    pthread_mutex_destroy(&mutex_db);
    pthread_attr_destroy(&th_attr);
    fprintf(stderr, "Shutting down server\n");
    exit(0);
}


int main(int argc, char **argv) {
    int ret_val;    /* needed for error-checking macros */
    struct hostent *server_host;
    struct in_addr server_in;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int server_sd, client_sd;
    int val = 1;

    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Usage: server -p <port>\n");
        return GEN_ERR_INV_ARGS;
    }

    int server_port;
    CHECK_ARGS((str_to_num(argv[2], (void *) &server_port, INT) < 0), "Invalid Port")

    /* set up connection queue */
    pthread_mutex_init(&mutex_conn_q, NULL);
    pthread_cond_init(&cond_conn_q_not_empty, NULL);
    pthread_cond_init(&cond_conn_q_not_full, NULL);
    /* conn_q position used by main thread to enqueue connections */
    int producer_pos = 0;

    /* make service threads detached */
    pthread_attr_init(&th_attr);
    pthread_attr_setdetachstate(&th_attr, PTHREAD_CREATE_DETACHED);

    pthread_mutex_init(&mutex_db, NULL);    /* for atomic DB operations */

    /* set up SIGINT (CTRL+C) signal handler to shut down server */
    struct sigaction keyboard_interrupt;
    keyboard_interrupt.sa_handler = shutdown_server;
    keyboard_interrupt.sa_flags = 0;
    sigemptyset(&keyboard_interrupt.sa_mask);
    sigaction(SIGINT, &keyboard_interrupt, NULL);

    /* set up DB */
    CHECK_FUNC_ERROR(db_init_db(), GEN_ERR_ANY)

    /* get server up & running */
    CHECK_FUNC_ERROR_WITH_ERRNO(server_sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), GEN_ERR_ANY)
    CHECK_SOCK_ERROR(setsockopt(server_sd, SOL_SOCKET, SO_REUSEADDR,
                                (char *) &val,sizeof(int)), server_sd)

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);

    CHECK_SOCK_ERROR(bind(server_sd, (struct sockaddr *) &server_addr, sizeof server_addr), server_sd)
    CHECK_SOCK_ERROR(listen(server_sd, LISTEN_BACKLOG), server_sd)

    /* now create thread pool */
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&thread_pool[i], &th_attr, service_thread, NULL);
    }

    /* get local IP address to print initial server log message */
    char hostname[256];
    CHECK_FUNC_ERROR_WITH_ERRNO(gethostname(hostname, 256), GEN_ERR_ANY)
    server_host = gethostbyname(hostname);
    CHECK_ERROR_WITH_ERRNO(!server_host, "Server gethostbyname error", GEN_ERR_ANY)
    memcpy(&server_in.s_addr,*(server_host->h_addr_list),sizeof(server_in.s_addr));

    printf("s> init server %s:%i\n", inet_ntoa(server_in), server_port); fflush(stdout);

    while (TRUE) {      /* main server loop: accept connections from clients and queue them */
        CHECK_FUNC_ERROR_WITH_ERRNO(client_sd = accept(server_sd, (struct sockaddr *) &client_addr,
                &addr_size), -1)

        /* add connection to conn_q backlog */
        pthread_mutex_lock(&mutex_conn_q);

        /* if the connection queue is full, the main server thread sleeps:
         * no new connections can be opened until one is processed */
        while (conn_q_size == MAX_CONN_BACKLOG)
            pthread_cond_wait(&cond_conn_q_not_full, &mutex_conn_q);

        /* enqueue new connection */
        conn_q[producer_pos] = client_sd;
        producer_pos = (producer_pos + 1) % MAX_CONN_BACKLOG;
        conn_q_size += 1;

        /* signal that there are connections to handle */
        if (conn_q_size == 1) pthread_cond_signal(&cond_conn_q_not_empty);

        pthread_mutex_unlock(&mutex_conn_q);
    } // END while
}
