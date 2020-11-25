#ifndef __LOCK_TABLE_H__
#define __LOCK_TABLE_H__


#include <stdlib.h>
#include <stdint.h>
#include <search.h>
#include <pthread.h>




typedef struct lock_t lock_t;

typedef struct Node {
	int TableID;
	int64_t RecordID;
	lock_t* Head;
	lock_t* Tail;
	struct Node *Next;
} Node;

typedef Node * List;

typedef struct HashTable {
    int TableSize;
    List *Table;
} HashTable;

HashTable *mem_hash_table;


int make_hash(int64_t key, int table_size);
HashTable * create_hashtable(int table_size);
Node *create_node(int table_id, int64_t key);
void value_set(HashTable *hashTable, int table_id, int64_t key);
Node* get_value(HashTable * hashTable, int table_id, int64_t key);
int clean_node(HashTable * hashTable, int table_id, int64_t key);
void destroy_node(Node *node);
void destroy_list(List list);
void destroy_hash_table(HashTable *hash);

/* APIs for lock table */
int init_lock_table();
lock_t* lock_acquire(int table_id, int64_t key);
int lock_release(lock_t* lock_obj);

#endif /* __LOCK_TABLE_H__ */
