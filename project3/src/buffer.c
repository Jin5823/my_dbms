#include "buffer.h"




int write_all(int tid){
	Buffer * t;
	t = mem_lru.Head;
	while (t != NULL){
		if (t->table_id == tid && t->is_dirty == 1)
			file_write_page(tid, t->page_num, &t->page);
		t = t->next_lru;
	}
	return 0;
}

int shutdown(){
	Buffer * t;
	t = mem_lru.Head;
	while (t != NULL){
		if (t->is_dirty == 1)
			file_write_page(t->table_id, t->page_num, &t->page);
		t = t->next_lru;
	}
	destroy_hash_table(mem_hash_table);
	free(mem_buff);
	return 0;
}

int make_hash(uint64_t pagenumtableid, int table_size){
	return pagenumtableid % table_size;
}

HashTable * create_hashtable(int table_size){
	HashTable *hashTable = (HashTable*)malloc(sizeof(HashTable));
	hashTable->Table = (List*)malloc(sizeof(List) * table_size);
	hashTable->TableSize = table_size;
	return hashTable;
}

Node *create_node(uint64_t pnum, int tid, Buffer* bufferaddr){
	Node * newNode = (Node*)malloc(sizeof(Node));
	newNode->Pagenum = pnum;
	newNode->Tableid = tid;
	newNode->BufferAddr = (Buffer*)malloc(sizeof(Buffer));
	newNode->BufferAddr = bufferaddr;
	newNode->Next = NULL;
	return newNode;
}

void value_set(HashTable *hashTable, uint64_t pnum, int tid, Buffer* bufferaddr){
	int address = make_hash(pnum, hashTable->TableSize);
	Node* newNode = create_node(pnum, tid, bufferaddr);
	if(hashTable->Table[address] == NULL){
		hashTable->Table[address] = newNode;
	}else{
		List list = hashTable->Table[address];
		newNode->Next = list;
		hashTable->Table[address] = newNode;
    }
}

Buffer* get_value(HashTable * hashTable, uint64_t pnum, int tid){
	int address = make_hash(pnum, hashTable->TableSize);
	List list = hashTable->Table[address];
	List target = NULL;
	if(list == NULL) return NULL;
	while(1){
		if(list->Pagenum == pnum && list->Tableid==tid){
			target = list;
			break;
		}
		if(list->Next == NULL) return NULL;

		else{
			list = list->Next;
		}
	}
	return target->BufferAddr;
}

int clean_node(HashTable * hashTable, uint64_t pnum, int tid){
	int address = make_hash(pnum, hashTable->TableSize);
	List list = hashTable->Table[address];
	List target = NULL;
	if(list == NULL) return -1;
	while(1){
		if(list->Pagenum == pnum && list->Tableid==tid){
			target = list;
			break;
		}
		if(list->Next == NULL) return -1;
		else{
			list = list->Next;
		}
	}
	target->BufferAddr = NULL;
	return 0;
}


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

int init_buffer_hash_lru(int buf_num){
	if (buf_num < 2) return -1;
	mem_buff = malloc(sizeof(Buffer) * buf_num);
	if (mem_buff == NULL) return -1;
	for (int i = 0; i < (buf_num-1); i++){
		mem_buff[i].next_lru = &mem_buff[i+1];
		mem_buff[i+1].prev_lru = &mem_buff[i];
		mem_buff[i].table_id = 0;
		mem_buff[i+1].table_id = 0;
		mem_buff[i].is_dirty = 0;
		mem_buff[i+1].is_dirty = 0;
	}
	mem_hash_table = create_hashtable(10000*buf_num);
	mem_lru.Tail = &mem_buff[buf_num-1];
	mem_lru.Head = &mem_buff[0];

	return 0;
}

int check_file_size(int tid){
	return check_size(tid);
}

pagenum_t buff_alloc_page(int tid) {
	return file_alloc_page(tid);
}

page_t* buff_read_page(int tid, pagenum_t pagenum) {
	Buffer *page_frame;
	page_frame = get_value(mem_hash_table, pagenum, tid);
	if (page_frame == NULL){
		// 해시로 못찾았을 경우 즉 버퍼에 없음
		// 버퍼에 올려주는 작업이 필요
		// lru를 뒤에서부터 빈공간 찾기
		if (mem_lru.Tail->table_id == 0){
			// lru의 tail이 가리키는 버퍼가 비어 있을 경우, 비어있기에 pin 등 체크할 필요 없음
			page_frame = mem_lru.Tail;
			page_frame ->prev_lru->next_lru = page_frame->next_lru;
			mem_lru.Tail = page_frame->prev_lru;

			page_frame->prev_lru = mem_lru.Head->prev_lru;
			mem_lru.Head->prev_lru = page_frame;
			page_frame->next_lru = mem_lru.Head;
			mem_lru.Head = page_frame;
		}else{
			// lru의 tail이 가리키는 버퍼에 무언가 들어 있을 경우 pin 체크 dirty 체크
			page_frame = mem_lru.Tail;
			while (page_frame->is_pinned != 0){
				page_frame = page_frame->prev_lru;
				if (page_frame == NULL){
					page_frame = mem_lru.Tail;
				}
				// unpin 된거 찾을 때까지 무한 반복
			}
			if (page_frame->is_dirty != 0){
				// dirty 일경우 우선 써 내려준다
				file_write_page(page_frame->table_id, page_frame->page_num, &page_frame->page);
			}

			if (page_frame != mem_lru.Head){
				page_frame->prev_lru->next_lru = page_frame->next_lru;
				if (page_frame->next_lru != NULL)
					page_frame->next_lru->prev_lru = page_frame->prev_lru;
				if (page_frame == mem_lru.Tail)
					mem_lru.Tail = page_frame->prev_lru;
				page_frame->prev_lru = mem_lru.Head->prev_lru;
				mem_lru.Head->prev_lru = page_frame;
				page_frame->next_lru = mem_lru.Head;
				mem_lru.Head = page_frame;
			}

			clean_node(mem_hash_table, page_frame->page_num, page_frame->table_id);
		}

		file_read_page(tid, pagenum, &page_frame->page);
	}else{
		if (page_frame == mem_lru.Tail){
			page_frame->prev_lru->next_lru = page_frame->next_lru;
			mem_lru.Tail = page_frame->prev_lru;

			page_frame->prev_lru = mem_lru.Head->prev_lru;
			mem_lru.Head->prev_lru = page_frame;
			page_frame->next_lru = mem_lru.Head;
			mem_lru.Head = page_frame;
		}
		if (page_frame != mem_lru.Head && page_frame != mem_lru.Tail){
			page_frame->prev_lru->next_lru = page_frame->next_lru;
			page_frame->next_lru->prev_lru = page_frame->prev_lru;

			page_frame->prev_lru = mem_lru.Head->prev_lru;
			mem_lru.Head->prev_lru = page_frame;
			page_frame->next_lru = mem_lru.Head;
			mem_lru.Head = page_frame;
		}

		//if (page_frame->is_pinned != 0)
			// 하지만 읽을때라서 누군가 읽고 있어도 무관

		if (page_frame->is_dirty != 0){
			// dirty 일경우 우선 써 내려준다
			while (page_frame->is_pinned != 0){
				// pin 되어 있을 경우 무한 반복 대기
			}
			file_write_page(page_frame->table_id, page_frame->page_num, &page_frame->page);
		}
	}

	value_set(mem_hash_table, pagenum, tid, page_frame);
	page_frame->is_pinned = page_frame->is_pinned +1;
	page_frame->is_dirty = 0;
	page_frame->page_num = pagenum;
	page_frame->table_id = tid;

	return &page_frame->page;
}


void buff_write_newpage(int tid, pagenum_t pagenum, page_t* src) {
	Buffer *page_frame;
	page_frame = get_value(mem_hash_table, pagenum, tid);
	if (page_frame == NULL){
		if (mem_lru.Tail->table_id == 0){
			page_frame = mem_lru.Tail;
			page_frame ->prev_lru->next_lru = page_frame->next_lru;
			mem_lru.Tail = page_frame->prev_lru;

			page_frame->prev_lru = mem_lru.Head->prev_lru;
			mem_lru.Head->prev_lru = page_frame;
			page_frame->next_lru = mem_lru.Head;
			mem_lru.Head = page_frame;
		}else{
			page_frame = mem_lru.Tail;
			while (page_frame->is_pinned != 0){
				page_frame = page_frame->prev_lru;
				if (page_frame == NULL){
					page_frame = mem_lru.Tail;
				}
			}
			if (page_frame->is_dirty != 0){
				file_write_page(page_frame->table_id, page_frame->page_num, &page_frame->page);
			}

			if (page_frame != mem_lru.Head){
				page_frame->prev_lru->next_lru = page_frame->next_lru;
				if (page_frame->next_lru != NULL)
					page_frame->next_lru->prev_lru = page_frame->prev_lru;
				if (page_frame == mem_lru.Tail)
					mem_lru.Tail = page_frame->prev_lru;
				page_frame->prev_lru = mem_lru.Head->prev_lru;
				mem_lru.Head->prev_lru = page_frame;
				page_frame->next_lru = mem_lru.Head;
				mem_lru.Head = page_frame;
			}
			clean_node(mem_hash_table, page_frame->page_num, page_frame->table_id);
		}
	}else{
		if (page_frame == mem_lru.Tail){
			page_frame->prev_lru->next_lru = page_frame->next_lru;
			mem_lru.Tail = page_frame->prev_lru;

			page_frame->prev_lru = mem_lru.Head->prev_lru;
			mem_lru.Head->prev_lru = page_frame;
			page_frame->next_lru = mem_lru.Head;
			mem_lru.Head = page_frame;
		}
		if (page_frame != mem_lru.Head && page_frame != mem_lru.Tail){
			page_frame->prev_lru->next_lru = page_frame->next_lru;
			page_frame->next_lru->prev_lru = page_frame->prev_lru;

			page_frame->prev_lru = mem_lru.Head->prev_lru;
			mem_lru.Head->prev_lru = page_frame;
			page_frame->next_lru = mem_lru.Head;
			mem_lru.Head = page_frame;
		}
		while (page_frame->is_pinned !=0){
			// printf("309 over pin write\n");
			// 쓰려하는데 누가 읽고 있는 경우
		}
		if (page_frame->is_dirty != 0){
			// dirty 일경우 우선 써 내려준다
			while (page_frame->is_pinned != 0){
				// pin 되어 있을 경우 무한 반복 대기
			}
			file_write_page(page_frame->table_id, page_frame->page_num, &page_frame->page);
		}
	}

	page_frame->is_pinned = page_frame->is_pinned +1;
	page_frame->page = *src;
	value_set(mem_hash_table, pagenum, tid, page_frame);
	page_frame->is_dirty = 1;
	page_frame->page_num = pagenum;
	page_frame->table_id = tid;
	page_frame->is_pinned = page_frame->is_pinned -1;
}

void pin_down(int tid, pagenum_t pagenum){
	Buffer *page_frame;
	page_frame = get_value(mem_hash_table, pagenum, tid);
	if (page_frame != NULL){
		page_frame->is_pinned = page_frame->is_pinned -1;
	}
}

void pin_up(int tid, pagenum_t pagenum){
	Buffer *page_frame;
	page_frame = get_value(mem_hash_table, pagenum, tid);
	if (page_frame != NULL){
		page_frame->is_pinned = page_frame->is_pinned + 1;
	}
}

void dirty_mark(int tid, pagenum_t pagenum){
	Buffer *page_frame;
	page_frame = get_value(mem_hash_table, pagenum, tid);
	if (page_frame != NULL){
		while (page_frame->is_pinned > 1){
		}
		page_frame->is_dirty = 1;
	}
}

void creat_header_root_buff(int tid, pagenum_t header, page_t* hsrc, pagenum_t root, page_t* rsrc){
	file_write_page(tid, header, hsrc);
	file_write_page(tid, root, rsrc);
}
