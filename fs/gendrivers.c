
#include <stdlib.h>
#include <stdio.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>


#include "libs32/klib.h"
#include "fs/vfs.h"
#include "fs/gendrivers.h"


device_driver_t dev_drv_table[DEV_DRIVER_TABLE_LEN];

file_ops_t vga_driver_ops;
file_ops_t keyb_driver_ops;
file_ops_t ide_driver_ops;
file_ops_t tty_driver_ops;

void init_file_ops_line(file_ops_t *pddops,
		void* llseek_proc,
		void* read_proc, void* write_proc,
		void* ioctl_proc,
		void* open_proc, void* release_proc, void* readdir_proc,
		void* readblk, void* writeblk, void* readpage, void* writepage)
{
	pddops->llseek = llseek_proc;
	pddops->read = read_proc;
	pddops->write = write_proc;
	pddops->ioctl = ioctl_proc;
	pddops->open = open_proc;
	pddops->release = release_proc;
	pddops->readdir = readdir_proc;

	pddops->readblk = readblk;
	pddops->writeblk = writeblk;

	pddops->readpage = readpage;
	pddops->writepage = writepage;
}


int readblk_ide(file_t* fil, uint32_t blk_index, char **buf)
{
	uint32_t dev_major = GET_MAJOR_DEVICE_NUMBER(fil->f_dentry->d_inode->i_device);
	uint32_t dev_minor = GET_MINOR_DEVICE_NUMBER(fil->f_dentry->d_inode->i_device);

	if (!*buf)
	{
		return -1;
	}

	int ret;

	if (dev_minor == 0)
	{
		//read from /dev/ide

		ret = lseek(hdd_fd, blk_index * IDE_BLKSIZE, SEEK_SET);
		if (ret < 0)
		{
			return -1;
		}

	  ret = read(hdd_fd, *buf, IDE_BLKSIZE);
	  if (ret < 0)
	  {
	  	return -1;
	  }

	}
	else if (dev_minor == 1)
	{
		//read from /dev/ide1
		ret = lseek(hdd_fd, (blk_index + 4096) * IDE_BLKSIZE, SEEK_SET);
		if (ret < 0)
		{
			return -1;
		}

	  ret = read(hdd_fd, *buf, IDE_BLKSIZE);
	  if (ret < 0)
	  {
	  	return -1;
	  }

	}

}

int writeblk_ide(file_t* fil, uint32_t blk_index, char *buf)
{
	//printf("writeblk_ide: blk_index = %d\n", blk_index);

	int ret;
	uint32_t dev_major = GET_MAJOR_DEVICE_NUMBER(fil->f_dentry->d_inode->i_device);
	uint32_t dev_minor = GET_MINOR_DEVICE_NUMBER(fil->f_dentry->d_inode->i_device);

	if (dev_minor == 0)
	{
		//read from /dev/ide

		if (blk_index > 4095)
		{
			printf("writeblk_ide: blkindex %d must be less than 4096.\n", blk_index);
			ret = -1;
			goto ende;
		}

		ret = lseek(hdd_fd, blk_index * IDE_BLKSIZE, SEEK_SET);
		if (ret < 0)
		{
			ret = -1;
			goto ende;
		}

	  ret = write(hdd_fd, buf, IDE_BLKSIZE);
	  if (ret < 0)
	  {
	  	goto ende;
	  }

	}
	else if (dev_minor == 1)
	{

		ret = lseek(hdd_fd, (blk_index + 4096) * IDE_BLKSIZE, SEEK_SET);
		if (ret < 0)
		{
			ret = -1;
			goto ende;
		}

	  ret = write(hdd_fd, buf, IDE_BLKSIZE);
	  if (ret < 0)
	  {
	  	goto ende;
	  }
	}
ende:
	return ret;
}


void init_file_ops()
{
	init_file_ops_line(&ide_driver_ops, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			&readblk_ide, &writeblk_ide, NULL, NULL);


}


void init_device(device_driver_t* pdd, uint32_t rdev, uint32_t pos, file_ops_t* pddfops)
{
	pdd->dd_rdev = rdev;
	pdd->dd_pos = 0;
	pdd->dd_blksize = 0;
	pdd->dd_fops = pddfops;
}

void init_blk_device(device_driver_t* pdd, uint32_t rdev, uint32_t blksize,
		uint32_t pos, file_ops_t* pddfops)
{
	pdd->dd_rdev = rdev;
	pdd->dd_pos = 0;
	pdd->dd_blksize = blksize;
	pdd->dd_fops = pddfops;
}


void init_device_table()
{
	init_file_ops();

	init_blk_device(&dev_drv_table[DEV_IDE_INDEX], MAKE_DEVICE_NUMBER(DEV_IDE_INDEX,0),
			IDE_BLKSIZE, 0, &ide_driver_ops);

}
