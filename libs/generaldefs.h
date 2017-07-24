#ifndef __libs_generaldefs_h
#define __libs_generaldefs_h

#include "kerneltypes.h"

#define KB 1024
#define MB (KB * KB)


#define BASE_PLUS_IDX_TIMES_MULT_TYP(base, index, mult, type) \
      (((type)base) + (((type)(index)) * mult))



#endif
