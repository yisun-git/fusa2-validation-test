#include "libcflat.h"
#include "desc.h"
#include "processor.h"

/**
 * @brief case name:When a vCPU attempts to read CPUID.01H_001
 *
 * Summary: When a vCPU attempts to read CPUID.01H, ACRN hypervisor shall write 1 to guest EDX[28].
 */
static void hyper_rqmid_29599_cpuid(void)
{
	report("%s", (((cpuid(0x1).d) >> 28) & 1) == 1, __FUNCTION__);
}

static void print_case_list(void)
{
	printf("Hyper_Threading feature case list:\n\r");
	printf("\t Case ID:%d case name:%s\n\r", 29599, "When a vCPU attempts to read CPUID.01H_001");
}

int main(void)
{
	setup_vm();
	setup_idt();

	print_case_list();

	hyper_rqmid_29599_cpuid();

	return report_summary();
}
