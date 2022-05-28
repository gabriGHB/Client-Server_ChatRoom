#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include "DS-Lab-Assignment/dbms/dbmsUtil.h"
#include "DS-Lab-Assignment/dbms/dbms.h"


int db_init_db(void) {
    /*** Initialize the DB: make sure that the DB db exists, create it if it doesn't ***/
    int ret_val;    /* needed for error-checking macros */
    DIR * db;
    CHECK_FUNC_ERROR(open_directory(DB_DIR, OVERWRITE, &db), DBMS_ERR_ANY)

    closedir(db);
    return DBMS_SUCCESS;
}


int db_get_pend_msg(entry_t *entry) {
    /*** Reads a pending message of a given user entry ***/
    int ret_val;    /* needed for error-checking macros */
    struct dirent *pend_msgs_entry;
    int pend_msgs_table_is_empty = TRUE;    /* assume that there are no pending messages */

    CHECK_ARGS(entry->type != ENT_TYPE_P_MSG, "Invalid Entry Type")

    /* set up the path to open pending messages table directory for given username */
    char table_path[strlen(DB_DIR) + strlen(entry->username) + strlen(PEND_MSGS_TABLE) + 9];
    sprintf(table_path, "%s/%s-table/%s", DB_DIR, entry->username, PEND_MSGS_TABLE);

    DIR *pend_msg_table;
    CHECK_FUNC_ERROR(open_directory(table_path, READ, &pend_msg_table), DBMS_ERR_ANY)

    /* read directory entries */
    while ((pend_msgs_entry = readdir(pend_msg_table)) != NULL && pend_msgs_table_is_empty) {
        if (!strcmp(pend_msgs_entry->d_name, ".") || !strcmp(pend_msgs_entry->d_name, "..")) continue;

        /* found a pending message: read it and break out of the loop */
        CHECK_FUNC_ERROR(str_to_num(pend_msgs_entry->d_name, (void *) &entry->msg.id, UINT),
                         DBMS_ERR_ANY)
        db_io_op_usr_ent(entry, READ);
        pend_msgs_table_is_empty = FALSE;
    }

    closedir(pend_msg_table);
    /* if no pending messages were found, return an error */
    return (!pend_msgs_table_is_empty) ? DBMS_SUCCESS : DBMS_ERR_NOT_EXISTS;
}


int db_empty_db(void) {
    /*** Simply deletes all files and directories in the DB root folder ***/
    return remove_recursive(DB_DIR);
}


int db_user_exists(const char *const username) {
    /*** Checks whether a given username exists in the DB ***/
    /* set up the path to open username user_table */
    size_t path_len = strlen(DB_DIR) + strlen(username) + 7;
    char user_path[path_len];
    sprintf(user_path, "%s/%s-table", DB_DIR, username);
    DIR *user_table;

    /* try to open it and see if it exists */
    int result = open_directory(user_path, READ, &user_table);

    if (result == DBMS_ERR_NOT_EXISTS) return FALSE;
    else if (result == DBMS_SUCCESS) {
        closedir(user_table);
        return TRUE;
    }

    /* some other error happened, so return its error code */
    return result;
}


int db_io_op_usr_ent(entry_t *entry, const char mode) {
    /*** Reads, writes or deletes a DB username entry;
     * entry type must be specified in given entry->type;
     * it can read, modify or delete an existing entry, or create a new one ***/
    char error[MAX_STR_SIZE];   /* message displayed in perror */

    CHECK_ARGS(entry->type != ENT_TYPE_UD && entry->type != ENT_TYPE_P_MSG, "Invalid Entry Type")
    CHECK_ARGS(mode != CREATE && mode != MODIFY && mode != READ && mode != DELETE, "Invalid File Mode")

    /* set up the path to read/write username entry:
     * set up path length */
    size_t entry_len = strlen(DB_DIR) + strlen(entry->username) + 9;   /* account for common path elements */
    /* account for specific path elements depending on entry type */
    entry_len += (entry->type == ENT_TYPE_UD) ? strlen(USERDATA_ENTRY) :
                 (strlen(PEND_MSGS_TABLE) + 1 + MSG_ID_MAX_STR_SIZE);

    char entry_path[entry_len];     /* set up actual path, depending on entry type */
    (entry->type == ENT_TYPE_UD) ?
    sprintf(entry_path, "%s/%s-table/%s", DB_DIR, entry->username, USERDATA_ENTRY) :
    sprintf(entry_path, "%s/%s-table/%s/%u", DB_DIR, entry->username, PEND_MSGS_TABLE, entry->msg.id);

    /* delete entry */
    if (mode == DELETE) {
        int result = remove_recursive(entry_path);
        return (result < 0) ? result : DBMS_SUCCESS;
    }

    /* open entry file to read/write it */
    int entry_fd = open_file(entry_path, mode);

    if (entry_fd == -1) {
        if (errno == ENOENT) {
            sprintf(error, "%s doesn't exist", entry_path); perror(error);
            return DBMS_ERR_NOT_EXISTS;
        }
        /* some other open() error */
        sprintf(error, "Error opening %s", entry_path); perror(error);
        return DBMS_ERR_ANY;
    }

    /* read/write entry */
    int result = ((mode == READ) ? read_entry : write_entry)(entry_fd, entry);
    close(entry_fd);
    return result;
}


int db_creat_usr_tbl(entry_t *entry) {
    /*** Creates a table for the given username (entire structure) ***/
    /* set up the path to open table directory for given username */
    char user_table_path[strlen(DB_DIR) + strlen(entry->username) + 8];
    sprintf(user_table_path, "%s/%s-table", DB_DIR, entry->username);

    /* set up the path for the userdata entry to be created */
    char ud_entry_path[strlen(user_table_path) + strlen(USERDATA_ENTRY) + 2];
    sprintf(ud_entry_path, "%s/%s", user_table_path, USERDATA_ENTRY);

    /* set up the path for the userdata entry to be created */
    char p_msg_table_path[strlen(user_table_path) + strlen(PEND_MSGS_TABLE) + 2];
    sprintf(p_msg_table_path, "%s/%s", user_table_path, PEND_MSGS_TABLE);

    /* try to create username directory */
    DIR *user_table;
    int result = open_directory(user_table_path, CREATE, &user_table);
    if (result == DBMS_ERR_EXISTS) {       /* user already exists */
        closedir(user_table);
        return DBMS_ERR_EXISTS;
    } else if (result < 0) return DBMS_ERR_ANY;     /* some other error */

    /* at this point user table has been successfully created, so we continue */

    /* create userdata entry for given username */
    if (db_io_op_usr_ent(entry, CREATE) < 0) return DBMS_ERR_ANY;

    /* now create pending messages table, within username table */
    DIR *p_msg_table;
    if (open_directory(p_msg_table_path, CREATE, &p_msg_table) < 0) return DBMS_ERR_ANY;
    closedir(p_msg_table);
    return DBMS_SUCCESS;
}


int db_del_usr_tbl(const char *const username) {
    /*** Deletes a given username table if it exists ***/
    /* set up the path to remove table directories for given username */
    char table_path[strlen(DB_DIR) + strlen(username) + 8];
    sprintf(table_path, "%s/%s-table", DB_DIR, username);

    return remove_recursive(table_path);
}
