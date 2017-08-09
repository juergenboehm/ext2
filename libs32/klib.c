
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include "libs32/klib.h"

#define PRINTF_BUFFER_LEN	2048

int outb_printf(char* format, ... )
{

	char buffer[PRINTF_BUFFER_LEN];

	va_list ap;

	va_start(ap, format);

	int nn = vsprintf(buffer, format, ap);

	va_end(ap);

if (OUTB_PRINTF_ON)
{
	printf("\noutb_printf: %s", buffer);
}

	return nn;
}



bool in_delim(char c, char* delims)
{
	char *q = delims;
	while (*q)
	{
		if (*q == c)
		{
			break;
		}
		++q;
	}
	return (bool)*q;
}


void parse_buf(char* buf, int len, char* delims, int* argc, char* argv[])
{
	char* p = buf;
	int argc_local = 0;

	*argc = argc_local;

	while (p - buf < len)
	{
		if (argc_local && *p)
		{
			*p = 0;
			++p;
		}
		while (in_delim(*p, delims))
		{
			++p;
		}

		if (*p)
		{
			argv[argc_local] = p;
			++argc_local;
			while (*p && !in_delim(*p, delims))
				++p;
		}
		else
		{
			break;
		}
	}

	*argc = argc_local;

	//printf("parse_buf: argc = %d\n", *argc);

}


