#include "db.h"




pthread_mutex_t trx_table_latch = PTHREAD_MUTEX_INITIALIZER;
int global_transaction_id = 0;
int tcnt = 0;


int init_trx_table(){
	trx_hash_table = lock_create_hashtable(10000);
	return 0;
}


int trx_begin(void){
	pthread_mutex_lock(&trx_table_latch);
	global_transaction_id = global_transaction_id + 1;
	int temp = global_transaction_id;
	pthread_mutex_unlock(&trx_table_latch);

	LockNode* entry;
	entry = lock_get_value(trx_hash_table, 1, temp);

	if (entry == NULL){
		lock_value_set(trx_hash_table, 1, temp);
	}

	// return 해야할 값이 충돌 없어야함
	// pthread_mutex_unlock(&trx_table_latch);
	return temp;
}


int trx_commit(int trx_id){
	LockNode* trxentry;
	lock_t* trxlock;
	lock_t* temp;
	trxentry = lock_get_value(trx_hash_table, 1, trx_id);

	trxlock = trxentry->Head;

	if (trxentry->Head == NULL){
		//printf("dead lock commit%d\n", trx_id);
	}

	while (trxlock != NULL){
		temp = trxlock->Trx_next_lock_ptr;
		lock_release(trxlock);
		trxlock = temp;
		//printf("*commit%d*  ", trx_id);
	}
	// trx_id 를 통해 테이블에 접근하고 lock_obj 갖고 오기
	// lock_release(lock_t* lock_obj) 갖고 온 오브젝트 하나하나식 풀어주기
	trxentry->isdead = 1;
	return trx_id;
}


int trx_abort(int trx_id){
	// abort, lock_release, rollback
	//printf("******abort\n");
	LockNode* trxentry;
	lock_t* trxlock;
	lock_t* temp;
	trxentry = lock_get_value(trx_hash_table, 1, trx_id);

	trxlock = trxentry->Head;

	while (trxlock != NULL){
		//printf("start\n");
		if (trxlock->Lock_mode == 1 && trxlock->tabid != -1){
			//printf("rollback in \n");
			update_trx(trxlock->tabid, trxlock->pagen, trxlock->key_history, trxlock->value_history);
			unlock_page(trxlock->tabid, trxlock->pagen);
			//printf("rollback out \n");
		}
		//printf("while \n");
		temp = trxlock->Trx_next_lock_ptr;
		lock_release(trxlock);
		trxlock = temp;
		//printf("*dead commit%d*  ", trx_id);
		//printf("while 2\n");
	}

	trxentry->Head = NULL;
	trxentry->isdead = 1;
	//printf("end%d\n", trx_id);
	return 0;
}


int init_db(int buf_num, int flag, int log_num, char* log_path, char* logmsg_path){
	init_lock_table();
	init_trx_table();

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


int db_find(int table_id, int64_t key, char* ret_val, int trx_id) {
	//printf("find%d   ", trx_id);
	if (trx_id == 0){
		//printf("no key\n");
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

	if (lock_get_value(trx_hash_table, 1, trx_id)->isdead == 1){
		//printf("find already dead\n");
		return -1;
	}
	// 이미 이전에 deadlock이 발생해 abort했을 경우
	// Deadlock Detection 이후 trx_abort()
    // --->  lock_release()
    // --->  rollback_results()

	// abort 즉, deadlock 탐지로 인한 에러 뿐만 아니라, 찾고자하는 key의 레코드가 없는 경우에도 abort를 해야합니다.
	// 위는 아직 미구현

	uint64_t rootnum;
	KeyValue * key_v;
	if (tcnt != 0 && is_disk_empty(table_id)==-1){
		rootnum = creat_header_root(table_id);
		if (rootnum < 1){
			//printf("no key%d\n", trx_id);
			return -1;
		}
	}
	if (is_disk_empty(table_id)!=-1){
		rootnum = read_header_trx(table_id);
		rootnum = find_leaf_trx(table_id, rootnum, key);
		// 위치한 leaf page를 찾음

		lock_t* flock;
		flock =  lock_acquire(table_id, key, trx_id, 0);
		//printf("lockacq%d   ", trx_id);
		if (flock == NULL){
			//printf("find dead%d\n", trx_id);
			trx_abort(trx_id);
			return -1;
		}
		// 걸어줌, 순번이 되면 아래를 실행, 이때 다시 page lock
		key_v = find_trx(table_id, rootnum, key);
		if (key_v == NULL){
			//printf("no key\n");
			unlock_page(table_id, rootnum);
			// trx_abort(trx_id);
			return -1;
		}else{
			strcpy(ret_val, key_v->Value);
		}
		unlock_page(table_id, rootnum);
		//printf("find good%d\n", trx_id);
		return 0;
	}else{
		//printf("no key\n");
		return -1;
	}
}


int db_update(int table_id, int64_t key, char* values, int trx_id){
	//printf("\nupdate%d  ", trx_id);
	if (lock_get_value(trx_hash_table, 1, trx_id)->isdead == 1){
		//printf("update already dead%d\n", trx_id);
		return -1;
	}
	// 이미 이전에 deadlock이 발생해 abort했을 경우

	uint64_t rootnum;
	KeyValue * key_v;
	if (tcnt != 0 && is_disk_empty(table_id)!=-1){
		rootnum = read_header_trx(table_id);
		rootnum = find_leaf_trx(table_id, rootnum, key);
		// 위치한 leaf page를 찾음

		lock_t* ulock;
		ulock =  lock_acquire(table_id, key, trx_id, 1);
		//printf("ulockacq%d   ", trx_id);
		if (ulock == NULL){
			//printf("update dead%d\n", trx_id);
			trx_abort(trx_id);
			return -1;
		}
		// 걸어줌, 순번이 되면 아래를 실행, 이때 다시 page lock

		ulock->key_history = key;
		ulock->pagen = rootnum;
		ulock->tabid = table_id;
		key_v = update_trx(table_id, rootnum, key, values);
		//printf("bufflockacq%d   ", trx_id);
		if (key_v == NULL){
			//printf("no key\n");
			ulock->tabid = -1;
			unlock_page(table_id, rootnum);
			// trx_abort(trx_id);
			return -1;
		}else{
			strcpy(ulock->value_history, key_v->Value);
			unlock_page(table_id, rootnum);
			//printf("update good%d\n", trx_id);
			return 0;
		}
	}else{
		//printf("no key\n");
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
	lock_destroy_hash_table(lock_hash_table);
	lock_destroy_hash_table(trx_hash_table);
	shutdown();
	return 0;
}
