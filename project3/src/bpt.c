#include "bpt.h"




pagenum_t delete(int tid, pagenum_t rootnum, int64_t key) {
    LeafPage_num key_leaf;
    KeyValue * key_v;

    key_v = find(tid, rootnum, key);
    key_leaf = find_leaf(tid, rootnum, key);
    dirty_mark(tid, key_leaf.pagenum);
    if (key_v != NULL ){
    	rootnum = delete_entry(tid, rootnum, key_leaf, *key_v);
    }else{
    	pin_down(tid, key_leaf.pagenum);
    }

    // 확인 바람
    return rootnum;
}


pagenum_t delete_entry(int tid, pagenum_t rootnum, LeafPage_num re, KeyValue key_value ) {

    remove_entry_from_leafpage(tid, re, key_value);
    if (re.leaf->NumberOfKeys > 0){
    	pin_down(tid, re.pagenum);
    	return rootnum;
    }

    if (re.pagenum == rootnum){
    	pin_down(tid, re.pagenum);
    	return rootnum;
    }
    return adjust_parent(tid, rootnum, re);
}


void remove_entry_from_leafpage(int tid, LeafPage_num re, KeyValue key_value) {
    int i;
    i = 0;

    while (re.leaf->LeafKeyValue[i].Key != key_value.Key)
    	i++;

    for (++i; i < re.leaf->NumberOfKeys; i++){
    	re.leaf->LeafKeyValue[i-1] = re.leaf->LeafKeyValue[i];
    }
    re.leaf->NumberOfKeys = re.leaf->NumberOfKeys - 1;

}


pagenum_t adjust_parent(int tid, pagenum_t rootnum, LeafPage_num child) {
	LeafPage_num re;
	void *ptrop;
	void *ptrp;
	void *ptrc;
	void *ptro;


	ptrp = buff_read_page(tid, child.leaf->ParentPageNumber);
	dirty_mark(tid, child.leaf->ParentPageNumber);

	uint64_t rpn = child.leaf->RightSiblingPN;
	InternalPage_num parent_re;
	parent_re.internal = ptrp;
	parent_re.pagenum = child.leaf->ParentPageNumber;

	// parent = *((InternalPage*)ptrp);
	// dirty_mark(child.leaf.ParentPageNumber);
	// pin_down(child.leaf.ParentPageNumber);
	// 정확한 위치 정하기

	int child_index = 0;
	if (((InternalPage*)ptrp)->OneMorePN == child.pagenum){
		child_index = -1;
	}else{
		while (child_index <= ((InternalPage*)ptrp)->NumberOfKeys){
			if (((InternalPage*)ptrp)->InternalKeyPN[child_index].PageNumber != child.pagenum)
				child_index++;
			else break;
		}
	}

	if (child_index == -1){
		int friend_index = 0;
		if (((InternalPage*)ptrp)->NumberOfKeys == 0){
			if (child.leaf->RightSiblingPN!=0){
				ptro = buff_read_page(tid, rpn);
				dirty_mark(tid, rpn);
				uint64_t cfpn = ((LeafPage*)ptro)->ParentPageNumber;

				// child_friend = *((LeafPage*)ptro);
				// pin_down(child.leaf.RightSiblingPN);
				// pind_down(child.pagenum);
				// dirty_mark(child.leaf.RightSiblingPN);

				pin_down(tid, child.pagenum);
				buff_write_newpage(tid, child.pagenum, ((page_t*)ptro));
				ptrop = buff_read_page(tid, cfpn);
				dirty_mark(tid, cfpn);

				//friend_parent = *((InternalPage*)ptrop);
				//pin_down(child_friend.ParentPageNumber);
				if (((InternalPage*)ptrop)->OneMorePN == rpn){
					friend_index = -1;
					((InternalPage*)ptrop)->OneMorePN = child.pagenum;
				}else{
					while (friend_index <= ((InternalPage*)ptrop)->NumberOfKeys){
						if (((InternalPage*)ptrop)->InternalKeyPN[friend_index].PageNumber != rpn)
							friend_index++;
						else break;
					}
					((InternalPage*)ptrop)->InternalKeyPN[friend_index].PageNumber = child.pagenum;
				}

				pin_down(tid, rpn);
				update_free_page(tid, rpn);
				pin_down(tid, cfpn);
				buff_write_newpage(tid, cfpn, ((page_t*)ptrop));
			}else{
				// 가장 오른쪽 지울때
				//printf(" is err? 1 %"PRIu64" %"PRIu64"  \n", child.pagenum, parent_re.pagenum );
				re = find_friend(tid, rootnum, parent_re.pagenum, parent_re.internal->ParentPageNumber );
				if (re.pagenum != 0){
					//printf("friend?  %"PRIu64"  \n",re.pagenum);
					dirty_mark(tid, re.pagenum);
					re.leaf->RightSiblingPN = 0;
					pin_down(tid, re.pagenum);

					//void *pcf;
					//pcf = &re.leaf;
					//buff_write_newpage(re.pagenum, ((page_t*)pcf));
				}
				pin_down(tid, child.pagenum);
				update_free_page(tid, child.pagenum);
				// 외쪽 친구 찾아서 rpn 0으로 설정해주기
				//printf(" is err? 2 %"PRIu64" %"PRIu64"  \n", child.pagenum, parent_re.pagenum );
			}

			if (parent_re.pagenum == rootnum){
				pin_down(tid, parent_re.pagenum);
				return rootnum;
			}

			// parent 조차 다 비웠을 경우
			return adjust_root(tid, rootnum, parent_re);

		}else{
			uint64_t rpn = child.leaf->RightSiblingPN;
			ptro = buff_read_page(tid, rpn);
			dirty_mark(tid, rpn);

			pin_down(tid, child.pagenum);
			buff_write_newpage(tid, child.pagenum, ((page_t*)ptro));

			int i = 0;
			for (++i; i<parent_re.internal->NumberOfKeys; i++)
				parent_re.internal->InternalKeyPN[i-1] = parent_re.internal->InternalKeyPN[i];
			parent_re.internal->NumberOfKeys = parent_re.internal->NumberOfKeys - 1;

			pin_down(tid, parent_re.pagenum);
			buff_write_newpage(tid, parent_re.pagenum, ((page_t*)ptrp));

			pin_down(tid, rpn);
			update_free_page(tid, rpn);
			return rootnum;
		}
	}

	// 친구 엮어 주기
	if (child_index == 0){

		ptrc = buff_read_page(tid, parent_re.internal->OneMorePN);
		dirty_mark(tid, parent_re.internal->OneMorePN);

		((LeafPage*)ptrc)->RightSiblingPN = rpn;
		pin_down(tid, parent_re.internal->OneMorePN);
		buff_write_newpage(tid, parent_re.internal->OneMorePN, ((page_t*)ptrc));

	}
	if (child_index > 0){
		ptrc = buff_read_page(tid, parent_re.internal->InternalKeyPN[child_index-1].PageNumber);
		dirty_mark(tid, parent_re.internal->InternalKeyPN[child_index-1].PageNumber);

		((LeafPage*)ptrc)->RightSiblingPN = rpn;
		pin_down(tid, parent_re.internal->InternalKeyPN[child_index-1].PageNumber);
		buff_write_newpage(tid, parent_re.internal->InternalKeyPN[child_index-1].PageNumber, ((page_t*)ptrc));
	}

	int i = 0;
	while (parent_re.internal->InternalKeyPN[i].PageNumber != child.pagenum)
		i++;
	for (++i; i<parent_re.internal->NumberOfKeys; i++)
		parent_re.internal->InternalKeyPN[i-1] = parent_re.internal->InternalKeyPN[i];
	parent_re.internal->NumberOfKeys = parent_re.internal->NumberOfKeys - 1;

	pin_down(tid, parent_re.pagenum);
	buff_write_newpage(tid, parent_re.pagenum, ((page_t*)ptrp));
	pin_down(tid, child.pagenum);
	update_free_page(tid, child.pagenum);

	return rootnum;
}


pagenum_t adjust_root(int tid, pagenum_t rootnum, InternalPage_num child_p) {
	void *ptrp;

	//printf(" is err? %"PRIu64" %"PRIu64"  \n",child_p.pagenum, child_p.internal->ParentPageNumber );
	ptrp = buff_read_page(tid, child_p.internal->ParentPageNumber);
	dirty_mark(tid, child_p.internal->ParentPageNumber);

	InternalPage_num parent_re;
	parent_re.internal = ptrp;
	parent_re.pagenum = child_p.internal->ParentPageNumber;



	int child_index = 0;
	if (parent_re.internal->OneMorePN == child_p.pagenum){
		child_index = -1;
	}else{
		while (child_index <= parent_re.internal->NumberOfKeys){
			if (parent_re.internal->InternalKeyPN[child_index].PageNumber != child_p.pagenum)
				child_index++;
			else break;
		}
	}

	if (child_index == -1){
		if (parent_re.internal->NumberOfKeys == 0){
			pin_down(tid, child_p.pagenum);
			update_free_page(tid, child_p.pagenum);

			if (parent_re.pagenum == rootnum){
				pin_down(tid, parent_re.pagenum);
				return rootnum;
			}
			// parent 조차 다 비웠을 경우
			return adjust_root(tid, rootnum, parent_re);
		}else{
			parent_re.internal->OneMorePN = parent_re.internal->InternalKeyPN[0].PageNumber;

			int i = 0;
			for (++i; i<parent_re.internal->NumberOfKeys; i++)
				parent_re.internal->InternalKeyPN[i-1] = parent_re.internal->InternalKeyPN[i];
			parent_re.internal->NumberOfKeys = parent_re.internal->NumberOfKeys - 1;

			pin_down(tid, parent_re.pagenum);
			buff_write_newpage(tid, parent_re.pagenum, ((page_t*)ptrp));
			pin_down(tid, child_p.pagenum);
			update_free_page(tid, child_p.pagenum);

			return rootnum;
		}
	}
	int i = 0;
	while (parent_re.internal->InternalKeyPN[i].PageNumber != child_p.pagenum)
		i++;
	for (++i; i<parent_re.internal->NumberOfKeys; i++)
		parent_re.internal->InternalKeyPN[i-1] = parent_re.internal->InternalKeyPN[i];
	parent_re.internal->NumberOfKeys = parent_re.internal->NumberOfKeys - 1;

	pin_down(tid, parent_re.pagenum);
	buff_write_newpage(tid, parent_re.pagenum, ((page_t*)ptrp));
	pin_down(tid, child_p.pagenum);
	update_free_page(tid, child_p.pagenum);

	return rootnum;
}


LeafPage_num find_friend(int tid, pagenum_t rootnum, pagenum_t ppn, pagenum_t parentpn) {
	pagenum_t gpn;
	pagenum_t fpn;
	gpn = parentpn;
	void *ptr;

	// ptr = &grandpa;
	// file_read_page(gpn, ((page_t*)ptr));

	ptr = buff_read_page(tid, gpn);
	// grandpa = *((InternalPage*)ptr);
	// dirty_mark(gpn);
	// pin_down(gpn);
	// 정확한 위치 정하기

	while (gpn != 0){
		if (((InternalPage*)ptr)->OneMorePN == ppn){
			if (gpn == rootnum){
				fpn = 0;
				break;
			}else{
				pin_down(tid, gpn);
				ptr = buff_read_page(tid, ((InternalPage*)ptr)->ParentPageNumber);
				ppn = gpn;
				gpn = ((InternalPage*)ptr)->ParentPageNumber;
			}
		}
		int i = 0;
		for (int j = 0; j < ((InternalPage*)ptr)->NumberOfKeys; j++){
			if (ppn != ((InternalPage*)ptr)->InternalKeyPN[j].PageNumber) i++;
			else break;
		}
		if (i == 0){
			fpn = ((InternalPage*)ptr)->OneMorePN;
			break;
		}else{
			fpn = ((InternalPage*)ptr)->InternalKeyPN[i-1].PageNumber;
			break;
		}
	}
	pin_down(tid, gpn);

	LeafPage_num err;
	err.pagenum = 0;
	if (fpn == 0){
		return err;
	}

	pagenum_t cnum = fpn;
	void *ptr2;
	ptr2 = buff_read_page(tid, cnum);

	while (((LeafPage*)ptr2)->IsLeaf == 0) {
		if (((InternalPage*)ptr2)->NumberOfKeys == 0){
			pin_down(tid, cnum);
			cnum = ((InternalPage*)ptr2)->OneMorePN;
			ptr2 = buff_read_page(tid, cnum);
		}else{
			pin_down(tid, cnum);
			cnum = ((InternalPage*)ptr2)->InternalKeyPN[((InternalPage*)ptr2)->NumberOfKeys-1].PageNumber;
			ptr2 = buff_read_page(tid, cnum);
		}
	}

	LeafPage_num re;
	re.pagenum = cnum;
	re.leaf = ptr2;

	return re;
}







LeafPage_num find_leaf(int tid, pagenum_t rootnum, int64_t key) {
	pagenum_t cnum = rootnum;
	void *ptr;
	ptr = buff_read_page(tid, cnum);

	while (((LeafPage*)ptr)->IsLeaf == 0) {
		int32_t i=-1;
		for (int j = 0; j < ((InternalPage*)ptr)->NumberOfKeys; j++){
			if (key >= ((InternalPage*)ptr)->InternalKeyPN[j].Key) i++;
			else break;
		}
		if ( i==-1 ){
			pin_down(tid, cnum);
			cnum = ((InternalPage*)ptr)->OneMorePN;
			ptr = buff_read_page(tid, cnum);
		}else{
			pin_down(tid, cnum);
			cnum = ((InternalPage*)ptr)->InternalKeyPN[i].PageNumber;
			ptr = buff_read_page(tid, cnum);
		}
	}

	LeafPage_num re;
	re.pagenum = cnum;
	re.leaf = ptr;

	return re;
}


KeyValue * find(int tid, pagenum_t rootnum, int64_t key ) {
	int32_t i = 0;
	LeafPage_num re = find_leaf(tid, rootnum, key );


    if (re.leaf->NumberOfKeys == 0) {
    	pin_down(tid, re.pagenum);
    	return NULL;
    }
    for (i = 0; i < re.leaf->NumberOfKeys; i++)
        if (re.leaf->LeafKeyValue[i].Key == key) break;
    if (i == re.leaf->NumberOfKeys){
    	pin_down(tid, re.pagenum);
    	return NULL;
    }else{
    	pin_down(tid, re.pagenum);
    	void * ptr;
    	ptr = &(re.leaf->LeafKeyValue[i]);
    	return (KeyValue *)ptr;
    }
}


int cut( int length ) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}


void update_free_page (int tid, pagenum_t fpnum){
	FreePage fp;
	pagenum_t p = 0;
	void *ptr;
	ptr = buff_read_page(tid, p);
	dirty_mark(tid, p);

	if (((HeaderPage*)ptr)->FreePageNumber != 0){
		fp.NextFreePageNumber = ((HeaderPage*)ptr)->FreePageNumber;
	}else{
		fp.NextFreePageNumber = 0;
	}

	((HeaderPage*)ptr)->FreePageNumber = fpnum;
	pin_down(tid, p);
	void *ptrf;
	ptrf = &fp;
	buff_write_newpage(tid, fpnum, ((page_t*)ptrf));
}


void update_header (int tid, pagenum_t fpnum, pagenum_t rpnum, pagenum_t numop){
	void *ptr;
	ptr = buff_read_page(tid, 0);
	dirty_mark(tid, 0);

	if (fpnum != 0){
		((HeaderPage*)ptr)->FreePageNumber = fpnum;
	}
	if (rpnum != 0){
		((HeaderPage*)ptr)->RootPageNumber = rpnum;
	}
	((HeaderPage*)ptr)->NumberOfPages = ((HeaderPage*)ptr)->NumberOfPages + numop;
	pin_down(tid, 0);
}


void update_child (int tid, InternalPage new_parent, pagenum_t new_parent_num){
	int i;
	void *ptr;
	ptr = buff_read_page(tid, new_parent.OneMorePN);
	dirty_mark(tid, new_parent.OneMorePN);

	if (((LeafPage*)ptr)->IsLeaf == 0){
		((InternalPage*)ptr)->ParentPageNumber = new_parent_num;
	}else{
		((LeafPage*)ptr)->ParentPageNumber = new_parent_num;
	}
	pin_down(tid, new_parent.OneMorePN);
	for (i = 0; i < new_parent.NumberOfKeys; i++){
		ptr = buff_read_page(tid, new_parent.InternalKeyPN[i].PageNumber);
		dirty_mark(tid, new_parent.InternalKeyPN[i].PageNumber);

		if (((LeafPage*)ptr)->IsLeaf == 0){
			((InternalPage*)ptr)->ParentPageNumber = new_parent_num;
		}else{
			((LeafPage*)ptr)->ParentPageNumber = new_parent_num;
		}

		pin_down(tid, new_parent.InternalKeyPN[i].PageNumber);
	}
}


void usage( void ) {
    printf("Enter any of the following commands after the prompt > :\n"
    "\tn <n>  -- Init <n>(an integer) as buffer number to init buffer.\n"
    "\to <p>  -- Open <p>(a string) as pathname to open file.\n"
    "\ti <u> <k> <v>  -- Insert <u>(an integer) as table_id, <k>(an integer) as key and <v>(a string) as value.\n"
    "\tf <u> <k>  -- Find <u>(an integer) as table_id, the value under key <k>(an integer).\n"
    "\td <u> <k>  -- Delete <u>(an integer) as table_id, key <k>(an integer) and its associated value.\n"
    "\tc <u>  -- Close <u>(an integer) as table_id to close file.\n"
    "\te -- Esc. (Or use Ctl-D.)\n");
}


int is_disk_empty(int tid){
	if (check_file_size(tid) < 8192){
		return -1;
	}
	return 0;
}


pagenum_t read_header(int tid){
	void *ptr;
	ptr = buff_read_page(tid, 0);

	return ((HeaderPage*)ptr)->RootPageNumber;
}


pagenum_t alloc_page(int tid){
	pagenum_t p = 0;
	pagenum_t fpn;
	void *ptr;
	void *ptrf;
	ptr = buff_read_page(tid, p);

	if (((HeaderPage*)ptr)->FreePageNumber != 0){
		fpn = ((HeaderPage*)ptr)->FreePageNumber;
		ptrf = buff_read_page(tid, fpn);
		dirty_mark(tid, p);
		((HeaderPage*)ptr)->FreePageNumber = ((FreePage*)ptrf)->NextFreePageNumber;
		pin_down(tid, fpn);
		pin_down(tid, p);
	}else{
		pin_down(tid, p);
		fpn = buff_alloc_page(tid);
		update_header(tid, 0, 0, 1);
	}
	return fpn;
}


void insert_into_leaf(int tid, LeafPage_num re, int64_t key, char * value ) {
	int i, insertion_point;
    insertion_point = 0;

    while (insertion_point < re.leaf->NumberOfKeys && re.leaf->LeafKeyValue[insertion_point].Key < key)
        insertion_point++;
    for (i = re.leaf->NumberOfKeys; i > insertion_point; i--) {
    	re.leaf->LeafKeyValue[i] = re.leaf->LeafKeyValue[i-1];
    }

    KeyValue tempkv;
    tempkv.Key = key;
    strcpy(tempkv.Value , value);

    re.leaf->NumberOfKeys = re.leaf->NumberOfKeys + 1;
    re.leaf->LeafKeyValue[insertion_point] = tempkv;
}


pagenum_t creat_header_root(int tid) {
	void *ptr;
	void *ptr2;

	HeaderPage firstheader;
	firstheader.FreePageNumber = 0;
	firstheader.RootPageNumber = 1;
	firstheader.NumberOfPages = 2;

	LeafPage firstroot;
	firstroot.ParentPageNumber = 0;
	firstroot.IsLeaf = 1;
	firstroot.NumberOfKeys = 0;
	firstroot.RightSiblingPN = 0;

	ptr = &firstheader;
	ptr2 = &firstroot;

	creat_header_root_buff(tid, 0, ((page_t*)ptr), 1, ((page_t*)ptr2));

	return 1;
}


pagenum_t insert( int tid, pagenum_t rootnum, int64_t key, char * value ) {
	LeafPage_num re;
	pagenum_t returnnum;

	if (check_size(tid) < 8192){
		rootnum = creat_header_root(tid);
	}

	if (find(tid, rootnum, key) != NULL){
		return rootnum;
	}

	re = find_leaf(tid, rootnum, key);
	dirty_mark(tid, re.pagenum);

	if (re.leaf->NumberOfKeys < LEAFPAGE_ORDER - 1) {
		insert_into_leaf(tid, re, key, value);
		pin_down(tid, re.pagenum);
		return rootnum;
	}
	returnnum = insert_into_leaf_after_splitting(tid, rootnum, re, key, value);
    return returnnum;
}


pagenum_t insert_into_leaf_after_splitting(int tid, pagenum_t rootnum, LeafPage_num re, int64_t key, char * value) {
	pagenum_t new_leaf_num = alloc_page(tid);

	LeafPage new_leaf;
	new_leaf.IsLeaf = 1;
	new_leaf.NumberOfKeys = 0;

	int64_t new_key;
	KeyValue tempKV[LEAFPAGE_ORDER];
	int insertion_index, split, i, j;

	insertion_index = 0;
	while (insertion_index < LEAFPAGE_ORDER - 1 && re.leaf->LeafKeyValue[insertion_index].Key < key)
		insertion_index++;

	for (i = 0, j = 0; i < re.leaf->NumberOfKeys; i++, j++) {
		if (j == insertion_index) j++;
		tempKV[j] = re.leaf->LeafKeyValue[i];
	}
	tempKV[insertion_index].Key = key;
	strcpy(tempKV[insertion_index].Value, value);

	re.leaf->NumberOfKeys = 0;
	split = cut(LEAFPAGE_ORDER - 1);

	for (i = 0; i < split; i++) {
		re.leaf->LeafKeyValue[i] = tempKV[i];
		re.leaf->NumberOfKeys = re.leaf->NumberOfKeys + 1;
	}
	for (i = split, j = 0; i < LEAFPAGE_ORDER; i++, j++) {
		new_leaf.LeafKeyValue[j] = tempKV[i];
		new_leaf.NumberOfKeys = new_leaf.NumberOfKeys + 1;
	}

	new_leaf.RightSiblingPN = re.leaf->RightSiblingPN;
	re.leaf->RightSiblingPN = new_leaf_num;

    new_leaf.ParentPageNumber = re.leaf->ParentPageNumber;
    new_key = new_leaf.LeafKeyValue[0].Key;

    LeafPage_num new_re;
    new_re.pagenum = new_leaf_num;
    new_re.leaf = &new_leaf;

    return insert_into_parent(tid, rootnum, re, new_key, new_re);
}


pagenum_t insert_into_new_root(int tid, LeafPage_num left, int64_t key, LeafPage_num right) {
	InternalPage new_root;
	pagenum_t new_root_num = alloc_page(tid);
	update_header(tid, 0, new_root_num, 0);

	new_root.ParentPageNumber = 0;
	new_root.IsLeaf = 0;
	new_root.NumberOfKeys = 1;
	new_root.OneMorePN = left.pagenum;
	new_root.InternalKeyPN[0].Key = key;
	new_root.InternalKeyPN[0].PageNumber = right.pagenum;

	void *ptr;
	ptr = &new_root;
	buff_write_newpage(tid, new_root_num, ((page_t*)ptr));

	left.leaf->ParentPageNumber = new_root_num;
	right.leaf->ParentPageNumber = new_root_num;

	ptr = right.leaf;
	buff_write_newpage(tid, right.pagenum, ((page_t*)ptr));

    return new_root_num;
}


pagenum_t insert_into_parent(int tid, pagenum_t rootnum, LeafPage_num left, int64_t key, LeafPage_num right) {
	pagenum_t returnum;
	if (left.leaf->ParentPageNumber == 0){
		pin_down(tid, left.pagenum);
		returnum = insert_into_new_root(tid, left, key, right);
		return returnum;
	}


	void *ptr;
	ptr = buff_read_page(tid, left.leaf->ParentPageNumber);
	InternalPage_num parent_re;
	parent_re.pagenum = left.leaf->ParentPageNumber;
	parent_re.internal = ptr;
	dirty_mark(tid, parent_re.pagenum);

	if (parent_re.internal->NumberOfKeys < INTERNAL_ORDER -1){
		returnum = insert_into_node(tid, rootnum, parent_re, left, key, right);
		pin_down(tid, left.pagenum);
		pin_down(tid, parent_re.pagenum);
		return returnum;
	}

	returnum = insert_into_node_after_splitting(tid, rootnum, parent_re, left, key, right);
	// pin_down(parent_re.pagenum);
	return returnum;
}


pagenum_t insert_into_node(int tid, pagenum_t rootnum, InternalPage_num parent, LeafPage_num left, int64_t key, LeafPage_num right) {
	int left_index = 0;
	int i;

	if (parent.internal->OneMorePN == left.pagenum){
		left_index = -1;
	}else{
		while (left_index <= parent.internal->NumberOfKeys){
			if (parent.internal->InternalKeyPN[left_index].PageNumber != left.pagenum)
				left_index++;
			else break;
		}
	}

	for (i = parent.internal->NumberOfKeys; i > left_index+1; i--){
		parent.internal->InternalKeyPN[i] = parent.internal->InternalKeyPN[i-1];
	}

	parent.internal->InternalKeyPN[left_index+1].Key = key;
	parent.internal->InternalKeyPN[left_index+1].PageNumber = right.pagenum;
	parent.internal->NumberOfKeys = parent.internal->NumberOfKeys + 1;

	void *ptr;
	ptr = right.leaf;
	buff_write_newpage(tid, right.pagenum, ((page_t*)ptr));

	return rootnum;
}


pagenum_t insert_into_node_after_splitting(int tid, pagenum_t rootnum, InternalPage_num parent, LeafPage_num left, int64_t key, LeafPage_num right) {
	int i, j, split;
	int64_t k_prime;
	KeyPN tempPN[INTERNAL_ORDER];
	InternalPage new_parent;
	pagenum_t new_parent_num = alloc_page(tid);

	new_parent.IsLeaf = 0;
	new_parent.NumberOfKeys = 0;


	int left_index = 0;
	if (parent.internal->OneMorePN == left.pagenum){
		left_index = -1;
	}else{
		while (left_index <= parent.internal->NumberOfKeys && parent.internal->InternalKeyPN[left_index].PageNumber != left.pagenum){
			left_index++;
		}
	}

	for (i = 0, j = 0; i < parent.internal->NumberOfKeys ; i++, j++){
		if (j == left_index + 1)j++;
		tempPN[j] = parent.internal->InternalKeyPN[i];
	}

	tempPN[left_index+1].Key =key;
	tempPN[left_index+1].PageNumber=right.pagenum;

	split = cut(INTERNAL_ORDER);
	parent.internal->NumberOfKeys = 0;
	for (i=0; i<split-1;i++){
		parent.internal->InternalKeyPN[i] = tempPN[i];
		parent.internal->NumberOfKeys = parent.internal->NumberOfKeys +1;
	}

	k_prime = tempPN[i].Key;
	new_parent.OneMorePN = tempPN[i].PageNumber;
	for (++i, j = 0;i<INTERNAL_ORDER; i++, j++){
		new_parent.InternalKeyPN[j] = tempPN[i];
		new_parent.NumberOfKeys = new_parent.NumberOfKeys +1;
	}
	new_parent.ParentPageNumber = parent.internal->ParentPageNumber;

	void *ptr;

	pin_down(tid, left.pagenum);
	ptr = right.leaf;
	buff_write_newpage(tid, right.pagenum, ((page_t*)ptr));

	ptr = &new_parent;
	buff_write_newpage(tid, new_parent_num, ((page_t*)ptr));

	update_child(tid, new_parent, new_parent_num);

	InternalPage_num n_parent;
	n_parent.pagenum = new_parent_num;
	n_parent.internal = &new_parent;


	return insert_internal_into_parent(tid, rootnum, parent, k_prime, n_parent);
}


pagenum_t insert_internal_into_parent(int tid, pagenum_t rootnum, InternalPage_num old_p, int64_t key, InternalPage_num new_p) {
	pagenum_t returnum;

	if (old_p.internal->ParentPageNumber == 0){
		returnum =  insert_internal_into_new_root(tid, old_p, key, new_p);
		pin_down(tid, old_p.pagenum);
		return returnum;
	}

	void *ptr;
	ptr = buff_read_page(tid, old_p.internal->ParentPageNumber);
	dirty_mark(tid, old_p.internal->ParentPageNumber);

	InternalPage_num parent_re;
	parent_re.pagenum = old_p.internal->ParentPageNumber;
	parent_re.internal = ptr;

	if (parent_re.internal->NumberOfKeys < INTERNAL_ORDER -1){
		returnum = insert_internal_into_node(tid, rootnum, parent_re, old_p, key, new_p);
		pin_down(tid, old_p.pagenum);
		pin_down(tid, parent_re.pagenum);
		return returnum;
	}
	returnum = insert_internal_into_node_after_splitting(tid, rootnum, parent_re, old_p, key, new_p);
	//pin_down(parent_re.pagenum);
    return returnum;
}


pagenum_t insert_internal_into_new_root(int tid, InternalPage_num left, int64_t key, InternalPage_num right) {
	InternalPage new_root;
	pagenum_t new_root_num = alloc_page(tid);
	update_header(tid, 0, new_root_num, 0);

	new_root.ParentPageNumber = 0;
	new_root.IsLeaf = 0;
	new_root.NumberOfKeys = 1;
	new_root.OneMorePN = left.pagenum;
	new_root.InternalKeyPN[0].Key = key;
	new_root.InternalKeyPN[0].PageNumber = right.pagenum;

	void *ptr;
	ptr = &new_root;
	buff_write_newpage(tid, new_root_num, ((page_t*)ptr));

    left.internal->ParentPageNumber = new_root_num;
    right.internal->ParentPageNumber = new_root_num;

    ptr = right.internal;
    buff_write_newpage(tid, right.pagenum, ((page_t*)ptr));

    return new_root_num;
}


pagenum_t insert_internal_into_node(int tid, pagenum_t rootnum, InternalPage_num parent, InternalPage_num left, int64_t key, InternalPage_num right) {
	int left_index = 0;
	int i;

	if (parent.internal->OneMorePN == left.pagenum){
		left_index = -1;
	}else{
		while (left_index <= parent.internal->NumberOfKeys){
			if (parent.internal->InternalKeyPN[left_index].PageNumber != left.pagenum)
				left_index++;
			else break;
		}
	}

	for (i = parent.internal->NumberOfKeys; i > left_index+1; i--){
		parent.internal->InternalKeyPN[i] = parent.internal->InternalKeyPN[i-1];
	}
	parent.internal->InternalKeyPN[left_index+1].Key = key;
	parent.internal->InternalKeyPN[left_index+1].PageNumber = right.pagenum;
	parent.internal->NumberOfKeys = parent.internal->NumberOfKeys + 1;


	void *ptr;

	ptr = right.internal;
	buff_write_newpage(tid, right.pagenum, ((page_t*)ptr));

   return rootnum;
}


pagenum_t insert_internal_into_node_after_splitting(int tid, pagenum_t rootnum, InternalPage_num parent, InternalPage_num left, int64_t key, InternalPage_num right) {
	int i, j, split;
	int64_t k_prime;
	KeyPN tempPN[INTERNAL_ORDER];
	InternalPage new_parent;
	pagenum_t new_parent_num = alloc_page(tid);

	new_parent.IsLeaf = 0;
	new_parent.NumberOfKeys = 0;


	int left_index = 0;
	if (parent.internal->OneMorePN == left.pagenum){
		left_index = -1;
	}else{
		while (left_index <= parent.internal->NumberOfKeys && parent.internal->InternalKeyPN[left_index].PageNumber != left.pagenum){
			left_index++;
		}
	}

	for (i = 0, j = 0; i < parent.internal->NumberOfKeys ; i++, j++){
		if (j == left_index + 1)j++;
		tempPN[j] = parent.internal->InternalKeyPN[i];
	}

	tempPN[left_index+1].Key =key;
	tempPN[left_index+1].PageNumber=right.pagenum;

	split = cut(INTERNAL_ORDER);
	parent.internal->NumberOfKeys = 0;
	for (i=0; i<split-1;i++){
		parent.internal->InternalKeyPN[i] = tempPN[i];
		parent.internal->NumberOfKeys = parent.internal->NumberOfKeys +1;
	}
	k_prime = tempPN[i].Key;
	new_parent.OneMorePN = tempPN[i].PageNumber;
	for (++i, j = 0;i<INTERNAL_ORDER; i++, j++){
		new_parent.InternalKeyPN[j] = tempPN[i];
		new_parent.NumberOfKeys = new_parent.NumberOfKeys +1;
	}
	new_parent.ParentPageNumber = parent.internal->ParentPageNumber;


	void *ptr;
	pin_down(tid, left.pagenum);
	ptr = right.internal;
	buff_write_newpage(tid, right.pagenum, ((page_t*)ptr));

	ptr = &new_parent;
	buff_write_newpage(tid, new_parent_num, ((page_t*)ptr));

	update_child(tid, new_parent, new_parent_num);


	InternalPage_num n_parent;
	n_parent.pagenum = new_parent_num;
	n_parent.internal = &new_parent;

	return insert_internal_into_parent(tid, rootnum, parent, k_prime, n_parent);
}
