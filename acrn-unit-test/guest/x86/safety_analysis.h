#ifndef __SAFETY_ANALYSIS_H__
#define __SAFETY_ANALYSIS_H__
#include "processor.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "vmalloc.h"
#include "pci_util.h"

#define SAFETY_ANALYSIS_DEBUG
#ifdef SAFETY_ANALYSIS_DEBUG
#define SAFETY_ANALYSIS_DBG(fmt, arg...) printf(fmt, ##arg)
#define DUMP_DATA_U32(str, addr, len)	do {\
	uint32_t ii = 0;\
	uint32_t __len = (len);\
	uint32_t *__addr = (uint32_t *)(addr);\
	printf("\n%s [addr=%p, len=%d]", str, __addr, __len);\
	for (ii = 0; ii < __len; ii++) {\
		if ((ii % 8) == 0) {\
			printf("\n");\
		} \
		printf(" %08x", *(__addr + ii));\
	} \
	printf("\n");\
} while (0)
#else
#define SAFETY_ANALYSIS_DBG(fmt, arg...)
#define DUMP_DATA_U32(str, addr, len)
#endif

#define FILL_DATA_START_0	(0x10000000UL)
#define FILL_DATA_START_1	(0x0UL)
#define FILL_DATA_CRC32_0	(0x6351c057U)
#define FILL_DATA_CRC32_1	(0xc2766ce4U)
#define FILL_DATA_COUNT	(0x40)
#define RSVD_START_MEM (0x1000)
#define RSVE_END_MEM (0x100)  //SP605 card can not read the last 160 bytes, so ignore it

static inline int busy_delay(uint32_t ms)
{
	volatile unsigned int i, j;
	for (i = 0 ; i < ms; i++) {
		for (j = 0; j < 0x40000; j++) {
		}
	}
	return 0;
}

static inline uint64_t gen_start_addr(int index, uint64_t start, uint64_t end, uint64_t count)
{
	assert((count * sizeof(uint32_t)) < ((end - start) / 3));
	switch (index) {
	case 1:
		return (start + end) >> 1;
	case 2:
		return (end -  RSVE_END_MEM) - (count * sizeof(uint32_t));
	case 0:
	default:
		return ((start < RSVD_START_MEM) ? RSVD_START_MEM : start);
	}
}

static inline void dump_mem(uint64_t start, uint64_t end, uint64_t count)
{
	DUMP_DATA_U32("Dump Start Memory:", gen_start_addr(0, start, end, count), count);
	DUMP_DATA_U32("Dump Middle Memory:", gen_start_addr(1, start, end, count), count);
	DUMP_DATA_U32("Dump End Memory:", gen_start_addr(2, start, end, count), count);
}

#ifdef NEED_CRC
static ulong Crc32_ComputeBuf(ulong, const void *, size_t);
static inline ulong calc_crc32(uint64_t start, uint64_t end, uint64_t count)
{
	ulong crc32 = 0;
	crc32 = Crc32_ComputeBuf(crc32, (const void *)gen_start_addr(0, start, end, count), count);
	crc32 = Crc32_ComputeBuf(crc32, (const void *)gen_start_addr(1, start, end, count), count);
	crc32 = Crc32_ComputeBuf(crc32, (const void *)gen_start_addr(2, start, end, count), count);
	return crc32;
}
#endif

static inline void write_mem_data(uint64_t start, uint64_t count, uint64_t data)
{
	uint32_t i = 0U;
	uint32_t *addr = (uint32_t *)start;
	for (i = 0; i < count; i++) {
		*(addr + i) = data + i * sizeof(uint32_t);
	}
}

static inline void write_mem_data_ex(uint64_t start, uint64_t end, uint64_t count, uint64_t data)
{
	uint64_t start1 = gen_start_addr(0, start, end, count);
	uint64_t start2 = gen_start_addr(1, start, end, count);
	uint64_t start3 = gen_start_addr(2, start, end, count);
	uint64_t data2 = data + (count * sizeof(uint32_t));
	write_mem_data(start1, count, data);
	write_mem_data(start2, count, data2);
	write_mem_data(start3, count, data2 + (count * sizeof(uint32_t)));
}

static inline bool check_data_from(uint64_t start, uint64_t count, uint64_t data)
{
	uint32_t i = 0U;
	uint32_t *addr = (uint32_t *)start;
	for (i = 0; i < count; i++) {
		if (*(addr + i) != (data + (i * sizeof(uint32_t)))) {
			return false;
		}
	}
	return true;
}

static inline void read_mem_data(uint64_t src, uint64_t dst, uint64_t count)
{
	uint32_t i = 0U;
	uint32_t *addr = (uint32_t *)dst;
	uint32_t *addr1 = (uint32_t *)src;
	for (i = 0; i < count; i++) {
		*(addr + i) = *(addr1 + i);
	}
}

static inline int sp605_card_detect(void)
{
	uint32_t reg_val = 0U;
	static PCI_MAKE_BDF(sp605, 2, 0x00, 0);//xilinx SP605
	reg_val = pci_pdev_read_cfg(PCI_GET_BDF(sp605), PCI_VENDOR_ID, 4);
	if ((reg_val == 0xFFFFFFFFU) || \
		(reg_val == 0U) || \
		(reg_val == 0xFFFF0000U) || \
		(reg_val == 0xFFFFU)) {
		printf("\r\n\033[41mNot find pci card, bdf=%x:%x.%x\033[0m\n",
		PCI_GET_BDF(sp605).bits.b, PCI_GET_BDF(sp605).bits.d,
		PCI_GET_BDF(sp605).bits.f);
		return -1;
	}
	printf("\r\n\033[42mFind pci card[ID:%08xH], bdf=%x:%x.%x\033[0m\n",
	reg_val, PCI_GET_BDF(sp605).bits.b, PCI_GET_BDF(sp605).bits.d,
	PCI_GET_BDF(sp605).bits.f);
	return 0;
}

#endif
