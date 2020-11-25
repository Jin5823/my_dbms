#include "bpt.h"




pagenum_t delete(pagenum_t rootnum, int64_t key) {
    LeafPage_num key_leaf;
    KeyValue * key_v;

    key_v = find(rootnum, key);
    key_leaf = find_leaf(rootnum, key);
    if (key_v != NULL ){
    	rootnum = delete_entry(rootnum, key_leaf, *key_v);
    }
    return rootnum;
}


pagenum_t delete_entry( pagenum_t rootnum, LeafPage_num re, KeyValue key_value ) {
    // min_keys = 0;

    re = remove_entry_from_leafpage(re, key_value);
    if (re.leaf.NumberOfKeys>0)
    	return rootnum;

    if (re.pagenum == rootnum)
    	return rootnum;

    return adjust_parent(rootnum, re);
}


LeafPage_num remove_entry_from_leafpage(LeafPage_num re, KeyValue key_value) {
    int i;
    i = 0;

    while (re.leaf.LeafKeyValue[i].Key != key_value.Key)
    	i++;

    for (++i; i < re.leaf.NumberOfKeys; i++){
    	re.leaf.LeafKeyValue[i-1] = re.leaf.LeafKeyValue[i];
    }
    re.leaf.NumberOfKeys = re.leaf.NumberOfKeys - 1;

    void *ptr;
    ptr = &re.leaf;
    file_write_page(re.pagenum, ((page_t*)ptr));

    return re;
}


pagenum_t adjust_parent(pagenum_t rootnum, LeafPage_num child) {
	LeafPage_num cf;
	InternalPage parent;
	InternalPage friend_parent;
	LeafPage child_friend;
	void *ptrop;
	void *ptrp;
	void *ptrc;
	void *ptro;
	ptrp = &parent;
	file_read_page(child.leaf.ParentPageNumber, ((page_t*)ptrp));

	int child_index = 0;
	if (parent.OneMorePN == child.pagenum){
		child_index = -1;
	}else{
		while (child_index <= parent.NumberOfKeys){
			if (parent.InternalKeyPN[child_index].PageNumber != child.pagenum)
				child_index++;
			else break;
		}
	}

	if (child_index == -1){
		int friend_index = 0;
		if (parent.NumberOfKeys == 0){
			if (child.leaf.RightSiblingPN!=0){
				ptro = &child_friend;
				file_read_page(child.leaf.RightSiblingPN, ((page_t*)ptro));
				file_write_page(child.pagenum, ((page_t*)ptro));
				ptrop = &friend_parent;
				file_read_page(child_friend.ParentPageNumber, ((page_t*)ptrop));
				if (friend_parent.OneMorePN == child.leaf.RightSiblingPN){
					friend_index = -1;
					friend_parent.OneMorePN = child.pagenum;
				}else{
					while (friend_index <= friend_parent.NumberOfKeys){
						if (friend_parent.InternalKeyPN[friend_index].PageNumber != child.leaf.RightSiblingPN)
							friend_index++;
						else break;
					}
					friend_parent.InternalKeyPN[friend_index].PageNumber = child.pagenum;
				}
				update_free_page(child.leaf.RightSiblingPN);
				file_write_page(child_friend.ParentPageNumber, ((page_t*)ptrop));
			}else{

				cf = find_friend( rootnum, child.leaf.ParentPageNumber, parent);
				if (cf.pagenum != 0){
					cf.leaf.RightSiblingPN = 0;
					void *pcf;
					pcf = &cf.leaf;
					file_write_page(cf.pagenum, ((page_t*)pcf));
				}
				update_free_page(child.pagenum);
			}

			InternalPage_num parent_re;
			parent_re.internal = parent;
			parent_re.pagenum = child.leaf.ParentPageNumber;

			if (child.leaf.ParentPageNumber == rootnum)
				return rootnum;

			// parent 조차 다 비웠을 경우
			return adjust_root(rootnum, parent_re);

		}else{
			ptro = &child_friend;
			file_read_page(child.leaf.RightSiblingPN, ((page_t*)ptro));
			file_write_page(child.pagenum, ((page_t*)ptro));

			int i = 0;
			for (++i; i<parent.NumberOfKeys; i++)
				parent.InternalKeyPN[i-1] = parent.InternalKeyPN[i];
			parent.NumberOfKeys = parent.NumberOfKeys - 1;

			file_write_page(child.leaf.ParentPageNumber, ((page_t*)ptrp));
			update_free_page(child.leaf.RightSiblingPN);
			//update_header() free

			return rootnum;
		}
	}

	// 친구 엮어 주기
	if (child_index == 0){
		ptrc = &child_friend;
		file_read_page(parent.OneMorePN, ((page_t*)ptrc));
		child_friend.RightSiblingPN = child.leaf.RightSiblingPN;
		file_write_page(parent.OneMorePN, ((page_t*)ptrc));
	}
	if (child_index > 0){
		ptrc = &child_friend;
		file_read_page(parent.InternalKeyPN[child_index-1].PageNumber, ((page_t*)ptrc));
		child_friend.RightSiblingPN = child.leaf.RightSiblingPN;
		file_write_page(parent.InternalKeyPN[child_index-1].PageNumber, ((page_t*)ptrc));
	}

	int i = 0;
	while (parent.InternalKeyPN[i].PageNumber != child.pagenum)
		i++;
	for (++i; i<parent.NumberOfKeys; i++)
		parent.InternalKeyPN[i-1] = parent.InternalKeyPN[i];
	parent.NumberOfKeys = parent.NumberOfKeys - 1;

	file_write_page(child.leaf.ParentPageNumber, ((page_t*)ptrp));
	update_free_page(child.pagenum);
	//update_header() free

	return rootnum;
}


pagenum_t adjust_root(pagenum_t rootnum, InternalPage_num child_p) {
	InternalPage parent;
	void *ptrp;
	ptrp = &parent;
	file_read_page(child_p.internal.ParentPageNumber, ((page_t*)ptrp));

	int child_index = 0;
	if (parent.OneMorePN == child_p.pagenum){
		child_index = -1;
	}else{
		while (child_index <= parent.NumberOfKeys){
			if (parent.InternalKeyPN[child_index].PageNumber != child_p.pagenum)
				child_index++;
			else break;
		}
	}

	if (child_index == -1){
		if (parent.NumberOfKeys == 0){
			update_free_page(child_p.pagenum);

			InternalPage_num parent_re;
			parent_re.internal = parent;
			parent_re.pagenum = child_p.internal.ParentPageNumber;

			if (child_p.internal.ParentPageNumber == rootnum){
				return rootnum;
			}

			// parent 조차 다 비웠을 경우
			return adjust_root(rootnum, parent_re);
		}else{
			parent.OneMorePN = parent.InternalKeyPN[0].PageNumber;

			int i = 0;
			for (++i; i<parent.NumberOfKeys; i++)
				parent.InternalKeyPN[i-1] = parent.InternalKeyPN[i];
			parent.NumberOfKeys = parent.NumberOfKeys - 1;

			file_write_page(child_p.internal.ParentPageNumber, ((page_t*)ptrp));
			update_free_page(child_p.pagenum);
			//update_header() free

			return rootnum;
		}
	}
	int i = 0;
	while (parent.InternalKeyPN[i].PageNumber != child_p.pagenum)
		i++;
	for (++i; i<parent.NumberOfKeys; i++)
		parent.InternalKeyPN[i-1] = parent.InternalKeyPN[i];
	parent.NumberOfKeys = parent.NumberOfKeys - 1;

	file_write_page(child_p.internal.ParentPageNumber, ((page_t*)ptrp));
	update_free_page(child_p.pagenum);
	//update_header() free

	return rootnum;
}


LeafPage_num find_friend( pagenum_t rootnum, pagenum_t ppn, InternalPage parent) {
	InternalPage grandpa;
	pagenum_t gpn;
	pagenum_t fpn;
	gpn = parent.ParentPageNumber;
	void *ptr;
	ptr = &grandpa;
	file_read_page(gpn, ((page_t*)ptr));

	while (gpn != 0){
		if (grandpa.OneMorePN == ppn){
			if (gpn == rootnum){
				fpn = 0;
				break;
			}else{
				file_read_page(grandpa.ParentPageNumber, ((page_t*)ptr));
				ppn = gpn;
				gpn = grandpa.ParentPageNumber;
			}
		}
		int i = 0;
		for (int j = 0; j < grandpa.NumberOfKeys; j++){
			if (ppn != grandpa.InternalKeyPN[j].PageNumber) i++;
			else break;
		}
		if (i == 0){
			fpn = grandpa.OneMorePN;
			break;
		}else{
			fpn = grandpa.InternalKeyPN[i-1].PageNumber;
			break;
		}
	}

	LeafPage_num err;
	err.pagenum = 0;
	if (fpn == 0){
		return err;
	}

	LeafPage c;
	pagenum_t cnum;
	void *ptr2;
	ptr2 = &c;
	file_read_page(fpn, ((page_t*)ptr2));

	while (c.IsLeaf == 0) {
		if (((InternalPage*)ptr2)->NumberOfKeys == 0){
			cnum = ((InternalPage*)ptr2)->OneMorePN;
			file_read_page(((InternalPage*)ptr2)->OneMorePN, ((page_t*)ptr2));
		}else{
			cnum = ((InternalPage*)ptr2)->InternalKeyPN[((InternalPage*)ptr2)->NumberOfKeys-1].PageNumber;
			file_read_page(((InternalPage*)ptr2)->InternalKeyPN[((InternalPage*)ptr2)->NumberOfKeys-1].PageNumber, ((page_t*)ptr2));
		}
	}

	LeafPage_num re;
	re.pagenum = cnum;
	re.leaf = c;

	return re;
}


LeafPage_num find_leaf( pagenum_t rootnum, int64_t key) {
	LeafPage c;
	pagenum_t cnum = rootnum;
	void *ptr;
	ptr = &c;
	file_read_page(rootnum, ((page_t*)ptr));

	while (c.IsLeaf == 0) {
		int32_t i=-1;
		for (int j = 0; j < ((InternalPage*)ptr)->NumberOfKeys; j++){
			if (key >= ((InternalPage*)ptr)->InternalKeyPN[j].Key) i++;
			else break;
		}
		if ( i==-1 ){
			cnum = ((InternalPage*)ptr)->OneMorePN;
			file_read_page(((InternalPage*)ptr)->OneMorePN, ((page_t*)ptr));
		}else{
			cnum = ((InternalPage*)ptr)->InternalKeyPN[i].PageNumber;
			file_read_page(((InternalPage*)ptr)->InternalKeyPN[i].PageNumber, ((page_t*)ptr));
		}
	}

	LeafPage_num re;
	re.pagenum = cnum;
	re.leaf = c;

	return re;
}


KeyValue * find( pagenum_t rootnum, int64_t key ) {
	int32_t i = 0;
	LeafPage_num re = find_leaf( rootnum, key );
	LeafPage c;
	c = re.leaf;

    if (c.NumberOfKeys == 0) return NULL;
    for (i = 0; i < c.NumberOfKeys; i++)
        if (c.LeafKeyValue[i].Key == key) break;
    if (i == c.NumberOfKeys){
    	return NULL;
    }else{
    	void * ptr;
    	ptr = &(c.LeafKeyValue[i]);
    	return (KeyValue *)ptr;
    }
}


int cut( int length ) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}


void update_free_page (pagenum_t fpnum){
	file_free_page(fpnum);
	FreePage fp;
	HeaderPage hp;
	pagenum_t p = 0;
	void *ptr;
	ptr = &hp;
	file_read_page(p, ((page_t*)ptr));

	if (hp.FreePageNumber != 0){
		fp.NextFreePageNumber = hp.FreePageNumber;
	}else{
		fp.NextFreePageNumber = 0;
	}

	hp.FreePageNumber = fpnum;
	void *ptrf;
	ptrf = &fp;
	file_write_page(p, ((page_t*)ptr));
	file_write_page(fpnum, ((page_t*)ptrf));
}


void update_header (pagenum_t fpnum, pagenum_t rpnum, pagenum_t numop){
	HeaderPage theader;
	void *ptr;
	ptr = &theader;

	file_read_page(0, ((page_t*)ptr));

	if (fpnum != 0){
		theader.FreePageNumber = fpnum;
	}
	if (rpnum != 0){
		theader.RootPageNumber = rpnum;
	}
	theader.NumberOfPages = theader.NumberOfPages + numop;

	file_write_page(0, ((page_t*)ptr));
}


void update_child (InternalPage new_parent, pagenum_t new_parent_num){
	int i;
	void *ptr;
	LeafPage child;
	ptr = &child;

	file_read_page(new_parent.OneMorePN, ((page_t*)ptr));

	if (child.IsLeaf == 0){
		((InternalPage*)ptr)->ParentPageNumber = new_parent_num;
	}else{
		child.ParentPageNumber = new_parent_num;
	}

	file_write_page(new_parent.OneMorePN, ((page_t*)ptr));
	for (i = 0; i < new_parent.NumberOfKeys; i++){
		file_read_page(new_parent.InternalKeyPN[i].PageNumber, ((page_t*)ptr));

		if (child.IsLeaf == 0){
			((InternalPage*)ptr)->ParentPageNumber = new_parent_num;
		}else{
			child.ParentPageNumber = new_parent_num;
		}

		file_write_page(new_parent.InternalKeyPN[i].PageNumber, ((page_t*)ptr));
	}
}


void usage( void ) {
    printf("Enter any of the following commands after the prompt > :\n"
    "\to <p>  -- Open <p> (a string) as pathname to open file.\n"
    "\ti <k> <v>  -- Insert <k> (an integer) as key and <v> (a string) as value.\n"
    "\tf <k>  -- Find the value under key <k>(an integer).\n"
    "\td <k>  -- Delete key <k>(an integer) and its associated value.\n"
    "\te -- Esc. (Or use Ctl-D.)\n");
}


int is_disk_empty(){
	if (check_size() < 8192){
		return -1;
	}
	return 0;
}


pagenum_t read_header(){
	HeaderPage hp;
	pagenum_t p = 0;
	void *ptr;
	ptr = &hp;
	file_read_page(p, ((page_t*)ptr));
	return hp.RootPageNumber;
}


pagenum_t alloc_page(){
	FreePage fp;
	HeaderPage hp;
	pagenum_t p = 0;
	pagenum_t fpn;
	void *ptr;
	void *ptrf;
	ptr = &hp;
	ptrf = &fp;
	file_read_page(p, ((page_t*)ptr));

	if (hp.FreePageNumber != 0){
		fpn = hp.FreePageNumber;
		file_read_page(hp.FreePageNumber, ((page_t*)ptrf));
		hp.FreePageNumber = fp.NextFreePageNumber;
		file_write_page(p,((page_t*)ptr));
	}else{
		fpn = file_alloc_page();
		update_header(0, 0, 1);
	}

	return fpn;
}


LeafPage insert_into_leaf( LeafPage_num re, int64_t key, char * value ) {

	int i, insertion_point;
    insertion_point = 0;

    while (insertion_point < re.leaf.NumberOfKeys && re.leaf.LeafKeyValue[insertion_point].Key < key)
        insertion_point++;
    for (i = re.leaf.NumberOfKeys; i > insertion_point; i--) {
    	re.leaf.LeafKeyValue[i] = re.leaf.LeafKeyValue[i-1];
    }

    KeyValue tempkv;
    tempkv.Key = key;
    strcpy(tempkv.Value , value);

    re.leaf.NumberOfKeys = re.leaf.NumberOfKeys + 1;
    re.leaf.LeafKeyValue[insertion_point] = tempkv;

    LeafPage leaf_w = re.leaf;
    void *ptrp;
    ptrp = &leaf_w;
    file_write_page(re.pagenum, ((page_t*)ptrp));

    return leaf_w;
}


pagenum_t creat_header_root() {

	HeaderPage firstheader;
	firstheader.FreePageNumber = 0;
	firstheader.RootPageNumber = 1;
	firstheader.NumberOfPages = 2;

	LeafPage firstroot;
	firstroot.ParentPageNumber = 0;
	firstroot.IsLeaf = 1;
	firstroot.NumberOfKeys = 0;
	firstroot.RightSiblingPN = 0;

	void *ptr;
	pagenum_t index;

	index = 0;
	ptr = &firstheader;
	file_write_page(index, ((page_t*)ptr));


	index = 1;
	ptr = &firstroot;
	file_write_page(index, ((page_t*)ptr));

	return index;
}


pagenum_t insert( pagenum_t rootnum, int64_t key, char * value ) {
	LeafPage_num re;
	LeafPage leaf;

	if (check_size() < 8192){
		rootnum = creat_header_root();
	}

	if (find(rootnum, key) != NULL){
		return rootnum;
	}

	re = find_leaf(rootnum, key);
	leaf = re.leaf;

	if (leaf.NumberOfKeys < LEAFPAGE_ORDER - 1) {
		leaf = insert_into_leaf(re, key, value);
		return rootnum;
	}

    return insert_into_leaf_after_splitting(rootnum, re, key, value);
}


pagenum_t insert_into_leaf_after_splitting( pagenum_t rootnum, LeafPage_num re, int64_t key, char * value) {
	pagenum_t new_leaf_num = alloc_page();

	LeafPage new_leaf;
	new_leaf.IsLeaf = 1;
	new_leaf.NumberOfKeys = 0;

	int64_t new_key;
	KeyValue tempKV[LEAFPAGE_ORDER];
	int insertion_index, split, i, j;

	insertion_index = 0;
	while (insertion_index < LEAFPAGE_ORDER - 1 && re.leaf.LeafKeyValue[insertion_index].Key < key)
		insertion_index++;

	for (i = 0, j = 0; i < re.leaf.NumberOfKeys; i++, j++) {
		if (j == insertion_index) j++;
		tempKV[j] = re.leaf.LeafKeyValue[i];
	}
	tempKV[insertion_index].Key = key;
	strcpy(tempKV[insertion_index].Value, value);

	re.leaf.NumberOfKeys = 0;
	split = cut(LEAFPAGE_ORDER - 1);

	for (i = 0; i < split; i++) {
		re.leaf.LeafKeyValue[i] = tempKV[i];
		re.leaf.NumberOfKeys = re.leaf.NumberOfKeys + 1;
	}
	for (i = split, j = 0; i < LEAFPAGE_ORDER; i++, j++) {
		new_leaf.LeafKeyValue[j] = tempKV[i];
		new_leaf.NumberOfKeys = new_leaf.NumberOfKeys + 1;
	}

	new_leaf.RightSiblingPN = re.leaf.RightSiblingPN;
	re.leaf.RightSiblingPN = new_leaf_num;

    new_leaf.ParentPageNumber = re.leaf.ParentPageNumber;
    new_key = new_leaf.LeafKeyValue[0].Key;

    LeafPage_num new_re;
    new_re.pagenum = new_leaf_num;
    new_re.leaf = new_leaf;

    return insert_into_parent(rootnum, re, new_key, new_re);
}


pagenum_t insert_into_new_root(LeafPage_num left, int64_t key, LeafPage_num right) {
	InternalPage new_root;
	pagenum_t new_root_num = alloc_page();
	update_header(0, new_root_num, 0);

	new_root.ParentPageNumber = 0;
	new_root.IsLeaf = 0;
	new_root.NumberOfKeys = 1;
	new_root.OneMorePN = left.pagenum;
	new_root.InternalKeyPN[0].Key = key;
	new_root.InternalKeyPN[0].PageNumber = right.pagenum;

	void *ptr;
	ptr = &new_root;
	file_write_page(new_root_num, ((page_t*)ptr));

    left.leaf.ParentPageNumber = new_root_num;
    LeafPage left_leaf = left.leaf;
    ptr = &left_leaf;
    file_write_page(left.pagenum, ((page_t*)ptr));

    right.leaf.ParentPageNumber = new_root_num;
    LeafPage right_leaf = right.leaf;
    ptr = &right_leaf;
    file_write_page(right.pagenum, ((page_t*)ptr));

    return new_root_num;
}


pagenum_t insert_into_parent( pagenum_t rootnum, LeafPage_num left, int64_t key, LeafPage_num right) {
	if (left.leaf.ParentPageNumber == 0)
		return insert_into_new_root(left, key, right);

	InternalPage parent;
	void *ptr;
	ptr = &parent;
	file_read_page(left.leaf.ParentPageNumber, ((page_t*)ptr));

	InternalPage_num parent_re;
	parent_re.pagenum = left.leaf.ParentPageNumber;
	parent_re.internal = parent;
	if (parent.NumberOfKeys < INTERNAL_ORDER -1)
		return insert_into_node(rootnum, parent_re, left, key, right);

    return insert_into_node_after_splitting(rootnum, parent_re, left, key, right);
}


pagenum_t insert_into_node( pagenum_t rootnum, InternalPage_num parent, LeafPage_num left, int64_t key, LeafPage_num right) {
	int left_index = 0;
	int i;

	if (parent.internal.OneMorePN == left.pagenum){
		left_index = -1;
	}else{
		while (left_index <= parent.internal.NumberOfKeys){
			if (parent.internal.InternalKeyPN[left_index].PageNumber != left.pagenum)
				left_index++;
			else break;
		}
	}

	for (i = parent.internal.NumberOfKeys; i > left_index+1; i--){
		parent.internal.InternalKeyPN[i] = parent.internal.InternalKeyPN[i-1];
	}

	parent.internal.InternalKeyPN[left_index+1].Key = key;
	parent.internal.InternalKeyPN[left_index+1].PageNumber = right.pagenum;
	parent.internal.NumberOfKeys = parent.internal.NumberOfKeys + 1;


	void *ptr;
	InternalPage parentpage = parent.internal;
	ptr = &parentpage;
	file_write_page(parent.pagenum, ((page_t*)ptr));
    LeafPage left_leaf = left.leaf;
    ptr = &left_leaf;
    file_write_page(left.pagenum, ((page_t*)ptr));
    LeafPage right_leaf = right.leaf;
    ptr = &right_leaf;
    file_write_page(right.pagenum, ((page_t*)ptr));

   return rootnum;
}


pagenum_t insert_into_node_after_splitting(pagenum_t rootnum, InternalPage_num parent, LeafPage_num left, int64_t key, LeafPage_num right) {
	int i, j, split;
	int64_t k_prime;
	KeyPN tempPN[INTERNAL_ORDER];
	InternalPage new_parent;
	pagenum_t new_parent_num = alloc_page();

	new_parent.IsLeaf = 0;
	new_parent.NumberOfKeys = 0;


	int left_index = 0;
	if (parent.internal.OneMorePN == left.pagenum){
		left_index = -1;
	}else{
		while (left_index <= parent.internal.NumberOfKeys && parent.internal.InternalKeyPN[left_index].PageNumber != left.pagenum){
			left_index++;
		}
	}

	for (i = 0, j = 0; i < parent.internal.NumberOfKeys ; i++, j++){
		if (j == left_index + 1)j++;
		tempPN[j] = parent.internal.InternalKeyPN[i];
	}

	tempPN[left_index+1].Key =key;
	tempPN[left_index+1].PageNumber=right.pagenum;

	split = cut(INTERNAL_ORDER);
	parent.internal.NumberOfKeys = 0;
	for (i=0; i<split-1;i++){
		parent.internal.InternalKeyPN[i] = tempPN[i];
		parent.internal.NumberOfKeys = parent.internal.NumberOfKeys +1;
	}

	k_prime = tempPN[i].Key;
	new_parent.OneMorePN = tempPN[i].PageNumber;
	for (++i, j = 0;i<INTERNAL_ORDER; i++, j++){
		new_parent.InternalKeyPN[j] = tempPN[i];
		new_parent.NumberOfKeys = new_parent.NumberOfKeys +1;
	}
	new_parent.ParentPageNumber = parent.internal.ParentPageNumber;

	void *ptr;
	InternalPage parentpage = parent.internal;
	ptr = &parentpage;
	file_write_page(parent.pagenum, ((page_t*)ptr));
	LeafPage left_leaf = left.leaf;
	ptr = &left_leaf;
	file_write_page(left.pagenum, ((page_t*)ptr));
	LeafPage right_leaf = right.leaf;
	ptr = &right_leaf;
	file_write_page(right.pagenum, ((page_t*)ptr));
	InternalPage newparentpage = new_parent;
	ptr = &newparentpage;
	file_write_page(new_parent_num, ((page_t*)ptr));

	update_child(new_parent, new_parent_num);


	InternalPage_num n_parent;
	n_parent.pagenum = new_parent_num;
	n_parent.internal = new_parent;


	return insert_internal_into_parent(rootnum, parent, k_prime, n_parent);
}


pagenum_t insert_internal_into_parent( pagenum_t rootnum, InternalPage_num old_p, int64_t key, InternalPage_num new_p) {
	if (old_p.internal.ParentPageNumber == 0)
		return insert_internal_into_new_root(old_p, key, new_p);

	InternalPage parent;
	void *ptr;
	ptr = &parent;
	file_read_page(old_p.internal.ParentPageNumber, ((page_t*)ptr));

	InternalPage_num parent_re;
	parent_re.pagenum = old_p.internal.ParentPageNumber;
	parent_re.internal = parent;
	if (parent.NumberOfKeys < INTERNAL_ORDER -1)
		return insert_internal_into_node(rootnum, parent_re, old_p, key, new_p);

    return insert_internal_into_node_after_splitting(rootnum, parent_re, old_p, key, new_p);
}


pagenum_t insert_internal_into_new_root(InternalPage_num left, int64_t key, InternalPage_num right) {
	InternalPage new_root;
	pagenum_t new_root_num = alloc_page();
	update_header(0, new_root_num, 0);

	new_root.ParentPageNumber = 0;
	new_root.IsLeaf = 0;
	new_root.NumberOfKeys = 1;
	new_root.OneMorePN = left.pagenum;
	new_root.InternalKeyPN[0].Key = key;
	new_root.InternalKeyPN[0].PageNumber = right.pagenum;

	void *ptr;
	ptr = &new_root;
	file_write_page(new_root_num, ((page_t*)ptr));

    left.internal.ParentPageNumber = new_root_num;
    InternalPage left_internal = left.internal;
    ptr = &left_internal;
    file_write_page(left.pagenum, ((page_t*)ptr));

    right.internal.ParentPageNumber = new_root_num;
    InternalPage right_internal = right.internal;
    ptr = &right_internal;
    file_write_page(right.pagenum, ((page_t*)ptr));

    return new_root_num;
}


pagenum_t insert_internal_into_node( pagenum_t rootnum, InternalPage_num parent, InternalPage_num left, int64_t key, InternalPage_num right) {
	int left_index = 0;
	int i;

	if (parent.internal.OneMorePN == left.pagenum){
		left_index = -1;
	}else{
		while (left_index <= parent.internal.NumberOfKeys){
			if (parent.internal.InternalKeyPN[left_index].PageNumber != left.pagenum)
				left_index++;
			else break;
		}
	}

	for (i = parent.internal.NumberOfKeys; i > left_index+1; i--){
		parent.internal.InternalKeyPN[i] = parent.internal.InternalKeyPN[i-1];
	}
	parent.internal.InternalKeyPN[left_index+1].Key = key;
	parent.internal.InternalKeyPN[left_index+1].PageNumber = right.pagenum;
	parent.internal.NumberOfKeys = parent.internal.NumberOfKeys + 1;


	void *ptr;
	InternalPage parentpage = parent.internal;
	ptr = &parentpage;
	file_write_page(parent.pagenum, ((page_t*)ptr));
	InternalPage left_internal = left.internal;
    ptr = &left_internal;
    file_write_page(left.pagenum, ((page_t*)ptr));
    InternalPage right_internal = right.internal;
    ptr = &right_internal;
    file_write_page(right.pagenum, ((page_t*)ptr));


   return rootnum;
}


pagenum_t insert_internal_into_node_after_splitting(pagenum_t rootnum, InternalPage_num parent, InternalPage_num left, int64_t key, InternalPage_num right) {
	int i, j, split;
	int64_t k_prime;
	KeyPN tempPN[INTERNAL_ORDER];
	InternalPage new_parent;
	pagenum_t new_parent_num = alloc_page();

	new_parent.IsLeaf = 0;
	new_parent.NumberOfKeys = 0;


	int left_index = 0;
	if (parent.internal.OneMorePN == left.pagenum){
		left_index = -1;
	}else{
		while (left_index <= parent.internal.NumberOfKeys && parent.internal.InternalKeyPN[left_index].PageNumber != left.pagenum){
			left_index++;
		}
	}

	for (i = 0, j = 0; i < parent.internal.NumberOfKeys ; i++, j++){
		if (j == left_index + 1)j++;
		tempPN[j] = parent.internal.InternalKeyPN[i];
	}

	tempPN[left_index+1].Key =key;
	tempPN[left_index+1].PageNumber=right.pagenum;

	split = cut(INTERNAL_ORDER);
	parent.internal.NumberOfKeys = 0;
	for (i=0; i<split-1;i++){
		parent.internal.InternalKeyPN[i] = tempPN[i];
		parent.internal.NumberOfKeys = parent.internal.NumberOfKeys +1;
	}
	k_prime = tempPN[i].Key;
	new_parent.OneMorePN = tempPN[i].PageNumber;
	for (++i, j = 0;i<INTERNAL_ORDER; i++, j++){
		new_parent.InternalKeyPN[j] = tempPN[i];
		new_parent.NumberOfKeys = new_parent.NumberOfKeys +1;
	}
	new_parent.ParentPageNumber = parent.internal.ParentPageNumber;

	void *ptr;
	InternalPage parentpage = parent.internal;
	ptr = &parentpage;
	file_write_page(parent.pagenum, ((page_t*)ptr));
	InternalPage left_internal = left.internal;
	ptr = &left_internal;
	file_write_page(left.pagenum, ((page_t*)ptr));
	InternalPage right_internal = right.internal;
	ptr = &right_internal;
	file_write_page(right.pagenum, ((page_t*)ptr));
	InternalPage newparentpage = new_parent;
	ptr = &newparentpage;
	file_write_page(new_parent_num, ((page_t*)ptr));

	update_child(new_parent, new_parent_num);


	InternalPage_num n_parent;
	n_parent.pagenum = new_parent_num;
	n_parent.internal = new_parent;

	return insert_internal_into_parent(rootnum, parent, k_prime, n_parent);
}
