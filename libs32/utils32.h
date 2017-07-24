#ifndef __libs32_utils32_h
#define __libs32_utils32_h

#include <stdlib.h>

#include "kerneltypes.h"



// some useful helpers

inline uint32_t align(uint32_t to_align, uint32_t alignator);

inline size_t mod_add(size_t pos, size_t add, size_t max);


/*
typedef struct list_node_s {
	void* next;
	void* prev;
} list_node_t;
*/

void display_buffer(uint8_t* buf, uint16_t size);

#endif
