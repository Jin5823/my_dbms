Project2 - Disk-based B+tree
=============
# Disk-based B+tree
이번 과제는 DBMS의 architecture에서 저장장치와 인덱싱 기능을 B+tree로 실제 DB와 유사하게 구현하는 것을 목적으로 두고 있다. 그리고 메모리에서 구현하는 방식이 아닌 디스크에 데이터들이 저장되는 형식을 갖춰야 한다. 이번 과제를 수행하기 위해서는 우선 B+tree의 알고리즘에 대해 심층적인 이해가 필요하며, DBMS에서 파일이 어떻게 구현되고 페이지와 레코드는 어떤 형식과 어떠한 관계를 갖고 있는지 알고 있어야 구조체를 올바르게 만들 수 있다. 또한 디스크에 저장하기 위해 이진파일에서 원하는 위치를 찾고, 읽고 저장을 원활하게 진행할 수 있어야 한다. 특히 이번 과제에서는 다른 사람의 데이터파일도 무리 없이 열람할 수 있기 위해서는 구조체를 명세서와 100%일치하게 만들도록 노력해야한다. 

## Milestone 2
Implement on-disk b+ tree and submit a report(Wiki) including your design.

## 목차
##### 1. 각 Layer 설명
##### 2. 실행 흐름
##### 3. 테스트 방식 

## 1. 각 Layer 설명
* ### 1.1 File Layer
File Layer는 모든 layer 중에서 가장 아래에 있는 layer이며, File Layer 에는 file.c 와 file.h가 포함된다. 디스크에 직접 작성하는 기능을 담당하고 있으며, 상위 layer가 무슨 작업하는 지에는 관심을 갖지 않는다. <U>즉 상위 layer가 LeafPage혹은 InternalPage 그 어떤 걸 요청하던 작성만 해주면 된다.</U> 
#### file.h
##### 변수와 구조체
```c
int fd;
typedef uint64_t pagenum_t;
typedef struct page_t {
	pagenum_t num[512];
}page_t;
```
- int fd 는 파일을 열기 위해 사용되는 변수이다. fd = open()방식으로 사용하게 된다. 작성할 파일이 변경된다면 fd 변수에 어떤 변화가 있을 것으로 예상된다.
- 구조체 page_t는 파일에 작성할 크기를 의미하고 있다. uint64_t는 8바이트의 크기이며, 총 512개 만들어 4096바이트의 크기를 갖고 있다.

#### file.c
##### 1. file_open, file_close, check_size
파일을 열고 닫는 기능을 갖고 있는 기능들과, 파일의 크기를 조회하는 함수들이다.

##### 2. file-page 관련 함수
나머지 4개의 함수는 파일에서 페이지 단위로 작업을 하는 함수들이다. 각각 file_alloc_page(), file_free_page(), file_read_page(), file_write_page()
- file_alloc_page()는 파일에 새로운 페이지를 할당 받는 기능을 갖고 있다. 파일의 가장 끝 단에 4096만큼의 크기의 페이지를 받는다. 여기서 파일의 끝 단은 반드시 4096의 약수의 크기를 맞춰야 하기에 파일의 크기/4096를 하여 페이지의 고유 번호를 받는다. 즉 크기가 4096일 경우 0번 크기가 8192일 경우 1번의 페이지 번호가 할당된다.
- file_free_page() 사용하지 않는 페이지를 비워주는 역할을 한다. 사용자는 변수로 페이지의 번호를 줘야한다.
- file_read_page(), file_write_page()는 파일에 페이지 위치에 쓰고 읽는 작업을 수행한다. 여기서 받는 변수 구조체는 page_t 이다. 포인터로 받고 있기에, 사용자가 다른 구조체를 변수로 입력한다 하더라도, 이를 page_t 의 형태로 받아서 쓰고 읽는다. 전체 프로그램에서 페이지는 모두 같은 크기를 갖고 있기에 문제가 생길 순 없다.





* ### 1.2 b+tree Layer
File Layer를 통해 파일에 <U>무엇을 어떻게 작성하고, 무엇을 어떻게 읽을지</U> 결정하는 layer이다. b+tree가 파일으로부터 주고 받는 정보는 크게 2가지가 있다. 1. 페이지의 위치(번호), 2. 페이지의 내용이며, b+tree는 이 정보를 바탕으로 페이지를 구성하고, 페이지간의 관계도 구성한다.

#### bpt.h
##### 변수와 구조체

```c
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
```
총 4개의 페이지를 구조체로 만들었다. 그리고 LeafPage와 InternalPage에 사용되는 key와 value는 별도로 구조체를 만들었다.
```c
typedef struct KeyPN {
	int64_t Key;
	uint64_t PageNumber;
} KeyPN;

typedef struct KeyValue {
	int64_t Key;
	char Value[120];
} KeyValue;
```
명세에 맞는 위치에 자리를 잡았으며, 편의성을 위해 페이지와 페이지의 번호를 묶어주는 구조체도 만들었다. 
```c
typedef struct LeafPage_num {
	uint64_t pagenum;
	LeafPage leaf;
} LeafPage_num;

typedef struct InternalPage_num {
	uint64_t pagenum;
	InternalPage internal;
} InternalPage_num;
```

- delete를 수행하는데 관련된 함수 리스트
```c
pagenum_t adjust_root(pagenum_t rootnum, InternalPage_num child_p);
pagenum_t adjust_parent(pagenum_t rootnum, LeafPage_num child);
LeafPage_num remove_entry_from_leafpage(LeafPage_num re, KeyValue key_value);
pagenum_t delete_entry( pagenum_t rootnum, LeafPage_num re, KeyValue key_value );
pagenum_t delete(pagenum_t rootnum, int64_t key);
```
- insert를 수행하는데 관련된 함수 리스트
```c
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
```
- 모든 수행 작업에 범용적으로 관련된 함수 리스트 (find 포함)
```c
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
```
#### bpt.c
(주의) spaghetti code와 같이 매우 난잡하며, 추후에 더 깔끔하게 주석을 추가하고 가독성을 높여 수정할 예정
##### 1. 단독 작업과 연관 작업에 따른 함수 분류
b+tree의 실행 흐름 정보는 <2.실행 흐름>에서 확인할 수 있다.                  
b+tree는 크게 입력과 삭제 기능이 있으며, find와 같은 기능은 다른 두 기능들이 모두 필요로 하고 있다. 때문에, b+tree를 구현할때 insert나 delete부터 구현하지 않고, 먼저 공용으로 사용될 find나 header에 기본적으로 필요로 하는 정보들을 우선 구현하는 방향으로 잡았다. 디스크에 새로운 페이지를 만들 때 혹은 페이지를 삭제할 때 root가 바뀔 때 모두 header에 정보를 갱신해줘야 한다.

##### 2. 페이지의 상태 관리
- 한번 디스크에 할당 받은 페이지는 삭제 된다고 해서, 디스크에서 잡은 공간이 없어지는 형식을 구현되지 않고, 빈 페이지라고 명시만 하고, header의 FreePageNumber를 통해 걸어 둔다. 새로운 페이지가 필요 한다면, 디스크에 빈 페이지를 곧바로 만들기 보다는, header에 사용할 수 있는 빈 페이지가 있는지 우선 확인하고, 사용할 수 있는 페이지가 있다면, 디스크에 새로운 공간을 만들지 않고, 페이지를 할 당할 수 있다. 
- 페이지가 비어있는다는 것은 물리적인 것이 아닌 상태적인 것이기에, 비어있는 상태를 유지 하기 위해 해당 페이지를 가리키고 있는 정보들은 모두 없애야 한다. 누군가가 오른쪽 방향으로 페이지를 RightSiblingPN로 가리키는 것과, 부모가 자식을 위에서 아래로 가리키는 방향이 존재 하며, 빈 페이지란, 해당하는 2개의 정보가 모두 없어야 비로서 완전히 빈 페이지라 부를 수 있다. 

<img src="https://raw.githubusercontent.com/Jin5823/Git-Test/master/src/img_18.png" />

- 때문에 리프트 페이지의 공용적으로 필요한 기능은 key를 통해 페이지를 찾는 것과, RightSiblingPN가 아닌 왼쪽의 페이지의 정보를 찾을 수 있어야 한다. 페이지를 비울 때는 페이지의 RightSiblingPN의 위치의 정보가 기존 페이지에 재작성하는 방식으로 간편하게 위치 정보를 업데이트 할 수 있으나, 가장 끝 단에 있는 페이지일 경우 이를 수행 할 수 없기에, 왼쪽 페이지의 정보를 갖을 수 있어야 한다.

<img src="https://raw.githubusercontent.com/Jin5823/Git-Test/master/src/img_19.png" />
          
* ### 1.3 DB Layer
DB Layer는 아직 별 특별한 기능을 갖고 있지 않고 있다. b+tree의 복잡한 구조를 건너서, 간편한 API를 통해 열고 쓰고 읽고 찾는 기능을 호출하는 것을 목적으로 설계한 bpt의 상위 layer이다.
#### db.h, db.c
##### 변수와 구조체
```c
typedef struct unique_table {
	int tableid;
	char pathname[100];
}unique_table;
unique_table filetable[100];
```
- 매번 사용자가 새로운 파일을 오픈하게 될때 그에 해당하는 유니크 번호를 주기 위해 만든 테이블이다. 아직 특별한 기능은 존재 하지 않지만, 향후에 여러 파일에 동시에 쓰고 읽게 하기 위해서 구분하는 정보인 것 같다.
```c
int open_table (char *pathname);
int db_insert (int64_t key, char * value);
int db_find (int64_t key, char * ret_val);
int db_delete (int64_t key);
```
- 간단한 4가지 api를 갖고 있으며, 이는 모두 bpt.c 파일을 통해 수행하게 된다. 삭제하거나 입력할 때 발생하는 에러를 일괄적으로 -1을 리턴하지만, 향후에 해당 layer의 수행해야 할 역할에 따라 수정이 생길 수도 있다. 



* ### 1.4 Main Layer
Main Layer는 일반 유저가 터미널을 통해 주어진 API내에서 DB를 직접 입력해보고 출력해보는 작업을 제공해준다.
#### main.c
##### 터미널 사용방법
```c
printf("Enter any of the following commands after the prompt > :\n"
    "\to <p>  -- Open <p> (a string) as pathname to open file.\n"
    "\ti <k> <v>  -- Insert <k> (an integer) as key and <v> (a string) as value.\n"
    "\tf <k>  -- Find the value under key <k>(an integer).\n"
    "\td <k>  -- Delete key <k>(an integer) and its associated value.\n"
    "\te -- Esc. (Or use Ctl-D.)\n");
```

- o를 입력하고 띄우고 새로운 파일을 열거나 없으면 생성할 <p>디렉토리를 입력한다.
- i를 입력하고 파일에 추가할 키<k>를 입력하고 띄우고 <v>값도 입력한다. 값은 띄어쓰기를 감지하기에, 엔터를 누른다.
- f를 입력하여 키<k>를 찾는다.
- d를 입력하여 <k>키를 파일에서 삭제해준다. 
- e를 입력해 종료한다.



## 2. 실행 흐름
* ### 2.1 insert




