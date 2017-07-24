

#include "libs32/klib.h"

#include "libs/lists.h"

#ifndef COMPILE_LIST_TESTS

void do_list_tests()
{

}

#endif


#ifdef COMPILE_LIST_TESTS

void print_list(struct list_head* lholder)
{
	printf("[");
	if (lholder){
		struct list_head *first = lholder;
		struct list_head *last = lholder->prev;

		struct list_head *p = first;
		int j = 0;
		do {
			printf("%d ", container_of(p, test_list_struct_t, link)->val);
			p = p->next;
			++j;
			if (j > 20) {
				printf ("...err");
				break;
			}
		} while (p != first);
	}
	printf("]\n");
}


void init_elems(test_list_struct_t * elem, int len)
{
	int i;

	for(i = 0; i < len; ++i) {
		elem[i].val = i;
	}

}




void compare_list(int* proofvals, list_head_t* lholder)
{
	int i = 0;
	list_head_t* p = lholder;
	list_head_t* last = lholder->prev;

	while (proofvals[i] != -1) {

		if (proofvals[i] != container_of(p, test_list_struct_t, link)->val) {
			printf("ERROR ERROR ERROR: ");
			printf("list = ");
			print_list(lholder);

			printf("expected = [");
			int j = 0;
			while (proofvals[j] != -1) {
				printf("%d ", proofvals[j]);
				++j;
			}
			printf(" ]\n");
		}

		++i;
		p = p->next;
		if (p == lholder) {
			break;
		}
	}

	if (proofvals[i] != -1) {
		printf("ERROR ERROR ERROR: generated list too short.\n");
	} else {

		if (p != lholder) {
			printf("ERROR ERROR ERROR: generated list too long.\n");
		}
	}

}




void check_prepend_list()
{
	const int nelem = 10;
	test_list_struct_t elem[nelem];

	int i;

	init_elems(elem, nelem);

	struct list_head* lholder = 0;

	printf("check_prepend_list:\n\n");

	print_list(lholder);

	int proofvals_ll[5][6] = {{0,-1},{1,0,-1},{2,1,0,-1},{3,2,1,0,-1},{4,3,2,1,0,-1}};

	// prepend 0 1 2 3 4 respectively in the loop
	for(i = 0; i < 5; ++i) {
		prepend_list(&lholder, &(elem[i].link));
		print_list(lholder);

		compare_list(proofvals_ll[i], lholder);
	}

	// list should be [4 3 2 1 0] here

	print_list(lholder);

}


void check_append_list()
{
	const int nelem = 10;
	test_list_struct_t elem[nelem];

	int i;

	init_elems(elem, nelem);

	struct list_head* lholder = 0;

	printf("check_append_list:\n\n");

	print_list(lholder);

	int proofvals_ll[5][6] = {{0,-1},{0,1,-1},{0,1,2,-1},{0,1,2,3,-1},{0,1,2,3,4,-1}};

	// append 0 1 2 3 4 respectively in the loop
	for(i = 0; i < 5; ++i) {
		append_list(&lholder, &(elem[i].link));
		print_list(lholder);

		compare_list(proofvals_ll[i], lholder);
	}

	// list should be [0 1 2 3 4] here

	print_list(lholder);



}

void check_delete_elem()
{
	const int nelem = 10;
	test_list_struct_t elem[nelem];

	int i;

	init_elems(elem, nelem);

	struct list_head* lholder = 0;

	print_list(lholder);

	printf("check_delete_elem:\n\n");

	int proofvals_ll[5][6] = {{0,-1},{1,0,-1},{2,1,0,-1},{3,2,1,0,-1},{4,3,2,1,0,-1}};

	// prepend 0 1 2 3 4 respectively in the loop
	for(i = 0; i < 5; ++i) {
		prepend_list(&lholder, &(elem[i].link));
		print_list(lholder);

		compare_list(proofvals_ll[i], lholder);

	}

	// list should be [4 3 2 1 0] here

	int proofvals_ll1[5][5] = {{3,2,1,0,-1},{2,1,0,-1},{1,0,-1},{0,-1},{-1}};


	// delete in the loop the first element respectively
	for(i = 0; i < 5; ++i) {
		printf("delete %d:", i);
		delete_elem(&lholder, lholder);
		print_list(lholder);

		compare_list(proofvals_ll1[i], lholder);
	}

	// list should be empty here

	// prepend 0 1 2 3 4 respectively in the loop
	for(i = 0; i < 5; ++i) {
		prepend_list(&lholder, &(elem[i].link));
		print_list(lholder);

		compare_list(proofvals_ll[i], lholder);
	}

	// list should be [4 3 2 1 0] here

	int proofvals_ll2[5][5] = {{4,3,2,1,-1},{4,3,2,-1},{4,3,-1},{4,-1},{-1}};

	// delete in the loop the last element respectively
	for(i = 0; i < 5; ++i) {
		printf("delete %d:", i);
		delete_elem(&lholder, lholder->prev);
		print_list(lholder);

		compare_list(proofvals_ll2[i], lholder);
	}


}

void check_insert_before()
{
	const int nelem = 10;
	test_list_struct_t elem[nelem];

	int i;

	init_elems(elem, nelem);

	struct list_head* lholder = 0;

	print_list(lholder);

	printf("check_insert_before:\n\n");

	prepend_list(&lholder, &(elem[0].link));
	print_list(lholder);

	prepend_list(&lholder, &(elem[9].link));
	print_list(lholder);

	// list should be [9 0] here

	int proofvals_ll[4][7] = {{9,1,0,-1},{9,2,1,0,-1},{9,3,2,1,0,-1},{9,4,3,2,1,0,-1}};

	// insert 1 2 3 4 before second element of list
	for(i = 1; i < 5; ++i) {
		insert_before(&lholder, &(elem[i].link), lholder->next );
		print_list(lholder);

		compare_list(proofvals_ll[i-1], lholder);
	}

	// delete the last element in each loop
	for(i = 0; i < 6; ++i) {
		delete_elem(&lholder, lholder->prev);
		print_list(lholder);
	}

	// put a 0 in the empty list
	prepend_list(&lholder, &(elem[0].link));
	print_list(lholder);

	int proofvals_ll1[4][6] = {{1,0,-1},{1,2,0,-1},{1,2,3,0,-1},{1,2,3,4,0,-1}};

	// insert 1 2 3 4 in the loop, everytime before the last element
	for(i = 1; i < 5; ++i) {
		insert_before(&lholder, &(elem[i].link), lholder->prev);
		print_list(lholder);

		compare_list(proofvals_ll1[i-1], lholder);
	}


}

void check_insert_after()
{
	const int nelem = 10;
	test_list_struct_t elem[nelem];

	int i;

	init_elems(elem, nelem);

	struct list_head* lholder = 0;

	print_list(lholder);

	printf("check_insert_after:\n\n");

	prepend_list(&lholder, &(elem[0].link));
	print_list(lholder);

	prepend_list(&lholder, &(elem[9].link));
	print_list(lholder);

	// list should be [9 0] here

	int proofvals_ll[4][7] = {{9,0,1,-1},{9,0,2,1,-1},{9,0,3,2,1,-1},{9,0,4,3,2,1,-1}};

	// insert 1 2 3 4 every loop after the second element
	for(i = 1; i < 5; ++i) {
		insert_after(&lholder, &(elem[i].link), lholder->next );
		print_list(lholder);

		compare_list(proofvals_ll[i-1], lholder);
	}

	// delete the last element in each loop
	for(i = 0; i < 6; ++i) {
		delete_elem(&lholder, lholder->prev);
		print_list(lholder);
	}

	// put a 0 in the empty list
	prepend_list(&lholder, &(elem[0].link));
	print_list(lholder);

	int proofvals_ll1[4][6] = {{0,1,-1},{0,1,2,-1},{0,1,2,3,-1},{0,1,2,3,4,-1}};

	// insert 1 2 3 4 in the loop, everytime after the last element
	for(i = 1; i < 5; ++i) {
		insert_after(&lholder, &(elem[i].link), lholder->prev);
		print_list(lholder);

		compare_list(proofvals_ll1[i-1], lholder);
	}

	// delete the last element in each loop
	for(i = 0; i < 5; ++i) {
		delete_elem(&lholder, lholder->prev);
		print_list(lholder);
	}

	// put a 0 in the empty list
	prepend_list(&lholder, &(elem[0].link));
	print_list(lholder);

	int proofvals_ll2[4][6] = {{0,1,-1},{0,2,1,-1},{0,3,2,1,-1},{0,4,3,2,1,-1}};

	// insert 1 2 3 4 in the loop, everytime after the first element
	for(i = 1; i < 5; ++i) {
		insert_after(&lholder, &(elem[i].link), lholder);
		print_list(lholder);

		compare_list(proofvals_ll2[i-1], lholder);
	}


}


void do_list_tests ()
{

	check_prepend_list();
	WAIT(5*(1<<24));
	waitkey();

	check_append_list();
	WAIT(5*(1<<24));
	waitkey();

	check_delete_elem();
	WAIT(5*(1<<24));
	waitkey();

	check_insert_before();
	WAIT(5*(1<<24));
	waitkey();

	check_insert_after();
	WAIT(5*(1<<24));
	waitkey();

	printf("checks finished.\n");

	while(1) {};
}

#endif
