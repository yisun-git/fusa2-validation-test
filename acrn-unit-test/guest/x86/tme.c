/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * Author:              yanting.jiang@intel.com
 * Date :                       2024/08/30
 * Description:         Test case/code for FuSa SRS TME
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


#define IA32_TME_CAPABILITY			0x00000981U
#define IA32_TME_ACTIVATE			0x00000982U
#define IA32_TME_EXCLUDE_MASK		0x00000983U
#define IA32_TME_EXCLUDE_BASE 		0x00000984U
#define MK_TME_CORE_ACTIVATE 		0x000009FFU

struct module_case {
	uint32_t case_id;
	const char *case_name;
};


#ifdef __x86_64__

static int pconfig_checking()
{
	asm volatile(ASM_TRY("1f")
		"pconfig \n\t"
		"1:":::);
	return exception_vector();
}


/**
 * @brief Case Name: cpuid_eax07_ecx0_ecx_bit13
 *
 * ACRN-10831: ACRN hypervisor shall guarantee that the TME/TME-MK is hidden from vCPU.
 *
 * Summary: Execute CPUID.(EAX=7H, ECX=0H), the value of ECX[13] shall be 0H.
 */
static void tme_acrn_t13738_cpuid_eax07_ecx0_ecx_bit13()
{
	struct cpuid r = cpuid_indexed(0x7, 0);

	report("%s", ((r.c & (1 << 13)) == 0), __FUNCTION__);
}

/**
 * @brief Case Name: pconfig_cpuid
 *
 * ACRN-10832: ACRN hypervisor shall hide PCONFIG from any vCPU.
 *
 * Summary: Execute CPUID.(EAX=7H, ECX=0H), the value of EDX[18] shall be 0H.
 * Execute CPUID.(EAX=1BH, ECX=0H), the value of EAX, EBX, ECX and EDX shall all be 0H.
 */
static void tme_acrn_t13739_pconfig_cpuid()
{
	u8 chk = 0;
	struct cpuid r = {0};
	
	/* CPUID.(EAX=7H, ECX=0H):EDX.PCONFIG[18] = 0 */
	r = cpuid_indexed(0x7, 0x0);
	if ((r.d & (1UL << 18)) == 0) {
		chk++;
	}

	/* CPUID.(EAX=1BH, ECX=0H) = 0 */
	r = cpuid_indexed(0x1b, 0x0);
	if ((r.a == 0) && (r.b == 0) && (r.c == 0) && (r.d == 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_TME_CAPABILITY_read
 *
 * ACRN-10831: ACRN hypervisor shall guarantee that the TME/TME-MK is hidden from vCPU.
 *
 * Summary: Read MSR IA32_TME_CAPABILITY shall generate #GP(0).
 */
static void tme_acrn_t13740_ia32_tme_capability_read()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_TME_CAPABILITY, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/**
 * @brief Case Name: MK_TME_CORE_ACTIVATE_read
 *
 * ACRN-10831: ACRN hypervisor shall guarantee that the TME/TME-MK is hidden from vCPU.
 *
 * Summary: Read MSR MK_TME_CORE_ACTIVATE shall generate #GP(0).
 */
static void tme_acrn_t13741_mk_tme_core_activate_read()
{
	u8 chk = 0;

	if (test_rdmsr_exception(MK_TME_CORE_ACTIVATE, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_TME_ACTIVATE_access
 *
 * ACRN-10831: ACRN hypervisor shall guarantee that the TME/TME-MK is hidden from vCPU.
 *
 * Summary: Read MSR IA32_TME_ACTIVATE shall generate #GP(0),
 * write MSR IA32_TME_ACTIVATE with 1H shall generate #GP(0).
 */
static void tme_acrn_t13742_ia32_tme_activate_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_TME_ACTIVATE, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_TME_ACTIVATE, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_TME_EXCLUDE_MASK_access
 *
 * ACRN-10831: ACRN hypervisor shall guarantee that the TME/TME-MK is hidden from vCPU.
 *
 * Summary: Read MSR IA32_TME_EXCLUDE_MASK shall generate #GP(0),
 * write MSR IA32_TME_EXCLUDE_MASK with 1H shall generate #GP(0).
 */
static void tme_acrn_t13743_ia32_tme_exclude_mask_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_TME_EXCLUDE_MASK, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_TME_EXCLUDE_MASK, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_TME_EXCLUDE_BASE_access
 *
 * ACRN-10831: ACRN hypervisor shall guarantee that the TME/TME-MK is hidden from vCPU.
 *
 * Summary: Read MSR IA32_TME_EXCLUDE_BASE shall generate #GP(0),
 * write MSR IA32_TME_EXCLUDE_BASE with 1H shall generate #GP(0).
 */
static void tme_acrn_t13744_ia32_tme_exclude_base_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_TME_EXCLUDE_BASE, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_TME_EXCLUDE_BASE, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: PCONFIG_execution
 *
 * ACRN-10832: ACRN hypervisor shall hide PCONFIG from any vCPU.
 *
 * Summary: Execute PCONFIG instruction shall generate #UD.
 */
static void tme_acrn_t13745_pconfig_execution()
{
	report("%s", (pconfig_checking() == UD_VECTOR), __FUNCTION__);
}

#endif

#ifdef __x86_64__

#define NUM_VM_CASES		7U
struct module_case vm_cases[NUM_VM_CASES] = {
	{13738u, "cpuid_eax07_ecx0_ecx_bit13"},
	{13739u, "pconfig_cpuid"},
	{13740u, "IA32_TME_CAPABILITY_read"},
	{13741u, "MK_TME_CORE_ACTIVATE_read"},
	{13743u, "IA32_TME_EXCLUDE_MASK_access"},
	{13744u, "IA32_TME_EXCLUDE_BASE_access"},
	{13745u, "PCONFIG_execution"},

};

#elif __i386__

#define NUM_VM_CASES		0U
struct module_case vm_cases[NUM_VM_CASES] = {};

#endif

static void print_case_list(void)
{
	__unused uint32_t i;

	printf("\t\t TME feature case list:\n\r");

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
	tme_acrn_t13738_cpuid_eax07_ecx0_ecx_bit13();
	tme_acrn_t13739_pconfig_cpuid();
	tme_acrn_t13740_ia32_tme_capability_read();
	tme_acrn_t13741_mk_tme_core_activate_read();
	tme_acrn_t13742_ia32_tme_activate_access();
	tme_acrn_t13743_ia32_tme_exclude_mask_access();
	tme_acrn_t13744_ia32_tme_exclude_base_access();
	tme_acrn_t13745_pconfig_execution();

#endif

	return report_summary();
}