#ifndef __libs32_klib_h
#define __libs32_klib_h

#include "kerneltypes.h"

#define OUTB_PRINTF_ON	1


// some typical defines

//#define	MAX_INT			((int)(~(unsigned int)(1 << (sizeof(int) - 1))))
//#define MAX_UINT    (~((unsigned int)(0)))


void parse_buf(char* buf, int len, char* delims, int* argc, char* argv[]);


#endif
