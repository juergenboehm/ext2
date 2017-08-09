#ifndef __fs_ext2_h
#define __fs_ext2_h

#include "fs/vfs.h"

#define SIZE_SB		1024
#define OFFSET_SB	1024
#define BLK_SIZE_EXT2	1024
#define LOG_IN_PHYS	2

#define MAX_PATH_COMPONENTS 32

#define AUX_BLOCK_SIZE	4096


#define EXT2_S_IFSOCK	0xC000
#define EXT2_S_IFLNK	0xA000
#define EXT2_S_IFREG	0x8000
#define EXT2_S_IFBLK	0x6000
#define EXT2_S_IFDIR	0x4000
#define EXT2_S_IFCHR	0x2000
#define EXT2_S_IFIFO	0x1000

#define EXT2_S_IRUSR	0x0100
#define EXT2_S_IWUSR	0x0080
#define EXT2_S_IXUSR	0x0040
#define EXT2_S_IRGRP	0x0020
#define EXT2_S_IWGRP	0x0010
#define EXT2_S_IXGRP	0x0008
#define EXT2_S_IROTH	0x0004
#define EXT2_S_IWOTH	0x0002
#define EXT2_S_IXOTH	0x0001

#define EXT2_FT_UNKNOWN		0
#define EXT2_FT_REG_FILE	1
#define EXT2_FT_DIR				2

#define EXT2_NAMELEN		255





#define SI_INDEX	12
#define DI_INDEX	13
#define TI_INDEX	14

#define BLKNUM_TO_OFFSET(blk, blk_size) ((blk) * (blk_size))

#define GET_BLOCKSIZE_EXT2(sb) (1024 << (sb->s_log_block_size))

typedef struct superblock_ext2_s {
/*0	4 */ uint32_t	s_inodes_count;
/*4	4	*/ uint32_t s_blocks_count;
/*8	4 */ uint32_t s_r_blocks_count;
/*12 4 */ uint32_t	s_free_blocks_count;
/*16	4	*/ uint32_t s_free_inodes_count;
/*20	4	*/ uint32_t s_first_data_block;
/*24	4	*/ uint32_t s_log_block_size;
/*28	4	*/ uint32_t s_log_frag_size;
/*32	4	*/ uint32_t s_blocks_per_group;
/*36	4	*/ uint32_t s_frags_per_group;
/*40	4	*/ uint32_t s_inodes_per_group;
/*44	4	*/ uint32_t s_mtime;
/*48	4	*/ uint32_t s_wtime;
/*52	2	*/ uint16_t s_mnt_count;
/*54	2	*/ uint16_t s_max_mnt_count;
/*56	2	*/ uint16_t s_magic;
/*58	2	*/ uint16_t s_state;
/*60	2	*/ uint16_t s_errors;
/*62	2	*/ uint16_t s_minor_rev_level;
/*64	4	*/ uint32_t s_lastcheck;
/*68	4	*/ uint32_t s_checkinterval;
/*72	4	*/ uint32_t s_creator_os;
/*76	4	*/ uint32_t s_rev_level;
/*80	2	*/ uint16_t s_def_resuid;
/*82	2	*/ uint16_t s_def_resgid;

// EXT2_DYNAMIC_REV Specific --

/*84	4	*/ uint32_t s_first_ino;
/*88	2	*/ uint16_t s_inode_size;
/*90	2	*/ uint16_t s_block_group_nr;
/*92	4	*/ uint32_t s_feature_compat;
/*96	4	*/ uint32_t s_feature_incompat;
/*100	4	*/ uint32_t s_feature_ro_compat;
/*104	16	*/ uint8_t s_uuid[16];
/*120	16	*/ uint8_t s_volume_name[16];
/*136	64	*/ uint8_t s_last_mounted[64];
/*200	4	*/ uint32_t s_algo_bitmap;

//  Performance Hints --

/*204	1	*/ uint8_t s_prealloc_blocks;
/*205	1	*/ uint8_t s_prealloc_dir_blocks;
/*206	2	*/ uint16_t xs_alignment; //(alignment)

// Journaling Support --

/*208	16	*/ uint8_t s_journal_uuid[16];
/*224	4	*/ uint32_t s_journal_inum;
/*228	4	*/ uint32_t s_journal_dev;
/*232	4	*/ uint32_t s_last_orphan;

//  Directory Indexing Support --

/*236	4 x 4	*/ uint32_t s_hash_seed[4];
/*252	1	*/ uint8_t s_def_hash_version;
/*253	3	*/ uint8_t xs_padding[3]; // - reserved for future expansion

//  Other options --

/*256	4	*/ uint32_t s_default_mount_options;
/*260	4	*/ uint32_t s_first_meta_bg;
/*264	*/ uint8_t s_unused_padding[760]; // 760	Unused - reserved for future revisions
} superblock_ext2_t;


typedef struct bg_desc_ext2_s {
/* 0	4 */	uint32_t bg_block_bitmap;
/* 4	4 */	uint32_t bg_inode_bitmap;
/* 8	4 */	uint32_t bg_inode_table;
/* 12	2 */	uint16_t bg_free_blocks_count;
/* 14	2 */	uint16_t bg_free_inodes_count;
/* 16	2 */	uint16_t bg_used_dirs_count;
/* 18	2 */	uint16_t bg_pad;
/* 20	12 */	uint8_t  bg_reserved[12];
} bg_desc_ext2_t;


typedef struct inode_ext2_s {
/* 0	2	*/ uint16_t i_mode;
/* 2	2	*/ uint16_t i_uid;
/* 4	4	*/ uint32_t i_size;
/* 8	4	*/	uint32_t i_atime;
/* 12	4	*/	uint32_t i_ctime;
/* 16	4	*/	uint32_t i_mtime;
/* 20	4	*/	uint32_t i_dtime;
/* 24	2	*/ uint16_t i_gid;
/* 26	2	*/ uint16_t i_links_count;
/* 28	4	*/	uint32_t i_blocks;
/* 32	4	*/	uint32_t i_flags;
/* 36	4	*/	uint32_t i_osd1;
/* 40	15 x 4	*/	uint32_t i_block[15];
/* 100	4	*/	uint32_t i_generation;
/* 104	4	*/	uint32_t i_file_acl;
/* 108	4	*/	uint32_t i_dir_acl;
/* 112	4	*/	uint32_t i_faddr;
/* 116	12 */	uint8_t i_osd2[12];
} inode_ext2_t;

typedef struct dir_entry_ext2_s
{
	uint32_t	inode;
	uint16_t	rec_len;
	uint8_t		name_len;
	uint8_t		file_type;
} dir_entry_ext2_t;

typedef struct blk_iterator_s
	blk_iterator_t;


typedef struct file_ext2_s
{
	file_t* dev_file;
	superblock_ext2_t* sb;

	inode_ext2_t* pinode;

	uint32_t inode_index;
	uint32_t offset_inode;

	bg_desc_ext2_t* pbgd;
	uint32_t pbgd_index;

	blk_iterator_t* it_lpos;
	blk_iterator_t* it_end;



} file_ext2_t;


typedef struct blk_iterator_s
{
	int mode;
	uint32_t index[4];
	char* blks[4];
	int blk_valid[4];
	uint32_t blks_blk[4];

	file_ext2_t* file;
	inode_ext2_t* pinode;
	uint32_t inode_pos;

	int code;
	file_t *dev_file;
	uint32_t offset;


} blk_iterator_t;


extern superblock_ext2_t* gsb_ext2;

extern uint32_t g_blocks_per_group_ext2;

extern bg_desc_ext2_t* gbgd_ext2;



// ext2 routines


int read_superblock_ext2(file_t* dev_file, superblock_ext2_t* sb);
int write_superblock_ext2(file_t* dev_file, superblock_ext2_t* sb);


int init_superblock_ext2(file_t* dev_file);
int init_ext2_system(file_t* dev_file);

int print_sb_ext2(superblock_ext2_t* sb);
int print_bgdesc_ext2(bg_desc_ext2_t* pbgdesc);
int print_inode_ext2(inode_ext2_t* pinode);


int read_inode_ext2(file_ext2_t* file, uint32_t inode_index);
int read_bgdesc_ext2(file_t* dev_file, bg_desc_ext2_t* pbgd, uint32_t bg_index);

int write_inode_ext2(file_ext2_t* filp);

inline uint32_t get_first_data_block_pos_in_block_group_bitmap(uint32_t bgd_index,
		superblock_ext2_t* sb, bg_desc_ext2_t* pbgd);

inline uint32_t get_last_data_block_pos_in_block_group_bitmap(uint32_t bgd_index,
		superblock_ext2_t *sb, bg_desc_ext2_t *pbgd);
inline uint32_t get_first_data_block_in_block_group(bg_desc_ext2_t* pbgd, superblock_ext2_t* sb);




int readdir_ext2(file_ext2_t* file, dir_entry_ext2_t* dir_entry,
		char* namebuf, uint32_t *dir_offset);
int read_file_ext2(file_ext2_t* file, char* buf, uint32_t counta, uint32_t offseta);
int write_file_ext2(file_ext2_t* file, char* buf, uint32_t counta, uint32_t offseta);

int parse_path_ext2(file_ext2_t* file_pwd, uint32_t mode, char* path,
		file_ext2_t *file, char* last_fname);


int get_indirect_blocks(uint32_t offset, uint32_t* index_arr, uint32_t *mode);
int test_get_indirect_blocks();


int create_file_ext2(file_ext2_t* filp_parent_dir,
		file_ext2_t *filp_new_file, char* fname, uint16_t imode, uint32_t iflags);

int create_directory_ext2(file_ext2_t* filp_parent_dir,
		file_ext2_t *filp_new_dir, char* fname, uint16_t imode, uint32_t iflags);

int delete_directory_ext2(file_ext2_t* filp_dir, char* fname);

int unlink_file_ext2(file_ext2_t* filp_dir, char* fname);



int display_directory_ext2(file_ext2_t *file);
int display_regular_file_ext2(file_ext2_t *file);
int display_inode_ext2(file_ext2_t *file);

int read_from_dev(file_t* dev_file, char* buf, uint32_t count, uint32_t offset);
int write_to_dev(file_t* dev_file, char* buf, uint32_t count, uint32_t offset);



int blk_iterator_init(blk_iterator_t *it, file_ext2_t* file,
		uint32_t offset);
int blk_iterator_destroy(blk_iterator_t* it);

int blk_iterator_next(blk_iterator_t *it, uint32_t offset_new);

uint32_t blk_iterator_get_blk_index(blk_iterator_t *it);



inode_ext2_t* alloc_inode_ext2();
bg_desc_ext2_t* alloc_bg_desc_ext2();
blk_iterator_t* alloc_blk_iterator();
superblock_ext2_t* alloc_superblock_ext2();
file_ext2_t* alloc_file_ext2();

int copy_file_ext2(file_ext2_t* fdest, file_ext2_t* fsrc);
int destroy_file_ext2(file_ext2_t* fdestr);

void init_file_ext2(file_ext2_t* filp, file_t* dev_file, superblock_ext2_t* sb);


// randomized testing ext2 file writes

int test_ext2_write(file_t* dev_file, superblock_ext2_t* sb, int no_runs);




#endif
