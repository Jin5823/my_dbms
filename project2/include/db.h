#ifndef DB_H_
#define DB_H_


#include "bpt.h"
#include "file.h"
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
	char pathname[100];
}unique_table;
unique_table filetable[100];


int open_table (char *pathname);
int db_insert (int64_t key, char * value);
int db_find (int64_t key, char * ret_val);
int db_delete (int64_t key);


#endif /* DB_H_ */
