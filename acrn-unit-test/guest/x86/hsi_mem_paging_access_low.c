/*
 * Test for x86 HSI memory management instructions
 *
 * Copyright (c) 2020 Intel
 *
 * Authors:
 *  HeXiang Li <hexiangx.li@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */
#define USE_DEBUG
#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "paging.h"
#include "vm.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "alloc_phys.h"
#include "alloc.h"
#include "misc.h"
#include "register_op.h"
#include "debug_print.h"
#include "delay.h"

/* native available address is start at 1M, the first 1M address is system ROM */
#define MEM_WR_LOW_ADDR_START     (0x00100000UL)
#define MEM_WR_LOW_ADDR_END       (0x10000000UL)

__unused static bool check_pages_p_and_rw_flag(u64 start_addr, u64 end_addr)
{
	/* Check all address for p r/w flag */
	unsigned long i;
	pteval_t *pte = NULL;
	for (i = start_addr; i < end_addr; i += LARGE_PAGE_SIZE) {
		pte = get_pte(phys_to_virt(read_cr3()), phys_to_virt(i));
		if (pte == NULL) {
			debug_print("address 0x%lx pte is null.", i);
			return false;
		}
		if (((*pte & (1ULL << PAGE_P_FLAG)) == 0) ||
			((*pte & (1ULL << PAGE_WRITE_READ_FLAG)) == 0)) {
			debug_print("address 0x%lx pte:0x%lx p or r/w flag is 0.", i, *pte);
			return false;
		}
	}

	return true;
}

/*
 * @brief case name: HSI_Memory_Management_Features_Paging_Access_Control_Low_Address_001
 *
 * Summary: On native board, test all available RAM addresses(Low address 1~256M) are present and
 * writeable, via checking P flag (bit 0) and R/W flag (bit 1) of page structure entries
 * which reference these RAM addresses.
 * write value to the memory, check value write corrently.
 */
static __unused void hsi_rqmid_40041_memory_management_features_paging_access_control_low_address_001(void)
{
	uint64_t start = MEM_WR_LOW_ADDR_START;
	uint64_t end = MEM_WR_LOW_ADDR_END;
	uint64_t size = 0UL;
	bool value_chk = true;
	bool pte_chk = true;
	u32 i;

	size = end - start;

	/* Checking P flag (bit 0) and R/W flag (bit 1) of page structure entries */
	pte_chk = check_pages_p_and_rw_flag(start, end);

	volatile u32 *start_addr = (volatile u32 *)start;
	size = size / sizeof(u32);
	for (i = 0; i < size; i++) {
		start_addr[i] = i;
	}

	delay(1000);

	for (i = 0; i < size; i++) {
		if (start_addr[i] != i) {
			debug_print("check fail num:0x%x write value:%u actual value:%u .\n",
				i, start_addr[i], i);
			value_chk = false;
			break;
		}
	}

	printf("pte_chk:%d value_chk:%d \n", pte_chk, value_chk);
	report("%s", (value_chk && pte_chk), __FUNCTION__);
}

static __unused void print_case_list(void)
{
	printf("\t\t HSI paging access control feature case list:\n\r");
	printf("\t\t Case ID:%d case name:%s\n\r", 40041u,
	"hsi_rqmid_40041_memory_management_features_paging_access_control_low_address_001");
}

int main(void)
{
	setup_idt();
	setup_vm();
	print_case_list();

	extern idt_entry_t boot_idt[256];

	printf("\n boot_idt=%p %s \n", boot_idt, __FILE__);
#if defined(IN_NATIVE) && defined(__x86_64__)
	hsi_rqmid_40041_memory_management_features_paging_access_control_low_address_001();
#endif

	return report_summary();
}

