#include "libcflat.h"
#include "desc.h"
#include "processor.h"

/**
 * @brief case name:RSM_unavailability_001
 *
 * Summary: When a vCPU attempts to execute RSM, ACRN hypervisor shall guarantee
 *          that the vCPU receives #UD.
 */
static void smm_rqmid_26882_rsm(void)
{
	asm volatile(ASM_TRY("1f")
		"rsm \n\t"
		"1:":::);

	report("%s", exception_vector() == UD_VECTOR, __FUNCTION__);

}

static void print_case_list(void)
{
	printf("SMM feature case list:\n\r");
	printf("\t Case ID:%d case name:%s\n\r", 26882, "RSM_unavailability_001");
}

int main(void)
{
	setup_vm();
	setup_idt();

	print_case_list();

	smm_rqmid_26882_rsm();

	return report_summary();
}
