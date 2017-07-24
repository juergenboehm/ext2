#ifndef __libs_utils_h
#define __libs_utils_h

#include "kerneltypes.h"

#define __NOINLINE	__attribute__((noinline))
#define __REGPARM		__attribute__ ((regparm(3)))
#define __PACKED		__attribute__((packed))
#define __NORETURN	__attribute__((noreturn))

#define __ALIGNED(x) __attribute__((aligned((x))))

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define EXOR(a,b) (((!(a)) && (b)) || ((a) && (!(b))))

#define DBGOUT(str, var) do {print_str(__FILE__);print_str(":");print_str(str); \
																							print_U32(var); print_newline(); } while (0)
#define DBGOUT32(str, var) do {printf(__FILE__);printf(":"); \
																printf(str); kprint_U32(var); kprint_newline(); } while (0)

#ifdef NODEBUG

#define DEBUGOUT(level, ...) do {} while(0)
#define DEBUGOUT1(level, ...) do {} while(0)
#define ASSERT(cond) do {} while (0)

#define X_DEBUGOUT(level,...) do { \
															 if (level <= DEBUG_LEVEL) \
															 { \
																 printf("[Debug: %s:%d]", __FILE__, __LINE__); \
																 printf(__VA_ARGS__); \
															 } \
															 } while (0)


#define X_ASSERT(cond) do { if (!(cond)) { DEBUGOUT(0, "%s %s\n", #cond, " failed"); while(1){}; } } while (0)


#else /* NODEBUG */


#define DEBUGOUT(level,...) do { \
															 if (level <= DEBUG_LEVEL) \
															 { \
																 printf("[Debug: %s:%d]", __FILE__, __LINE__); \
																 printf(__VA_ARGS__); \
															 } \
															 } while (0)


// We need DEBUGOUT1 for cases, where no screen is available
// to output the debugging message (before initializing the virtual vga screens).


#define DEBUGOUT1(level,...) do { \
															 if (level <= DEBUG_LEVEL) \
															 { \
																 outb_printf("[Debug: %s:%d]", __FILE__, __LINE__); \
																 outb_printf(__VA_ARGS__); \
															 } \
															 } while (0)



#define ASSERT(cond) do { if (!(cond)) { DEBUGOUT(0, "%s %s\n", #cond, " failed"); while(1){}; } } while (0)

#endif /* NODEBUG */

#define STOP while(1);

#define WAIT(n) do { long __qqq; for(__qqq = 0; __qqq < (n); ++__qqq) {volatile __xxx = 0; }; } while (0)
#define WAITVAR(var, n) do { for(var = 0; var < (n); ) {volatile var = ++var; }; } while (0)

// bit fields setting in byte



#define CLEAR_WINDOW_BYTE(byt, pos, wid) \
			(((byt) & (0xff & (uint8_t)(~(((1 << (wid)) - 1)<<(pos))))))

#define SET_BYTE_GENERIC(byt, val, pos, wid) \
			(byt) = CLEAR_WINDOW_BYTE(byt, pos, wid) | (uint8_t)((val) << (pos))


// byte strings as bits strings


#define TESTBIT(buf, i) (buf[i/8] & ((uint8_t)(1 << (i % 8))))
#define SETBIT(buf, i) buf[i/8] = buf[i/8] | ((uint8_t)(1 << (i % 8)))
#define CLRBIT(buf, i) buf[i/8] = buf[i/8] & ~((uint8_t)(1 << (i % 8)))


#endif
