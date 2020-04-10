
asm(".code16gcc");
#include "rmode_lib.h"

static inline u32 read_cr0(void)
{
    u32 val;
    asm volatile ("mov %%cr0, %0" : "=r"(val) : : "memory");
    return val;
}

/**
 * @brief Case name: CPU Mode of Operation INITIAL GUEST MODE FOLLOWING INIT_001.
 *
 * Summary: ACRN hypervisor shall set initial guest CR0.PE to 0H following INIT. 
 *
 */

void cpumode_rqmid_27841_CPU_Mode_of_Operation_INITIAL_GUEST_MODE_FOLLOWING_INIT_001(void)
{
	u32 cr0 = read_cr0();
	if (cr0 & 0x01)
	{
		report(__FUNCTION__, 0);
		return;
	}

	report(__FUNCTION__, 1);
}

void main()
{
	cpumode_rqmid_27841_CPU_Mode_of_Operation_INITIAL_GUEST_MODE_FOLLOWING_INIT_001();
	report_summary();
}

