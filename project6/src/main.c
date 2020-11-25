#include "db.h"
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
#include <time.h>

#define THREAD_NUMBER	(3)
int uid;


void* thread_func(void* arg){
	/*int data[10], i, temp, x, y;
	for (i=0;i<10;i++) data[i]=i+1;
	for (i=0;i<10;i++){
		x = rand()%10;
		y = rand()%10;
		if (x!=y){
			temp = data[x];
			data[x] = data[y];
			data[y] = temp;
		}
	}*/

	int trxnum;
	char fvalue1[120];
	char fvalue2[120];
	char fvalue3[120];
	char fvalue4[120];
	char fvalue5[120];

	trxnum = trx_begin();
	db_find(uid, rand()%10, fvalue1, trxnum);
	db_find(uid, rand()%10, fvalue2, trxnum);
	db_find(uid, rand()%10, fvalue3, trxnum);
	// db_update(uid, rand()%10, "test", trxnum);
	db_find(uid, rand()%10, fvalue4, trxnum);
	// db_find(uid, data[5], fvalue5, trxnum);
	db_find(uid, rand()%10, fvalue5, trxnum);
	db_update(uid, rand()%10, "test", trxnum);
	trx_commit(trxnum);

	// printf("%d thread is done. find: %d, %d, %d, %d, %d, update: %d\n",trxnum, data[1], data[2], data[3], data[4], data[5], data[6]);
	printf("%d thread is done.\n",trxnum);
	return NULL;
}


int main(){
	init_db(20);
	uid = open_table("sample_10000.db");

	pthread_t	threads[THREAD_NUMBER];
	srand(time(NULL));


	/* thread create */
	for (int i = 0; i < THREAD_NUMBER; i++) {
		pthread_create(&threads[i], 0, thread_func, NULL);
	}

	/* thread join */
	for (int i = 0; i < THREAD_NUMBER; i++) {
		pthread_join(threads[i], NULL);
	}

	close_table(uid);
	shutdown_db();

	return 0;
}
