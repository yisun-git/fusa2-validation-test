/*
 * Test for x86 cpumode 
 *
 * Copyright (c) intel, 2020
 *
 * Authors:
 *  wenwumax <wenwux.ma@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "msr.h"
#include "vmalloc.h"
#include "alloc.h"


/**
 * @brief Case name: CPU Mode of Operation Forbidden switch back to real address mode_001.
 *
 * Summary: When a vCPU attempts to write CR0.PE, the old CR0.PE is 1 and the new CR0.PE is 0,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0). 
 *
 */
void cpumode_rqmid_28141_CPU_Mode_of_Operation_Forbidden_switch_back_to_real_address_mode_001(void)
{
	u32 cr0 = read_cr0();
	if ((cr0 & X86_CR0_PE) && (cr0 & X86_CR0_PG))
	{
		u32 cur_cr0 = cr0 & ~X86_CR0_PE;

		asm volatile(ASM_TRY("1f")
			"mov %0, %%cr0\n\t"
			"1:"
			::"r"(cur_cr0):"memory");
	}
	else
	{
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	report("\t\t %s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);

	return;
}


static u32 bp_eax_ia32_efer = 0xffffffff;
void read_bp_startup(void)
{
	//ia32_efer
	asm ("mov (0x8100) ,%%eax\n\t"
		"mov %%eax, %0\n\t"
		: "=m"(bp_eax_ia32_efer));
}


/**
 * @brief Case name: CPU Mode of Operation_INITIAL GUEST IA32 EFER LME FOLLOWING STARTUP_001.
 *
 * Summary: ACRN hypervisor shall set initial guest IA32_EFER.LME [bit 8] to 0H following startup. 
 *
 */
void cpumode_rqmid_27815_CPU_Mode_of_Operation_INITIAL_GUEST_IA32_EFER_LME_FOLLOWING_STARTUP_001(void)
{
	uint32_t efer = 0;

	read_bp_startup();
	efer = bp_eax_ia32_efer;
	report("\t\t %s", (((efer >> 8) & 0x1) == 0), __FUNCTION__);

	return;
}

static u32 ap_eax_cr0 = 0xffffffff;
void read_ap_init(void)
{
	//read cr0
	asm ("mov (0x8200) ,%%eax\n\t"
		"mov %%eax, %0\n\t"
		: "=m"(ap_eax_cr0));
}

/**
 * @brief Case name: CPU Mode of Operation INITIAL GUEST MODE FOLLOWING INIT_001.
 *
 * Summary: ACRN hypervisor shall set initial guest CR0.PE to 0H following INIT. 
 *
 */

void cpumode_rqmid_27841_CPU_Mode_of_Operation_INITIAL_GUEST_MODE_FOLLOWING_INIT_001(void)
{
	read_ap_init();
	u32 cr0 = ap_eax_cr0;

	report("\t\t %s", (cr0 & 0x01) == 0, __FUNCTION__);
}


int main(int ac, char **av)
{
	setup_idt();

	cpumode_rqmid_28141_CPU_Mode_of_Operation_Forbidden_switch_back_to_real_address_mode_001();
	cpumode_rqmid_27815_CPU_Mode_of_Operation_INITIAL_GUEST_IA32_EFER_LME_FOLLOWING_STARTUP_001();
	cpumode_rqmid_27841_CPU_Mode_of_Operation_INITIAL_GUEST_MODE_FOLLOWING_INIT_001();

	return report_summary();
}
