#ifndef __fs_file_h
#define __fs_file_h

#include "kerneltypes.h"
#include "libs32/utils32.h"
#include "libs/lists.h"


#define FI_KEYB				0
#define	FI_SCREEN			1
#define FI_ERR				-1

#define INODE_TYPE_GENERIC		0



struct superblk_ops_s;
struct inode_ops_s;
struct dentry_ops_s;
struct file_ops_s;

struct dentry_s;
struct inode_s;


typedef struct superblock_s {

	uint32_t	s_device_id;
	struct inode_s* s_first_inode;
	struct inode_s* s_mount_point;

	struct dentry_s* s_root;

	short s_blocksize;
	short s_blocksize_bits;  // blocksize in bits;
	
	uint32_t	s_filesystem_type;
	struct superblk_ops_s* s_ops;
	
} superblock_t;


typedef struct superblk_ops_s {

	int (*read_inode)(struct inode_s* inode);
	int (*write_inode)(struct inode_s* inode);
} superblk_ops_t;



typedef	struct inode_s {

	unsigned long i_ino;				// inode number !! uniquely identifies this inode !!

	uint32_t	i_device;								// device number of device holding this inode
	
	uint32_t i_mode;									// mode of this inode
	
	uint32_t i_user;									// user who owns this inode
	uint32_t i_group;								// group who owns this inode
	
	uint32_t i_creat_time;						// creation time
	uint32_t i_mod_time;							// modification time
	uint32_t i_write_time;						// last write time

	struct inode_ops_s* i_ops;	// inode operations
	struct file_ops_s* i_fops;	// associated file operations
	
	unsigned int i_blkbits;	 		// blocksize in bits

	unsigned long	i_blocks;	 		// number of blocks in file
	unsigned short i_bytes;  		// number of bytes in last block
	loff_t i_size;					 		// file size in bytes

} inode_t;


typedef struct inode_ops_s {

	int (*create)(inode_t* dir, struct dentry_s* dentry, uint32_t mode);
	struct dentry_s* (*lookup)(inode_t* dir, struct dentry_s* dentry);
	int (*unlink)(inode_t* dir, struct dentry_s* dentry);
	int (*mkdir)(inode_t* dir, struct dentry_s* dentry, uint32_t mode);
	int (*rmdir)(inode_t* dir, struct dentry_s* dentry);
	int (*mknod)(inode_t* dir, struct dentry_s* dentry, uint32_t mode, uint32_t rdev);
	int (*rename)(inode_t* old_dir, struct dentry_s* old_dentry, inode_t* new_dir, struct dentry_s* new_dentry);
	
} inode_ops_t;


typedef struct dentry_s {

	char* d_name;
	
	struct dentry_s* d_parent;
	
	list_head_t d_subdirs;
	list_head_t d_siblings;

	inode_t* d_inode;
	
	int	d_count;

} dentry_t;


typedef struct dentry_ops_s {


} dentry_ops_t;




typedef struct file_s {
	
	uint32_t f_mode;
	loff_t f_pos;									// current file offset
	
	struct dentry_s* f_dentry;
	
	struct file_ops_s* f_fops;

	uint32_t f_flags;

	int f_count;

} file_t;


typedef struct dirent_s {
	
	inode_t* d_inode;
	char d_name[1];

} dirent_t;

typedef int (*filldir_t)(void* dirent,
		const char* fname, int fname_len, loff_t pos, uint32_t inode , uint32_t file_type);

typedef struct file_ops_s {

	int (*llseek)(file_t* fil, size_t offset, size_t origin);
	int (*read)(file_t* fil, char* buf, size_t count, size_t* offset);
	int (*write)(file_t* fil, char* buf, size_t count, size_t* offset);
	int (*ioctl)(inode_t* inode, file_t* fil, uint32_t cmd, uint32_t arg);
	int (*open)(inode_t* inode, file_t* fil);
	int (*release)(inode_t* inode, file_t* fil);
	int (*readdir)(file_t* file, void* dirent, filldir_t filldir);

	//device driver operations for blockdrivers (dd_blksize > 0)
	//readblk, writeblk communicate with the hardware
	//in implementations of concrete drivers
	//read and write are programmed generically to use
	//readpage, writepage
	// fil is here for example
	// a file pointing to a dentry pointing to the inode /dev/sda<x>
	// or /dev/sda
	int (*readblk)(file_t* fil, uint32_t blk_index, char **buf);
	int (*writeblk)(file_t* fil, uint32_t blk_index, char *buf);
	//readpage and writepage use readblk and writeblk,
	//readpage and writepage are part of the
	//global buffer system
	int (*readpage)(file_t* fil, uint32_t page_index, char **buf);
	int (*writepage)(file_t* fil, uint32_t page_index, char *buf);



} file_ops_t;



int do_open(char* fname, uint32_t fmode);

void init_base_files();

inode_t * create_inode(int type, inode_t* buf);
file_t* create_file(int type, file_t* buf);
dirent_t* create_dirent(int type, dirent_t* buf);

file_ops_t* create_file_ops(int type, file_ops_t* buf);


#define INIT_FIXED_LISTS_LEN 32

#define DEV_IDE		8
#define DEV_IDE1	9




extern file_t fixed_file_list[INIT_FIXED_LISTS_LEN];
extern dentry_t fixed_dentry_list[INIT_FIXED_LISTS_LEN];
extern inode_t fixed_inode_list[INIT_FIXED_LISTS_LEN];

#define MAX_NUM_FILE_OPS_TYPES 16

extern file_ops_t file_ops_table[MAX_NUM_FILE_OPS_TYPES];

extern int hdd_fd;


#endif
