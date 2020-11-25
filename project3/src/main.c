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




int main(){
	int uid;
	int bufnum;
	char instruction;
	char pathname[20];
	char ivalue[120];
	char fvalue[120];
	int64_t key;
	usage();
	printf("> ");
	while (scanf("%s", &instruction) != EOF) {
		switch (instruction) {
		case 'n':
			scanf("%d", &bufnum);
			init_db(bufnum);
			printf("init succeeded\n");
			break;
		case 'o':
			scanf("%s", pathname);
			uid = open_table(pathname);
			if (uid!=-1)
				printf("unique table id: %d\n", uid);
			break;
		case 'i':
			scanf("%d %"PRIu64" %[^\n]s",&uid, &key, ivalue);
			db_insert(uid, key, ivalue);
			break;
		case 'f':
			scanf("%d %"PRIu64"", &uid, &key);
			if (db_find(uid, key, fvalue)!= -1)
				printf("find value: %s\n", fvalue);
			else printf("can not find \n");
			break;
		case 'd':
			scanf("%d %"PRIu64"", &uid, &key);
			db_delete(uid, key);
			break;
		case 'c':
			scanf("%d", &uid);
			close_table(uid);
			printf("closed\n");
			break;
		case 'e':
			shutdown_db();
			while (getchar() != (int)'\n');
			return EXIT_SUCCESS;
			break;
		default:
			break;
			usage();
			}
		while (getchar() != (int)'\n');
		printf("> ");
	}
	printf("\n");

	//file_close();
	return EXIT_SUCCESS;
}
