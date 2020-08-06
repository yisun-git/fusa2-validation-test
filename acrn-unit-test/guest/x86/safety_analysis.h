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
	printf("\n%s: len=0x%x", str, __len);\
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
#define FILL_DATA_CRC32_0	(0x1bf529deU)
#define FILL_DATA_CRC32_1	(0x89b56dd4U)

static inline int busy_delay(uint32_t ms)
{
	volatile unsigned int i, j;
	for (i = 0 ; i < ms; i++) {
		for (j = 0; j < 0x40000; j++) {
		}
	}
	return 0;
}

static inline void write_mem_data(uint64_t start, uint64_t length, uint64_t data)
{
	uint32_t i = 0U;
	uint32_t *addr = (uint32_t *)start;
	for (i = 0; i < length; i++) {
		*(addr + i) = data + i * sizeof(uint32_t);
	}
}

static inline bool check_data_from(uint64_t start, uint64_t length, uint64_t data)
{
	uint32_t i = 0U;
	uint32_t *addr = (uint32_t *)start;
	for (i = 0; i < length; i++) {
		if (*(addr + i) != (data + (i * sizeof(uint32_t)))) {
			return false;
		}
	}
	return true;
}

static inline void read_mem_data(uint64_t src, uint64_t dst, uint64_t length)
{
	uint32_t i = 0U;
	uint32_t *addr = (uint32_t *)dst;
	uint32_t *addr1 = (uint32_t *)src;
	for (i = 0; i < length; i++) {
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
		printf("\r\n\033[43mNot find pci card, bdf=%x:%x.%x\033[0m\n",
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
