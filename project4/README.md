Project4 - Lock Table
=============
# Lock Table
Lock table을 사용해서 multiple threds를 관리할 수 있다. 관리라는 것은 특정 스레드 condition 을 잠재울 수 있으며, 또 특정 스레드를 깨울 수 있다는 것이다. 이번 프로젝트에서는 lock table에 먼저 lock대기를 한 스레드를 우선 깨우고, 늦은 스레드는 대기를 하게 된다.           

## Requirement
Design your lock table and describe it on hconnect Wiki page. The module should be correctly working with the given test code.
             
## 목차
##### 1. 구조체 설명
##### 2. 기능 함수 설명
##### 3. 동작 흐름 설명

## 1. 구조체 설명
* ### 1.1 Hash 테이블과 Lock 테이블

```c
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
```

- 해시테이블은 위와 같은 구조를 갖고 있으며, 같은 정보를 갖는 hash 값이 생길 경우 chain 해서 공간을 확장하는 방식이다. hash의 식별은 Tableid 와 RecordID이다.                     


```c
if(list->RecordID == key && list->TableID == table_id){
    target = list;
    break;
}
```

- 우선 RecordID으로 `RecordID% table_size` 해시를 만들어주고, 식별을 위해 get_value시 node의 Tableid값을 대상과 비교를 한다. 같은 RecordID임에도 tableid가 다를 경우 chain을 이어가며, 같은 tableid가 나올 때까지 찾는다.


- Tableid 와 RecordID의 해시를 통해 LockTable에 접근하고, 올바른 Node 즉 Hash Table Entry를 찾아낸다. 없을 경우 NULL 값을 반환하고, 이 경우 value_set을 통해 해시 추가해준다.             


* ### 1.2 Lock Object와 Lock List

```c
pthread_mutex_t lock_table_latch = PTHREAD_MUTEX_INITIALIZER;

struct lock_t {
	lock_t* prev_pointer;
	lock_t* next_pointer;
	Node* Sentinel_pointer;
	pthread_cond_t Conditional_Variable;
};

typedef struct lock_t lock_t;
```

- 글로벌 변수로 mutex를 생성해준다. 그리고 각 lock object 마다 pthread_cond_t 변수를 생성하여, mutex lock unlock과 cond wait signal의 동작을 수행할 수 있게끔 한다.

- 과제 명세에 맞게 lock_t 구조체는 링크드 리스트로 서로 연결되어 있으며, Sentinel_pointer으로 Hash table entry인 Node를 가리킨다.                    



## 2. 기능 함수 설명
* ### 2.1 int init_lock_table()

```c
int init_lock_table() {
    mem_hash_table = create_hashtable(10000);
    return 0;
}

HashTable* create_hashtable(int table_size){
    HashTable* hashTable = (HashTable*)malloc(sizeof(HashTable));
    hashTable->Table = (List*)malloc(sizeof(List) * table_size);
    hashTable->TableSize = table_size;
    return hashTable;
}
```

- 해시 테이블을 생성해주는데, 크기는 10000으로 초기화한다.              
                   
                    
* ### 2.2 lock_t* lock_acquire(int table_id, int64_t key)
          
함수는 lock을 획득하는 것인데, 동일한 레코드에 동시에 접속하는 것을 막고, 아래의 기능을 포함하고 있다.
1. 주어진 tableid와 recordid로 해시 조회를 하고, 없을 경우에는 해시의 value를 set해준다. 
2. 새로운 lock object를 생성한다.
3. 생성한 lock object를 hash entry의 링크드 리스트 가장 뒤에 걸어준다.
4. 시작과 끝은 모두 mutex lock unlock을 하여, 데이터를 보호한다.
                    
* ### 2.3 int lock_release(lock_t* lock_obj)
          
함수는 lock을 풀어주는데, 원한는 조건에 맞게 설계하여 풀어줄 수 있다. 이번 프로젝트에서 이 함수는 순수대로 앞에 Head가 가리키는 것을 우선 풀어주고 있고, 아래의 기능을 포함하고 있다.
1. 시그널을 보내 다음 lock obj를 깨워준다.
2. hash entry의 Head와 Tail을 수정해준다.
3. lock obj의 condition을 파괴해준다.
4. 그리고 lock obj가 잡고 있던 메모리를 free해준다. 
5. 시작과 끝은 모두 mutex lock unlock을 하여, 데이터를 보호한다.
                    

## 3. 동작 흐름 설명
* ### 3.1 test code flow

테스트코드를 실행하게 되면, 여러 스레드가 동일한 레코드를 사용하고 변경하게 되는 경우가 생기는데, mutex만 사용하는 방식은 다른 스레드가 수정 못하게 막을 수는 있지만, 대기열을 사용하여 순서를 부여할 수는 없다. 
           
           
레코드를 사용하고 변경시 lock acquire는 일종의 대기열에 들어간다고 생각된다. 앞줄에 lock obj가 있으면 대기열에서 대기를 계속 하고, 링크드 리스트의 가장 앞줄일 경우 레코드 사용 변경 작업을 하고 대기열을 빠지고, 뒤에 lock obj을 가장 앞줄로 만든다.
             
                
- 결과 화면
<img src="https://raw.githubusercontent.com/Jin5823/Git-Test/master/src/img_23.png" />
