#include <linux/pagemap.h>
#include <crypto/hash.h>
#include <crypto/md5.h>
#include <crypto/sha.h>
#include <crypto/algapi.h>
#include "dedupe.h"
#include <linux/f2fs_fs.h>
#include <linux/vmalloc.h>

int f2fs_dedupe_calc_hash(struct page *p, u8 hash[], struct dedupe_info *dedupe_info)
{
	//int i;
	int ret;
	
	struct {
		struct shash_desc desc;
		char ctx[dedupe_info->crypto_shash_descsize];
	} sdesc;
	char *d;

	sdesc.desc.tfm = dedupe_info->tfm;
	sdesc.desc.flags = 0;
	ret = crypto_shash_init(&sdesc.desc);
	if (ret)
		return ret;

	d = kmap(p);
	ret = crypto_shash_digest(&sdesc.desc, d, PAGE_SIZE, hash);
	kunmap(p);

	/*for(i=0;i<4;i++)
	{
		printk("%llx",be64_to_cpu(*(long long*)&hash[i*8]));
	}
	printk("\n");*/

	return ret;
}

#ifdef F2FS_BLOOM_FILTER
int f2fs_dedupe_bloom_filter(u8 hash[], struct dedupe_info *dedupe_info)
{
	int i;
	unsigned int *pos = (unsigned int *)hash;
	for(i=0;i<dedupe_info->bloom_filter_hash_fun_count;i++)
	{
		if(0 == dedupe_info->bloom_filter[*(pos++)&dedupe_info->bloom_filter_mask])
		{
			return 1;
		}
	}
	return 0;
}

void init_f2fs_dedupe_bloom_filter(struct dedupe_info *dedupe_info)
{
	struct dedupe *cur;
	int i;
	for(cur = dedupe_info->dedupe_md; cur < dedupe_info->dedupe_md + dedupe_info->dedupe_block_count * DEDUPE_PER_BLOCK;cur++)
	{
		if(unlikely(cur->ref))
		{
			unsigned int *pos = (unsigned int *)cur->hash;
			for(i=0;i<dedupe_info->bloom_filter_hash_fun_count;i++)
			{
				dedupe_info->bloom_filter[*(pos++)&dedupe_info->bloom_filter_mask]++;
			}
		}
	}
}
#endif


struct dedupe *f2fs_dedupe_search(u8 hash[], struct dedupe_info *dedupe_info)
{
	struct dedupe *c = &dedupe_info->dedupe_md[(*(unsigned int *)hash)%(dedupe_info->dedupe_block_count/64) * DEDUPE_PER_BLOCK*64],*cur;
#ifdef F2FS_NO_HASH
	c = dedupe_info->dedupe_md;
#endif

#ifdef F2FS_BLOOM_FILTER
	if(f2fs_dedupe_bloom_filter(hash, dedupe_info)) return NULL;
#endif

	for(cur=c; cur < dedupe_info->dedupe_md + dedupe_info->dedupe_block_count * DEDUPE_PER_BLOCK; cur++)
	{
		if(unlikely(cur->ref&&!memcmp(hash, cur->hash, dedupe_info->digest_len)))
		{
			dedupe_info->logical_blk_cnt++;
			return cur;
		}
	}
	for(cur = dedupe_info->dedupe_md; cur < c; cur++)
	{
		if(unlikely(cur->ref&&!memcmp(hash, cur->hash, dedupe_info->digest_len)))
		{
			dedupe_info->logical_blk_cnt++;
			return cur;
		}
	}

	return NULL;
}

void set_dedupe_dirty(struct dedupe_info *dedupe_info, struct dedupe *dedupe)
{
	set_bit((dedupe - dedupe_info->dedupe_md)/DEDUPE_PER_BLOCK,  (long unsigned int *)dedupe_info->dedupe_md_dirty_bitmap);
}

struct dedupe  *f2fs_dedupe_delete_addr(block_t addr, struct dedupe_info *dedupe_info)
{
	struct dedupe *cur,*c = dedupe_info->last_delete_dedupe;

	spin_lock(&dedupe_info->lock);
	if(NEW_ADDR == addr) 
		//return -1;
		return NULL;

#ifdef F2FS_REVERSE_ADDR
	if(-1 == dedupe_info->reverse_addr[addr])
	{
		//return -1;
		return NULL;
	}
	cur = &dedupe_info->dedupe_md[dedupe_info->reverse_addr[addr]];
	if(cur->ref)
	{
		goto aa;
	}
	else
	{
		//return -1;
		return NULL;
	}
#endif

	for(cur=c; cur < dedupe_info->dedupe_md + dedupe_info->dedupe_block_count * DEDUPE_PER_BLOCK; cur++)
	{
		if(unlikely(cur->ref && addr == cur->addr))
			{
			int i;
			  	for(i=0;i<2;i++)
	{
		printk("%llx",be64_to_cpu(*(long long*)&cur->hash[i*8]));
		
	}
			printk("\nthis is\n");
			
		     /* memcpy(hash,cur->hash,dedupe_info->digest_len); //----------------lhg
		      	for(i=0;i<2;i++)
	{
		printk("%llx",be64_to_cpu(*(long long*)&hash[i*8]));
		
	}*/
			printk("\nthis is in search addr\n");
			cur->ref--;
			dedupe_info->logical_blk_cnt--;
			dedupe_info->last_delete_dedupe = cur;
			set_dedupe_dirty(dedupe_info, cur);
			if(0 == cur->ref)
			{
#ifdef F2FS_BLOOM_FILTER
				int i;
				unsigned int *pos = (unsigned int *)cur->hash;
				for(i=0;i<dedupe_info->bloom_filter_hash_fun_count;i++)
				{
					dedupe_info->bloom_filter[*(pos++)&dedupe_info->bloom_filter_mask]--;
				}
#endif
				cur->addr = 0;
				dedupe_info->physical_blk_cnt--;
				//return 0;
				return cur;
			}
			else
			{
				//return cur->ref;
				return cur;
			}
		}
	}
	for(cur = dedupe_info->dedupe_md; cur < c; cur++)
	{
		if(unlikely(cur->ref && addr == cur->addr))
		{
#ifdef F2FS_REVERSE_ADDR
aa:
#endif
			cur->ref--;
			dedupe_info->logical_blk_cnt--;
			dedupe_info->last_delete_dedupe = cur;
			set_dedupe_dirty(dedupe_info, cur);
			if(0 == cur->ref)
			{
#ifdef F2FS_BLOOM_FILTER
				int i;
				unsigned int *pos = (unsigned int *)cur->hash;
				for(i=0;i<dedupe_info->bloom_filter_hash_fun_count;i++)
				{
					dedupe_info->bloom_filter[*(pos++)&dedupe_info->bloom_filter_mask]--;
				}
#endif
				cur->addr = 0;
				dedupe_info->physical_blk_cnt--;

#ifdef F2FS_REVERSE_ADDR
				dedupe_info->reverse_addr[addr] = -1;
#endif
				//return 0;
				return cur;
			}
			else
			{
				//return cur->ref;
				return cur;
			}
		}
	}
	//return -1;
	return NULL;
}


int f2fs_dedupe_add(u8 hash[], struct dedupe_info *dedupe_info, block_t addr)
{
	int ret = 0;
	int search_count = 0;
	struct dedupe* cur = &dedupe_info->dedupe_md[(*(unsigned int *)hash)%(dedupe_info->dedupe_block_count/64) * DEDUPE_PER_BLOCK* 64];
#ifdef F2FS_NO_HASH
	cur = dedupe_info->dedupe_md;
#endif
	while(cur->ref)
	{
		if(likely(cur != dedupe_info->dedupe_md + dedupe_info->dedupe_block_count * DEDUPE_PER_BLOCK - 1))
		{
			cur++;
		}
		else
		{
			cur = dedupe_info->dedupe_md;
		}
		search_count++;
		if(search_count>dedupe_info->dedupe_block_count * DEDUPE_PER_BLOCK)
		{
			printk("can not add f2fs dedupe md.\n");
			ret = -1;
			break;
		}
	}
	if(0 == ret)
	{
#ifdef F2FS_BLOOM_FILTER
		unsigned int *pos;
		int i;
#endif
		cur->addr = addr;
		cur->ref = 1;
		memcpy(cur->hash, hash, dedupe_info->digest_len);
#ifdef F2FS_REVERSE_ADDR
		dedupe_info->reverse_addr[addr] = cur - dedupe_info->dedupe_md;
#endif
#ifdef F2FS_BLOOM_FILTER
		pos = (unsigned int *)cur->hash;
		for(i=0;i<dedupe_info->bloom_filter_hash_fun_count;i++)
		{
			dedupe_info->bloom_filter[*(pos++)&dedupe_info->bloom_filter_mask]++;
			//printk("add %d\n", *(pos++)&dedupe_info->bloom_filter_mask);
		}
#endif
		set_dedupe_dirty(dedupe_info, cur);
		dedupe_info->logical_blk_cnt++;
		dedupe_info->physical_blk_cnt++;
	}
	return ret;
}

int f2fs_dedupe_O_log2(unsigned int x)
{
  unsigned char log_2[256] = {
    0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
  };
  int l = -1;
  while (x >= 256) { l += 8; x >>= 8; }
  return l + log_2[x];
}

int init_dedupe_info(struct dedupe_info *dedupe_info)
{
	int ret = 0;
	dedupe_info->digest_len = 16;
	spin_lock_init(&dedupe_info->lock);
	INIT_LIST_HEAD(&dedupe_info->queue);
	dedupe_info->dedupe_md = vmalloc(dedupe_info->dedupe_size);
	memset(dedupe_info->dedupe_md, 0, dedupe_info->dedupe_size);
	dedupe_info->dedupe_md_dirty_bitmap = kzalloc(dedupe_info->dedupe_bitmap_size, GFP_KERNEL);
	dedupe_info->dedupe_segment_count = DEDUPE_SEGMENT_COUNT;
#ifdef F2FS_BLOOM_FILTER
	dedupe_info->bloom_filter_mask = (1<<(f2fs_dedupe_O_log2(dedupe_info->dedupe_block_count) + 10)) -1;
	dedupe_info->bloom_filter = vmalloc((dedupe_info->bloom_filter_mask + 1) * sizeof(unsigned int));
	memset(dedupe_info->bloom_filter, 0, dedupe_info->bloom_filter_mask * sizeof(unsigned int));
	dedupe_info->bloom_filter_hash_fun_count = 4;
#endif

	dedupe_info->last_delete_dedupe = dedupe_info->dedupe_md;
	dedupe_info->tfm = crypto_alloc_shash("md5", 0, 0);
	dedupe_info->crypto_shash_descsize = crypto_shash_descsize(dedupe_info->tfm);
	dedupe_info->pty=vmalloc(10*1024*1024);  //-------------------lhg
	//f2fs_init_parity_table(dedupe_info->pty);//---------------lhg
	memset(dedupe_info->pty,0,PARITY_NUM*sizeof(struct parity_table));
	return ret;
}

void exit_dedupe_info(struct dedupe_info *dedupe_info)
{
    vfree(dedupe_info->pty);  //---------------------lhg
	vfree(dedupe_info->dedupe_md);
	kfree(dedupe_info->dedupe_md_dirty_bitmap);
	kfree(dedupe_info->dedupe_bitmap);
#ifdef F2FS_REVERSE_ADDR
	vfree(dedupe_info->reverse_addr);
#endif
	crypto_free_shash(dedupe_info->tfm);
#ifdef F2FS_BLOOM_FILTER
	vfree(dedupe_info->bloom_filter);
#endif
}


//-----------------------------------lhg-------------------------------------//
int f2fs_init_parity_table(struct dedupe_info *dedupe_info)
{
   
   struct parity_table *tmp;
   for(tmp=dedupe_info->pty;tmp<dedupe_info->pty+((sizeof(struct parity_table))*PARITY_NUM);tmp++)
   	{
   	   //tmp->hash=NULL;
       tmp->blkaddr1=0;
	   tmp->blkaddr2=0;
	   tmp->flag=0;
   }
   return 0;
}

int f2fs_parity_insert1( u8 hash[], block_t blkaddr,struct dedupe_info *dedupe_info)
{
	struct parity_table *tmp;
	int i;
   // printk("insert_hash_start\n");
	//allocate dhe's mem,init dhe;
		
		for(tmp=dedupe_info->pty;tmp<dedupe_info->pty+PARITY_NUM*sizeof(struct parity_table);tmp++)
			{
			  if(tmp->flag!=1)
			      {
			      tmp->flag=1;
                   tmp->blkaddr1=blkaddr;
                 memcpy(tmp->hash,hash,dedupe_info->digest_len);
				 break;
				 	}
			}
	for(i=0;i<2;i++)
	{
		printk("%llx",be64_to_cpu(*(long long*)&hash[i*8]));
	}
	printk("\n");
	for(i=0;i<2;i++)
	{
		printk("%llx",be64_to_cpu(*(long long*)&tmp->hash[i*8]));
	}
	printk("\n");
    printk("blkadrr1 is %u\n",tmp->blkaddr1);
	return 0;
}

int f2fs_parity_insert2(u8 hash[],block_t blkaddr,struct dedupe_info *dedupe_info)
{
	struct parity_table *tmp;
	//tmp=NULL;
    tmp=f2fs_search_ptye_hash(hash,dedupe_info);
	if(NULL==tmp)
		{printk("**********meiyouchazhaodao***********");
	     printk("ptye->blkaddr2=%u     ",blkaddr);
            return 1;
	     }
	
	if(tmp->flag==1)
	{tmp->blkaddr2=blkaddr;
	printk("ptye->blkaddr1=%u     ",tmp->blkaddr1);
	printk("ptye->blkaddr2=%u     ",tmp->blkaddr2);}
		
	return 0;
}

struct parity_table *f2fs_search_ptye_hash(u8 hash[],struct dedupe_info *dedupe_info)
{
      struct parity_table  *tmp;
	  
	 for(tmp=dedupe_info->pty;tmp<dedupe_info->pty+PARITY_NUM*sizeof(struct parity_table);tmp++)
	 	{
           if(tmp->flag==1 && !memcmp(hash,tmp->hash,dedupe_info->digest_len))
		   	  return tmp;
		    }
	  return NULL;
}

//block_t *f2fs_delete_parity1(u8 hash[],struct dedupe_info *dedupe_info)

//-----------------------------------lhg---------------------xia----------------//


