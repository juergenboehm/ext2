#ifndef __libs_kerneldefs_h
#define __libs_kerneldefs_h

// definitions for the kernel16 phase

#define KERNEL16_SEG	0x2000
#define	VGA_SEG				0xb800
#define	KERNEL16_SP		0xf000

#define BIOS_MEM_AREA_HANDLE 				0xff00
#define BIOS_MEM_AREA_TABLE_ADDR		0x30000

#define KERNEL_16_CS_GDT_INDEX  2
#define KERNEL_16_DS_GDT_INDEX  3

#define KERNEL_32_CS_KERNEL_GDT_INDEX  4
#define KERNEL_32_DS_KERNEL_GDT_INDEX  5

#define KERNEL_32_CS_USER_GDT_INDEX  6
#define KERNEL_32_DS_USER_GDT_INDEX  7

// common definitions

#define UPL 3
#define KPL 0


// definitions for the kernel32 phase

#define	KERNEL32_CS			(KERNEL_32_CS_KERNEL_GDT_INDEX * 8)
#define	KERNEL32_DS			(KERNEL_32_DS_KERNEL_GDT_INDEX * 8)

#define	USER32_CS			(KERNEL_32_CS_USER_GDT_INDEX * 8 + 3)
#define	USER32_DS			(KERNEL_32_DS_USER_GDT_INDEX * 8 + 3)


#define KERNEL_STACK_PROVIS 0x1ff00

// paging

#define PAGE_DIR_ADDRESS_PROVIS 0x52000

#define PAGE_TABLE_4M_PROVIS 0x50000
#define PAGE_TABLE_8M_PROVIS 0x51000

// memory layout

// 0x40000000 = 1G
//#define MAX_ADDRESS_PHYS 0x3fffffff
#define MAX_ADDRESS_PHYS 0x0fffffff


#define KERNEL_UPPER	0xc0000000

#define TOP_ADDR 0xffffffff

#define KERNEL_32_START 0x100000

#define KERNEL_32_START_UPPER (KERNEL_32_START + KERNEL_UPPER)

#define USER_PROC_RESERVED_UPPER	0x100

#define USER32_STACK	(0xc0000000 - USER_PROC_RESERVED_UPPER)


// initially allocated PAGES

#define NUM_INIT_PAGE_DIR_ENTRIES 7

#define VGA_ADDR	((VGA_SEG << 4) + KERNEL_UPPER)


#define __PHYS(x) ((void*)((uint32_t)(x) - KERNEL_UPPER))
#define __VADDR(x) ((void*)((uint32_t)(x) + KERNEL_UPPER))

// irq system

#define	IRQ_JUMP_TABLE_ALIGN	16

// ERRORS

#define ERR_BUDDY_INIT_FAILED 1




#endif
