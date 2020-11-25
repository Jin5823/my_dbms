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
	char instruction;
	char pathname[100];
	char ivalue[120];
	char fvalue[120];
	int64_t key;
	usage();
	printf("> ");
	while (scanf("%s", &instruction) != EOF) {
		switch (instruction) {
		case 'o':
			scanf("%s", pathname);
			uid = open_table(pathname);
			if (uid!=-1)
				printf("unique table id: %d\n", uid);
			break;
		case 'i':
			scanf("%"PRIu64" %[^\n]s",&key, ivalue);
			db_insert (key, ivalue);
			break;
		case 'f':
			scanf("%"PRIu64"", &key);
			if (db_find (key, fvalue)!= -1)
				printf("find value: %s\n", fvalue);
			else printf("can not find \n");
			break;
		case 'd':
			scanf("%"PRIu64"", &key);
			db_delete(key);
			break;
		case 'e':
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
