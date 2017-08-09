

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>



#include "libs32/klib.h"

#include "fs/ext2.h"
#include "fs/gendrivers.h"
#include "fs/vfs.h"


file_t fixed_file_list[INIT_FIXED_LISTS_LEN];
dentry_t fixed_dentry_list[INIT_FIXED_LISTS_LEN];
inode_t fixed_inode_list[INIT_FIXED_LISTS_LEN];


file_ops_t device_driver_file_ops;

file_ops_t device_driver_blk_file_ops;

file_ops_t ext2_file_ops;

int hdd_fd;



int ddriv_llseek(file_t* fil, size_t offset, size_t origin)
{
	int ret = -1;

	fil->f_pos = offset;

	ret = 0;

	uint32_t dev_major = GET_MAJOR_DEVICE_NUMBER(fil->f_dentry->d_inode->i_device);
	device_driver_t* pdd = &dev_drv_table[dev_major];
	file_ops_t* pfops = pdd->dd_fops;

	if (pfops && pfops->llseek)
	{
		ret = pfops->llseek(fil, offset, origin);
	}
	return ret;
}

int ddriv_read(file_t* fil, char* buf, size_t count, size_t* offset)
{
	int ret = -1;

	uint32_t dev_major = GET_MAJOR_DEVICE_NUMBER(fil->f_dentry->d_inode->i_device);
	device_driver_t* pdd = &dev_drv_table[dev_major];

	//outb_printf("ddriv_write reached: dev_major = %d ", dev_major);

	file_ops_t* pfops = pdd->dd_fops;

	if (pfops && pfops->read)
	{
		ret = pfops->read(fil, buf, count, offset);
	}
	return ret;
}

int ddriv_write(file_t* fil, char* buf, size_t count, size_t* offset)
{
	int ret = -1;

	uint32_t dev_major = GET_MAJOR_DEVICE_NUMBER(fil->f_dentry->d_inode->i_device);
	device_driver_t* pdd = &dev_drv_table[dev_major];

	//outb_printf("ddriv_write reached: dev_major = %d ", dev_major);

	file_ops_t* pfops = pdd->dd_fops;

	if (pfops && pfops->write)
	{
		ret = pfops->write(fil, buf, count, offset);
	}
	return ret;
}

int ddriv_ioctl(inode_t* inode, file_t* fil, uint32_t cmd, uint32_t arg)
{
	return -1;
}

int ddriv_open(inode_t* inode, file_t* fil)
{
	int ret = -1;
	uint32_t dev_major = GET_MAJOR_DEVICE_NUMBER(inode->i_device);
	device_driver_t* pdd = &dev_drv_table[dev_major];

	file_ops_t* pfops = pdd->dd_fops;

	if (pfops && pfops->open)
	{
		ret = pfops->open(inode, fil);
	}
	return ret;
}


int ddriv_release(inode_t* inode, file_t* fil)
{
	int ret = -1;
	uint32_t dev_major = GET_MAJOR_DEVICE_NUMBER(inode->i_device);
	device_driver_t* pdd = &dev_drv_table[dev_major];

	file_ops_t* pfops = pdd->dd_fops;

	if (pfops && pfops->release)
	{
		ret = pfops->release(inode, fil);
	}

	return ret;
}

int ddriv_readdir(file_t* file, void* dirent, filldir_t filldir)
{
	return -1;
}

//TO DO: test the code, first test done
int ddriv_read_blk(file_t* fil, char* buf, size_t count, size_t* offset)
{
	//int ret = -1;

	uint32_t dev_major = GET_MAJOR_DEVICE_NUMBER(fil->f_dentry->d_inode->i_device);
	device_driver_t* pdd = &dev_drv_table[dev_major];

	file_ops_t* pfops = pdd->dd_fops;

	uint32_t offset_akt = fil->f_pos;

	uint32_t blksize = pdd->dd_blksize;

	uint32_t akt_count = count;

	char* blk_buf = (char*)malloc(blksize);
	char* pbuf = buf;

	uint32_t nrd_total = 0;

	while (akt_count > 0)
	{

		uint32_t blk_rd_index = offset_akt/blksize;
		uint32_t blk_offset = offset_akt % blksize;

		uint32_t ncnt = min(blksize - blk_offset, akt_count);

		int nrd = pfops->readblk(fil, blk_rd_index, &blk_buf);

		if (nrd < 0)
		{
			goto ende;
		}

		nrd = min(nrd, ncnt);

		//outb_printf("ddrv_read_blk: nrd = %d, blk_rd_index = %d, blk_offset = %d\n",
		//		nrd, blk_rd_index, blk_offset);

		memcpy(pbuf, &blk_buf[blk_offset], nrd );

		pbuf += nrd;

		offset_akt += nrd;
		akt_count -= nrd;

		nrd_total += nrd;
	}

ende:
	free(blk_buf);

	fil->f_pos = offset_akt;

	return nrd_total;
}

//
// for block devices some driver functions are special
//

//TO DO: test the code, doing first test
int ddriv_write_blk(file_t* fil, char* buf, size_t count, size_t* offset)
{
	//int ret = -1;

	uint32_t dev_major = GET_MAJOR_DEVICE_NUMBER(fil->f_dentry->d_inode->i_device);
	device_driver_t* pdd = &dev_drv_table[dev_major];

	//outb_printf("ddriv_write reached: dev_major = %d ", dev_major);

	file_ops_t* pfops = pdd->dd_fops;

	uint32_t offset_akt = fil->f_pos;
	uint32_t blksize = pdd->dd_blksize;

	uint32_t akt_count = count;

	char* blk_buf = (char*)malloc(blksize);
	char* pbuf = buf;

	uint32_t nwrt_total = 0;

	while (akt_count > 0)
	{

		uint32_t blk_wrt_index = offset_akt/blksize;
		uint32_t blk_offset = offset_akt % blksize;

		uint32_t ncnt = min(blksize - blk_offset, akt_count);

		int nrd;
		if (ncnt < blksize)
		{
			nrd = pfops->readblk(fil, blk_wrt_index, &blk_buf);
		}

		if (nrd < 0)
		{
			goto ende;
		}


		memcpy(&blk_buf[blk_offset], pbuf, ncnt );

		uint32_t nwrt = blksize;
		//outb_printf("simu writeblk: pbo = %d blk_wrt_index = %d : ",
		//		(uint32_t)(pbuf - buf), blk_wrt_index);
		nwrt = pfops->writeblk(fil, blk_wrt_index, blk_buf);

		if (nwrt < 0)
		{
			goto ende;
		}

		nwrt_total += ncnt;

		pbuf += ncnt;

		offset_akt += ncnt;
		akt_count -= ncnt;
	}

ende:

	free(blk_buf);

	fil->f_pos = offset_akt;


	return nwrt_total;
}



int ddriv_readblk(file_t* fil, uint32_t blk_index, char **buf)
{
	int ret = -1;

	uint32_t dev_major = GET_MAJOR_DEVICE_NUMBER(fil->f_dentry->d_inode->i_device);
	device_driver_t* pdd = &dev_drv_table[dev_major];

	file_ops_t* pfops = pdd->dd_fops;

	if (pfops && pfops->readblk)
	{
		ret = pfops->readblk(fil, blk_index, buf);
	}
	return ret;
}

int ddriv_writeblk(file_t* fil, uint32_t blk_index, char *buf)
{
	int ret = -1;

	uint32_t dev_major = GET_MAJOR_DEVICE_NUMBER(fil->f_dentry->d_inode->i_device);
	device_driver_t* pdd = &dev_drv_table[dev_major];

	file_ops_t* pfops = pdd->dd_fops;

	if (pfops && pfops->writeblk)
	{
		ret = pfops->writeblk(fil, blk_index, buf);
	}
	return ret;
}



int set_file_ops(file_ops_t* fops,
		void *llseek,
		void *read,
		void *write,
		void *ioctl,
		void *open,
		void *release,
		void *readdir,
		void *readblk,
		void *writeblk,
		void *readpage,
		void *writepage)
{
	fops->llseek = llseek;
	fops->read = read;
	fops->write = write;
	fops->ioctl = ioctl;
	fops->open = open;
	fops->release = release;
	fops->readdir = readdir;

	fops->readblk = readblk;
	fops->writeblk = writeblk;
	fops->readpage = readpage;
	fops->writepage = writepage;

	return 0;
}

void create_device_driver_file_ops(file_ops_t* fops)
{
	set_file_ops(fops, &ddriv_llseek, &ddriv_read, &ddriv_write, &ddriv_ioctl, &ddriv_open, NULL, NULL,
			NULL, NULL, NULL, NULL);
}

void create_device_driver_blk_file_ops(file_ops_t* fops)
{
	set_file_ops(fops, &ddriv_llseek, &ddriv_read_blk, &ddriv_write_blk, &ddriv_ioctl, &ddriv_open, NULL, NULL,
			&ddriv_readblk, &ddriv_writeblk, NULL, NULL);
}

/*

#define INODE_TYPE_GENERIC		0

inode_t * create_inode(int type, inode_t* buf)
{
	return buf;
}

file_t* create_file(int type, file_t* buf)
{
	return buf;
}

dirent_t* create_dirent(int type, dirent_t* buf)
{
	return buf;
}

file_ops_t* create_file_ops(int type, file_ops_t* buf)
{
	return buf;
}

*/

void init_base_files()
{

	init_device_table();

	create_device_driver_file_ops(&device_driver_file_ops);
	create_device_driver_blk_file_ops(&device_driver_blk_file_ops);

	int i;

	int dev_ide_code[2] = { DEV_IDE, DEV_IDE1 };


	for(i = 0; i < 2; ++i)
	{
		// make inode for /dev/ide<i>

		inode_t * dev_ide_inode;

		dev_ide_inode = &fixed_inode_list[dev_ide_code[i]];

		dev_ide_inode->i_device = MAKE_DEVICE_NUMBER(DEV_IDE_INDEX, i);
		dev_ide_inode->i_fops = &device_driver_blk_file_ops;
		dev_ide_inode->i_ino = i;

		dentry_t* dev_ide_dentry = &fixed_dentry_list[dev_ide_code[i]];

		dev_ide_dentry->d_inode = dev_ide_inode;

		file_t* dev_ide_file = &fixed_file_list[dev_ide_code[i]];

		dev_ide_file->f_pos = 0;
		dev_ide_file->f_dentry = dev_ide_dentry;
		dev_ide_file->f_fops = dev_ide_dentry->d_inode->i_fops;


	}

	char* hdd_filename = "/home/juergen/src-test/c.img";

  hdd_fd = open(hdd_filename, O_RDWR);

  if(hdd_fd == -1){
      perror("open");
  }


}

//
// a read/write test for the underlying dev files
//

void test_read_write(file_t* dev_file)
{

	const int buf_size = 16 * 1024;

	const uint32_t len1 = 137;
	const uint32_t len2 = 111;

	uint32_t offset_a = 19;

	//uint32_t key;

	printf("test_read: len1 = %d len2 = %d\n", len1, len2);

	char* buf1 = (char*) malloc(buf_size);
	char* buf2 = (char*) malloc(buf_size);

	char* out_buf1 = (char*) malloc(buf_size);
	char* out_buf2 = (char*) malloc(buf_size);

	memset(buf1, 0, buf_size);
	memset(buf2, 1, buf_size);

	ASSERT(buf1);
	ASSERT(buf2);

	char* pbuf1 = buf1;
	char* pbuf2 = buf2;

	size_t offs;

	//size_t loffs = 0;

	uint32_t cnt1 = offset_a;
	uint32_t cnt2 = 0;

	while (cnt2 < len2)
	{
		dev_file->f_fops->llseek(dev_file, cnt1, 0);
		int nrd = dev_file->f_fops->read(dev_file, pbuf1, len1, &offs );

		ASSERT(nrd == len1);

		pbuf1 += len1;
		cnt1 += len1;
		++cnt2;
	}

	ASSERT(pbuf1 - buf1 == len1 * len2);

	cnt1 = offset_a;
	cnt2 = 0;

	while (cnt2 < len1)
	{
		dev_file->f_fops->llseek(dev_file, cnt1, 0);
		int nrd = dev_file->f_fops->read(dev_file, pbuf2, len2, &offs );

		ASSERT(nrd == len2);

		pbuf2 += len2;
		cnt1 += len2;
		++cnt2;
	}

	ASSERT(pbuf2 - buf2 == len1 * len2);

	int i;
	for(i = 0; i < len1 * len2; ++i)
	{
		//outb_printf("i = %d ", i);
		ASSERT(buf1[i] == buf2[i]);
	}

	printf("Test read ok.\n");


	offset_a = 512 * 1024 + 17;

	//memcpy(out_buf1, buf1, buf_size);
	//memcpy(out_buf2, buf2, buf_size);

	for(i = 0; i < buf_size; ++i)
	{
		uint8_t val = rand() & 0xff;
		out_buf1[i] = val;
		out_buf2[i] = val;
	}

	cnt1 = offset_a;
	cnt2 = 0;

	pbuf1 = out_buf1;

	while (cnt2 < len2)
	{
		dev_file->f_fops->llseek(dev_file, cnt1, 0);
		uint32_t nwrt = dev_file->f_fops->write(dev_file, pbuf1, len1, &offs );

		ASSERT(nwrt == len1);

		pbuf1 += len1;
		cnt1 += len1;
		++cnt2;
	}

	ASSERT(pbuf1 - out_buf1 == len1 * len2);

	cnt1 = offset_a + buf_size;
	cnt2 = 0;

	pbuf2 = out_buf2;

	while (cnt2 < len1)
	{
		dev_file->f_fops->llseek(dev_file, cnt1, 0);
		uint32_t nwrt = dev_file->f_fops->write(dev_file, pbuf2, len2, &offs );

		ASSERT(nwrt == len2);

		pbuf2 += len2;
		cnt1 += len2;
		++cnt2;
	}

	ASSERT(pbuf2 - out_buf2 == len1 * len2);

	memset(buf1, 0xab, buf_size);
	memset(buf2, 0xcd, buf_size);

	dev_file->f_fops->llseek(dev_file, offset_a, 0);
	int nrd = dev_file->f_fops->read(dev_file, buf1, len1 * len2, &offs);

	ASSERT(nrd == len1 * len2);

	dev_file->f_fops->llseek(dev_file, offset_a + buf_size, 0);
	nrd = dev_file->f_fops->read(dev_file, buf2, len1 * len2, &offs);

	ASSERT(nrd == len1 * len2);

	for(i = 0; i < len1 * len2; ++i)
	{
		//outb_printf("i = %d :", i);
		ASSERT(buf1[i] == buf2[i]);
	}

	outb_printf("\n");
	printf("Test write ok.\n");



	free(out_buf2);
	free(out_buf1);

	free(buf2);
	free(buf1);
}

static file_ext2_t file_pwd;


int execute_bgd(int argc, char* argv[])
{
	int retval = -1;
	if (argc < 2)
	{
		printf("usage:bgd <index>\n");
		goto ende;
	}
	file_t* dev_ide1 = &fixed_file_list[DEV_IDE+1];

	bg_desc_ext2_t bgd;

	uint32_t bg_index = atoi(argv[1]);
	read_bgdesc_ext2(dev_ide1, &bgd, bg_index);
	print_bgdesc_ext2(&bgd);

	uint32_t first_bit = get_first_data_block_pos_in_block_group_bitmap(bg_index, gsb_ext2, &bgd);
	uint32_t last_bit = get_last_data_block_pos_in_block_group_bitmap(bg_index, gsb_ext2, &bgd);

	uint32_t first_data_blk = get_first_data_block_in_block_group(&bgd, gsb_ext2);

	printf("first data block = %d\n", first_data_blk);
	printf("first data bit in data block bitmap = %d\n", first_bit);
	printf("last data bit in data block bitmap = %d\n", last_bit);

ende:
	return retval;
}

int execute_ext2_write_test(int argc, char* argv[])
{
	file_t* dev_ide1 = &fixed_file_list[DEV_IDE+1];

	if (argc < 2)
	{
		printf("usage: dotst <num_tests>\n");
		return -1;
	}

	int no_runs = atoi(argv[1]);

	test_ext2_write(dev_ide1, gsb_ext2, no_runs);

	printf("Ext2 write test done.\n");

	return 0;
}

int execute_cat(int argc, char* argv[])
{

	if (argc < 2)
	{
		printf("usage: cat <pathname>\n");
		return -1;
	}

	file_ext2_t file;
	init_file_ext2(&file, file_pwd.dev_file, file_pwd.sb);

	char last_fname[EXT2_NAMELEN];

	int ret = parse_path_ext2(&file_pwd, 0, argv[1], &file, last_fname);

	if (ret >= 0)
	{
		display_inode_ext2(&file);
	}

	destroy_file_ext2(&file);
	return 0;
}

int execute_cp(int argc, char* argv[])
{
	if (argc < 3)
	{
		printf("usage: cp <path_to_file> <filename>\n");
		return -1;
	}

	file_ext2_t file_rootdir;
	init_file_ext2(&file_rootdir, file_pwd.dev_file, file_pwd.sb);

	read_inode_ext2(&file_rootdir, 2);

	file_ext2_t file_in;
	init_file_ext2(&file_in, file_pwd.dev_file, file_pwd.sb);

	char last_fname[EXT2_NAMELEN];

	int ret = parse_path_ext2(&file_rootdir, 0, argv[1], &file_in, last_fname);

	if (ret >= 0)
	{
		file_ext2_t filp_new_file;
		init_file_ext2(&filp_new_file, file_pwd.dev_file, file_pwd.sb);

		create_file_ext2(&file_pwd, &filp_new_file, argv[2], EXT2_S_IFREG | 0644, 0);

		uint32_t size_infile = file_in.pinode->i_size;

		char* file_buf = malloc(size_infile);

		read_file_ext2(&file_in, file_buf, size_infile, 0);

		write_file_ext2(&filp_new_file, file_buf, size_infile, 0);

		free(file_buf);

		destroy_file_ext2(&filp_new_file);
	}

	destroy_file_ext2(&file_in);

	destroy_file_ext2(&file_rootdir);



	return 0;
}

int execute_cd(int argc, char* argv[])
{

	if (argc < 2)
	{
		printf("usage: cd <pathname>\n");
		return -1;
	}

	file_ext2_t file;
	init_file_ext2(&file, file_pwd.dev_file, file_pwd.sb);

	char last_fname[EXT2_NAMELEN];

	int ret = parse_path_ext2(&file_pwd, 0, argv[1], &file, last_fname);

	copy_file_ext2(&file_pwd, &file);

	FILE* fd = fopen("lastdir.cd", "w");

	fprintf(fd, "cd %s\n", argv[1]);

	fclose(fd);

	destroy_file_ext2(&file);
	return ret;
}

int execute_mkdir(int argc, char* argv[])
{

	if (argc < 2)
	{
		printf("usage: mkdir <pathname>\n");
		return -1;
	}

	file_ext2_t file;
	init_file_ext2(&file, file_pwd.dev_file, file_pwd.sb);

	char last_fname[EXT2_NAMELEN];

	int ret = parse_path_ext2(&file_pwd, 1, argv[1], &file, last_fname);

	file_ext2_t filp_new_dir;
	init_file_ext2(&filp_new_dir, file.dev_file, file.sb);

	create_directory_ext2(&file, &filp_new_dir, last_fname, 0755, 0);

	destroy_file_ext2(&file);
	destroy_file_ext2(&filp_new_dir);
	return ret;
}


int execute_rmdir(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("usage: rmdir <pathname>\n");
		return -1;
	}

	file_ext2_t file;
	init_file_ext2(&file, file_pwd.dev_file, file_pwd.sb);

	char last_fname[EXT2_NAMELEN];

	int ret = parse_path_ext2(&file_pwd, 1, argv[1], &file, last_fname);

	delete_directory_ext2(&file, last_fname);

	destroy_file_ext2(&file);
	return ret;
}

int execute_rm(int argc, char* argv[])
{

	if (argc < 2)
	{
		printf("usage: rm <pathname>\n");
		return -1;
	}

	file_ext2_t file;
	init_file_ext2(&file, file_pwd.dev_file, file_pwd.sb);

	char last_fname[EXT2_NAMELEN];

	int ret = parse_path_ext2(&file_pwd, 1, argv[1], &file, last_fname);

	unlink_file_ext2(&file, last_fname);

	destroy_file_ext2(&file);
	return ret;
}



int execute_ino(int argc, char* argv[1])
{
	int retval = -1;
	file_t* dev_ide1 = &fixed_file_list[DEV_IDE+1];

	file_ext2_t file;
	init_file_ext2(&file, dev_ide1, gsb_ext2);

	if (argc < 2)
	{
		printf("usage: ino <inode_index>\n");
		goto ende;
	}


	uint32_t inode_index = atoi(argv[1]);

	retval = read_inode_ext2(&file, inode_index);

	print_inode_ext2(file.pinode);

	destroy_file_ext2(&file);

ende:
	return retval;

}

int execute_rd(int argc, char* argv[])
{
	char blk_buf[512];

	file_t* dev_ide = &fixed_file_list[DEV_IDE];

	int blk_num = atoi(argv[1]);

	int aux_num = 0;
	int upper_mode = 0;

	if (argc > 2)
	{
		aux_num = atoi(argv[2]);
		upper_mode = 1;
	}

	size_t offset = !upper_mode ? blk_num * IDE_BLKSIZE : (4096 + 2 * blk_num + aux_num) * IDE_BLKSIZE;

	size_t dummy_offs;
	dev_ide->f_fops->llseek(dev_ide, offset, 0);
	dev_ide->f_fops->read(dev_ide, blk_buf, IDE_BLKSIZE, &dummy_offs);

	display_buffer((uint8_t*)blk_buf, IDE_BLKSIZE);
	return 0;
}

int execute_tst(int argc, char* argv[])
{
	file_t* dev_ide = &fixed_file_list[DEV_IDE];
	test_read_write(dev_ide);
	return 0;
}


typedef struct exec_struct_s {
	char* command_name;
	int (*command)(int argc, char* argv[]);
} exec_struct_t;

exec_struct_t my_commands[] = { {"cat", execute_cat},
																	{"rd", execute_rd},
																	{"tst", execute_tst},
																	{"bgd", execute_bgd},
																	{"ino", execute_ino},
																	{"cd", execute_cd},
																	{"mkdir", execute_mkdir},
																	{"rm", execute_rm},
																	{"rmdir", execute_rmdir},
																	{"cp", execute_cp},
																	{"dotst", execute_ext2_write_test}};


void dispatch_op(exec_struct_t *commands, int ncommands, int argc, char* argv[])
{
	int i = 0;
	for(i = 0; i < ncommands; ++i)
	{
		if (!strcmp(commands[i].command_name, argv[0]))
		{
			(*commands[i].command)(argc, argv);
		}
	}
}

int do_cmdline(int argc, char* argv[])
{
	int i;
	for(i = 1; i < argc; ++i)
	{
		argv[i-1] = argv[i];
	}
	--argc;

	printf("**>");
	for(i = 0; i < argc; ++i)
	{
		printf("%s ", argv[i]);
	}
	printf("\n");

	// argv[0] is not equal "cd"
	if (strcmp(argv[0], "cd"))
	{
		FILE* fd;
		if ((fd = fopen("lastdir.cd", "r")))
		{
			char buf[256];
			int argc_cd;
			char* argv_cd[32];
			fgets(buf, 256, fd);
			fclose(fd);
			int i;
			for(i = 0; i < 256; ++i)
			{
				if (buf[i] == '\n')
				{
					buf[i] = 0;
				}
			}

			printf("lastdir.cd: content = %s\n", buf);

			parse_buf(buf, 256, " ", &argc_cd, argv_cd);
			for(i = 0; i < argc_cd; ++i)
			{
				printf("lastdir.cd: argv[%d] = >%s<\n", i, argv_cd[i]);
			}
			dispatch_op(my_commands, sizeof(my_commands)/sizeof(my_commands[0]), argc_cd, argv_cd);
		}
	}

	dispatch_op(my_commands, sizeof(my_commands)/sizeof(my_commands[0]), argc, argv);



	return 0;

}


int main(int argc, char* argv[])
{
	init_base_files();

	//file_t* dev_ide = &fixed_file_list[DEV_IDE];
	file_t* dev_ide1 = &fixed_file_list[DEV_IDE + 1];

	init_ext2_system(dev_ide1);

	init_file_ext2(&file_pwd, dev_ide1, gsb_ext2);

	// file_pwd becomes root dir
	read_inode_ext2(&file_pwd, 2);

	if (argc > 1)
	{
		do_cmdline(argc, argv);
		close(hdd_fd);
		return 0;
	}

	char *buf;

	while ((buf = readline("> ")) != NULL) {
		if (strlen(buf) > 0) {
			add_history(buf);
		}

		read_inode_ext2(&file_pwd, file_pwd.inode_index);

		char buffer[4096];

		strcpy(buffer, buf);

		int line_argc;
		char* line_argv[32];

		parse_buf(buffer, sizeof(buffer), " ", &line_argc, line_argv);

		dispatch_op(my_commands, sizeof(my_commands)/sizeof(my_commands[0]), line_argc, line_argv);

		if (!strcmp(line_argv[0], "quit"))
		{
			close(hdd_fd);
			free(buf);
			goto ende;
		}

		printf("[%s]\n", buf);

		// readline malloc's a new buffer every time.
		free(buf);
	}

ende:
	return 0;



}

