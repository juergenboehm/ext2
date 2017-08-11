

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <time.h>


#include "kerneltypes.h"
#include "libs/utils.h"
#include "libs32/klib.h"

#include "fs/gendrivers.h"
#include "fs/vfs.h"
#include "fs/ext2.h"


superblock_ext2_t* gsb_ext2;
bg_desc_ext2_t* gbgd_ext2;

uint32_t g_blocks_per_group_ext2;

uint32_t g_ext2_blocksize;

inode_ext2_t* g_root_inode_ext2;


static char* zero_block = 0;


uint32_t get_timestamp()
{
	time_t time_val = time(0);
	return (uint32_t) time_val;

}


inline uint32_t num_block_groups(superblock_ext2_t* sb)
{
	uint32_t blocks_per_group = sb->s_blocks_per_group;
	uint32_t total_blocks_count = sb->s_blocks_count;

	uint32_t num_block_groups = (total_blocks_count + (blocks_per_group - 1))/blocks_per_group;

	return num_block_groups;
}


inline uint32_t blk_num_to_blk_group(superblock_ext2_t* sb, uint32_t blk_num)
{
	uint32_t blocks_per_blk_group = sb->s_blocks_per_group;
	uint32_t blk_group_index = (blk_num - sb->s_first_data_block)/blocks_per_blk_group;
	return blk_group_index;
}

inline uint32_t blk_group_to_blk_num(superblock_ext2_t* sb, uint32_t blk_group_index)
{
	return blk_group_index * sb->s_blocks_per_group + sb->s_first_data_block;
}

inline uint32_t get_first_data_block_in_block_group(bg_desc_ext2_t* pbgd, superblock_ext2_t* sb)
{
	return pbgd->bg_inode_table + sb->s_inodes_per_group * sizeof(inode_ext2_t) / GET_BLOCKSIZE_EXT2(sb);
}

inline uint32_t get_first_data_block_pos_in_block_group_bitmap(uint32_t bgd_index,
		superblock_ext2_t* sb, bg_desc_ext2_t* pbgd)
{
	uint32_t offset_in_blks =
			get_first_data_block_in_block_group(pbgd, sb) - blk_group_to_blk_num(sb, bgd_index);
	return offset_in_blks;

}

inline uint32_t get_last_data_block_pos_in_block_group_bitmap(uint32_t bgd_index,
		superblock_ext2_t *sb, bg_desc_ext2_t *pbgd)
{
	uint32_t num_blk_grps = num_block_groups(sb);
	if (bgd_index == num_blk_grps - 1)
	{
		return sb->s_blocks_count - (num_blk_grps - 1) * sb->s_blocks_per_group - 1;
	}
	else
	{
		return sb->s_blocks_per_group - 1;
	}

}




inode_ext2_t* alloc_inode_ext2()
{
	inode_ext2_t* p = (inode_ext2_t*) malloc(sizeof(inode_ext2_t));
	memset(p, 0, sizeof(inode_ext2_t));
	return p;
}

bg_desc_ext2_t* alloc_bg_desc_ext2()
{
	bg_desc_ext2_t* p = (bg_desc_ext2_t*) malloc(sizeof(bg_desc_ext2_t));
	memset(p, 0, sizeof(bg_desc_ext2_t));
	return p;
}

blk_iterator_t* alloc_blk_iterator()
{
	blk_iterator_t* p = (blk_iterator_t*) malloc(sizeof(blk_iterator_t));
	memset(p, 0, sizeof(blk_iterator_t));
	return p;
}

superblock_ext2_t* alloc_superblock_ext2()
{
	superblock_ext2_t* p = (superblock_ext2_t*) malloc(sizeof(superblock_ext2_t));
	memset(p, 0, sizeof(superblock_ext2_t));
	return p;
}

file_ext2_t* alloc_file_ext2()
{
	file_ext2_t* p = (file_ext2_t*) malloc(sizeof(file_ext2_t));
	memset(p, 0, sizeof(file_ext2_t));
	return p;
}

void init_file_ext2(file_ext2_t* filp, file_t* dev_file, superblock_ext2_t* sb)
{
	memset(filp, 0, sizeof(file_ext2_t));
	filp->dev_file = dev_file;
	filp->sb = sb;
}

int alloc_if_empty(char **buf, uint32_t size)
{
	if (!*buf)
	{
		*buf = (char*) malloc(size);
	}
	return 0;
}

typedef void* (alloc_fun_t)();


int alloc_if_empty_fun(void** buf, alloc_fun_t* fun)
{
	if (!*buf)
	{
		*buf = (*fun)();
	}
	return 0;
}

void free_if_nonzero(void *p)
{
	if (p)
	{
		free(p);
	}
}

int copy_file_ext2(file_ext2_t* fdest, file_ext2_t* fsrc)
{
	fdest->dev_file = fsrc->dev_file;
	fdest->sb = fsrc->sb;

	alloc_if_empty_fun((void**)&fdest->pinode, (alloc_fun_t*) &alloc_inode_ext2);
	memcpy(fdest->pinode, fsrc->pinode, sizeof(inode_ext2_t));

	alloc_if_empty_fun((void**)&fdest->pbgd, (alloc_fun_t*) &alloc_bg_desc_ext2);
	memcpy(fdest->pbgd, fsrc->pbgd, sizeof(bg_desc_ext2_t));

	fdest->inode_index = fsrc->inode_index;
	fdest->offset_inode = fsrc->offset_inode;
	fdest->pbgd_index = fsrc->pbgd_index;

	alloc_if_empty_fun((void**)&fdest->it_lpos, (alloc_fun_t*) &alloc_blk_iterator);

	blk_iterator_init(fdest->it_lpos, fdest, fsrc->it_lpos->offset);


	alloc_if_empty_fun((void**)&fdest->it_end, (alloc_fun_t*) &alloc_blk_iterator);

	blk_iterator_init(fdest->it_end, fdest, fsrc->it_end->offset);

	return 0;
}

int destroy_file_ext2(file_ext2_t* fdestr)
{
	if (fdestr->it_lpos)
	{
		blk_iterator_destroy(fdestr->it_lpos);
		free(fdestr->it_lpos);
	}
	if (fdestr->it_end)
	{
		blk_iterator_destroy(fdestr->it_end);
		free(fdestr->it_end);
	}
	free_if_nonzero(fdestr->pbgd);
	free_if_nonzero(fdestr->pinode);

	return 0;

}



#define SB_PRINT_FIELD(sb, sb_field) printf("%s = %d\n", #sb_field, sb->sb_field)

int print_sb_ext2(superblock_ext2_t* sb)
{
	SB_PRINT_FIELD(sb, s_inodes_count);
	SB_PRINT_FIELD(sb, s_blocks_count);
	SB_PRINT_FIELD(sb, s_first_data_block);
	SB_PRINT_FIELD(sb, s_log_block_size);
	SB_PRINT_FIELD(sb, s_inodes_per_group);
	SB_PRINT_FIELD(sb, s_blocks_per_group);
	SB_PRINT_FIELD(sb, s_inode_size);

	return 0;
}

#define BGD_PRINT_FIELD(bgd, bgd_field) printf("%s = %d\n", #bgd_field, bgd->bgd_field)

int print_bgdesc_ext2(bg_desc_ext2_t* pbgdesc)
{
	BGD_PRINT_FIELD(pbgdesc, bg_block_bitmap);
	BGD_PRINT_FIELD(pbgdesc, bg_inode_bitmap);
	BGD_PRINT_FIELD(pbgdesc, bg_inode_table);
	BGD_PRINT_FIELD(pbgdesc, bg_free_blocks_count);
	BGD_PRINT_FIELD(pbgdesc, bg_free_inodes_count);
	BGD_PRINT_FIELD(pbgdesc, bg_used_dirs_count);

	return 0;
}

#define INO_PRINT_FIELD(ino, ino_field) printf("%s = %d\n", #ino_field, ino->ino_field)


int print_inode_ext2(inode_ext2_t* pinode)
{
	int i;

	INO_PRINT_FIELD(pinode, i_file_acl);
	INO_PRINT_FIELD(pinode, i_dir_acl);
	for(i = 0; i < 15; ++i)
	{
		printf("blk[%d] = %d ", i, pinode->i_block[i]);
		if (i && (!(i% 4)))
		{
			printf("\n ");
		}
	}
	printf("\n");

	return 0;
}



int read_superblock_ext2(file_t* dev_file, superblock_ext2_t* sb)
{
	const int size_sb = SIZE_SB;
	const int offset_sb = OFFSET_SB;

	int nrd = read_from_dev(dev_file, (char*)sb, size_sb, offset_sb);
	return nrd;
}

int write_superblock_ext2(file_t* dev_file, superblock_ext2_t* sb)
{
	const int size_sb = SIZE_SB;
	const int offset_sb = OFFSET_SB;

	int nwrt = write_to_dev(dev_file, (char*)sb, size_sb, offset_sb);
	return nwrt;

}



int init_superblock_ext2(file_t* dev_file)
{
	const int size_sb = SIZE_SB;

	gsb_ext2 = alloc_superblock_ext2();

	int nrd = read_superblock_ext2(dev_file, gsb_ext2);
	ASSERT(nrd == size_sb);

	return 0;
}

static file_ext2_t* g_file = 0;

int init_ext2_system(file_t* dev_file)
{

	// superblock

	init_superblock_ext2(dev_file);

	g_blocks_per_group_ext2 = gsb_ext2->s_blocks_per_group;
	g_ext2_blocksize = GET_BLOCKSIZE_EXT2(gsb_ext2);

	free_if_nonzero(zero_block);
	zero_block = 0;
	alloc_if_empty(&zero_block, g_ext2_blocksize);
	memset(zero_block, 0, g_ext2_blocksize);


	print_sb_ext2(gsb_ext2);

	uint32_t blocks_per_group = gsb_ext2->s_blocks_per_group;
	uint32_t total_blocks_count = gsb_ext2->s_blocks_count;

	uint32_t num_block_groups = (total_blocks_count + (blocks_per_group - 1))/blocks_per_group;

	printf("\nglobal_ext2_blocksize = %d\n", g_ext2_blocksize);
	printf("\ntotal_blocks_count = %d\n", total_blocks_count);
	printf("blocks_per_group = %d\n", blocks_per_group);
	printf("number of block groups = %d\n", num_block_groups);


	// block group descriptor 0

	bg_desc_ext2_t bgdesc;

	read_bgdesc_ext2(dev_file, &bgdesc, 0);

	printf("\n");

	print_bgdesc_ext2(&bgdesc);


	// init global root inode

	g_file = alloc_file_ext2();
	init_file_ext2(g_file, dev_file, gsb_ext2);

	read_inode_ext2(g_file, 2);

	g_root_inode_ext2 = g_file->pinode;

	uint32_t root_inode_blks = g_root_inode_ext2->i_blocks;

	printf("root_inode_blks = %d\n", root_inode_blks);
	printf("root_inode size = %d\n", g_root_inode_ext2->i_size);

#if 0
	printf("Testing get_indirect_blocks...\n");
	test_get_indirect_blocks();

	printf("Test ok.\n");
#endif

	return 0;
}


int read_bgdesc_ext2(file_t* dev_file, bg_desc_ext2_t* pbgd, uint32_t bg_index)
{
	int nrd = -1;

	uint32_t offset_bg_index = OFFSET_SB + SIZE_SB;

	offset_bg_index += bg_index * sizeof(bg_desc_ext2_t);

	//file->pbgd_index = bg_index;

	nrd = read_from_dev(dev_file, (char*) pbgd, sizeof(bg_desc_ext2_t), offset_bg_index);

	ASSERT(nrd == sizeof(bg_desc_ext2_t));

	return nrd;

}

int write_bgdesc_ext2(file_t* dev_file, bg_desc_ext2_t* pbgd, uint32_t bg_index)
{
	int nwrt = -1;

	uint32_t offset_bg_index = OFFSET_SB + SIZE_SB;

	offset_bg_index += bg_index * sizeof(bg_desc_ext2_t);

	//file->pbgd_index = bg_index;

	nwrt = write_to_dev(dev_file, (char*) pbgd, sizeof(bg_desc_ext2_t), offset_bg_index);

	ASSERT(nwrt == sizeof(bg_desc_ext2_t));

	return nwrt;

}



int read_inode_ext2(file_ext2_t* file, uint32_t inode_index)
{
	int nrd = -1;

	uint32_t ext2_blocksize = GET_BLOCKSIZE_EXT2(file->sb);

	uint32_t inode_index0 = inode_index - 1;

	uint32_t inode_bgrp_index = inode_index0 / file->sb->s_inodes_per_group;
	uint32_t inode_bgrp_local_index = inode_index0 % file->sb->s_inodes_per_group;


	alloc_if_empty_fun((void**)&file->pbgd, (alloc_fun_t*) & alloc_bg_desc_ext2);

	read_bgdesc_ext2(file->dev_file, file->pbgd, inode_bgrp_index);

	file->pbgd_index = inode_bgrp_index;

	uint32_t inode_start_blk = file->pbgd->bg_inode_table;

	uint32_t offs = BLKNUM_TO_OFFSET(inode_start_blk, ext2_blocksize)  +
			inode_bgrp_local_index * sizeof(inode_ext2_t);


	file->inode_index = inode_index;
	file->offset_inode = offs;

	alloc_if_empty_fun((void**) &file->pinode, (alloc_fun_t*) & alloc_inode_ext2);

	//printf("read_inode_ext2: offs = %d\n", offs);

	nrd = read_from_dev(file->dev_file, (char*) file->pinode, sizeof(inode_ext2_t), offs);


	alloc_if_empty_fun((void**)&file->it_lpos, (alloc_fun_t*) &alloc_blk_iterator);

	blk_iterator_init(file->it_lpos, file, 0);

	uint32_t file_size = file->pinode->i_size;


	alloc_if_empty_fun((void**)&file->it_end, (alloc_fun_t*) &alloc_blk_iterator);

	blk_iterator_init(file->it_end, file, file_size);

	return nrd;

}

int write_inode_ext2(file_ext2_t* filp)
{
	int nwrt = -1;

	uint32_t ext2_blocksize = GET_BLOCKSIZE_EXT2(filp->sb);

	uint32_t inode_index0 = filp->inode_index - 1;

	uint32_t inode_bgrp_local_index = inode_index0 % filp->sb->s_inodes_per_group;

	uint32_t inode_start_blk = filp->pbgd->bg_inode_table;

	uint32_t offs = BLKNUM_TO_OFFSET(inode_start_blk, ext2_blocksize)  +
			inode_bgrp_local_index * sizeof(inode_ext2_t);

	nwrt = write_to_dev(filp->dev_file, (char*) filp->pinode, sizeof(inode_ext2_t), offs);

	return nwrt;
}

int get_indirect_blocks(uint32_t offset, uint32_t* index_arr, uint32_t *mode)
{

	int links_per_blk = g_ext2_blocksize / sizeof(uint32_t);

	//outb_printf("get_indirect_blocks: offset = %d\n", offset);

	uint32_t offset_in_blks = offset / g_ext2_blocksize;

	uint32_t limits[4];
	uint32_t index[4];

	uint32_t dd = 1;

	limits[0] = 12;
	dd *= links_per_blk;
	limits[1] = limits[0] + dd;
	dd *= links_per_blk;
	limits[2] = limits[1] + dd;
	dd *= links_per_blk;
	limits[3] = limits[2] + dd;
	memset(index, 0, sizeof(index));

	int i = 0;

	while (offset_in_blks >= limits[i])
	{
		++i;
	}

	*mode = i;

	int j = i;
	offset_in_blks -= (i > 0 ? limits[i-1]: 0);
/*
	while (j > 0)
	{
		index[i-j+1] = offset_in_blks / powers[j-1];
		offset_in_blks -= index[i-j+1] * powers[j-1];
		--j;
	}
*/

	while (j > 0)
	{
		index[j] = offset_in_blks % links_per_blk;
		offset_in_blks -= index[j];
		offset_in_blks /= links_per_blk;
		--j;
	}

	index[0] = (i == 0) ? offset_in_blks : 11 + i;

	memcpy(index_arr, &index[0], sizeof(index));

	//outb_printf("### get_indirect_blocks: %d (%d %d %d %d)\n",
	//		offset_in_blks_old, index[0], index[1], index[2], index[3]);

	//uint32_t key = getc(stdin);

	return *mode;

}

int test_get_indirect_blocks()
{
	uint32_t offset;

	uint32_t offset_raw;

	uint32_t limits[4];
	uint32_t index[4];
	uint32_t powers[4];

	int links_per_blk = g_ext2_blocksize / sizeof(uint32_t);

	powers[0] = 1;
	int i = 1;
	while (i < 4)
	{
		powers[i] = links_per_blk * powers[i-1];
		++i;
	}

	limits[0] = 12;
	limits[1] = limits[0] + powers[1];
	limits[2] = limits[1] + powers[2];
	limits[3] = limits[2] + powers[3];
	memset(index, 0, sizeof(index));

	uint32_t limits_old = 0;

	int crs = 0;

	for(i = 0; i < 4; ++i)
	{
		int j;
		printf("\ntesting: i = %d ", i);
		for(j = 0; j + limits_old < limits[i]; ++j)
		{
			uint32_t mode;
			offset = (limits_old + j) * g_ext2_blocksize;
			offset_raw = limits_old + j;

			if (!offset && j)
			{
				printf("\noffset overflow at i = %d j = %d\n", i, j);
				break;
			}

			get_indirect_blocks(offset, &index[0], &mode);

			if (j % (64 * 1024) == 0)
			{
				printf(".");
				++crs;
				if (crs == 70)
				{
					printf("\n");
					crs = 0;
				}
			}


			if (mode != i)
			{
				outb_printf("mode != i at: i = %d, j = %d : ", i, j);
				getc(0);
			}

			uint32_t k = 1;
			uint32_t offset_synth = index[k];
			++k;
			while (k <= mode)
			{
				offset_synth = offset_synth * powers[1] + index[k];
				++k;
			}
			offset_synth += ((mode == 0) ? index[0]: limits[mode-1]);


			if (offset_synth != offset_raw)
			{
				outb_printf("offset_synth != offset_raw at: i = %d, j = %d : ", i, j);
				getc(0);
			}

		}
		limits_old = limits[i];
	}

	return 0;

}

int read_from_dev(file_t* dev_file, char* buf, uint32_t count, uint32_t offset)
{
	int ret = -1;
	size_t offs_dummy;

	ret = dev_file->f_fops->llseek(dev_file, offset, 0);
	ret = dev_file->f_fops->read(dev_file, buf, count, &offs_dummy);

	return ret;
}

int write_to_dev(file_t* dev_file, char* buf, uint32_t count, uint32_t offset)
{
	int ret = -1;
	size_t offs_dummy;

	ret = dev_file->f_fops->llseek(dev_file, offset, 0);
	ret = dev_file->f_fops->write(dev_file, buf, count, &offs_dummy);

	return ret;
}


uint32_t get_blk_from_idx_blk(char* index_blk, uint32_t index)
{
	return *(((uint32_t*)index_blk) + index);
}


#define ITC_ERR_SI_RD		1
#define ITC_ERR_DI_RD		2
#define ITC_ERR_TI_RD 	3

int display_iterator(blk_iterator_t* it)
{
	int i;
	for(i = 0; i < 4; ++i)
	{
		printf("it: index[%d] = %d: valid = %d\n", i, it->index[i], it->blk_valid[i]);
		printf("it: blk_index = %d\n", blk_iterator_get_blk_index(it));
	}
	return 0;
}


int blk_iterator_init(blk_iterator_t *it, file_ext2_t* file,
		uint32_t offset)
{
	uint32_t index[4];
	uint32_t mode;

	int ext2_blocksize = GET_BLOCKSIZE_EXT2(file->sb);

	//printf("\nblk_iterator_init: offset = %d\n", offset);

	it->offset = offset;
	it->file = file;

	get_indirect_blocks(it->offset, &index[0], &mode);

	it->mode = mode;
	memcpy(it->index, index, sizeof(index));
	memset(it->blk_valid, 0, sizeof(it->blk_valid));
	it->pinode = file->pinode;
	it->dev_file = file->dev_file;
	int i;
	for(i = 0; i < 4; ++i)
	{
		it->blks[i] = 0;
	}

	memset(it->blks_blk, 0, sizeof(it->blks_blk));
	it->code = 0;

	switch (mode)
	{
	case 0:

		it->blks_blk[0] = file->pinode->i_block[it->index[0]];
		it->blk_valid[0] = it->blks_blk[0];
		break;
	case 1:

		alloc_if_empty(&it->blks[1], ext2_blocksize);

		it->blks_blk[1] = file->pinode->i_block[SI_INDEX];

		break;

	case 2:

		alloc_if_empty(&it->blks[1], ext2_blocksize);
		alloc_if_empty(&it->blks[2], ext2_blocksize);

		it->blks_blk[1] = file->pinode->i_block[DI_INDEX];
		break;

	case 3:

		alloc_if_empty(&it->blks[1], ext2_blocksize);
		alloc_if_empty(&it->blks[2], ext2_blocksize);
		alloc_if_empty(&it->blks[3], ext2_blocksize);

		it->blks_blk[1] = file->pinode->i_block[TI_INDEX];
		break;
	default:
		ASSERT("illegal mode." && 0);
		break;
	}

	int j = 1;

	// it->blk_valid[j] true means: the block index of indirection blk level
	// j in indirection blk level j-1 is not zero
	// it->blk_valid[0] true means: it->blks_blk[0] is not zero.

	//it->blks_blk[j] contains the block index of indirection block of level j

	while (j <= mode)
	{
		if (!it->blks_blk[j])
		{
			break;
		}
		it->blk_valid[j] = 1;
		read_from_dev(it->dev_file, it->blks[j], ext2_blocksize,
				BLKNUM_TO_OFFSET(it->blks_blk[j], ext2_blocksize));
		++j;
		if (j > 3)
		{
			break;
		}
		it->blks_blk[j] = get_blk_from_idx_blk(it->blks[j-1], it->index[j-1]);
	}


	return 0;

}

int blk_iterator_destroy(blk_iterator_t* it)
{
	int i;
	for(i = 0; i < 4; ++i)
	{
		if (it->blks[i])
		{
			free(it->blks[i]);
		}
	}
	return 0;
}

static int ncnt = 0;

int blk_iterator_next(blk_iterator_t *it, uint32_t offset_new)
{

	int retval = -1;

	uint32_t index_new[4];
	uint32_t mode_new;

	get_indirect_blocks(offset_new, &index_new[0], &mode_new);

	if (!(memcmp(index_new, it->index, sizeof(index_new))))
	{
		it->offset = offset_new;
		retval = 0;
		goto ende;
	}

	int mode = it->mode;


	if (mode != mode_new)
	{
		retval = blk_iterator_init(it, it->file, offset_new);
		goto ende;
	}

	if (mode == 0)
	{
		memcpy(it->index, index_new, sizeof(it->index));

		it->blks_blk[0] = it->pinode->i_block[it->index[0]];
		it->blk_valid[0] = it->blks_blk[0];
		it->offset = offset_new;

		retval = 0;

		goto ende;

	}
	else if (mode > 0)
	{
		int j = 1;
		while (j <= mode && it->index[j] == index_new[j])
		{
			++j;
		}
		if (j >= mode)
		{
			it->index[mode] = index_new[mode];
			it->offset = offset_new;

			//printf("blk_iterator_next: index[%d] = %d ncnt = %d, mode = %d ",
			//		mode, it->index[mode], ncnt++, mode);

			retval = 0;

			goto ende;
		}
		else
		{
			//printf("\nblk_iterator_init: offset_new = %d\n", offset_new);
			retval = blk_iterator_init(it, it->file, offset_new);
			goto ende;
		}
	}

ende:
	return retval;

}

inline int test_bit(uint32_t bitpos, uint8_t* bitmap)
{
	return bitmap[bitpos / 8] & (1 << (bitpos % 8));

}

inline void set_bit(uint32_t bitpos, uint8_t* bitmap)
{
	bitmap[bitpos / 8 ] |= (1 << (bitpos % 8));
}

inline void reset_bit(uint32_t bitpos, uint8_t* bitmap)
{
	bitmap[bitpos / 8 ] &= ~((uint8_t)(1 << (bitpos % 8)));
}


int allocate_block(file_ext2_t* filp, uint32_t goal_blk_num, uint32_t *new_blk_num)
{
	int retval = -1;
	superblock_ext2_t* sb = filp->sb;

	uint32_t num_blk_groups = num_block_groups(sb);

	uint32_t ext2_blocksize = GET_BLOCKSIZE_EXT2(sb);

	uint32_t goal_blk_bgd_index = blk_num_to_blk_group(sb, goal_blk_num);

	uint32_t goal_blk_bgd_akt = goal_blk_bgd_index;

	char* blk_bitmap = 0;
	alloc_if_empty(&blk_bitmap, ext2_blocksize);

	bg_desc_ext2_t bgd_akt;

	uint32_t allocated_blk_num = 0;

	do
	{

		read_bgdesc_ext2(filp->dev_file, &bgd_akt, goal_blk_bgd_akt);

		uint32_t blk_num_bitmap = bgd_akt.bg_block_bitmap;

		read_from_dev(filp->dev_file, (char*)blk_bitmap, ext2_blocksize,
				BLKNUM_TO_OFFSET(blk_num_bitmap, ext2_blocksize));

		uint32_t first_bit_akt =
				get_first_data_block_pos_in_block_group_bitmap(goal_blk_bgd_akt, sb, &bgd_akt);

		uint32_t last_bit_akt =
				get_last_data_block_pos_in_block_group_bitmap(goal_blk_bgd_akt, sb, &bgd_akt);

		uint32_t bit_akt = first_bit_akt;

		while (bit_akt <= last_bit_akt)
		{
			if (!test_bit(bit_akt, (uint8_t*)blk_bitmap))
			{
					set_bit(bit_akt, (uint8_t*)blk_bitmap);
					write_to_dev(filp->dev_file, (char*)blk_bitmap, ext2_blocksize,
							BLKNUM_TO_OFFSET(blk_num_bitmap, ext2_blocksize));
					allocated_blk_num = get_first_data_block_in_block_group(&bgd_akt, sb) + bit_akt - first_bit_akt;

					--bgd_akt.bg_free_blocks_count;

					write_bgdesc_ext2(filp->dev_file, &bgd_akt, goal_blk_bgd_akt);

					break;
			}
			++bit_akt;
		}

		if (allocated_blk_num)
		{
			break;
		}

		++goal_blk_bgd_akt;
		if (goal_blk_bgd_akt == num_blk_groups)
		{
			goal_blk_bgd_akt = 0;
		}

	}
	while (goal_blk_bgd_akt != goal_blk_bgd_index);

	if (allocated_blk_num)
	{
		*new_blk_num = allocated_blk_num;

		write_to_dev(filp->dev_file, zero_block, ext2_blocksize,
				BLKNUM_TO_OFFSET(allocated_blk_num, ext2_blocksize));

		retval = 0;

	}


	free(blk_bitmap);

	return retval;
}


int allocate_new_data_block(blk_iterator_t *it, uint32_t to_write)
{
	int retval = -1;

	file_ext2_t* filp = it->file;
	superblock_ext2_t* sb = filp->sb;
	inode_ext2_t* pinode = filp->pinode;

	uint32_t ext2_blocksize = GET_BLOCKSIZE_EXT2(sb);


	uint32_t loffset = it->offset;
	uint32_t wrt_end = loffset + to_write;


	uint32_t blocks_new_allocated = 0;

	if (!sb->s_free_blocks_count)
	{
		retval = -1;
		goto ende;
	}

	uint32_t goal_blk = blk_group_to_blk_num(sb, filp->pbgd_index);
	uint32_t new_blk = 0;

	uint32_t first_new_blk = 0;

	if (it->mode == 0)
	{
		ASSERT(it->blks_blk[0] == 0);
		allocate_block(filp, goal_blk, &new_blk);
		pinode->i_block[it->index[0]] = new_blk;

		first_new_blk = new_blk;

		blocks_new_allocated = 1;
	}
	else if (it->mode > 0)
	{
		uint32_t* scratch_blk = 0;
		alloc_if_empty((char**)&scratch_blk, ext2_blocksize);

		int mode = it->mode;

		int j = mode;
		uint32_t new_blk;
		allocate_block(filp, goal_blk, &new_blk);
		++blocks_new_allocated;

		first_new_blk = new_blk;

		while (j > 0 && !it->blks_blk[j])
		{

			memset(scratch_blk, 0, ext2_blocksize);
			scratch_blk[it->index[j]] = new_blk;

			// with the following statement new_blk changes!

			allocate_block(filp, goal_blk, &new_blk);
			++blocks_new_allocated;

			int nwrt = write_to_dev(filp->dev_file, (char*)scratch_blk, ext2_blocksize,
					BLKNUM_TO_OFFSET(new_blk, ext2_blocksize));

			ASSERT(nwrt == ext2_blocksize);

			--j;
		}
		if (j > 0)
		{
			read_from_dev(filp->dev_file, (char*)scratch_blk, ext2_blocksize,
						BLKNUM_TO_OFFSET(it->blks_blk[j], ext2_blocksize));
			scratch_blk[it->index[j]] = new_blk;
			int nwrt = write_to_dev(filp->dev_file, (char*)scratch_blk, ext2_blocksize,
					BLKNUM_TO_OFFSET(it->blks_blk[j], ext2_blocksize));

			ASSERT(nwrt == ext2_blocksize);
		}
		else if (j == 0)
		{
			pinode->i_block[it->index[0]] = new_blk;
		}

		free(scratch_blk);
	}


	blk_iterator_init(it, filp, loffset);

	ASSERT(blk_iterator_get_blk_index(it) == first_new_blk);



	uint32_t pinode_size = filp->pinode->i_size;

	if (wrt_end > pinode_size)
	{
		pinode->i_size = wrt_end;
	}
	pinode->i_blocks += blocks_new_allocated * ext2_blocksize/512;

	sb->s_free_blocks_count -= blocks_new_allocated;

	write_inode_ext2(filp);
	write_superblock_ext2(filp->dev_file, sb);

ende:
	return retval;

}


inline uint32_t get_first_inode_index_in_block_group(uint32_t bg_index, superblock_ext2_t* sb)
{
	uint32_t first_inode_index = 1 + bg_index * sb->s_inodes_per_group;
	return first_inode_index;
}


// filp here only contains valid dev_file and sb fields
// the others are filled, when the inode is created
int allocate_new_inode(file_ext2_t* filp, uint32_t goal_bgd_index, uint16_t imode, uint32_t iflags)
{

	int retval = -1;

	superblock_ext2_t* sb = filp->sb;

	uint32_t num_blk_groups = num_block_groups(sb);

	uint32_t ext2_blocksize = GET_BLOCKSIZE_EXT2(sb);

	uint32_t inodes_per_group = sb->s_inodes_per_group;


	char* inode_bitmap = 0;
	alloc_if_empty(&inode_bitmap, ext2_blocksize);

	bg_desc_ext2_t bgd_akt;

	uint32_t bgd_akt_index = goal_bgd_index;

	uint32_t new_inode_index = 0;
	do
	{

		read_bgdesc_ext2(filp->dev_file, &bgd_akt, bgd_akt_index);

		uint32_t blk_num_bitmap = bgd_akt.bg_inode_bitmap;

		read_from_dev(filp->dev_file, (char*)inode_bitmap, ext2_blocksize,
				BLKNUM_TO_OFFSET(blk_num_bitmap, ext2_blocksize));

		uint32_t first_bit_akt = 0;
		uint32_t last_bit_akt = inodes_per_group - 1;

		uint32_t bit_akt = first_bit_akt;

		while (bit_akt <= last_bit_akt)
		{
			if (!test_bit(bit_akt, (uint8_t*)inode_bitmap))
			{
					set_bit(bit_akt, (uint8_t*)inode_bitmap);
					write_to_dev(filp->dev_file, (char*)inode_bitmap, ext2_blocksize,
							BLKNUM_TO_OFFSET(blk_num_bitmap, ext2_blocksize));

					new_inode_index =
							get_first_inode_index_in_block_group(bgd_akt_index, sb) + bit_akt - first_bit_akt;

					--bgd_akt.bg_free_inodes_count;

					if (imode & EXT2_S_IFDIR)
					{
						++bgd_akt.bg_used_dirs_count;
					}

					write_bgdesc_ext2(filp->dev_file, &bgd_akt, bgd_akt_index);

					break;
			}
			++bit_akt;
		}

		if (new_inode_index)
		{
			break;
		}


		++bgd_akt_index;
		if (bgd_akt_index == num_blk_groups)
		{
			bgd_akt_index = 0;
		}

	}
	while (bgd_akt_index != goal_bgd_index);

	if (!new_inode_index)
	{
		goto ende;
	}

	uint32_t local_inode_index = (new_inode_index - 1) % inodes_per_group;

	uint32_t inode_offset =
			BLKNUM_TO_OFFSET(bgd_akt.bg_inode_table, ext2_blocksize) + local_inode_index * sb->s_inode_size;

	alloc_if_empty_fun((void**)&filp->pinode, (alloc_fun_t*)&alloc_inode_ext2);
	memset(filp->pinode, 0, sizeof(inode_ext2_t));

	alloc_if_empty_fun((void**)&filp->pbgd, (alloc_fun_t*)&alloc_bg_desc_ext2);
	memset(filp->pbgd, 0, sizeof(bg_desc_ext2_t));

	alloc_if_empty_fun((void**)&filp->it_lpos, (alloc_fun_t*)&alloc_blk_iterator);
	memset(filp->it_lpos, 0, sizeof(blk_iterator_t));

	alloc_if_empty_fun((void**)&filp->it_end, (alloc_fun_t*)&alloc_blk_iterator);
	memset(filp->it_end, 0, sizeof(blk_iterator_t));

	filp->inode_index = new_inode_index;
	filp->offset_inode = inode_offset;

	filp->pbgd_index = bgd_akt_index;

	read_bgdesc_ext2(filp->dev_file, filp->pbgd, filp->pbgd_index);

	inode_ext2_t* pinode = filp->pinode;

	//better set when real link is done
	//pinode->i_links_count = 1;

	pinode->i_mode = imode;
	pinode->i_flags = iflags;

	uint32_t t_stamp = get_timestamp();

	pinode->i_atime = t_stamp;
	pinode->i_ctime = t_stamp;
	pinode->i_mtime = t_stamp;

	write_inode_ext2(filp);

	--sb->s_free_inodes_count;
	sb->s_wtime = t_stamp;

	write_superblock_ext2(filp->dev_file, sb);

	blk_iterator_init(filp->it_lpos, filp, 0);
	blk_iterator_init(filp->it_end, filp, 0);

	retval = 0;


	free(inode_bitmap);
	//free(pbgd_akt);

	ende:
	return retval;

}

int deallocate_block(file_ext2_t* file_del, uint32_t del_blk_num)
{

	if (!del_blk_num)
	{
		return 0;
	}

	superblock_ext2_t* sb = file_del->sb;

	uint32_t ext2_blocksize = GET_BLOCKSIZE_EXT2(sb);

	uint32_t del_blk_bgd_index = blk_num_to_blk_group(sb, del_blk_num);

	uint32_t del_blk_bgd_akt = del_blk_bgd_index;

	char* blk_bitmap = 0;
	alloc_if_empty(&blk_bitmap, ext2_blocksize);


	bg_desc_ext2_t bgd_akt;

	read_bgdesc_ext2(file_del->dev_file, &bgd_akt, del_blk_bgd_akt);

	uint32_t blk_num_bitmap = bgd_akt.bg_block_bitmap;

	read_from_dev(file_del->dev_file, blk_bitmap, ext2_blocksize,
			BLKNUM_TO_OFFSET(blk_num_bitmap, ext2_blocksize));

#if 1
	uint32_t first_bit_akt =
			get_first_data_block_pos_in_block_group_bitmap(del_blk_bgd_akt, sb, &bgd_akt);

	uint32_t last_bit_akt =
			get_last_data_block_pos_in_block_group_bitmap(del_blk_bgd_akt, sb, &bgd_akt);
#endif

	uint32_t bit_akt = del_blk_num - blk_group_to_blk_num(sb, del_blk_bgd_akt);

	ASSERT(first_bit_akt <= bit_akt);
	ASSERT(bit_akt <= last_bit_akt);

	ASSERT(test_bit(bit_akt, (uint8_t*) blk_bitmap));

	reset_bit(bit_akt, (uint8_t*) blk_bitmap);
	write_to_dev(file_del->dev_file, blk_bitmap, ext2_blocksize,
			BLKNUM_TO_OFFSET(blk_num_bitmap, ext2_blocksize));

	++bgd_akt.bg_free_blocks_count;

	write_bgdesc_ext2(file_del->dev_file, &bgd_akt, del_blk_bgd_akt);


	++sb->s_free_blocks_count;

	sb->s_wtime = get_timestamp();

	write_superblock_ext2(file_del->dev_file, sb);

	free(blk_bitmap);

	return 0;

}

int deallocate_data_blocks_level(file_ext2_t* file_del, uint32_t blk_num, int level)
{

	if (!blk_num)
	{
		return 0;
	}

	if (!level)
	{
		int retval = deallocate_block(file_del, blk_num);
		return retval;
	}

	uint32_t ext2_blocksize = GET_BLOCKSIZE_EXT2(file_del->sb);

	char* scratch = (char*) malloc(ext2_blocksize);

	read_from_dev(file_del->dev_file, scratch, ext2_blocksize,
			BLKNUM_TO_OFFSET(blk_num, ext2_blocksize));

	int i;
	for(i = 0; i < ext2_blocksize/sizeof(uint32_t); ++i)
	{
		uint32_t del_blk_num = get_blk_from_idx_blk(scratch, i);

		deallocate_data_blocks_level(file_del, del_blk_num, level - 1);
	}
	deallocate_block(file_del, blk_num);

	free(scratch);

	return 0;

}

int deallocate_data_blocks(file_ext2_t* file_del)
{
	int i;

	inode_ext2_t* pinode = file_del->pinode;

	for(i = 0; i < SI_INDEX; ++i)
	{
		uint32_t akt_blk = pinode->i_block[i];
		deallocate_block(file_del, akt_blk);
		pinode->i_block[i] = 0;
	}
	deallocate_data_blocks_level(file_del, pinode->i_block[SI_INDEX], 1);
	pinode->i_block[SI_INDEX] = 0;
	deallocate_data_blocks_level(file_del, pinode->i_block[DI_INDEX], 2);
	pinode->i_block[DI_INDEX] = 0;
	deallocate_data_blocks_level(file_del, pinode->i_block[TI_INDEX], 3);
	pinode->i_block[TI_INDEX] = 0;

	pinode->i_blocks = 0;
	pinode->i_size = 0;

	return 0;
}

int deallocate_inode(file_ext2_t* file_del)
{

	superblock_ext2_t* sb = file_del->sb;

	uint32_t inode_index = file_del->inode_index;
	uint32_t ext2_blocksize = GET_BLOCKSIZE_EXT2(sb);

	uint32_t inode_index0 = inode_index - 1;

	uint32_t inode_bgrp_index = inode_index0 / sb->s_inodes_per_group;
	uint32_t inode_bgrp_local_index = inode_index0 % sb->s_inodes_per_group;

	bg_desc_ext2_t akt_bgd;

	read_bgdesc_ext2(file_del->dev_file, &akt_bgd, inode_bgrp_index);

	uint32_t blk_num_inode_bitmap = akt_bgd.bg_inode_bitmap;

	char* inode_bitmap = (char*) malloc(ext2_blocksize);

	read_from_dev(file_del->dev_file, inode_bitmap, ext2_blocksize,
			BLKNUM_TO_OFFSET(blk_num_inode_bitmap, ext2_blocksize));

	uint32_t bit_akt = inode_bgrp_local_index;

	ASSERT(test_bit(bit_akt, (uint8_t*)inode_bitmap));

	reset_bit(bit_akt, (uint8_t*)inode_bitmap);

	write_to_dev(file_del->dev_file, inode_bitmap, ext2_blocksize,
			BLKNUM_TO_OFFSET(blk_num_inode_bitmap, ext2_blocksize));

	int is_directory = file_del->pinode->i_mode & EXT2_S_IFDIR;

	++akt_bgd.bg_free_inodes_count;

	if (is_directory)
	{
		--akt_bgd.bg_used_dirs_count;
	}

	write_bgdesc_ext2(file_del->dev_file, &akt_bgd, inode_bgrp_index);

	++sb->s_free_inodes_count;
	sb->s_wtime = get_timestamp();

	write_superblock_ext2(file_del->dev_file, sb);

	free(inode_bitmap);

	return 0;
}

int remove_file_ext2(file_ext2_t* file_del)
{
	inode_ext2_t* pinode = file_del->pinode;

	deallocate_data_blocks(file_del);

	uint32_t t_stamp = get_timestamp();

	pinode->i_dtime = t_stamp;

	memset(pinode, 0, sizeof(inode_ext2_t));

	write_inode_ext2(file_del);

	deallocate_inode(file_del);

	return 0;
}



uint32_t blk_iterator_get_blk_index(blk_iterator_t *it)
{
	int mode = it->mode;

	int ret_blk = 0;

	if (mode == 0)
	{
		ret_blk = it->blk_valid[0] ? it->blks_blk[0] : 0;
	}
	else if (mode > 0)
	{
		ret_blk = it->blk_valid[mode] ? get_blk_from_idx_blk(it->blks[mode], it->index[mode]) : 0;
	}

	return ret_blk;

}


int read_file_ext2(file_ext2_t* file, char* buf, uint32_t counta, uint32_t offseta)
{
	uint32_t pinode_size = file->pinode->i_size;

	uint32_t ext2_blocksize = GET_BLOCKSIZE_EXT2(file->sb);


	char* pbuf = buf;

	uint32_t nrd_total = 0;

	uint32_t count = counta;
	uint32_t offset = offseta;

	offset = min(offset, pinode_size);
	count = min(count, pinode_size - offset);

	if (offset != file->it_lpos->offset)
	{
		blk_iterator_next(file->it_lpos, offset);
	}

	int nrd = 0;

	while (count > 0)
	{

		uint32_t blk_num;

		blk_num = blk_iterator_get_blk_index(file->it_lpos);

		uint32_t in_blk_offset = offset % ext2_blocksize;

		uint32_t to_read = min(ext2_blocksize - in_blk_offset, count);

		if (!blk_num)
		{
			// file with hole case
			//printf("read_file_ext2: file with hole case.\n");
			memset(pbuf, 0, to_read);
			nrd = to_read;
		}
		else
		{
			//printf("read_file_ext2: to_read = %d, blk_num = %d, in_blk_offset = %d\n",
			//	to_read, blk_num, in_blk_offset);

			//uint32_t key = getc(0);

			nrd = read_from_dev(file->dev_file, pbuf, to_read,
					BLKNUM_TO_OFFSET(blk_num, ext2_blocksize)  + in_blk_offset);
		}
		ASSERT(nrd == to_read);

	  offset += nrd;

		blk_iterator_next(file->it_lpos, offset);

		nrd_total += nrd;
		pbuf += nrd;
		count -= nrd;

	}

	file->pinode->i_atime = get_timestamp();
	write_inode_ext2(file);

	return nrd_total;
}

int write_file_ext2(file_ext2_t* file, char* buf, uint32_t counta, uint32_t offseta)
{

	uint32_t ext2_blocksize = GET_BLOCKSIZE_EXT2(file->sb);

	char* pbuf = buf;

	uint32_t nwrt_total = 0;

	uint32_t count = counta;
	uint32_t offset = offseta;

	if (offset != file->it_lpos->offset)
	{
		blk_iterator_next(file->it_lpos, offset);
	}

	int nwrt = 0;

	while (count > 0)
	{

		uint32_t blk_num;

		blk_num = blk_iterator_get_blk_index(file->it_lpos);

		uint32_t in_blk_offset = offset % ext2_blocksize;

		uint32_t to_write = min(ext2_blocksize - in_blk_offset, count);

		if (!blk_num)
		{
			// provide missing block
			// does all the necessary updates
			allocate_new_data_block(file->it_lpos, to_write);

			blk_num = blk_iterator_get_blk_index(file->it_lpos);

			ASSERT(blk_num != 0);

		}

		//printf("read_file_ext2: to_read = %d, blk_num = %d, in_blk_offset = %d\n",
		//	to_read, blk_num, in_blk_offset);

		//uint32_t key = getc(0);

		nwrt = write_to_dev(file->dev_file, pbuf, to_write,
				BLKNUM_TO_OFFSET(blk_num, ext2_blocksize)  + in_blk_offset);

		ASSERT(nwrt == to_write);

	  offset += nwrt;

		blk_iterator_next(file->it_lpos, offset);

		nwrt_total += nwrt;
		pbuf += nwrt;
		count -= nwrt;

	}

	uint32_t t_stamp = get_timestamp();

	file->pinode->i_atime = t_stamp;
	file->pinode->i_mtime = t_stamp;

	uint32_t last_offset = file->it_lpos->offset;

	if (last_offset > file->pinode->i_size)
	{
		file->pinode->i_size = last_offset;
	}

	write_inode_ext2(file);


	return nwrt_total;

}


int readdir_ext2(file_ext2_t* file, dir_entry_ext2_t* dir_entry,
		char* namebuf, uint32_t *dir_offset)
{
	dir_entry_ext2_t current_entry;

	read_file_ext2(file, (char*)&current_entry, sizeof(dir_entry_ext2_t), *dir_offset);

	uint32_t name_len = current_entry.name_len;
	uint32_t rec_len = current_entry.rec_len;

	//printf("rec_len = %d : name_len = %d\n", rec_len, name_len);

	read_file_ext2(file, namebuf, name_len, *dir_offset + sizeof(dir_entry_ext2_t));

	namebuf[name_len] = 0;

	memcpy(dir_entry, &current_entry, sizeof(dir_entry_ext2_t));

	*dir_offset += rec_len;

	return 0;
}

int writedir_ext2(file_ext2_t* file, dir_entry_ext2_t* dir_entry, char* fname, uint32_t dir_offset)
{

	write_file_ext2(file, (char*)dir_entry, sizeof(dir_entry_ext2_t), dir_offset);

	if (fname)
	{
		uint32_t name_len = dir_entry->name_len;
		uint32_t rec_len = dir_entry->rec_len;
		uint32_t buf_len = rec_len - sizeof(dir_entry_ext2_t);

		char* aux_buf = malloc(buf_len);
		memset(aux_buf, 0, buf_len);
		memcpy(aux_buf, fname, name_len);

		write_file_ext2(file, aux_buf, buf_len, dir_offset + sizeof(dir_entry_ext2_t));

		free(aux_buf);
	}

	return 0;

}

int read_dir_entry_ext2(file_ext2_t* filp_dir, uint32_t offset, dir_entry_ext2_t* entry)
{
	int nrd = read_file_ext2(filp_dir, (char*) entry, sizeof(dir_entry_ext2_t), offset);

	return nrd;
}

int write_dir_entry_ext2(file_ext2_t* filp_dir, uint32_t offset, dir_entry_ext2_t* entry)
{
	int nwrt = write_file_ext2(filp_dir, (char*) entry, sizeof(dir_entry_ext2_t), offset);

	return nwrt;
}


int lookup_name_in_dir_ext2(file_ext2_t* file_dir, char* fname,
		dir_entry_ext2_t *found_entry, uint32_t* offset, uint32_t* offset_before)
{
	int retval = -1;

	uint32_t dir_offset = 0;

	char namebuf[EXT2_NAMELEN];

	int found = 0;

	*offset = dir_offset;
	*offset_before = dir_offset;

	while (dir_offset < file_dir->pinode->i_size)
	{

		*offset_before = *offset;
		*offset = dir_offset;

		readdir_ext2(file_dir, found_entry, namebuf, &dir_offset);

		if (!strcmp(namebuf, fname))
		{
				found = 1;
				break;
		}

	}

	if (found)
	{
		retval = 1;
	}

	return retval;
}

int lookup_last_in_dir_ext2(file_ext2_t *file_dir, dir_entry_ext2_t* last_entry, uint32_t* offset_last )
{
	int retval = -1;

	uint32_t dir_offset = 0;

	blk_iterator_next(file_dir->it_lpos, dir_offset);

	char namebuf[EXT2_NAMELEN];

	uint32_t dir_size = file_dir->pinode->i_size;

	*offset_last = dir_offset;

	while (dir_offset < dir_size)
	{
		*offset_last = dir_offset;
		readdir_ext2(file_dir, last_entry, namebuf, &dir_offset);

	}

	retval = 1;

	return retval;

}

#define PPE_FLAG_LOOKUP_PREVIOUS		0x00000001


int parse_path_ext2(file_ext2_t* file_pwd, uint32_t mode, char* path,
		file_ext2_t *file, char* last_fname)
{
	int retval = -1;

	dir_entry_ext2_t akt_entry;

	int argc;
	char* argv[MAX_PATH_COMPONENTS];

	int nlen = strlen(path);
	char* path_copy = malloc(nlen + 1);
	memcpy(path_copy, path, nlen + 1);

	parse_buf(path_copy, strlen(path), "/", &argc, argv);

	if (argc > 0)
	{
		strcpy(last_fname, argv[argc-1]);
	}

	int i;

#if 0
	outb_printf("parse_path_ext2: argc = %d\n", argc);


	for(i = 0; i < argc; ++i)
	{
		outb_printf("parse_path_ext2: argv[%d] = >%s<\n", i, argv[i]);
	}
#endif


	file_ext2_t file_akt;
	init_file_ext2(&file_akt, file->dev_file, file->sb);

	copy_file_ext2(&file_akt, file_pwd);

	i = 0;

	uint32_t goal = argc;

	if (mode & PPE_FLAG_LOOKUP_PREVIOUS)
	{
		--goal;
	}

	while (i < goal)
	{
		int found = 0;

		//outb_printf("parse_path_ext2: argv[%d] = %s\n", i, argv[i]);

		uint32_t akt_offset;
		uint32_t before_offset;

		found = lookup_name_in_dir_ext2(&file_akt, argv[i], &akt_entry, &akt_offset, &before_offset);

		if (found == 1)
		{
			uint32_t akt_inode_index = akt_entry.inode;

			outb_printf("parse_path_ext2: akt_inode_index = %d\n", akt_inode_index);

			read_inode_ext2(&file_akt, akt_inode_index);

			if (file_akt.pinode->i_mode & EXT2_S_IFREG)
			{
				++i;
				outb_printf("parse_path_ext2: regular file found.\n");
				break;
			}
		}
		else
		{
			printf("parse_path_ext2: could not resolve %s at %s.\n", path, argv[i]);
			goto ende;
		}
		++i;
	}

	if (i < goal)
	{
		printf("parse_path_ext2: is not a directory: %s\n", argv[i]);
		goto ende;
	}

	ASSERT(file_akt.pinode);
	ASSERT(file_akt.pbgd);
	ASSERT(file_akt.it_lpos);
	ASSERT(file_akt.it_end);

	copy_file_ext2(file, &file_akt);


	retval = 0;

ende:

	destroy_file_ext2(&file_akt);
	free(path_copy);

	return retval;
}

#define CDE_NULL_INODE_FOUND	1
#define CDE_NON_NULL_INODE_FOUND	2


int create_dir_entry_ext2(file_ext2_t* filp_parent_directory, file_ext2_t* filp_new, char* fname)
{

	int retval = -1;

	uint32_t ext2_blocksize = GET_BLOCKSIZE_EXT2(filp_parent_directory->sb);

	// prepare new entry

	dir_entry_ext2_t new_entry;

	new_entry.inode = filp_new->inode_index;

	uint16_t new_imode = filp_new->pinode->i_mode;

	new_entry.file_type = new_imode & EXT2_S_IFREG ?
			EXT2_FT_REG_FILE : new_imode & EXT2_S_IFDIR? EXT2_FT_DIR : EXT2_FT_UNKNOWN;

	uint32_t fnamelen = strlen(fname);

	if (fnamelen > EXT2_NAMELEN)
	{
		retval = -1;
		goto ende;

	}

	new_entry.name_len = fnamelen;

	uint16_t rec_len_prelim = sizeof(dir_entry_ext2_t) + fnamelen;

	rec_len_prelim = align(rec_len_prelim, 4);

	printf("fname = %s : rec_len_prelim = %d\n", fname, fnamelen);


	// iterate through parent directory

	dir_entry_ext2_t last_entry;
	uint32_t offset_last;

	uint32_t dir_offset = 0;

	blk_iterator_next(filp_parent_directory->it_lpos, dir_offset);

	char namebuf[EXT2_NAMELEN];

	uint32_t dir_size = filp_parent_directory->pinode->i_size;

	offset_last = dir_offset;

	uint16_t last_entry_necess_len = 0;
	uint16_t last_entry_free_space = 0;

	uint16_t last_entry_rec_len = 0;

	int space_found = 0;

	while (dir_offset < dir_size)
	{
		offset_last = dir_offset;
		readdir_ext2(filp_parent_directory, &last_entry, namebuf, &dir_offset);

		last_entry_rec_len = last_entry.rec_len;

		if (!last_entry.inode)
		{
			last_entry_necess_len = 0;
			last_entry_free_space = last_entry.rec_len;
		}
		else
		{
			last_entry_necess_len = sizeof(dir_entry_ext2_t) + last_entry.name_len;
			last_entry_necess_len = align(last_entry_necess_len, 4);
			last_entry_free_space = last_entry.rec_len - last_entry_necess_len;
		}

		if (last_entry_free_space >= rec_len_prelim)
		{
			space_found = !last_entry.inode ? CDE_NULL_INODE_FOUND : CDE_NON_NULL_INODE_FOUND;
			break;
		}


	}


	uint32_t parent_dir_size = filp_parent_directory->pinode->i_size;

	int dir_empty = !parent_dir_size;

	uint32_t last_entry_real_len = 0;


	// calculate start of new entry
	// must be on a 4 byte boundary and entry must not cross ext2 block border

	uint32_t start_new_entry1;

	if (space_found)
	{
		start_new_entry1 = offset_last + last_entry_necess_len;
	}
	else
	{
		// space not found
		// last_entry is really the *last* entry in directory (if not dir_empty)

		if (!dir_empty)
		{
			last_entry_real_len = sizeof(dir_entry_ext2_t) + last_entry.name_len;
			last_entry_real_len = align(last_entry_real_len, 4);
		}

		uint32_t start_new_entry = offset_last + last_entry_real_len;

		start_new_entry1 = start_new_entry;

		uint32_t rest_in_block = ext2_blocksize - start_new_entry1 % ext2_blocksize;

		if (rest_in_block < rec_len_prelim)
		{
			start_new_entry1 += rest_in_block;
		}

	}
	// rewrite rec_len of last entry

	if (!dir_empty && space_found != CDE_NULL_INODE_FOUND)
	{
		last_entry.rec_len = start_new_entry1 - offset_last;

		writedir_ext2(filp_parent_directory, &last_entry, 0, offset_last);
	}

	// calculate the end of the new entry

	uint32_t end_new_entry = start_new_entry1 + rec_len_prelim;
	uint32_t end_new_entry1 = end_new_entry;

	uint32_t end_last_entry = offset_last + last_entry_rec_len;

	if (space_found)
	{
		end_new_entry1 = end_last_entry;
	}

	if (!space_found)
	{
		end_new_entry1 = align(max(end_new_entry, parent_dir_size), ext2_blocksize);
	}

	new_entry.rec_len = end_new_entry1 - start_new_entry1;

	// now write the new directory entry

	writedir_ext2(filp_parent_directory, &new_entry, fname, start_new_entry1);

	ASSERT(filp_parent_directory->pinode->i_size % ext2_blocksize == 0);

	++filp_new->pinode->i_links_count;
	write_inode_ext2(filp_new);


	retval = 0;

	ende:
	return retval;

}


// filp_new_file is only rawly filled with dev_file and sb
// it gets filled in allocate_new_inode
int create_file_ext2(file_ext2_t* filp_parent_dir,
		file_ext2_t *filp_new_file, char* fname, uint16_t imode, uint32_t iflags)
{

	int retval = -1;

	retval = allocate_new_inode(filp_new_file, filp_parent_dir->pbgd_index, imode, iflags);
	if (retval < 0)
	{
		goto ende;
	};

	retval = create_dir_entry_ext2(filp_parent_dir, filp_new_file, fname);

	ende:
	return retval;
}

// filp_new_file is only rawly filled with dev_file and sb
// it gets filled in allocate_new_inode
int create_directory_ext2(file_ext2_t* filp_parent_dir,
		file_ext2_t *filp_new_dir, char* fname, uint16_t imode, uint32_t iflags)
{

	int retval = -1;

	imode |= EXT2_S_IFDIR;
	imode &= ~EXT2_S_IFREG;

	retval = allocate_new_inode(filp_new_dir, filp_parent_dir->pbgd_index, imode, iflags);
	if (retval < 0)
	{
		goto ende;
	};

	retval = create_dir_entry_ext2(filp_parent_dir, filp_new_dir, fname);
	if (retval < 0)
	{
		goto ende;
	}

	retval = create_dir_entry_ext2(filp_new_dir, filp_new_dir, ".");
	retval = create_dir_entry_ext2(filp_new_dir, filp_parent_dir, "..");

	ende:
	return retval;
}


int unlink_file_ext2(file_ext2_t* filp_dir, char* fname)
{
	int retval = -1;

	dir_entry_ext2_t akt_entry;

	uint32_t akt_offset = 0;
	uint32_t before_offset = 0;

	uint32_t ext2_blocksize = GET_BLOCKSIZE_EXT2(filp_dir->sb);

	int found = lookup_name_in_dir_ext2(filp_dir, fname, &akt_entry, &akt_offset, &before_offset);

	if (found != 1)
	{
		goto ende;
	}

	int is_first = (akt_offset == before_offset);

	int crosses_block = 0;

	if (!is_first)
	{
		dir_entry_ext2_t before_entry;
		read_dir_entry_ext2(filp_dir, before_offset, &before_entry);

		uint16_t before_rec_len = before_entry.rec_len;

		uint16_t new_before_rec_len = before_rec_len + akt_entry.rec_len;
		uint16_t new_before_rec_len1 = new_before_rec_len;

		crosses_block = ((before_offset % ext2_blocksize) + new_before_rec_len) > ext2_blocksize;

		if (crosses_block)
		{
			new_before_rec_len1 = ext2_blocksize - (before_offset % ext2_blocksize);
		}

		before_entry.rec_len = new_before_rec_len1;

		write_dir_entry_ext2(filp_dir, before_offset, &before_entry);

	}

	uint32_t del_inode = akt_entry.inode;

	file_ext2_t file_del;
	init_file_ext2(&file_del, filp_dir->dev_file, filp_dir->sb);

	read_inode_ext2(&file_del, del_inode);

	if (! --file_del.pinode->i_links_count)
	{
		remove_file_ext2(&file_del);
		read_inode_ext2(filp_dir, filp_dir->inode_index);
	}
	else
	{
		//outb_printf("unlink_file_ext2: file_del inode = %d file_del i_links_count = %d\n",
		//		file_del.inode_index, file_del.pinode->i_links_count);
		write_inode_ext2(&file_del);

		//read_inode_ext2(&file_del, file_del.inode_index);
		//outb_printf(
		//		"unlink_file_ext2: read after write:file_del inode = %d file_del i_links_count = %d\n",
		//		file_del.inode_index, file_del.pinode->i_links_count);

		read_inode_ext2(filp_dir, filp_dir->inode_index);

	}

	destroy_file_ext2(&file_del);

	akt_entry.inode = 0;

	if (crosses_block)
	{
		akt_entry.file_type = 0;
		akt_entry.name_len = 0;
		writedir_ext2(filp_dir, &akt_entry, "", akt_offset);
	}
	else
	{
		write_dir_entry_ext2(filp_dir, akt_offset, &akt_entry);
	}

	retval = 0;
	ende:
	return retval;
}

int delete_directory_ext2(file_ext2_t* filp_dir, char* fname)
{
	int retval = -1;

	dir_entry_ext2_t del_entry;
	uint32_t offset;
	uint32_t offset_before;

	int found = lookup_name_in_dir_ext2(filp_dir, fname, &del_entry, &offset, &offset_before);

	if (found != 1)
	{
		goto ende;
	}

	file_ext2_t f_del_dir;
	init_file_ext2(&f_del_dir, filp_dir->dev_file, filp_dir->sb);

	read_inode_ext2(&f_del_dir, del_entry.inode);

	dir_entry_ext2_t last_entry;

	uint32_t offset_last;

	lookup_last_in_dir_ext2(&f_del_dir, &last_entry, &offset_last);

	if (offset_last == sizeof(dir_entry_ext2_t) + 4)
	{
		unlink_file_ext2(&f_del_dir, "..");
		unlink_file_ext2(&f_del_dir, ".");
	}
	else
	{
		retval = -1;
		goto ende;
	}

	//read_inode_ext2(&f_del_dir, f_del_dir.inode_index);

	//outb_printf("delete_directory_ext2: after read: inode = %d links_count = %d\n",
	//		f_del_dir.inode_index, f_del_dir.pinode->i_links_count);

	read_inode_ext2(filp_dir, filp_dir->inode_index);

	//outb_printf("delete_directory_ext2: filp_dir: inode = %d links_count = %d\n",
	//		filp_dir->inode_index, filp_dir->pinode->i_links_count);


	unlink_file_ext2(filp_dir, fname);

	--filp_dir->pbgd->bg_used_dirs_count;
	write_bgdesc_ext2(filp_dir->dev_file, filp_dir->pbgd, filp_dir->pbgd_index);

	ende:
	destroy_file_ext2(&f_del_dir);
	return retval;
}



int display_directory_ext2(file_ext2_t* file)
{

	uint32_t dir_offset = 0;

	blk_iterator_next(file->it_lpos, dir_offset);

	dir_entry_ext2_t akt_entry;

	char namebuf[EXT2_NAMELEN];

	//printf("display_directory_ext2: pinode->i_blocks = %d : ", pinode->i_blocks);
	//printf("pinode->i_size = %d\n", pinode->i_size);

	uint32_t pinode_size = file->pinode->i_size;

	do
	{
		readdir_ext2(file, &akt_entry, namebuf, &dir_offset);

		uint32_t akt_inode_index = akt_entry.inode;
		int is_dir = 0;
		uint32_t akt_size = 0;
		uint32_t akt_blocks = 0;

		uint32_t akt_flags = 0;

		if (akt_inode_index)
		{

			file_ext2_t file1;
			init_file_ext2(&file1, 0, 0);

			copy_file_ext2(&file1, file);

			read_inode_ext2(&file1, akt_inode_index);

			is_dir = file1.pinode->i_mode & EXT2_S_IFDIR;

			akt_size = file1.pinode->i_size;

			akt_blocks = file1.pinode->i_blocks;

			akt_flags = file1.pinode->i_flags;

			destroy_file_ext2(&file1);

		}

		printf(
				"%s : %c : %d bytes : %d blocks(512b) : i_flags = %08x : inode = %d : rec_len = %d : dir_offset = %d\n",
				namebuf, is_dir ? 'd' : 'r', akt_size, akt_blocks, akt_flags, akt_entry.inode, akt_entry.rec_len,
						dir_offset);
	}
	while(dir_offset < pinode_size);

	return 0;
}

int display_regular_file_ext2(file_ext2_t *file)
{

	ncnt = 0;

	const int aux_buf_size = 16 * 1024;
	char* auxbuf = (char*)malloc(aux_buf_size);

	int cnt_zeros = 0;

	uint32_t rest = file->pinode->i_size;
	uint32_t offset = 0;

	uint32_t nrd_total = 0;
	int nrd = 0;

	blk_iterator_init(file->it_lpos, file, offset);

	while (rest > 0)
	{
		uint32_t to_read = min(aux_buf_size - 1, rest);
		nrd = read_file_ext2(file, auxbuf, to_read, offset);

		if (nrd < 0)
		{
			printf("\n\ndisplay_regular_file_ext2: break: nrd = %d\n", nrd);
			break;
		}

		char *p;

		int i;
		for(i = 0; i < nrd; ++i)
		{
			if (!auxbuf[i])
			{
				++cnt_zeros;
			}
		}
		for(i = 0; i < nrd; ++i)
		{
			p = &auxbuf[i];
			if (*p)
			{
				break;
			}
		}


		auxbuf[nrd] = 0x0;


		printf("%s", p);

		nrd_total += nrd;
		rest -= nrd;
		offset += nrd;
	}

	ASSERT(nrd_total == file->pinode->i_size);

	printf("\ndisplay_regular_file_ext2: read = %d : zeros in file: %d\n", nrd_total, cnt_zeros);

	free(auxbuf);

	return nrd >= 0 ? nrd_total: nrd;
}


int display_inode_ext2(file_ext2_t *file)
{
	if (file->pinode->i_mode & EXT2_S_IFDIR)
	{
		display_directory_ext2(file);
	}
	else if (file->pinode->i_mode & EXT2_S_IFREG)
	{
		display_regular_file_ext2(file);
	}
	return 0;
}


int test_ext2_write(file_t* dev_file, superblock_ext2_t* sb, int no_runs)
{
	file_ext2_t file_testa;
	init_file_ext2(&file_testa, dev_file, sb);

	file_ext2_t file_testb;
	init_file_ext2(&file_testb, dev_file, sb);

	file_ext2_t file_testc;
	init_file_ext2(&file_testc, dev_file, sb);

	file_ext2_t file_dir_testdir;
	init_file_ext2(&file_dir_testdir, dev_file, sb);

	file_ext2_t f_root_dir;
	init_file_ext2(&f_root_dir, dev_file, sb);

	read_inode_ext2(&f_root_dir, 2);

	create_directory_ext2(&f_root_dir, &file_dir_testdir, "testdir", 0755, 0);

	uint16_t access_mode = 0644;
	uint16_t file_mode = access_mode | EXT2_S_IFREG;


	int creat_del_loops = 1;

	while (creat_del_loops >= 0)
	{

		init_file_ext2(&file_testa, dev_file, sb);
		init_file_ext2(&file_testb, dev_file, sb);
		init_file_ext2(&file_testc, dev_file, sb);

		create_file_ext2(&f_root_dir, &file_testa, "newtesta", file_mode, 0);
		create_file_ext2(&f_root_dir, &file_testb, "newtestb", file_mode, 0);
		create_file_ext2(&f_root_dir, &file_testc, "newtestc", file_mode, 0);


#if 0
		parse_path_ext2(&file_testa, "/testa");
		parse_path_ext2(&file_testb, "/testb");
		parse_path_ext2(&file_testc, "/testc");
#endif

#if 1
		uint8_t* auxbuf = 0;
		alloc_if_empty((char**)&auxbuf, 17 * 1024);

		int runs = no_runs;
		int i;

		int upper_limit = 512 * 1024 + 37;
		int write_limit = 16 * 1024 + 19;

		for(i = 0; i < runs; ++i)
		{
			int wrt_end;
			int wrt_len;
			while (!(wrt_end = (rand() % upper_limit)));
			while (!(wrt_len = (rand() % write_limit)));

			int wrt_start = max(wrt_end - wrt_len + 1, 0);
			wrt_len = wrt_end - wrt_start + 1;

			ASSERT(wrt_len <= write_limit);

			int j = 0;
			for(j = 0; j < wrt_len; ++j)
			{
				auxbuf[j] = rand() % 256;
			}
			int nwrt;
			nwrt = write_file_ext2(&file_testa, (char*) auxbuf, wrt_len, wrt_start);
			ASSERT(nwrt = wrt_len);

			nwrt = write_file_ext2(&file_testb, (char*) auxbuf, wrt_len, wrt_start);
			ASSERT(nwrt = wrt_len);

			nwrt = write_file_ext2(&file_testc, (char*) auxbuf, wrt_len, wrt_start);
			ASSERT(nwrt = wrt_len);

		}

#endif

		if (creat_del_loops > 0)
		{
			printf("deleting: creat_del_loops = %d\n", creat_del_loops);

			unlink_file_ext2(&f_root_dir, "newtesta");
			unlink_file_ext2(&f_root_dir, "newtestb");
			unlink_file_ext2(&f_root_dir, "newtestc");

		}

		destroy_file_ext2(&file_testa);
		destroy_file_ext2(&file_testb);
		destroy_file_ext2(&file_testc);


		--creat_del_loops;
	}

	//ende:

	destroy_file_ext2(&f_root_dir);

#if 0
	destroy_file_ext2(&file_testa);
	destroy_file_ext2(&file_testb);
	destroy_file_ext2(&file_testc);
#endif

	destroy_file_ext2(&file_dir_testdir);

	return 0;


}

