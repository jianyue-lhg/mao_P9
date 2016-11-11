#ifndef _DEDUPE_H
#define _DEDUPE_H

#define F2FS_BLOOM_FILTER 1
//#define F2FS_NO_HASH 1
#define F2FS_REVERSE_ADDR 1
#define F2FS_DEDUPE_PERSISTENCE 1

#define DEDUPE_PER_BLOCK (PAGE_CACHE_SIZE/sizeof(struct dedupe))

typedef u32 block_t;

struct dedupe
{
	block_t addr;
	int ref;
	u8 hash[16];
};

struct dedupe_aa
{
	int dedupe_segment_count;
	int dedupe_block_count;
	char *dedupe_md_dirty_bitmap;	/*bitmap for dirty dedupe blocks*/
	char *dedupe_bitmap;				/*bitmap for dedupe checkpoint*/
	struct mutex mem_lock;
	int need_free;

	struct dedupe *dedupe_md;
	struct dedupe* last_delete_dedupe;

#ifdef F2FS_BLOOM_FILTER
	unsigned int bloom_filter_mask;
	unsigned int *bloom_filter;
	unsigned int bloom_filter_hash_fun_count;
#endif
};

struct dedupe_info
{
	spinlock_t lock;
	int digest_len;
	struct crypto_shash *tfm;
	unsigned int crypto_shash_descsize;
	unsigned int dedupe_base_blkaddr;

	unsigned int logical_blk_cnt;
	unsigned int physical_blk_cnt;
	char *dedupe_md_dirty_bitmap;	/*bitmap for dirty dedupe blocks*/
	char *dedupe_bitmap;				/*bitmap for dedupe checkpoint*/
	unsigned int dedupe_bitmap_size;	/*bitmap size of dedupe_md_dirty_bitmap&dedupe_bitmap*/

	unsigned int dedupe_size;			/*size of dedupes in memory*/
	unsigned int dedupe_block_count;
	struct dedupe_aa *dedupe_aa;
	int dedupe_aa_count;

#ifdef F2FS_REVERSE_ADDR
	int *reverse_addr;
#endif
};


extern int f2fs_dedupe_calc_hash(struct page *p, u8 hash[], struct dedupe_info *dedupe_info);
extern struct dedupe *f2fs_dedupe_search(u8 hash[], struct dedupe_info *dedupe_info, int aa_addr);
extern int f2fs_dedupe_add(u8 hash[], struct dedupe_info *dedupe_info, block_t addr, int aa_addr);
extern int init_dedupe_info(struct dedupe_info *dedupe_info);
extern void init_f2fs_dedupe_bloom_filter(struct dedupe_info *dedupe_info, int aa_addr);
extern void exit_dedupe_info(struct dedupe_info *dedupe_info);
extern int f2fs_dedupe_delete_addr(block_t addr, struct dedupe_info *dedupe_info, int aa_addr);
extern void set_dedupe_dirty(struct dedupe_info *dedupe_info, struct dedupe *dedupe, int aa_addr);

#endif

