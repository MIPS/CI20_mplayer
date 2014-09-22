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

#define LOG_TAG "DMMU"
//#define LOG_NDEBUG 0

#include "Log.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "sys/stat.h"
#ifdef HAVE_ANDROID_OS
#include "fcntl.h"
#else
#include "sys/fcntl.h"
#endif	/* BUILD_WITH_ANDROID */
#include "sys/ioctl.h"
#include "sys/mman.h"
#include "fcntl.h"
#include <unistd.h>

//#include <cutils/atomic.h>

//#include <hardware/hardware.h>

#include "dmmu.h"

#define PHYS_ADDR_TABLE_TEST 0
#define PAGE_SIZE (4096)


//#define DMMU_DBG 1


#if 0
#define ENTER()												\
	do {													\
		ALOGW("%03d ENTER %s\n", __LINE__, __PRETTY_FUNCTION__);	\
	} while (0)
#define LEAVE()												\
	do {													\
		ALOGW("%03d LEAVE %s\n", __LINE__, __PRETTY_FUNCTION__);	\
	} while (0)
#define MY_DBG(sss, aaa...)											\
	do {															\
		ALOGW("%03d DEBUG %s, " sss, __LINE__, __PRETTY_FUNCTION__, ##aaa);	\
	} while (0)
#else

#define ENTER()									\
	do {										\
	} while (0)
#define LEAVE()									\
	do {										\
	} while (0)
#define MY_DBG(sss, aaa...)						\
	do {										\
	} while (0)
#endif


static int dmmu_fd = -1;
static volatile int g_dmmu_open_count = 0;


int dmmu_init()
{
	ENTER();

	MY_DBG("DMMU_GET_BASE_PHYS=0x%x, DMMU_MAP_USER_MEM=0x%x, DMMU_UNMAP_USER_MEM=0x%x", 
		   DMMU_GET_BASE_PHYS, DMMU_MAP_USER_MEM, DMMU_UNMAP_USER_MEM);

	if (dmmu_fd < 0) {
		dmmu_fd = open(DMMU_DEV_NAME, O_RDWR);
		if (dmmu_fd < 0) {
			ALOGD("DMMU: can't open device: %s\n", DMMU_DEV_NAME);
			perror("dmmu open failed reason:");
			return -1;
		}
		ALOGD("open %s success, dmmu_fd = %d\n", DMMU_DEV_NAME, dmmu_fd);
	}
	//android_atomic_inc(&g_dmmu_open_count);
	g_dmmu_open_count++;
	MY_DBG("<------g_dmmu_open_count: %d\n", g_dmmu_open_count);

	return 0;
}

int dmmu_deinit()
{
	ENTER();

	if (dmmu_fd < 0) {
		ALOGD("dmmu_fd < 0");
		return -1;
	}

	//android_atomic_dec(&g_dmmu_open_count);
	g_dmmu_open_count--;

	if (g_dmmu_open_count == 0) {
		ALOGD("g_dmmu_open_count is zero!\n");
		close(dmmu_fd); 		/* close fd */
		dmmu_fd = -1;
	}

	return 0;
}

int dmmu_set_table_flag(void)
{
	int ret = 0;
	int flag = VIDEO_TABLE_FLAGE;
	
	if (dmmu_fd < 0) {
		ALOGD("dmmu_fd < 0");
		return -1;
	}

	ret = ioctl(dmmu_fd, DMMU_SET_TABLE_FLAG, &flag);
	if (ret < 0) {
		ALOGD("DMMU_SET_TABLE_FLAG failed!!!");
		return -1;
	}
	return 0;
}

int dmmu_get_page_table_base_phys(unsigned int *phys_addr)
{
	int ret = 0;
	ENTER();

	if (phys_addr == NULL) {
		ALOGD("phys_addr is NULL!\n");
		return -1;
	}

	if (dmmu_fd < 0) {
		ALOGD("dmmu_fd < 0");
		return -1;
	}

	ret = ioctl(dmmu_fd, DMMU_GET_PAGE_TABLE_BASE_PHYS, phys_addr);
	if (ret < 0) {
		ALOGD("dmmu_get_page_table_base_phys_addr ioctl(DMMU_GET_BASE_PHYS) failed, ret=%d\n", ret);
		return -1;
	}

	MY_DBG("table_base phys_addr = 0x%08X", *phys_addr);
	return 0;
}

/* NOTE: page_start and page_end maybe used both by two buffer. */
int dmmu_map_user_mem(void * vaddr, int size)
{
	ENTER();
	int i;

	if (dmmu_fd < 0) {
		ALOGD("dmmu_fd < 0");
		return -1;
	}

        //dmmu_match_user_mem_tlb(vaddr, size);

	struct dmmu_mem_info info;
	info.vaddr = vaddr;
	info.size = size;
	info.paddr = 0;
	info.pages_phys_addr_table = NULL;
	/* page count && offset */
	init_page_count(&info);

	int ret = 0;
	ret = ioctl(dmmu_fd, DMMU_MAP_USER_MEM, &info);
	if (ret < 0) {
		ALOGD("dmmu_map_user_memory ioctl(DMMU_MAP_USER_MEM) failed, ret=%d\n", ret);
		return -1;
	}

	return 0;
}

int dmmu_unmap_user_mem(void * vaddr, int size)
{
	ENTER();

	if (dmmu_fd < 0) {
		ALOGD("dmmu_fd < 0");
		return -1;
	}

	struct dmmu_mem_info info;
	info.vaddr = vaddr;
	info.size = size;

	int ret = 0;
	ret = ioctl(dmmu_fd, DMMU_UNMAP_USER_MEM, &info);
	if (ret < 0) {
		ALOGE("dmmu_unmap_user_memory ioctl(DMMU_UNMAP_USER_MEM) failed, ret=%d\n", ret);
		return -1;
	}
	return 0;
}

/* NOTE: page_start and page_end maybe used both by two buffer. */
int dmmu_get_memory_physical_address(struct dmmu_mem_info * mem)
{
	ENTER();
	int i;

	if (dmmu_fd < 0) {
		ALOGD("dmmu_fd < 0");
		return -1;
	}

	if ( mem == NULL ) {
		ALOGD("mem == NULL");
		return -1;
	}
	if ( mem->pages_phys_addr_table != NULL ) {
		ALOGD("mem->pages_phys_addr_table != NULL");
		return -1;
	}

	mem->paddr = 0;
	/* page count && offset */
	init_page_count(mem);

	/* alloc page table space, pages_table filled by dmmu kernel driver. */
	if ( 1 ) {
		void *pages_phys_addr;
		int page_table_size = mem->page_count * sizeof(int);
		pages_phys_addr = (void *)malloc(page_table_size);
		memset((void*)pages_phys_addr, 0, page_table_size);
		MY_DBG("pages_phys_addr: %p\n", pages_phys_addr);
		mem->pages_phys_addr_table = pages_phys_addr;
	}

	if ( mem->pages_phys_addr_table ) {
		int ret = 0;
		ret = ioctl(dmmu_fd, DMMU_GET_TLB_PHYS, mem);
		if (ret < 0) {
			ALOGD("get dmmu tlb phys addr failed!\n");
			return -1;
		}
		MY_DBG("Map mem phys_addr = 0x%08x", mem->paddr);
	}
	return 0;
}

int dmmu_release_memory_physical_address(struct dmmu_mem_info* mem)
{
	if ( mem->pages_phys_addr_table ) {
	  free(mem->pages_phys_addr_table);
	  mem->pages_phys_addr_table = NULL;
	}
	return 0;
}


/* NOTE: page_start and page_end maybe used both by two buffer. */
int dmmu_map_user_memory(struct dmmu_mem_info* mem)
{
  return dmmu_map_user_mem(mem->vaddr, mem->size);
}

/* NOTE: page_start and page_end maybe used both by two buffer. */
int dmmu_unmap_user_memory(struct dmmu_mem_info* mem)
{
  return dmmu_unmap_user_mem(mem->vaddr, mem->size);
}

int dmmu_match_user_mem_tlb(void * vaddr, int size)
{
    //ALOGD("dmmu_match_user_mem_tlb() // make sure tlb match. vaddr=%p, size=%d", vaddr, size);
    if (vaddr==NULL) 
        return 1;
    volatile unsigned char * pc;
    pc = (unsigned char *)vaddr;
    int pn = size/PAGE_SIZE;
    //ALOGD("pn=%d", pn);
    int pg;
    for(pg=0; pg<(pn + 1); pg++ ) {
        //ALOGD("pg=%d pc=%p", pg, pc);
        volatile unsigned char c = *( volatile unsigned char*)pc;
        pc += PAGE_SIZE;
    }
    return 0;
}
