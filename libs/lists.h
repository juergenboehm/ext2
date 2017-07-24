#ifndef __libs_lists_h
#define __libs_lists_h

typedef struct list_head {
	struct list_head *next, *prev;
} list_head_t;

#define offsetof(type, member)  __builtin_offsetof (type, member)

#define container_of(ptr,type,member) \
	((type*) (((char*)(1 ? (ptr) : &(((type*) 0)->member))) - offsetof(type, member)))


// list_holder is of type list_head_t**, p_new_el, p_next_el are of type "list_head_t*"
// list_holder is the head/holder of the list
//
#define prepend_list(list_holder,p_new_el_arg) \
		do { \
			struct list_head* p_new_el = p_new_el_arg; \
			if (! *list_holder) { \
				(*list_holder) = p_new_el; \
				(p_new_el)->next = p_new_el; \
				(p_new_el)->prev = p_new_el; \
			} else { \
				struct list_head* first = *list_holder; \
				struct list_head* last = (*list_holder)->prev; \
				first->prev = p_new_el; \
				(p_new_el)->next = first; \
				(p_new_el)->prev = last; \
				last->next = p_new_el; \
				(*list_holder) = p_new_el; \
			} \
		} while (0)


#define append_list(list_holder,p_new_el_arg) \
		do { \
			struct list_head* p_new_el = p_new_el_arg; \
			if (! *list_holder) { \
				(*list_holder) = p_new_el; \
				p_new_el->next = p_new_el; \
				p_new_el->prev = p_new_el; \
			} else { \
				struct list_head* last = (*list_holder)->prev; \
				p_new_el->next = last->next; \
				p_new_el->prev = last; \
				(last->next)->prev = p_new_el; \
				last->next = p_new_el; \
			} \
		} while (0)

#define insert_before(list_holder,p_new_el_arg,p_next_el_arg) \
\
do { \
	struct list_head* first = *list_holder; \
	struct list_head* p_new_el = p_new_el_arg; \
	struct list_head* p_next_el = p_next_el_arg; \
	p_new_el->next = p_next_el; \
	p_new_el->prev = (p_next_el)->prev; \
	(p_next_el->prev)->next = p_new_el; \
	p_next_el->prev = p_new_el; \
	if (p_next_el == first) { \
		(*list_holder) = p_new_el; \
	} \
} while (0)


#define insert_after(list_holder,p_new_el_arg,p_prev_el_arg) \
\
do { \
	struct list_head* p_new_el = p_new_el_arg; \
	struct list_head* p_prev_el = p_prev_el_arg; \
	p_new_el->next = p_prev_el->next; \
	p_new_el->prev = p_prev_el; \
	(p_prev_el->next)->prev = p_new_el; \
	p_prev_el->next = p_new_el; \
} while (0)


#define delete_elem(list_holder,p_del_el_arg) \
\
	do { \
		struct list_head* p_del_el = p_del_el_arg; \
		struct list_head *first = (*list_holder); \
		struct list_head *last = (*list_holder)->prev; \
		if (p_del_el == first) { \
				if (first == last) { \
					*list_holder = 0; \
				} else { \
					last->next = first->next; \
					(first->next)->prev = last; \
					(*list_holder) = first->next; \
				} \
		} else { \
			(p_del_el->prev)->next = p_del_el->next; \
			(p_del_el->next)->prev = p_del_el->prev; \
		} \
	} while (0)



// for iterating

#define INIT_LISTVAR(var) \
	struct list_head* var

#define FORLIST(var, init) \
	var = (init); \
	if (var) \
	{ \
		do

#define END_FORLIST(var, final) \
		while (var != (final)); }

// for testing

#define COMPILE_LIST_TESTS

void do_list_tests ();

#ifdef COMPILE_LIST_TESTS

typedef struct test_list_struct_s {
	int val;
	struct list_head link;
} test_list_struct_t;



#endif
#endif
