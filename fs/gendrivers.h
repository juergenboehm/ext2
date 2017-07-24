#ifndef __fs_gendrivers_h
#define __fs_gendrivers_h


#include "kerneltypes.h"
#include "libs32/utils32.h"

#include "fs/vfs.h"

#define MAKE_DEVICE_NUMBER(major,minor) (((major) << 16) + (minor))
#define GET_MINOR_DEVICE_NUMBER(x) ((x) & ((1 << 16)-1))
#define GET_MAJOR_DEVICE_NUMBER(x) ((x) >> 16)

#define IS_BLK_DEVICE(x) ((x)->dd_blksize > 0)


#define DEV_VGA_INDEX		1
#define DEV_KBD_INDEX	0
#define DEV_IDE_INDEX		2
#define DEV_TTY_INDEX		3

#define IDE_BLKSIZE	512

//typedef struct device_driver_ops_s device_driver_ops_t;


typedef struct device_driver_s {

	uint32_t dd_pos;

	uint32_t dd_rdev;

	file_ops_t* dd_fops;

	uint32_t dd_blksize; // if it is a blockdriver

} device_driver_t;

/*
typedef struct device_driver_ops_s {

	int (*llseek)(device_driver_t* drv, size_t offset, size_t origin);
	int (*read)(device_driver_t* drv, char* buf, size_t count, size_t offset);
	int (*write)(device_driver_t* drv, char* buf, size_t count, size_t offset);
	int (*ioctl)(device_driver_t* drv, uint32_t cmd, uint32_t arg);
	int (*open)(device_driver_t* drv);
	int (*close)(device_driver_t* drv);

} device_driver_ops_t;
*/

extern file_ops_t vga_driver_ops;
extern file_ops_t keyb_driver_ops;
extern file_ops_t ide_driver_ops;
extern file_ops_t tty_driver_ops;

#define DEV_DRIVER_TABLE_LEN	32

extern device_driver_t dev_drv_table[DEV_DRIVER_TABLE_LEN];


void init_file_ops();
void init_device(device_driver_t* pdd, uint32_t major, uint32_t pos, file_ops_t* pddops);
void init_device_table();

















#endif
