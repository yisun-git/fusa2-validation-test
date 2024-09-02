/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * Author:              yanting.jiang@intel.com
 * Date :                       2024/08/30
 * Description:         Test case/code for FuSa SRS user interrupts
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


#define IA32_UINTR_RR			0x00000985U
#define IA32_UINTR_HANDLER		0x00000986U
#define IA32_UINTR_STACKADJUST 	0x00000987U
#define IA32_UINTR_MISC 		0x00000988U
#define IA32_UINTR_PD 			0x00000989U
#define IA32_UINTR_TT			0x0000098AU

struct module_case {
	uint32_t case_id;
	const char *case_name;
};


#ifdef __x86_64__

static int uiret_checking()
{
	asm volatile(ASM_TRY("1f")
		"uiret \n\t"
		"1:":::);
	return exception_vector();
}

static int clui_checking()
{
	asm volatile(ASM_TRY("1f")
		"clui \n\t"
		"1:":::);
	return exception_vector();
}

static int stui_checking()
{
	asm volatile(ASM_TRY("1f")
		"stui \n\t"
		"1:":::);
	return exception_vector();
}

static int testui_checking()
{
	asm volatile(ASM_TRY("1f")
		"testui \n\t"
		"1:":::);
	return exception_vector();
}

static int senduipi_checking()
{
	uint64_t reg = 0;
	asm volatile(ASM_TRY("1f")
		"senduipi %0\n\t"
		"1:" : : "r"(reg) : );
	return exception_vector();
}


/**
 * @brief Case Name: cpuid_eax07_ecx0_edx_bit5
 *
 * ACRN-10684: ACRN hypervisor shall guarantee that the UINTR is hidden for vCPU.
 *
 * Summary: Execute CPUID.(EAX=7H,ECX=0H), the value of EDX[5] shall be 0H.
 */
static void user_interrupts_acrn_t13699_cpuid_eax07_ecx0_edx_bit5()
{
	struct cpuid r = cpuid_indexed(0x7, 0);

	report("%s", ((r.d & (1 << 5)) == 0), __FUNCTION__);
}

/**
 * @brief Case Name: CR4_bit25_access
 *
 * ACRN-10684: ACRN hypervisor shall guarantee that the UINTR is hidden for vCPU.
 *
 * Summary: Read CR4.UINTR[bit 25] shall be 0, write a non-zero value to CR4.UINTR[bit 25] shall generate #GP(0).
 */
static void user_interrupts_acrn_t13700_cr4_bit25_access()
{
	u8 chk = 0;
	uint64_t value = read_cr4();
	if ((value & (1UL << 25)) == 0) {
		chk++;
	}
	value |= (1UL << 25);
	write_cr4_checking(value);
	if ((exception_vector() == GP_VECTOR) && (exception_error_code() == 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: XCR0_bit14_access
 *
 * ACRN-10684: ACRN hypervisor shall guarantee that the UINTR is hidden for vCPU.
 *
 * Summary: With CR4.OSXSAVE[18] set to 1, read XCR0[14] shall be 0,
 * write a non-zero value to XCR0[14] shall generate #GP.
 */
static void user_interrupts_acrn_t13702_xcr0_bit14_access()
{
	u8 chk = 0;
	uint64_t value = 0;

	/* Set CR4.OSXSAVE[18] to 1 */
	write_cr4_osxsave(1);

	xsave_getbv(0, &value);
	if ((value & (1UL << 14)) == 0) {
		chk++;
	}
	value |= (1UL << 14);
	xsetbv_checking(0, value);
	if (exception_vector() == GP_VECTOR) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_UINTR_RR_access
 *
 * ACRN-10684: ACRN hypervisor shall guarantee that the UINTR is hidden for vCPU.
 *
 * Summary: Read MSR IA32_UINTR_RR shall generate #GP(0), write MSR IA32_UINTR_RR with 1H shall generate #GP(0).
 */
static void user_interrupts_acrn_t13703_ia32_uintr_rr_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_UINTR_RR, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_UINTR_RR, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_UINTR_HANDLER_access
 *
 * ACRN-10684: ACRN hypervisor shall guarantee that the UINTR is hidden for vCPU.
 *
 * Summary: Read MSR IA32_UINTR_HANDLER shall generate #GP(0),
 * write MSR IA32_UINTR_HANDLER with 1H shall generate #GP(0).
 */
static void user_interrupts_acrn_t13704_ia32_uintr_handler_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_UINTR_HANDLER, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_UINTR_HANDLER, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_UINTR_STACKADJUST_access
 *
 * ACRN-10684: ACRN hypervisor shall guarantee that the UINTR is hidden for vCPU.
 *
 * Summary: Read MSR IA32_UINTR_STACKADJUST shall generate #GP(0),
 * write MSR IA32_UINTR_STACKADJUST with 1H shall generate #GP(0).
 */
static void user_interrupts_acrn_t13705_ia32_uintr_stackadjust_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_UINTR_STACKADJUST, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_UINTR_STACKADJUST, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_UINTR_MISC_access
 *
 * ACRN-10684: ACRN hypervisor shall guarantee that the UINTR is hidden for vCPU.
 *
 * Summary: Read MSR IA32_UINTR_MISC shall generate #GP(0), write MSR IA32_UINTR_MISC with 1H shall generate #GP(0).
 */
static void user_interrupts_acrn_t13706_ia32_uintr_misc_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_UINTR_MISC, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_UINTR_MISC, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_UINTR_PD_access
 *
 * ACRN-10684: ACRN hypervisor shall guarantee that the UINTR is hidden for vCPU.
 *
 * Summary: Read MSR IA32_UINTR_PD shall generate #GP(0), write MSR IA32_UINTR_PD with 1H shall generate #GP(0).
 */
static void user_interrupts_acrn_t13707_ia32_uintr_pd_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_UINTR_PD, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_UINTR_PD, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: IA32_UINTR_TT_access
 *
 * ACRN-10684: ACRN hypervisor shall guarantee that the UINTR is hidden for vCPU.
 *
 * Summary: Read MSR IA32_UINTR_TT shall generate #GP(0), write MSR IA32_UINTR_TT with 1H shall generate #GP(0).
 */
static void user_interrupts_acrn_t13708_ia32_uintr_tt_access()
{
	u8 chk = 0;

	if (test_rdmsr_exception(IA32_UINTR_TT, GP_VECTOR, 0)) {
		chk++;
	}
	if (test_wrmsr_exception(IA32_UINTR_TT, 1, GP_VECTOR, 0)) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/**
 * @brief Case Name: UIRET_execution
 *
 * ACRN-10684: ACRN hypervisor shall guarantee that the UINTR is hidden for vCPU.
 *
 * Summary: Execute UIRET instruction shall generate #UD.
 */
static void user_interrupts_acrn_t13709_uiret_execution()
{
	report("%s", (uiret_checking() == UD_VECTOR), __FUNCTION__);
}

/**
 * @brief Case Name: CLUI/STUI/TESTUI_execution
 *
 * ACRN-10684: ACRN hypervisor shall guarantee that the UINTR is hidden for vCPU.
 *
 * Summary: Execute CLUI/STUI/TESTUI instructions shall generate #UD.
 */
static void user_interrupts_acrn_t13710_clui_stui_testui_execution()
{
	report("%s", ((clui_checking() == UD_VECTOR) && (stui_checking() == UD_VECTOR)
		&& (testui_checking() == UD_VECTOR)), __FUNCTION__);
}

/**
 * @brief Case Name: SENDUIPI_execution
 *
 * ACRN-10684: ACRN hypervisor shall guarantee that the UINTR is hidden for vCPU.
 *
 * Summary: Execute SENDUIPI instruction shall generate #UD.
 */
static void user_interrupts_acrn_t13711_senduipi_execution()
{
	report("%s", (senduipi_checking() == UD_VECTOR), __FUNCTION__);
}

#endif

#ifdef __x86_64__

#define NUM_VM_CASES		12U
struct module_case vm_cases[NUM_VM_CASES] = {
	{13699u, "cpuid_eax07_ecx0_edx_bit5"},
	{13700u, "CR4_bit25_access"},
	{13702u, "XCR0_bit14_access"},
	{13703u, "IA32_UINTR_RR_access"},
	{13704u, "IA32_UINTR_HANDLER_access"},
	{13705u, "IA32_UINTR_STACKADJUST_access"},
	{13706u, "IA32_UINTR_MISC_access"},
	{13707u, "IA32_UINTR_PD_access"},
	{13708u, "IA32_UINTR_TT_access"},
	{13709u, "UIRET_execution"},
	{13710u, "CLUI/STUI/TESTUI_execution"},
	{13711u, "SENDUIPI_execution"},

};

#elif __i386__

#define NUM_VM_CASES		0U
struct module_case vm_cases[NUM_VM_CASES] = {};

#endif

static void print_case_list(void)
{
	__unused uint32_t i;

	printf("\t\t User Interrupts feature case list:\n\r");

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
	user_interrupts_acrn_t13699_cpuid_eax07_ecx0_edx_bit5();
	user_interrupts_acrn_t13700_cr4_bit25_access();
	user_interrupts_acrn_t13702_xcr0_bit14_access();
	user_interrupts_acrn_t13703_ia32_uintr_rr_access();
	user_interrupts_acrn_t13704_ia32_uintr_handler_access();
	user_interrupts_acrn_t13705_ia32_uintr_stackadjust_access();
	user_interrupts_acrn_t13706_ia32_uintr_misc_access();
	user_interrupts_acrn_t13707_ia32_uintr_pd_access();
	user_interrupts_acrn_t13708_ia32_uintr_tt_access();
	user_interrupts_acrn_t13709_uiret_execution();
	user_interrupts_acrn_t13710_clui_stui_testui_execution();
	user_interrupts_acrn_t13711_senduipi_execution();
#endif

	return report_summary();
}