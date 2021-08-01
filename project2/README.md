Project2 - Disk-based B+tree
=============
# Disk-based B+tree
이번 과제는 DBMS의 architecture에서 저장장치와 인덱싱 기능을 B+tree로 실제 DB와 유사하게 구현하는 것을 목적으로 두고 있다. 그리고 메모리에서 구현하는 방식이 아닌 디스크에 데이터들이 저장되는 형식을 갖춰야 한다. 이번 과제를 수행하기 위해서는 우선 B+tree의 알고리즘에 대해 심층적인 이해가 필요하며, DBMS에서 파일이 어떻게 구현되고 페이지와 레코드는 어떤 형식과 어떠한 관계를 갖고 있는지 알고 있어야 구조체를 올바르게 만들 수 있다. 또한 디스크에 저장하기 위해 이진파일에서 원하는 위치를 찾고, 읽고 저장을 원활하게 진행할 수 있어야 한다. 특히 이번 과제에서는 다른 사람의 데이터파일도 무리 없이 열람할 수 있기 위해서는 구조체를 명세서와 100%일치하게 만들도록 노력해야한다. 

## 1. Milestone 1
Analyze the given b+ tree code and submit a report to the hconnect Wiki.

### 1.1 Possible call path of the insert/delete operation
* #### insert
    insert operation에는 3가지 case가 존재 한다. 
	1. 첫 번째 node가 생성인 경우에는 트리에서 root node로 사용한다.  (start_new_tree)
	2. leaf node에 공간이 충분할 경우, 즉 key의 개수가 지정한 공간의 크기인 order-1보다 작을 시  (insert_into_leaf)
	3. leaf node에 공간이 충분하지 않을 경우, 하나의 node를 두 개로 나눠주는 작업  (insert_into_node_after_splitting)     
                            
                   
- node의 구조체는 아래와 같다.    
<pre>
<code>typedef struct node {
    void ** pointers;
    int * keys;
    struct node * parent;
    bool is_leaf;
    int num_keys;
    struct node * next; // Used for queue.
} node;
</code>
</pre>
      
* leaf node일 경우 pointers는 record의 주소를 가리키고 있고, 입력된 key는 최대 order-1 만큼 담을 수 있다. 반면 pointers는 keys 보다 하나 더 담을 수 있는데, 이것은 옆으로 가는 node의 주소를 담기 위한 용도이다. parent는 부모의 노드를 가리키고, num_keys는 node가 담고 있는 key의 총 개수이다. is_leaf로 leaf여부를 판단한다.   
       
* leaf node가 아닐 경우 pointers는 record 주소가 아닌 자식 node를 가리킨다. 이 경우 key 값은 가리키는 자식 node에 있을 수도 없을 수도 있으며, index 용도로 사용된다.     
         
* insert/delete operation은 모두 find operation를 가장 우선 사용한다. 올바른 leaf node를 찾고, 이를 통해 record를 찾는다. 찾을 시 사용하는 비교 값이 key 값이다.    
      
<img src="https://raw.githubusercontent.com/Jin5823/Git-Test/master/src/img_14.svg" />
    
* #### delete 
    delete operation에는 3가지 case가 존재 한다. 
	1. root에서 삭제를 할 경우, 간단하게 삭제만 진행한다.
	2. node에서 삭제를 했으나, node가 갖고 있어야 할 최소한의 record수의 기준에 적합한 경우, 간단하게 삭제만 진행한다.
	3. node가 갖고 있어야 할 최소한의 record수의 기준에 미달일 경우, 병합 혹은 재분배를 이웃의 크기에 따라 결정한다.
                            
                   
- delete에서 1, 2 번과 같이 단순히 삭제를 할 것인지, 3번처럼 삭제 후 부분적으로 tree에 포함된 node들의 연결 구조를 병합, 분배할 지는 아래의 조건에 따라 결정된다.    
<pre>
<code>min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

if (n->num_keys >= min_keys)
    return root;
</code>
</pre>
          
* delete node시 주의할 점은 free를 통해 기존에 차지하던 공간을 비워줘야 한다.       
         
* delete operation에서 가장 복잡하고 중요한 부분은 coalesce_nodes, redistribute_nodes이다. leaf node가 가리키고 있는 부모의 같은 자식의 leaf node인 이웃 노드와 병합, 재 분배를 통해, 다음 search시 더 효율적으로 찾을 수 있다. 상세한 것은 1.2에서 설명된다.       
       
<img src="https://raw.githubusercontent.com/Jin5823/Git-Test/master/src/img_15.svg" />
        
### 1.2 Detail flow of the structure modification (split, merge)

* ##### coalesce
부모의 두 자식 노드가 병합 가능한 조건은, 두 노드의 key가 한 노드가 갖고 있을 수 있는 key보다 적을 경우이다.     
<img src="https://raw.githubusercontent.com/Jin5823/Git-Test/master/src/img_16.svg" />
       
가령 한 노드가 가질 수 있는 key가 5개보다 적는 것이 제한일 경우 위의 그림은 성립되지만, 4개보다 적는 것이 제한이라면 병합이 안된다. order에 맞게 조건이 바뀐다.            
병합시 부모의 자식 노드간에 병합이웃을 정해야하는데, 부모의 key는 sorted 되어 있는 관계로, 부모의 key에서 자기보다 작은 형제의 노드가 병합이웃으로 지정된다. 두 노드를 병합하고 난 후에는 자기보다 작은 병합이웃 leaf node를 부모노드를 인자로 delete_entry를 통해 key까지 올바르게 삭제를 하고 root값에 변동이 있을시 반영도 해야한다. 자신보다 작은 형제 노드가 없을 경우 자기보다 큰 형제노드와 병합을 한다.        
* ##### redistribute
부모의 두 자식 노드가 병합이 불가능할 경우, 두 노드는 재분배를 해야한다.           
<img src="https://raw.githubusercontent.com/Jin5823/Git-Test/master/src/img_17.svg" />
              
노드를 재분배할때의 핵심은 가장 왼쪽과 가장 오른쪽이다. node의 record는 sorted 되어 있기 때문에, 재분배를 하기 위해서는 상대적으로 작은 노드의 가장 큰 값을 상대적으로 큰 노드의 가장 작은 값으로 받으면 된다. 그리고 이는 부모 노드에도 적용을 하면된다. 상대적으로 작은 노드의 가장 큰 값을 부모의 key로 사용하면 된다. 이동의 횟수는 order에 맞게 변한다.

### 1.3 (Naïve) designs or required changes for building on-disk b+ tree
- 1단계, 우선 구조체를 명세서의 조건에 정확히 맞게 만들어 볼 것이다. 4096 Bytes 크기의 page 여러 개를 한 파일에 올바른 크기만큼 잘 저장되어야 하며, 위치를 기반으로 정확하게 호출하여야 한다. 여기서의 목적은 다른사람의 데이터 파일을 읽는 것에도 무리가 없고, 읽고 쓰는 기능을 정확하게 구현하는 것이다.          
- 2단계, 명세서에 요구된 page들의 관계를 b+ tree에 맞게 구현해본다. 단 2단계에서는 insert 기능만 구현하여 page가 파일에 올바르게 위치하여, 간단하게 읽을 수 있는지 확인해본다.            
- 3단계, delete 와 merge 기능을 명세서에 맞게 구현한다. 조건이 page의 Number of Keys가 0이 될 때로 하며, 디스크에서 위치가 sequence일 필요는 없기에, page의 "포인터"에 해당하는 부분만 open하여 수정하는 방식으로 진행하고, insert시 발생하는 split 기능도 구현해볼 것이다.         
- 4단계, find 기능을 명세에 맞게 구현한다. 이전에는 debug용으로 간단하게 구현하고, 4단계부터는 명세에 맞게 구현한다. 또한 구현한 기능들의 최적화를 검토하면서 open_table, db_insert와 같은 api를 작성한다.        
