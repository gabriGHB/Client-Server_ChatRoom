#ifndef UTILS_H
#define UTILS_H

/*** Utilities ***/
#define TRUE 1
#define FALSE 0
#define MAX_STR_SIZE 512                /* generic string size */
#define MAX_MSG_SIZE 256                /* size of message content string */
#define MSG_ID_MAX_VALUE 4294967295     /* max message ID value (actually max unsigned int value on amd64) */
#define MSG_ID_MAX_STR_SIZE 10          /* max length of message ID as a string */

/********** Services: Operation Codes **********/

/***** Services Called By Client, Served By Server *****/
#define REGISTER "REGISTER"
#define UNREGISTER "UNREGISTER"
#define CONNECT "CONNECT"
#define DISCONNECT "DISCONNECT"
#define SEND "SEND"

/***** Services Called By Server, Served By Client Listening Thread *****/
#define SEND_MESSAGE "SEND_MESSAGE"
#define SEND_MESS_ACK "SEND_MESS_ACK"


/******************** ERROR CODES ********************/

/********** General Error Codes **********/
#define GEN_SUCCESS 0
#define GEN_ERR_ANY -1
#define GEN_ERR_INV_ARGS -2

/********** Server Error Codes **********/

/****** General Server Error Codes ******/
#define SRV_SUCCESS 0
#define TEST_ERR_CODE 100

/****** Register Service ******/
#define SRV_ERR_REG_USR_ALREADY_REG 1
#define SRV_ERR_REG_ANY 2

/****** Unregister Service ******/
#define SRV_ERR_UNREG_USR_NOT_EXISTS 1
#define SRV_ERR_UNREG_ANY 2

/****** Connect Service ******/
#define SRV_ERR_CN_USR_NOT_EXISTS 1
#define SRV_ERR_CN_USR_ALREADY_CN 2
#define SRV_ERR_CN_ANY 3

/****** Disconnect Service ******/
#define SRV_ERR_DCN_USR_NOT_EXISTS 1
#define SRV_ERR_DCN_USR_NOT_CN 2
#define SRV_ERR_DCN_ANY 3

/****** Send Service ******/
#define SRV_ERR_SEND_USR_NOT_EXISTS 1
#define SRV_ERR_SEND_ANY 2

/********** DBMS Error Codes **********/
#define DBMS_SUCCESS 100
#define DBMS_ERR_ANY -100
#define DBMS_ERR_NOT_EXISTS -101
#define DBMS_ERR_EXISTS -102


/***** Client Connection States *****/
/** values that userdata.status can take **/
#define STATUS_DCN 0
#define STATUS_CN 1

/******** DBMS Stuff ********/

/**** File & Directory Opening Modes ****/
#define READ 'r'
#define CREATE 'c'
#define OVERWRITE 'o'   /* for directories only */
#define MODIFY 'm'      /* for files only */

/** Additional DB Entry File Manipulation Mode **/
#define DELETE 'd'      /* for files only */

/**** SCHEMA ****/
#define DB_DIR "users"                          /* database directory name */
#define USERDATA_ENTRY "userdata.entry"         /* userdata entry name */
#define PEND_MSGS_TABLE "pend_msgs-table"       /* pending messages table name */

/**** DB Entry Types ****/
#define ENT_TYPE_UD 'u'       /* userdata entry type */
#define ENT_TYPE_P_MSG 'm'    /* pending message entry type */


/**** Number Casting Stuff ****/
#define INT 'i'
#define UINT 'u'
#define FLOAT 'f'
int str_to_num(const char *string, void *number, char type);

/**** File/Socket Descriptor I/O Functions ****/
int write_bytes(int d, const char *buffer, int len);
int read_bytes(int d, char *buffer, int len);
int read_line(int d, char *buffer, int buf_space);


/********** Macros **********/

/***** General Error Checking *****/
#define CHECK_ERROR(CONDITION, ERR_MSG, RETURN_ERROR) \
        if (CONDITION) { \
            fprintf(stderr, "Runtime Error: %s\n at %s:%d\n", ERR_MSG, __FILE__, __LINE__); \
            return RETURN_ERROR; \
        }

/***** General Error Checking With perror() Call To Print errno Message *****/
#define CHECK_ERROR_WITH_ERRNO(CONDITION, ERR_MSG, RETURN_ERROR) \
        if (CONDITION) { \
            fprintf(stderr, "Runtime Error: %s\n at %s:%d\n", ERR_MSG, __FILE__, __LINE__); \
            perror(ERR_MSG); \
            return RETURN_ERROR; \
        }

/***** General Function Error Checking *****/
#define CHECK_FUNC_ERROR(FUNCTION_CALL, RETURN_ERROR) \
    ret_val = (FUNCTION_CALL); \
    if (ret_val < 0) { \
        fprintf(stderr, "Runtime Error: %s\n returned %d\n at %s:%d\n", #FUNCTION_CALL, ret_val, __FILE__, __LINE__); \
        return (RETURN_ERROR); \
    }

/***** General Function Error Checking With perror() Call To Print errno Message *****/
#define CHECK_FUNC_ERROR_WITH_ERRNO(FUNCTION_CALL, RETURN_ERROR) \
    ret_val = (FUNCTION_CALL); \
    if (ret_val < 0) { \
        fprintf(stderr, "Runtime Error: %s\n returned %d\n at %s:%d\n", #FUNCTION_CALL, ret_val, __FILE__, __LINE__); \
        perror(#FUNCTION_CALL); \
        return (RETURN_ERROR); \
    }

/***** Remove File/Directory Function Error Checking With perror() Call To Print errno Message *****/
#define CHECK_RM_ERROR(FUNCTION_CALL, RETURN_ERROR) \
    ret_val = (FUNCTION_CALL); \
    if (ret_val < 0) { \
        fprintf(stderr, "Runtime Error: %s\n returned %d\n at %s:%d\n", #FUNCTION_CALL, ret_val, __FILE__, __LINE__); \
        perror(#FUNCTION_CALL); \
        closedir(directory); \
        return (RETURN_ERROR); \
    }

/***** Socket Function Error Checking With perror() Call To Print errno Message *****/
#define CHECK_SOCK_ERROR(FUNCTION_CALL, SOCKET) \
    ret_val = (FUNCTION_CALL); \
    if (ret_val < 0) { \
        fprintf(stderr, "Runtime Error: %s\n returned %d\n at %s:%d\n", #FUNCTION_CALL, ret_val, __FILE__, __LINE__); \
        perror(#FUNCTION_CALL); \
        close(SOCKET); \
        return GEN_ERR_ANY; \
    }

/***** Argument Checking *****/
#define CHECK_ARGS(CONDITION, ERR_MSG) \
    if (CONDITION) { \
        fprintf(stderr, "Invalid Arguments Error: %s\n at %s:%d\n", ERR_MSG, __FILE__, __LINE__); \
        return GEN_ERR_INV_ARGS; \
    }


/***** Types Used For Process Communication *****/
typedef struct {
    /*** Message Sent Between Server & Client, Both Ways ***/
    char sender[MAX_STR_SIZE];      /* username of sender client */
    unsigned int id;            /* message ID */
    char content[MAX_MSG_SIZE]; /* message content */
} message_t;

typedef struct {
    /*** Client Request ***/
    char op_code[MAX_STR_SIZE];     /* operation code that indicates the service called */
    union {
        struct {
            char username[MAX_STR_SIZE];    /* member used for REGISTER, UNREGISTER, CONNECT & DISCONNECT services */
            char client_port[MAX_STR_SIZE]; /* client listening thread port; member only used for CONNECT service */
        };
        struct {    /* members used for SEND, SEND_MESSAGE & SEND_MESS_ACK services */
            char recipient[MAX_STR_SIZE];    /* username of recipient client */
            message_t message;
        };
    };
} request_t;

typedef struct {
    /*** Server Reply ***/
    unsigned char server_error_code;        /* error code returned by the server; client interprets it
 *                                          to figure out whether the transaction was successful */
} reply_t;

#include <stdint.h>
#include <netinet/in.h>

struct userdata {
    /*** User Data Stored In DB ***/
    unsigned char status;   /* STATUS_DCN := disconnected; STATUS_CN := connected */
    struct in_addr ip;      /* client IP for receiving thread */
    uint16_t port;          /* client port for receiving thread */
    unsigned int last_msg_id;
};


typedef struct {
    /*** General DB Entry Struct With Entry Type ***/
    char username[MAX_STR_SIZE];
    char type;                  /* ENT_TYPE_UD or ENT_TYPE_P_MSG */
    union {
        struct userdata user;   /* userdata entry */
        message_t msg;          /* pending message list entry */
    };
} entry_t;

#endif //UTILS_H

/*register, unregister, disconnect*/
//receive op_code
//receive username

/*connect*/
//receive op_code
//receive username
//receive port

/*send*/
//receive op_code
//receive sender username
//receive recipient username
//receive message content
//set message ID
//send message ID to sender client (first ACK: server got the message)

/*content passing from server to client*/

/*send content to recipient*/
//send op_code
//send sender username
//send message ID
//send message content

/*send second ack to sender (message got delivered)*/
//send op_code
//send message ID
