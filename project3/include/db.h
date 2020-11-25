#ifndef DB_H_
#define DB_H_


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




typedef struct unique_table {
	int tableid;
	char pathname[20];
}unique_table;
unique_table filetable[10];

int init_db(int buf_num);
int open_table(char *pathname);
int db_insert(int table_id, int64_t key, char * value);
int db_find(int table_id, int64_t key, char * ret_val);
int db_delete(int table_id, int64_t key);
int close_table(int table_id);
int shutdown_db();


#endif /* DB_H_ */
