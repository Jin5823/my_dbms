#include "db.h"




int tcnt = 0;

int init_db(int buf_num){
	return init_buffer_hash_lru(buf_num);
}

int open_table(char *pathname) {
	int utid = 0;
	int i=0;
	while (i<10){
		if (strcmp(filetable[i].pathname, pathname) == 0){
			utid = filetable[i].tableid;
			break;
		}else{
			i++;
		}
	}
	if (utid == 0 && tcnt<10){
		strcpy(filetable[tcnt].pathname, pathname);
		filetable[tcnt].tableid = tcnt+1;
		utid = tcnt+1;
		tcnt++;
	}
	if (utid == 0 && tcnt>9)
		return -1;
	if (file_open(pathname, utid)==0){
		return utid;
	}else{
		return -1;
	}
}

int db_insert(int table_id, int64_t key, char* value) {
	uint64_t rootnum;
	if (tcnt != 0 && is_disk_empty(table_id)==-1){
		rootnum = creat_header_root(table_id);
		if (rootnum < 1)
			return -1;
	}
	if (is_disk_empty(table_id)!=-1){
		rootnum = read_header(table_id);
		pin_down(table_id, 0);
		insert(table_id, rootnum, key, value );
		return 0;
	}else{
		return -1;
	}
}

int db_find(int table_id, int64_t key, char* ret_val) {
	uint64_t rootnum;
	KeyValue * key_v;
	if (tcnt != 0 && is_disk_empty(table_id)==-1){
		rootnum = creat_header_root(table_id);
		if (rootnum < 1)
			return -1;
	}
	if (is_disk_empty(table_id)!=-1){
		rootnum = read_header(table_id);
		pin_down(table_id, 0);
		key_v = find(table_id, rootnum, key);
		if (key_v == NULL)
			return -1;
		else
			strcpy(ret_val, key_v->Value);
		return 0;
	}else{
		return -1;
	}
}

int db_delete(int table_id, int64_t key) {
	if (tcnt != 0 && is_disk_empty(table_id)!=-1){
		uint64_t rootnum = read_header(table_id);
		pin_down(table_id, 0);
		delete(table_id, rootnum, key);
		return 0;
	}else{
		return -1;
	}
}

int close_table(int table_id){
	write_all(table_id);
	return 0;
}

int shutdown_db(void){
	shutdown();
	return 0;
}
