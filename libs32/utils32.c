
#include <stdlib.h>
#include <stdio.h>

#include "libs32/klib.h"
#include "libs32/utils32.h"


// useful little functions

inline uint32_t align(uint32_t to_align, uint32_t alignator)
{
	return ((to_align + alignator - 1)/alignator) * alignator;
}


inline size_t mod_add(size_t pos, size_t add, size_t max)
{
  pos += add;
  pos = pos % max;
  return pos;
}

// bigger utils


void display_buffer(uint8_t* buf, uint16_t size)
{
	int i = 0;
	int j = 0;

	for(i = 0; i < size / 16; ++i)
	{
		printf("%02x : ", i);

		for(j = 0; j < 16; ++j)
		{
			printf("%02x ", buf[i * 16 + j]);
		}

		for(j = 0; j < 16; ++j)
		{
			uint8_t val = buf[i * 16 + j];
			printf("%c", (val >= 0x20 && val <= 0x7f) ? val : '.' );
		}
		//printf("\n");

		getc(stdin);
	}
}
