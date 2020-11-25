#include "db.h"




int tcnt = 0;

int open_table (char *pathname) {
	int utid = 0;
	int i=0;
	while (i<tcnt){
		if (filetable[i].pathname == pathname){
			utid = filetable[i].tableid;
			break;
		}else{
			i++;
		}
	}
	if (i==tcnt){
		strcpy(filetable[tcnt].pathname, pathname);
		filetable[tcnt].tableid = tcnt;
		utid = tcnt;
		tcnt++;
	}
	if (file_open(pathname)==0){
		return utid;
	}else{
		return -1;
	}
}


int db_insert (int64_t key, char * value) {
	uint64_t rootnum;
	if (tcnt != 0 && is_disk_empty()==-1){
		rootnum = creat_header_root();
		if (rootnum < 1)
			return -1;
	}
	if (is_disk_empty()!=-1){
		rootnum = read_header();
		insert( rootnum, key, value );
		return 0;
	}else{
		return -1;
	}
}


int db_find (int64_t key, char * ret_val) {
	uint64_t rootnum;
	KeyValue * key_v;
	if (tcnt != 0 && is_disk_empty()==-1){
		rootnum = creat_header_root();
		if (rootnum < 1)
			return -1;
	}
	if (is_disk_empty()!=-1){
		rootnum = read_header();
		key_v = find(rootnum, key);
		if (key_v == NULL)
			return -1;
		else
			strcpy(ret_val, key_v->Value);
		return 0;
	}else{
		return -1;
	}
}


int db_delete (int64_t key) {
	if (tcnt != 0 && is_disk_empty()!=-1){
		uint64_t rootnum = read_header();
		delete(rootnum, key);
		return 0;
	}else{
		return -1;
	}
}
