/*
 * Test for x86 debugging facilities
 *
 * Copyright (c) Siemens AG, 2014
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "msr.h"
#include "vmalloc.h"
#include "alloc.h"

typedef void (*trigger_func)(void *data);
void cpumode_clear_cr0_pe(void *data)
{
	u32 cr0 = read_cr0();
	if (cr0 & X86_CR0_PE)
	{
		if (cr0 & X86_CR0_PG)
		{
			write_cr0(cr0 & ~X86_CR0_PE);

			return;
		}
	}

	return;
}

/**
 * @brief Case name: CPU Mode of Operation Forbidden switch back to real address mode_001.
 *
 * Summary: When a vCPU attempts to write CR0.PE, the old CR0.PE is 1 and the new CR0.PE is 0, ACRN hypervisor shall guarantee that the vCPU receives #GP(0). 
 *
 */

void cpumode_rqmid_28141_CPU_Mode_of_Operation_Forbidden_switch_back_to_real_address_mode_001(void)
{
	int ret = 0;

	trigger_func func;
	func = cpumode_clear_cr0_pe;

	ret = test_for_exception(GP_VECTOR, func, NULL);

	report("\t\t %s", (ret = true), __FUNCTION__);

	return;
}


#ifndef __x86_64__	/*i386*/
/**
 * @brief Case name: CPU Mode of Operation_INITIAL GUEST IA32 EFER LME FOLLOWING STARTUP_001.
 *
 * Summary: ACRN hypervisor shall set initial guest IA32_EFER.LME [bit 8] to 0H following startup. 
 *
 */
void cpumode_rqmid_27815_CPU_Mode_of_Operation_INITIAL_GUEST_IA32_EFER_LME_FOLLOWING_STARTUP_001(void)
{
	uint64_t efer = 0;
	efer = rdmsr(0xc0000080);
	if (((efer >> 8) & 0x1) == 1)
	{
		report("\t\t %s", 0, __FUNCTION__);
	}
	else
	{
		report("\t\t %s", 1, __FUNCTION__);
	}

	return;
}
#endif

int main(int ac, char **av)
{
#ifndef __x86_64__	/*i386*/
	cpumode_rqmid_27815_CPU_Mode_of_Operation_INITIAL_GUEST_IA32_EFER_LME_FOLLOWING_STARTUP_001();
#endif
	cpumode_rqmid_28141_CPU_Mode_of_Operation_Forbidden_switch_back_to_real_address_mode_001();

	return report_summary();
}
