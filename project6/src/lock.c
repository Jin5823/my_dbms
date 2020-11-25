#include "lock.h"




pthread_mutex_t lock_table_latch = PTHREAD_MUTEX_INITIALIZER;


int lock_make_hash(int64_t key, int table_size){
	return key % table_size;
}


LockHashTable* lock_create_hashtable(int table_size){
	LockHashTable* hashTable = (LockHashTable*)malloc(sizeof(LockHashTable));
	hashTable->Table = (LockList*)malloc(sizeof(LockList) * table_size);
	hashTable->TableSize = table_size;
	return hashTable;
}


LockNode* lock_create_node(int table_id, int64_t key){
	LockNode* newNode = (LockNode*)malloc(sizeof(LockNode));
	newNode->TableID = table_id;
	newNode->RecordID = key;
	newNode->Head = NULL;
	newNode->Tail = NULL;
	newNode->Next = NULL;
	newNode->isdead = 0;
	return newNode;
}


void lock_value_set(LockHashTable* hashTable, int table_id, int64_t key){
	int address = lock_make_hash(key, hashTable->TableSize);
	LockNode* newNode = lock_create_node(table_id, key);
	if(hashTable->Table[address] == NULL){
		hashTable->Table[address] = newNode;
	}else{
		LockList list = hashTable->Table[address];
		newNode->Next = list;
		hashTable->Table[address] = newNode;
    }
}


LockNode* lock_get_value(LockHashTable* hashTable, int table_id, int64_t key){
	int address = lock_make_hash(key, hashTable->TableSize);
	LockList list = hashTable->Table[address];
	LockList target = NULL;
	if(list == NULL) return NULL;
	while(1){
		if(list->RecordID == key && list->TableID == table_id){
			target = list;
			break;
		}
		if(list->Next == NULL) return NULL;

		else{
			list = list->Next;
		}
	}
	return target;
}


int lock_clean_node(LockHashTable* hashTable, int table_id, int64_t key){
	int address = lock_make_hash(key, hashTable->TableSize);
	LockList list = hashTable->Table[address];
	LockList target = NULL;
	if(list == NULL) return -1;
	while(1){
		if(list->RecordID == key && list->TableID == table_id){
			target = list;
			break;
		}
		if(list->Next == NULL) return -1;
		else{
			list = list->Next;
		}
	}
	lock_t* temp = target->Head;
	target->Head = target->Head->next_pointer;
	free(temp);
	return 0;
}


void lock_destroy_node(LockNode *node){
	free(node);
}


void lock_destroy_list(LockList list){
	if(list == NULL){
        return;
    }
    if(list->Next != NULL){
    	lock_destroy_list(list->Next);
    }
    lock_destroy_node(list);
}


void lock_destroy_hash_table(LockHashTable *hash){
	for(int i = 0 ; i < hash->TableSize ; i ++){
		LockList list = hash->Table[i];
		lock_destroy_list(list);
	}
}


int init_lock_table(){
	lock_hash_table = lock_create_hashtable(10000);
	return 0;
}


int dead_recursion(int trx_id, int track){
	LockNode* trxentry;
	lock_t* trxlock;
	trxentry = lock_get_value(trx_hash_table, 1, trx_id);
	trxlock = trxentry->Head;
	
	if (track == trx_id){
		return 1;
	}
	// 같은 trx를 기다리고 있기에 dead lock

	while (trxlock != NULL){
		if (trxlock->prev_pointer != NULL){
			if (trxlock->prev_pointer->Owner_Transaction_ID == track && trxlock->iswait == 1){
				return 1;
			}
			if (trxlock->prev_pointer->Owner_Transaction_ID != track && trxlock->iswait == 1){
				return dead_recursion(trxlock->prev_pointer->Owner_Transaction_ID, track);
			}
		}
		trxlock = trxlock->Trx_next_lock_ptr;
	}
	return 0;
}


lock_t* lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode){
	pthread_mutex_lock(&lock_table_latch);
	LockNode* entry;
	entry = lock_get_value(lock_hash_table, table_id, key);

	if (entry == NULL){
		lock_value_set(lock_hash_table, table_id, key);
		entry = lock_get_value(lock_hash_table, table_id, key);
	}
	// hash 에 없을 경우 해시를 만들어준다


	if (entry->Tail != NULL && lock_mode == 1){
		LockNode* trxentry;
		lock_t* trxlock;
		trxentry = lock_get_value(trx_hash_table, 1, entry->Tail->Owner_Transaction_ID);
		trxlock = trxentry->Head;

		if (entry->Tail->Owner_Transaction_ID == trx_id){
			//trxentry->isdead = 1;
			pthread_mutex_unlock(&lock_table_latch);
			return NULL;
		}
		// 같은 trx를 기다리고 있기에 dead lock

		while (trxlock != NULL){
			//printf("Deadlock Detection\n");
			if (trxlock->prev_pointer != NULL){
				if (trxlock->prev_pointer->Owner_Transaction_ID == trx_id && trxlock->iswait == 1){
					//printf("*****************dead lock**************\n");
					//trxentry->isdead = 1;
					pthread_mutex_unlock(&lock_table_latch);
					return NULL;
				}
				if (trxlock->prev_pointer->Owner_Transaction_ID != trx_id && trxlock->iswait == 1){
					if (dead_recursion(trxlock->prev_pointer->Owner_Transaction_ID, trx_id) == 1){
						//trxentry->isdead = 1;
						pthread_mutex_unlock(&lock_table_latch);
						return NULL;
					}
				}
			}
			trxlock = trxlock->Trx_next_lock_ptr;
		}
		//printf("no Deadlock\n");
	}
	// Deadlock Detection


	lock_t* newlock = (lock_t*)malloc(sizeof(lock_t));
	pthread_cond_init(&newlock->Conditional_Variable, NULL);
	newlock->Sentinel_pointer = entry;
	newlock->next_pointer = NULL;
	newlock->prev_pointer = entry->Tail;

	newlock->Lock_mode = lock_mode;
	newlock->Owner_Transaction_ID = trx_id;
	newlock->Trx_next_lock_ptr = NULL;
	newlock->iswait = 0;
	newlock->tabid = -1;
	// 새로운 lock을 만든다

	// lock 은 trx에 걸어주기

	LockNode* trxentry;
	lock_t* trxlock;
	trxentry = lock_get_value(trx_hash_table, 1, trx_id);

	if (trxentry->Head == NULL){
		trxentry->Head = newlock;
	}else{
		trxlock = trxentry->Head;
		while (trxlock->Trx_next_lock_ptr != NULL){
			trxlock = trxlock->Trx_next_lock_ptr;
		}
		trxlock->Trx_next_lock_ptr = newlock;
	}


	entry->Tail = newlock;
	if (entry->Head == NULL){
		entry->Head = newlock;
	}else{
		newlock->prev_pointer->next_pointer = newlock; // 뒤에 걸기

		int mode_check = 0;
		lock_t* temp;
		temp = entry->Head;
		while (temp != newlock){
			if (temp->Lock_mode == 1){
				mode_check = 1;
				break;
			}
			temp = temp->next_pointer;
		}
		//printf("check mode\n");
		//Shared / Exclusive Lock 구현

		if (mode_check == 1 || newlock->Lock_mode == 1){
			newlock->iswait = 1;
			//printf("sleep%d wait%d  ", trx_id, newlock->prev_pointer->Owner_Transaction_ID);
			pthread_cond_wait(&newlock->Conditional_Variable, &lock_table_latch); // sleep
			//printf("wake%d  ", trx_id);

			if (newlock->prev_pointer != NULL && newlock->prev_pointer->prev_pointer != NULL){

				//printf("**iswait%d **", newlock->iswait);

				newlock->iswait = 1;
				LockNode* trxentry;
				lock_t* trxlock;
				trxentry = lock_get_value(trx_hash_table, 1, newlock->prev_pointer->prev_pointer->Owner_Transaction_ID);
				trxlock = trxentry->Head;

				if (entry->Tail->Owner_Transaction_ID == trx_id){
					//trxentry->isdead = 1;
					pthread_mutex_unlock(&lock_table_latch);
					return NULL;
				}
				// 같은 trx를 기다리고 있기에 dead lock

				while (trxlock != NULL){
					if (trxlock->prev_pointer != NULL){
						if (trxlock->prev_pointer->Owner_Transaction_ID == trx_id && trxlock->iswait == 1){
							//trxentry->isdead = 1;
							pthread_mutex_unlock(&lock_table_latch);
							return NULL;
						}
						if (trxlock->prev_pointer->Owner_Transaction_ID != trx_id && trxlock->iswait == 1){
							if (dead_recursion(trxlock->prev_pointer->Owner_Transaction_ID, trx_id) == 1){
								//trxentry->isdead = 1;
								pthread_mutex_unlock(&lock_table_latch);
								return NULL;
							}
						}
					}
					trxlock = trxlock->Trx_next_lock_ptr;
				}
				pthread_cond_wait(&newlock->Conditional_Variable, &lock_table_latch); // re sleep
			}
			// RE Deadlock Detection

		}
	}
	// entry에 걸고 잘지 말지 판단

	pthread_mutex_unlock(&lock_table_latch);
	return newlock;
}


int lock_release(lock_t* lock_obj){
	pthread_mutex_lock(&lock_table_latch);

	// 시그널 보내서 Shared / Exclusive Lock 부합한 것  깨워주기
	// head tail 포인터 수정  Shared / Exclusive Lock 부합하게

	// cond 파괴
	// free 해주기

	// 해제할 lock이 가장 앞에 있을 경우
	if (lock_obj->Sentinel_pointer->Head == lock_obj){
		if (lock_obj->next_pointer != NULL && lock_obj->next_pointer->Lock_mode == 1){
			lock_obj->next_pointer->iswait = 0;
			//printf("wakeup%d  ", lock_obj->next_pointer->Owner_Transaction_ID);
			pthread_cond_signal(&lock_obj->next_pointer->Conditional_Variable);
		}else{
			if (lock_obj->Lock_mode == 1){
				lock_t* temp;
				temp = lock_obj;
				while (temp->next_pointer != NULL){
					temp = temp->next_pointer;
					if (temp->Lock_mode == 0){
						temp->iswait = 0;
						//printf("wakeup%d  ", temp->Owner_Transaction_ID);
						pthread_cond_signal(&temp->Conditional_Variable);
					}else{
						break;
					}
				}
			}
		}
		lock_obj->Sentinel_pointer->Head = lock_obj->next_pointer;
		if (lock_obj->Sentinel_pointer->Tail == lock_obj){
			lock_obj->Sentinel_pointer->Tail = NULL;
		}else{
			lock_obj->next_pointer->prev_pointer = NULL;
		}
	}else{
		// 해제할 lock이 중간부터 들어 올경우


		lock_t* temp;
		int mode_check = 0;
		temp = lock_obj;
		while (temp != lock_obj->Sentinel_pointer->Head){
			temp = temp->prev_pointer;

			if (temp->Lock_mode == 1){
				mode_check = 1;
				break;
			}
		}


		if (mode_check == 0 && lock_obj->Lock_mode == 1){
			temp = lock_obj;
			while (temp->next_pointer != NULL){
				temp = temp->next_pointer;
				if (temp->Lock_mode == 0){
					temp->iswait = 0;
					//printf("wakeup%d  ", temp->Owner_Transaction_ID);
					pthread_cond_signal(&temp->Conditional_Variable);
				}else{
					break;
				}
			}
		}


		if (mode_check == 0 && lock_obj->next_pointer != NULL){
			if (lock_obj->next_pointer->Lock_mode == 1){
				//printf("fwakeup%d  ", lock_obj->next_pointer->Owner_Transaction_ID);
				pthread_cond_signal(&lock_obj->next_pointer->Conditional_Variable);
			}
		}

		if (mode_check == 1 && lock_obj->next_pointer != NULL){
			//printf("fwakeup%d  ", lock_obj->next_pointer->Owner_Transaction_ID);
			pthread_cond_signal(&lock_obj->next_pointer->Conditional_Variable);
		}

		lock_obj->prev_pointer->next_pointer = lock_obj->next_pointer;
		if (lock_obj->Sentinel_pointer->Tail == lock_obj){
			lock_obj->Sentinel_pointer->Tail = lock_obj->prev_pointer;
		}else{
			lock_obj->next_pointer->prev_pointer = lock_obj->prev_pointer;
		}
	}

	pthread_cond_destroy(&lock_obj->Conditional_Variable);
	//free(lock_obj);

	pthread_mutex_unlock(&lock_table_latch);
	return 0;
}
