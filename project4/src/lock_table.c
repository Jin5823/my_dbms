#include "lock_table.h"




pthread_mutex_t lock_table_latch = PTHREAD_MUTEX_INITIALIZER;

struct lock_t {
	lock_t* prev_pointer;
	lock_t* next_pointer;
	Node* Sentinel_pointer;
	pthread_cond_t Conditional_Variable;
};

typedef struct lock_t lock_t;


int make_hash(int64_t key, int table_size){
	return key % table_size;
}

HashTable* create_hashtable(int table_size){
	HashTable* hashTable = (HashTable*)malloc(sizeof(HashTable));
	hashTable->Table = (List*)malloc(sizeof(List) * table_size);
	hashTable->TableSize = table_size;
	return hashTable;
}

Node* create_node(int table_id, int64_t key){
	Node* newNode = (Node*)malloc(sizeof(Node));
	newNode->TableID = table_id;
	newNode->RecordID = key;
	newNode->Head = NULL;
	newNode->Tail = NULL;
	newNode->Next = NULL;
	return newNode;
}

void value_set(HashTable* hashTable, int table_id, int64_t key){
	int address = make_hash(key, hashTable->TableSize);
	Node* newNode = create_node(table_id, key);
	if(hashTable->Table[address] == NULL){
		hashTable->Table[address] = newNode;
	}else{
		List list = hashTable->Table[address];
		newNode->Next = list;
		hashTable->Table[address] = newNode;
    }
}

Node* get_value(HashTable* hashTable, int table_id, int64_t key){
	int address = make_hash(key, hashTable->TableSize);
	List list = hashTable->Table[address];
	List target = NULL;
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

// 사용안함
int clean_node(HashTable* hashTable, int table_id, int64_t key){
	int address = make_hash(key, hashTable->TableSize);
	List list = hashTable->Table[address];
	List target = NULL;
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

// 사용안함
void destroy_node(Node *node){
	free(node);
}

void destroy_list(List list){
	if(list == NULL){
        return;
    }
    if(list->Next != NULL){
    	destroy_list(list->Next);
    }
    destroy_node(list);
}

void destroy_hash_table(HashTable *hash){
	for(int i = 0 ; i < hash->TableSize ; i ++){
		List list = hash->Table[i];
		destroy_list(list);
	}
}


int
init_lock_table()
{
	/* DO IMPLEMENT YOUR ART !!!!! */
	mem_hash_table = create_hashtable(10000);
	return 0;
}

lock_t*
lock_acquire(int table_id, int64_t key)
{
	/* ENJOY CODING !!!! */
	pthread_mutex_lock(&lock_table_latch);
	Node* entry;
	entry = get_value(mem_hash_table, table_id, key);

	if (entry == NULL){
		value_set(mem_hash_table, table_id, key);
		entry = get_value(mem_hash_table, table_id, key);
	}
	// hash 에 없을 경우 해시를 만들어준다

	lock_t* newlock = (lock_t*)malloc(sizeof(lock_t));
	pthread_cond_init(&newlock->Conditional_Variable, NULL);
	newlock->Sentinel_pointer = entry;
	newlock->next_pointer = NULL;
	newlock->prev_pointer = entry->Tail;
	// 새로운 lock을 만든다

	entry->Tail = newlock;
	if (entry->Head == NULL){
		entry->Head = newlock;
	}else{
		newlock->prev_pointer->next_pointer = newlock;
		pthread_cond_wait(&newlock->Conditional_Variable, &lock_table_latch);
	}
	// entry에 걸어주기

	pthread_mutex_unlock(&lock_table_latch);
	return newlock;
}

int
lock_release(lock_t* lock_obj)
{
	/* GOOD LUCK !!! */
	pthread_mutex_lock(&lock_table_latch);

	// 시그널 보내서 깨워주기
	// head tail 포인터 수정
	// cond 파괴
	// free 해주기

	if (lock_obj->next_pointer != NULL){
		pthread_cond_signal(&lock_obj->next_pointer->Conditional_Variable);
	}

	lock_obj->Sentinel_pointer->Head = lock_obj->next_pointer;
	if (lock_obj->Sentinel_pointer->Tail == lock_obj){
		lock_obj->Sentinel_pointer->Tail = lock_obj->next_pointer;
	}

	pthread_cond_destroy(&lock_obj->Conditional_Variable);
	free(lock_obj);

	pthread_mutex_unlock(&lock_table_latch);
	return 0;
}
