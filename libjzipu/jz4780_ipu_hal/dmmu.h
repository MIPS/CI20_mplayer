/*
 * Copyright (C) 2006-2014 Ingenic Semiconductor CO.,LTD.
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _JZ_DMMU_H_
#define _JZ_DMMU_H_


__BEGIN_DECLS

//#include <hardware/hardware.h>
#include "jz_ipu.h"

#define DMMU_DEV_NAME "/dev/dmmu"
#define DMMU_PAGE_SIZE 4096

#define DMMU_IOCTL_MAGIC 'd'

#define DMMU_GET_PAGE_TABLE_BASE_PHYS		_IOW(DMMU_IOCTL_MAGIC, 0x01, unsigned int)
#define DMMU_GET_BASE_PHYS                  _IOR(DMMU_IOCTL_MAGIC, 0x02, unsigned int)
#define DMMU_MAP_USER_MEM		            _IOWR(DMMU_IOCTL_MAGIC, 0x11, struct dmmu_mem_info)
#define DMMU_UNMAP_USER_MEM		            _IOW(DMMU_IOCTL_MAGIC, 0x12, struct dmmu_mem_info)
#define DMMU_GET_TLB_PHYS                   _IOWR(DMMU_IOCTL_MAGIC, 0x13, struct dmmu_mem_info)
#define DMMU_SET_TABLE_FLAG 	            _IOW(DMMU_IOCTL_MAGIC, 0x34, int)

#define VIDEO_TABLE_FLAGE 1

struct dmmu_mem_info {
	int size;
	int page_count;

	unsigned int paddr;

	void *vaddr;
	void *pages_phys_addr_table;

	unsigned int start_offset;
	unsigned int end_offset;
};

static inline void init_page_count(struct dmmu_mem_info *info)
{
	int page_count;
	unsigned int start;			/* page start */
	unsigned int end;			/* page end */

	start = ((unsigned int)info->vaddr) & (~(DMMU_PAGE_SIZE-1));
	end = ((unsigned int)info->vaddr + (info->size-1)) & (~(DMMU_PAGE_SIZE-1));
	page_count = (end - start)/(DMMU_PAGE_SIZE) + 1;

	info->page_count = page_count;
	info->start_offset = (unsigned int)info->vaddr - start;
	info->end_offset = ((unsigned int)info->vaddr + info->size) - end; 
        //        ALOGD("<----start_offset: %x, end_offset: %x, page_count: %d\n", info->start_offset, info->end_offset, page_count);

	return;
}

static inline int dump_mem_info(struct dmmu_mem_info *mem, char * description)
{
	if (mem == NULL) {
		ALOGD("mem is NULL!\n");
		return -1;
	}
	ALOGD("mem: %p, \t%s", mem, description?description:"");
	ALOGD("\tvaddr= %p,", mem->vaddr);
	ALOGD("\tsize= %d (0x%x)", mem->size, mem->size);
	ALOGD("\tpaddr= 0x%08x", mem->paddr);
	ALOGD("\tpage_count= %d", mem->page_count);
	ALOGD("\tpages_phys_addr_table=%p", mem->pages_phys_addr_table);
	ALOGD("\tstart_offset= %d", mem->start_offset);
	ALOGD("\tend_offset= %d", mem->end_offset);

	return 0;
}

enum REQUIRE_ALLOC_PAGE_TABLE {
	NO_REQUIRED = 0,
	REQUIRED = 1,
};

extern int dmmu_init(void);
extern int dmmu_set_table_flag(void);
extern int dmmu_init_with_set_page_table(unsigned int * phys_addr, int size);
extern int dmmu_deinit(void);
extern int dmmu_get_page_table_base_phys(unsigned int * phys_addr);
extern int dmmu_set_page_table(unsigned int * phys_addr, int size);
extern int dmmu_map_user_memory(struct dmmu_mem_info* mem);
extern int dmmu_unmap_user_memory(struct dmmu_mem_info* mem);

extern int dmmu_map_user_mem(void * vaddr, int size);
extern int dmmu_unmap_user_mem(void * vaddr, int size);

extern int dmmu_match_user_mem_tlb(void * vaddr, int size);

extern int dmmu_get_memory_physical_address(struct dmmu_mem_info* mem);
extern int dmmu_release_memory_physical_address(struct dmmu_mem_info* mem);

__END_DECLS

#endif	/*  _JZ_DMMU_H_ */
