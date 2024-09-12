/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * Author:              yanting.jiang@intel.com
 * Date :                       2024/09/06
 * Description:         Test case/code for FuSa SRS LBR
 *
 */

#include "processor.h"
#include "libcflat.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "delay.h"

#include "asm/io.h"
#include "fwcfg.h"
#include "alloc_page.h"

#include "register_op.h"
#include "instruction_common.h"

#include "debug_print.h"


#define IA32_LBR_CTL			0x000014CEU
#define IA32_LBR_DEPTH			0x000014CFU
#define IA32_LBR_0_FROM_IP 		0x00001500U
#define IA32_LBR_0_TO_IP 		0x00001600U
#define IA32_LBR_0_INFO 		0x00001200U

#define LBR_MAX_DEPTH			0x20

#define IA32_LER_FROM_IP		0x000001DDU
#define IA32_LER_TO_IP			0x000001DEU
#define IA32_LER_INFO			0x000001E0U

struct module_case {
	uint32_t case_id;
	const char *case_name;
};


#ifdef __x86_64__


/**
 * @brief Case Name: lbr_cpuid
 *
 * ACRN-10834: ACRN hypervisor shall guarantee that the architectural LBR feature is hidden from vCPU.
 *
 * Summary: Execute CPUID.(EAX=7H, ECX=0H), the value of EDX[19] shall be 0H.
 * Execute CPUID.1CH, the value of EAX, EBX, ECX and EDX shall all be 0H.
 */
static void lbr_acrn_t13751_lbr_cpuid()
{
	u8 chk = 0;
	struct cpuid r = {0};
	
	/* CPUID.(EAX=7H, ECX=0H):EDX.LBR[19] = 0 */
	r = cpuid_indexed(0x7, 0x0);
	if ((r.d & (1UL << 19)) == 0) {
		chk++;
	}

	/* CPUID.1CH = 0 */
	r = cpuid(0x1c);
	if ((r.a == 0) && (r.b == 0) && (r.c == 0) && (r.d == 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

uint64_t ytval = 0;
/**
 * @brief Case Name: IA32_LBR_CTL_access
 *
 * ACRN-10834: ACRN hypervisor shall guarantee that the architectural LBR feature is hidden from vCPU.
 *
 * Summary: Read MSR IA32_LBR_CTL shall generate #GP(0), write MSR IA32_LBR_CTL with 1H shall generate #GP(0).
 */
static void lbr_acrn_t13752_ia32_lbr_ctl_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_LBR_CTL, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_LBR_CTL, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_LBR_DEPTH_access
 *
 * ACRN-10834: ACRN hypervisor shall guarantee that the architectural LBR feature is hidden from vCPU.
 *
 * Summary: Read MSR IA32_LBR_DEPTH shall generate #GP(0),
 * write MSR IA32_LBR_DEPTH with 1H shall generate #GP(0).
 */
static void lbr_acrn_t13753_ia32_lbr_depth_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_LBR_DEPTH, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_LBR_DEPTH, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_LBR_x_FROM_IP_access
 *
 * ACRN-10834: ACRN hypervisor shall guarantee that the architectural LBR feature is hidden from vCPU.
 *
 * Summary: Read MSR IA32_LBR_x_FROM_IP shall generate #GP(0),
 * write MSR IA32_LBR_x_FROM_IP with 1H shall generate #GP(0).
 */
static void lbr_acrn_t13754_ia32_lbr_x_from_ip_access()
{
	int i = 0;
	uint32_t msr;

	for (int i = 0; i < LBR_MAX_DEPTH; i++) {
		msr = IA32_LBR_0_FROM_IP + i;
		if (!test_rdmsr_exception(msr, GP_VECTOR, 0)) {
			break;
		}
		if (!test_wrmsr_exception(msr, 1, GP_VECTOR, 0)) {
			break;
		}
	}

	report("%s", (i == LBR_MAX_DEPTH), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_LBR_x_TO_IP_access
 *
 * ACRN-10834: ACRN hypervisor shall guarantee that the architectural LBR feature is hidden from vCPU.
 *
 * Summary: Read MSR IA32_LBR_x_TO_IP shall generate #GP(0),
 * write MSR IA32_LBR_x_TO_IP with 1H shall generate #GP(0).
 */
static void lbr_acrn_t13755_ia32_lbr_x_to_ip_access()
{
	int i = 0;
	uint32_t msr;

	for (int i = 0; i < LBR_MAX_DEPTH; i++) {
		msr = IA32_LBR_0_TO_IP + i;
		if (!test_rdmsr_exception(msr, GP_VECTOR, 0)) {
			break;
		}
		if (!test_wrmsr_exception(msr, 1, GP_VECTOR, 0)) {
			break;
		}
	}

	report("%s", (i == LBR_MAX_DEPTH), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_LBR_x_INFO_access
 *
 * ACRN-10834: ACRN hypervisor shall guarantee that the architectural LBR feature is hidden from vCPU.
 *
 * Summary: Read MSR IA32_LBR_x_INFO shall generate #GP(0),
 * write MSR IA32_LBR_x_INFO with 1H shall generate #GP(0).
 */
static void lbr_acrn_t13756_ia32_lbr_x_info_access()
{
	int i = 0;
	uint32_t msr;

	for (int i = 0; i < LBR_MAX_DEPTH; i++) {
		msr = IA32_LBR_0_INFO + i;
		if (!test_rdmsr_exception(msr, GP_VECTOR, 0)) {
			break;
		}
		if (!test_wrmsr_exception(msr, 1, GP_VECTOR, 0)) {
			break;
		}
	}

	report("%s", (i == LBR_MAX_DEPTH), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_LER_FROM_IP_access
 *
 * ACRN-10834: ACRN hypervisor shall guarantee that the architectural LBR feature is hidden from vCPU.
 *
 * Summary: Read MSR IA32_LER_FROM_IP shall generate #GP(0),
 * write MSR IA32_LER_FROM_IP with 1H shall generate #GP(0).
 */
static void lbr_acrn_t13757_ia32_ler_from_ip_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_LER_FROM_IP, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_LER_FROM_IP, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_LER_TO_IP_access
 *
 * ACRN-10834: ACRN hypervisor shall guarantee that the architectural LBR feature is hidden from vCPU.
 *
 * Summary: Read MSR IA32_LER_TO_IP shall generate #GP(0),
 * write MSR IA32_LER_TO_IP with 1H shall generate #GP(0).
 */
static void lbr_acrn_t13758_ia32_ler_to_ip_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_LER_TO_IP, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_LER_TO_IP, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_LER_INFO_access
 *
 * ACRN-10834: ACRN hypervisor shall guarantee that the architectural LBR feature is hidden from vCPU.
 *
 * Summary: Read MSR IA32_LER_INFO shall generate #GP(0),
 * write MSR IA32_LER_INFO with 1H shall generate #GP(0).
 */
static void lbr_acrn_t13759_ia32_ler_info_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_LER_INFO, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_LER_INFO, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

#endif

#ifdef __x86_64__

#define NUM_VM_CASES		9U
struct module_case vm_cases[NUM_VM_CASES] = {
	{13751u, "lbr_cpuid"},
	{13752u, "IA32_LBR_CTL_access"},
	{13753u, "IA32_LBR_DEPTH_access"},
	{13754u, "IA32_LBR_x_FROM_IP_access"},
	{13755u, "IA32_LBR_x_TO_IP_access"},
	{13756u, "IA32_LBR_x_INFO_access"},
	{13757u, "IA32_LER_FROM_IP_access"},
	{13758u, "IA32_LER_TO_IP_access"},
	{13759u, "IA32_LER_INFO_access"},
};

#elif __i386__

#define NUM_VM_CASES		0U
struct module_case vm_cases[NUM_VM_CASES] = {};

#endif

static void print_case_list(void)
{
	__unused uint32_t i;

	printf("\t\t LBR feature case list:\n\r");

	for (i = 0U; i < NUM_VM_CASES; i++) {
		printf("\t\t Case ID:%d Case Name::%s\n\r",
			vm_cases[i].case_id, vm_cases[i].case_name);
	}

	printf("\t\t \n\r \n\r");
}

int main(void)
{
	setup_idt();
	setup_vm();
	setup_ring_env();

	print_case_list();

#ifdef __x86_64__
	lbr_acrn_t13751_lbr_cpuid();
	lbr_acrn_t13752_ia32_lbr_ctl_access();
	lbr_acrn_t13753_ia32_lbr_depth_access();
	lbr_acrn_t13754_ia32_lbr_x_from_ip_access();
	lbr_acrn_t13755_ia32_lbr_x_to_ip_access();
	lbr_acrn_t13756_ia32_lbr_x_info_access();
	lbr_acrn_t13757_ia32_ler_from_ip_access();
	lbr_acrn_t13758_ia32_ler_to_ip_access();
	lbr_acrn_t13759_ia32_ler_info_access();
#endif

	return report_summary();
}