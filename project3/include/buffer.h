#ifndef BUFFER_H_
#define BUFFER_H_


#include "file.h"
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




typedef struct Buffer {
	page_t page;
	int table_id;
	uint64_t page_num;
	int8_t is_dirty;
	int8_t is_pinned;
	struct Buffer* next_lru;
	struct Buffer* prev_lru;
} Buffer;

typedef struct LRU {
	Buffer* Head;
	Buffer* Tail;
} LRU;

typedef struct Node {
	uint64_t Pagenum;
	int Tableid;
	Buffer* BufferAddr;
	struct Node *Next;
} Node;

typedef Node * List;

typedef struct HashTable {
    int TableSize;
    List *Table;
} HashTable;


LRU mem_lru;
Buffer *mem_buff;
HashTable *mem_hash_table;


int make_hash(uint64_t pagenumtableid, int table_size);
HashTable * create_hashtable(int table_size);
Node *create_node(uint64_t pnum, int tid, Buffer* bufferaddr);
void value_set(HashTable *hashTable, uint64_t pnum, int tid, Buffer* bufferaddr);
Buffer* get_value(HashTable * hashTable, uint64_t pnum, int tid);
int clean_node(HashTable * hashTable, uint64_t pnum, int tid);
void destroy_node(Node *node);
void destroy_list(List list);
void destroy_hash_table(HashTable *hash);

int init_buffer_hash_lru(int buf_num);
int write_all(int tid);
int shutdown();

int check_file_size(int tid);
pagenum_t buff_alloc_page(int tid);
page_t* buff_read_page(int tid, pagenum_t pagenum);
void buff_write_newpage(int tid, pagenum_t pagenum, page_t* src);
void creat_header_root_buff(int tid, pagenum_t header, page_t* hsrc, pagenum_t root, page_t* rsrc);
void pin_down(int tid, pagenum_t pagenum);
void pin_up(int tid, pagenum_t pagenum);
void dirty_mark(int tid, pagenum_t pagenum);


#endif /* BUFFER_H_ */
