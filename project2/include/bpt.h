#ifndef BPT_H_
#define BPT_H_


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




#define LEAFPAGE_ORDER 32
#define INTERNAL_ORDER 249


typedef struct KeyPN {
	int64_t Key;
	uint64_t PageNumber;
} KeyPN;

typedef struct KeyValue {
	int64_t Key;
	char Value[120];
} KeyValue;

typedef struct HeaderPage {
	uint64_t FreePageNumber;
	uint64_t RootPageNumber;
	uint64_t NumberOfPages;
	uint64_t Reserved[509];
} HeaderPage;

typedef struct FreePage {
	uint64_t NextFreePageNumber;
	uint64_t Reserved[511];
} FreePage;

typedef struct InternalPage {
	uint64_t ParentPageNumber;
	uint32_t IsLeaf;
	uint32_t NumberOfKeys;
	uint64_t Reserved[13];
	uint64_t OneMorePN;
	KeyPN InternalKeyPN[248];
} InternalPage;

typedef struct LeafPage {
	uint64_t ParentPageNumber;
	uint32_t IsLeaf;
	uint32_t NumberOfKeys;
	uint64_t Reserved[13];
	uint64_t RightSiblingPN;
	KeyValue LeafKeyValue[31];
} LeafPage;

typedef struct LeafPage_num {
	uint64_t pagenum;
	LeafPage leaf;
} LeafPage_num;

typedef struct InternalPage_num {
	uint64_t pagenum;
	InternalPage internal;
} InternalPage_num;


pagenum_t adjust_root(pagenum_t rootnum, InternalPage_num child_p);
pagenum_t adjust_parent(pagenum_t rootnum, LeafPage_num child);
LeafPage_num remove_entry_from_leafpage(LeafPage_num re, KeyValue key_value);
pagenum_t delete_entry( pagenum_t rootnum, LeafPage_num re, KeyValue key_value );
pagenum_t delete(pagenum_t rootnum, int64_t key);

LeafPage_num find_friend( pagenum_t rootnum, pagenum_t ppn, InternalPage parent);
LeafPage_num find_leaf( pagenum_t rootnum, int64_t key );
KeyValue * find( pagenum_t rootnum, int64_t key );

int cut( int length );
void update_free_page (pagenum_t fpnum);
void update_header (pagenum_t fpnum, pagenum_t rpnum, pagenum_t numop);
void update_child (InternalPage new_parent, pagenum_t new_parent_num);
void usage( void );
int is_disk_empty();
pagenum_t read_header();
pagenum_t alloc_page();

LeafPage insert_into_leaf( LeafPage_num re, int64_t key, char * value );
pagenum_t creat_header_root();
pagenum_t insert( pagenum_t rootnum, int64_t key, char * value );
pagenum_t insert_into_leaf_after_splitting( pagenum_t rootnum, LeafPage_num re, int64_t key, char * value);

pagenum_t insert_into_new_root(LeafPage_num left, int64_t key, LeafPage_num right);
pagenum_t insert_into_parent( pagenum_t rootnum, LeafPage_num left, int64_t key, LeafPage_num right);
pagenum_t insert_into_node( pagenum_t rootnum, InternalPage_num parent, LeafPage_num left, int64_t key, LeafPage_num right);
pagenum_t insert_into_node_after_splitting(pagenum_t rootnum, InternalPage_num parent, LeafPage_num left, int64_t key, LeafPage_num right);
pagenum_t insert_internal_into_parent( pagenum_t rootnum, InternalPage_num old_p, int64_t key, InternalPage_num new_p);
pagenum_t insert_internal_into_new_root(InternalPage_num left, int64_t key, InternalPage_num right);
pagenum_t insert_internal_into_node( pagenum_t rootnum, InternalPage_num parent, InternalPage_num left, int64_t key, InternalPage_num right);
pagenum_t insert_internal_into_node_after_splitting(pagenum_t rootnum, InternalPage_num parent, InternalPage_num left, int64_t key, InternalPage_num right);


#endif /* BPT_H_ */
