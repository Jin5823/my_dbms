#ifndef DB_H_
#define DB_H_


#include "lock.h"
#include "bpt.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>




typedef struct unique_table {
	int tableid;
	char pathname[20];
}unique_table;
unique_table filetable[10];


int init_trx_table();
int trx_begin(void);
int trx_commit(int trx_id);
int trx_abort(int trx_id);


int init_db(int buf_num, int flag, int log_num, char* log_path, char* logmsg_path);
int open_table(char *pathname);
int db_insert(int table_id, int64_t key, char * value);


// int db_find(int table_id, int64_t key, char * ret_val);
int db_find(int table_id, int64_t key, char * ret_val, int trx_id);
int db_update(int table_id, int64_t key, char* values, int trx_id);


int db_delete(int table_id, int64_t key);
int close_table(int table_id);
int shutdown_db();


#endif /* DB_H_ */
