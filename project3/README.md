Project3 - DBMS Included Buffer Management
=============
# Buffer Management 
DBMS는 디스크에 읽고 쓰는 작업을 관리하는 Disk Space Management가 있다. Project2에서는 디스크에 어떻게, 무엇을 작성하는지 관리하는 Files and Index Management를 구현했다. 이번 과제로 구현한 Buffer Management는 새로운 기능을 추가한 다기 보다는 Files and Index Management의 성능을 향상 시키고 DBMS에서 여러명이 동시 작업을 할 수 있게 관리 layer를 구현했다.

## Requirement
Implement in-memory buffer manager and submit a report(Wiki) including your design

## 목차
##### 1. 현재 layer상태
##### 2. 버퍼와 LRU
##### 3. 변경된 Files and Index Management

## 1. 현재 layer상태
* ### 1.1 각 Layer가 수행하는 작업


<img src="https://raw.githubusercontent.com/Jin5823/Git-Test/master/src/img_20.png" />


db를 통해 DB를 사용할 수 있는 간편한 API를 호출할 수 있다. db layer에 있는 함수들은 간단한 기능만 하는 것 같지만, 아래 layer의 많은 함수들을 연달아서 호출하면서 원하는 작업을 수행한다.                
db가 일반적으로 유저에게 받는 정보는 크게 두가지로 나뉠 수 있다 하나는 DB의 상태를 설정하는 것이고, 다른 하는 본격적으로 DB에 작업을 하는 insert find delete 기능이 있다. insert find delete를 호출하게 되면, 가장 먼저 Files and Index Management를 통해 페이지의 위치를 탐색하게 되는데 이 작업은 bpt 에서 수행하게 된다. bpt에서는 디스크에 있는 페이지를 접근하지 않고, 단지 디스크에 있는 페이지를 메모리에 올려줄 것을 요청하는 작업만 수행한다. 메모리에 올리는 동작은 buffer에서 수행한다. 

## 2. 버퍼와 LRU
* ### 2.1 버퍼 그리고 해시테이블 

<img src="https://raw.githubusercontent.com/Jin5823/Git-Test/master/src/img_21.png" />



```c
typedef struct Buffer {
	page_t page;
	int table_id;
	uint64_t page_num;
	int8_t is_dirty;
	int8_t is_pinned;
	struct Buffer* next_lru;
	struct Buffer* prev_lru;
} Buffer;
```

- 버퍼의 구조는 위와 같다. 버퍼를 식별할 수 있는 정보는 page_num 과 table_id이며 버퍼의 상태 정보는 is_dirty, is_pinned이 있다. 그리고 저장하고 있는 메인 정보인 페이지는 page에 저장되어 있다. 또한 buffer는 LRU에서 링크드 리스트의 동작을 위해 다음 lru 의 주소 정보와 이전 lru의 주소정보를 갖고 있다. 


```c
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
```

- 해시테이블은 위와 같은 구조를 갖고 있으며, 같은 정보를 갖는 hash 값이 생길 경우 chain 해서 공간을 확장하는 방식이다. hash의 식별은 Tableid 와 Pagenum이다. 우선 pagenum으로 `pagenum % table_size` 해시를 만들어주고, 식별을 위해 get_value시 node의 Tableid값을 대상과 비교를 한다. 같은 pagenum임에도 tableid가 다를 경우 chain을 이어가며, 같은 tableid가 나올 때까지 찾는다.


* ### 2.1 LRU, read write 

```c
typedef struct LRU {
	Buffer* Head;
	Buffer* Tail;
} LRU;
```

- LRU는 가장 최근에 사용한 page의 주소를 Head에 저장하고, 사용한지 가장 오래된 page를 Tail에 가리킨다. Buffer들은 서로 링크드 리스크로 연결되어 있는 관계로, Head를 알게되면 Next를 탐색하면서 끝까지 찾을 수도 있다. 그렇기에 LRU에서 위치가 변동할 경우 해당 buffer가 prev,next 가리키는 모든 정보를 끊어주고, 만약 buffer의 prev와 next가 있다면, 서로를 이어 줘야 한다. 

- LRU는 버퍼에 값이 변동 될 때마다 변경되어야 한다. 가장 대표적으로 LRU 동작이 수행되는 함수는 buff_read_page 와 buff_write_newpage이다.


- buff_read_page는 요청 받은 페이지를 file layer를 통해 읽고 버퍼에 저장해준다. 여기서 버퍼는 크게 3가지 상태가 있을 수 있다.
1. 처음에는 비어 있는 상태이다. 그렇기에 비어있는 버퍼에 곧장 file에서 읽은 페이지를 저장한다.
2. 두 번째는 요청받은 페이지가 저장된 버퍼가 없어서, 사용한지 가장 오래된, LRU에서 Tail이 가리키는 버퍼에 file을 통해 읽는 페이지를 저장해야 한다. 이때 두 가지 선택을 해야 하는데, 절대 사용중인 즉 pinned가 1이상은 아닌 버퍼이어야한다. 그렇기에 tail에서 가리키는 버퍼의 pin이 1이상일 경우 prev 를 통해 pinned 0인 버퍼를 찾는다.
3. 마지막은 요청받은 페이지가 버퍼에 저장되어 있을 경우다. 하지만 버퍼에 해당 페이지의 pin이 1이상일 경우 무한대기 상태에 들어간다. 누군 가가 사용을 멈출 시 사용하게 된다.           
그리고 2번 3번에서 만약 해당 페이지가 dirty 페이지이고 unpin일 경우 반드시 file layer를 통해 디스크에 write해줘야 한다.         
              
buff_write_newpage도 위와 유사하고, 다이어그램으로 표시하면 아래와 같다.
          
<img src="https://raw.githubusercontent.com/Jin5823/Git-Test/master/src/img_22.png" />



        
테스트 해본 결과 10000개 이하의 데이터를 insert 할 경우 2개의 버퍼로도 소화가 가능했으나 이상부터는 3개 이상의 버퍼가 요구된다. 페이지가 많은 DB를 관리하는데 buffer가 소요되지는 않지만 tree의 구조가 변결될 때 버퍼가 동시에 많이 사용하게 된다. 그렇기에 insert 뿐만 아니라 delete도 tree 구조를 변경 시키기에 버퍼가 동시에 많이 사용되는 작업이 될 수 있다. 


## 3. 변경된 Files and Index Management
* ### 3.1 page의 저장 위치

Project 2 때도 page는 디스크에 저장되어 있을 뿐 관리는 메모리에 올려서 관리하는 방식이었다. 하지만 이의 단점은 지정된 수량과 필요 시에 만 메모리에 올린다는 것이다. buff는 메모리의 어느 위치에 디스크에 저장된 혹은 저장할 정보가 담겨있다. 
        
Files and Index Management 는 이러한 상황에 맞게 버퍼 위치에 저장된 페이지를 가리키는 포인터로만 정보를 읽고 작업을 해야 한다.


```c
void pin_down(int tid, pagenum_t pagenum){
	Buffer *page_frame;
	page_frame = get_value(mem_hash_table, pagenum, tid);
	if (page_frame != NULL){
		page_frame->is_pinned = page_frame->is_pinned -1;
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

```

- 버퍼를 통해 페이지를 읽을 시 pin은 자동으로 올리게 되어있다. 하지만 pin을 내리고 dirty 마킹은 작업마다 다르기에, Files and Index Management 에서 조건에 맞게 포인터가 가리키는 값을 변경하기 직전에 dirty 마킹을 해주고, 더 이상 해당 포인터를 사용하지 않을 시 pin을 내려 주면 된다.

- pin을 내려주는 작업은 find 에서 꼭 해야 한다. 그리고, 마지막으로 찾은 leafpage를 가리키는 포인터의 pin 은 작업이 완료된 후 내려 준다. 

- insert 시 만약 splitting을 하지 않는다면, 하나의 dirty pinned 페이지를 사용하게 되고, splitting을 하고 부모가 root가 아닐 경우 최대 4개의 dirty pinned 페이지를 사용하게 된다. 

