#ifndef LOCK_H_a
#define LOCK_H_


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





typedef struct lock_t {
	struct lock_t* prev_pointer;
	struct lock_t* next_pointer;
	struct LockNode* Sentinel_pointer;
	pthread_cond_t Conditional_Variable;
	int Lock_mode;
	struct lock_t* Trx_next_lock_ptr;
	int Owner_Transaction_ID;

	int iswait;
	int tabid;
	uint64_t pagen;
	int64_t key_history;
	char value_history[120];
}lock_t;
// typedef struct lock_t lock_t;

typedef struct LockNode {
	int TableID;
	int64_t RecordID;
	lock_t* Head;
	lock_t* Tail;
	struct LockNode *Next;

	int isdead;
} LockNode;

typedef LockNode * LockList;

typedef struct LockHashTable {
    int TableSize;
    LockList *Table;
} LockHashTable;

LockHashTable *lock_hash_table;
LockHashTable *trx_hash_table;


int lock_make_hash(int64_t key, int table_size);
LockHashTable * lock_create_hashtable(int table_size);
LockNode *lock_create_node(int table_id, int64_t key);
void lock_value_set(LockHashTable *hashTable, int table_id, int64_t key);
LockNode* lock_get_value(LockHashTable * hashTable, int table_id, int64_t key);
int lock_clean_node(LockHashTable * hashTable, int table_id, int64_t key);
void lock_destroy_node(LockNode *node);
void lock_destroy_list(LockList list);
void lock_destroy_hash_table(LockHashTable *hash);

int dead_recursion(int trx_id, int track);
/* APIs for lock table */
int init_lock_table();
lock_t* lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode);
int lock_release(lock_t* lock_obj);


#endif /* LOCK_H_ */
