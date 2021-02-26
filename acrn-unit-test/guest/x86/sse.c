#include "processor.h"
#include "instruction_common.h"
#include "xsave.h"
#include "libcflat.h"
#include "desc.h"
#include "desc.c"
#include "alloc.h"
#include "alloc.c"
#include "misc.h"
#include "misc.c"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"
#include "xsave.h"
#include "sse.h"
#include "delay.h"

#define SSE_DEBUG
#ifdef SSE_DEBUG
#define sse_debug(fmt, arg...) printf(fmt, ##arg)
#else
#define sse_debug(fmt, arg...)
#endif

#define BIT_SHIFT(n)	(1 << (n))
#define CR4_OSFXSR_BIT_MASK	BIT_SHIFT(9)

enum sse_instuction_e {
	SSE_INS = 0,
	SSE2_INS,
	SSE3_INS,
	SSSE3_INS,
	SSE4_1_INS,
	SSE4_2_INS,
};

typedef struct fxsave_struct {
	u16 fcw;
	u16 fsw;
	u8  ftw;
	u8  revd1;
	u16 fop;
	u32 fip;
	u16 fcs;
	u16 rsvd2;
	u32 fdp;
	u16 fds;
	u16 rsvd3;
	u32 mxcsr;
	u32 mxcsr_mask;
	long double fpregs[8];
	sse_xmm xmm_regs[8];
} __attribute__((packed)) fxsave_non64bit_t;

typedef struct fxsave_64bit {
	u16 fcw;
	u16 fsw;
	u8  ftw;
	u8  revd1;
	u16 fop;
	u32 fip;
	u16 fcs;
	u16 rsvd2;
	u32 fdp;
	u16 fds;
	u16 rsvd3;
	u32 mxcsr;
	u32 mxcsr_mask;
	long double fpregs[8];
	long double xmm_regs[16];
	sse_xmm reserve[3];
	sse_xmm available[3];
} __attribute__((packed)) fxsave_64bit_t;


static volatile uint32_t bp_cr4 = 0x0U;
static volatile uint32_t ap_cr4 = 0x0U;
int ap_start_count = 0;

static volatile int wait_ap = 0;
volatile u32 set_mxcsr1 = 0;
volatile u32 set_mxcsr2 = 0;

volatile long double xmm_unchange_regs1[16];
volatile long double xmm_unchange_regs2[16];
static volatile int cur_case_id = 0;

void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}

#ifdef __i386__
/*
 *
 *This function for i386 compiling, do nothing
 */
void save_unchanged_reg()
{
	asm volatile ("pause");
}
#elif __x86_64__
/*this function is called when macro (AP_UNCHANGED_CHECK) is defined*/
void save_unchanged_reg(void)
{
	asm volatile ("pause");
}
#endif

static bool is_sse_x_support(enum sse_instuction_e ins_type)
{
	bool is = false;
	switch (ins_type) {
	case SSE_INS:
		is = cpuid_sse_to_1();
		break;
	case SSE2_INS:
		is = cpuid_sse2_to_1();
		break;
	case SSE3_INS:
		is = cpuid_sse3_to_1();
		break;
	case SSSE3_INS:
		is = cpuid_ssse3_to_1();
		break;
	case SSE4_1_INS:
		is = cpuid_sse4_1_to_1();
		break;
	case SSE4_2_INS:
		is = cpuid_sse4_2_to_1();
		break;
	default:
		break;
	}
	return is;
}

static bool test_sse_instruction_PF_exception(enum sse_instuction_e ins_type, gp_trigger_func instruction_fun, int size)
{
	bool is_pass = false;
	int cnt = 0;
	gp_trigger_func fun = NULL;
	void *add2 = NULL;
	bool ret = false;
	/*Confirm processor support SSE2 Feature*/
	ret = is_sse_x_support(ins_type);
	if (ret == true) {
		cnt++;
	} else {
		printf("\nWarning:sse_x is not support. \n");
		goto REPORT_END;
	}
	/*Clear Emulation flag*/
	write_cr0(read_cr0() & ~(X86_CR0_EM));
	/*Enable FXSAVE and FXRSTOR feature set*/
	write_cr4(read_cr4() | X86_CR4_OSFXSR);
	add2 = malloc(size);
	if (add2 == NULL) {
		goto REPORT_END;
	}
	memset(add2, 0x00, size);
	/*Modify current PML4's PTE paging table, to create the page fault conditions*/
	set_page_control_bit((void *)add2, PAGE_PTE, PAGE_P_FLAG, 0, true);
	/*Execute the sse instrution  to generate #PF(fault-code) exception.*/
	fun = instruction_fun;
	ret = test_for_exception(PF_VECTOR, fun, add2);
	if (ret == true) {
		cnt++;
	}
	set_page_control_bit((void *)add2, PAGE_PTE, PAGE_P_FLAG, 1, true);
	free(add2);
REPORT_END:
	is_pass = (cnt == 2) ? true : false;
	return is_pass;
}

#ifdef __x86_64__

#ifdef IN_NATIVE
/*
 * @brief case name:Physical SSE support_001
 *
 * Summary:Physical platform: SSE shall be available on the physical platform.
 */
static __unused void sse_rqmid_27859_physical_sse_support_001(void)
{
	bool is_pass = false;
	is_pass = cpuid_sse_to_1();
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:Physical POPCNT instruction support_001
 *
 * Summary:Physical platform: POPCNT instruction shall be available on the physical platform.
 */
static __unused void sse_rqmid_27868_Physical_POPCNT_instruction_support_001(void)
{
	bool is_pass = false;
	uint32_t check_bit = 0U;
	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= (1 << FEATURE_INFORMATION_23);
	is_pass = (check_bit == (1 << FEATURE_INFORMATION_23)) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:Physical SSE2 support_001
 *
 * Summary:Physical platform: SSE2 shall be available on the physical platform.
 */
static __unused void sse_rqmid_36247_physical_sse2_support_001(void)
{
	bool is_pass = false;
	is_pass = cpuid_sse2_to_1();
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:Physical SSE3 support_001
 *
 * Summary:Physical platform: SSE3 shall be available on the physical platform.
 */
static __unused void sse_rqmid_36245_physical_sse3_support_001(void)
{
	bool is_pass = false;
	is_pass = cpuid_sse3_to_1();
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:Physical SSSE3 support_001
 *
 * Summary:Physical platform: SSSE3 shall be available on the physical platform.
 */
static __unused void sse_rqmid_36244_physical_ssse3_support_001(void)
{
	bool is_pass = false;
	is_pass = cpuid_ssse3_to_1();
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:Physical SSE4 support_001
 *
 * Summary:Physical platform: SSE4 shall be available on the physical platform.
 */
static __unused void sse_rqmid_36243_physical_sse4_support_001(void)
{
	bool is_pass = false;
	is_pass = cpuid_sse4_1_to_1();
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:Physical SSE4_2 support_001
 *
 * Summary:Physical platform: SSE4_2 shall be available on the physical platform.
 */
static __unused void sse_rqmid_36242_physical_sse4_2_support_001(void)
{
	bool is_pass = false;
	is_pass = cpuid_sse4_2_to_1();
	report("%s \n", is_pass, __FUNCTION__);
}

#endif

/*
 * @brief case name: SSE CR4.OSFXSR initial state following startup_001
 *
 * Summary: ACRN hypervisor shall set initial guest CR4.OSFXSR to 0 following start up.
 */
static __unused void sse_rqmid_23188_SSE_CR4_OSFXSR_initial_state_following_startup_001(void)
{
	bool  is_pass = false;
	bp_cr4 = *(volatile uint32_t *)STARTUP_CR4_SAVE_ADDR;
	sse_debug("\nDump bp_cr4 = 0x%x\n", bp_cr4);
	is_pass = ((CR4_OSFXSR_BIT_MASK & bp_cr4) == 0);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: SSE MXCSR initial state following start up_001
 *
 * Summary: ACRN hypervisor shall set initial guest MXCSR to 1F80H following start up.
 */
static __unused void sse_rqmid_23207_SSE_MXCSR_initial_state_following_startup_001(void)
{
	volatile fxsave_non64bit_t *sse_startup;
	sse_startup = (fxsave_non64bit_t *)STARTUP_SSE_XSAVE_ADDR;
	report("%s \n", sse_startup->mxcsr == 0x1F80, __FUNCTION__);
}


/*
 * @brief case name: SSE XMM0-XMM15 initial state following start up_001
 *
 * Summary: ACRN hypervisor shall set initial guest XMM0 through XMM15 to 0H following start up.
 */
static __unused void sse_rqmid_23208_SSE_XMM0_XMM15_initial_state_following_startup_001(void)
{
	u8 *ptr;

	volatile fxsave_64bit_t *sse_startup;
	sse_startup = (fxsave_64bit_t *)STARTUP_SSE64BIT_XSAVE_ADDR;

	ptr = (u8 *)sse_startup->xmm_regs;
	for (int i = 0; i < sizeof(sse_startup->xmm_regs); i++) {
		if (*(ptr + i) != 0) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}
	report("%s", 1, __FUNCTION__);
}

#ifdef IN_NON_SAFETY_VM

/*
 * @brief case name: SSE MXCSR initial state following INIT_002
 *
 * Summary: ACRN hypervisor shall set initial guest MXCSR to 0x1F80 folllowing INIT.
 */

static void sse_rqmid_42027_SSE_MXCSR_initial_state_following_INIT_002(void)
{
	volatile fxsave_non64bit_t *sse_save;

	sse_save = (fxsave_non64bit_t *)INIT_SSE_XSAVE_ADDR;
	report("%s ", sse_save->mxcsr == 0x1F80, __FUNCTION__);
}

/*
 * @brief case name: SSE MXCSR initial state following INIT_001
 *
 * Summary: ACRN hypervisor shall keep unchange guest MXCSR folllowing INIT.
 */

static void sse_rqmid_23197_SSE_MXCSR_unchanged_following_INIT_001(void)
{
	volatile u32 mxcsr1;
	volatile u32 mxcsr2;

	cur_case_id = 23197;/*trigger ap_main function entering switch  23197*/
	wait_ap_ready();
	/*get mxcsr value modified*/
	mxcsr1 = set_mxcsr1;

	/*send sipi to ap again*/
	send_sipi();
	cur_case_id = 23197;
	wait_ap_ready();

	/*get mxcsr value again after ap reset*/
	mxcsr2 = set_mxcsr2;
	/*compare init value with unchanged */
	report("%s ", mxcsr1 == mxcsr2, __FUNCTION__);

	/*send sipi to ap again for restoring mxcsr init value*/
	send_sipi();
	cur_case_id = 23197;
	wait_ap_ready();
	ap_start_count = 0;
}

/*
 * @brief case name: SSE XMM0-XMM15 initial state following INIT
 *
 * Summary: ACRN hypervisor shall keep unchange guest XMM0 through XMM15 folllowing INIT.
 */

static void sse_rqmid_27437_SSE_XMM0_XMM15_unchanged_following_INIT_001(void)
{
	/*HV flush fpu, need init fpu if we use fpu*/
	asm volatile("fninit");
	cur_case_id = 27437;/*trigger ap_main function entering switch  27437*/
	wait_ap_ready();
	/*send sipi to ap  trigger ap_main function was called to get XMM0 through XMM15 again.*/
	send_sipi();
	cur_case_id = 27437;
	wait_ap_ready();
	for (int i = 0; i < 16; i++) {
		if (xmm_unchange_regs1[i] != xmm_unchange_regs2[i]) {
			report("%s ", 0, __FUNCTION__);
			return;
		}
	}
	report("%s ", 1, __FUNCTION__);

	/*send sipi to ap again for restoring XMM0 through XMM15 initial value*/
	send_sipi();
	cur_case_id = 27437;
	wait_ap_ready();
	ap_start_count = 0;
}

#endif

/*
 * @brief case name: SSE CR4.OSFXSR initial state following INIT_001
 *
 * Summary: ACRN hypervisor shall set initial guest CR4.OSFXSR to 0 following INIT.
 */
static __unused void sse_rqmid_23189_SSE_CR4_OSFXSR_initial_state_following_INIT_001(void)
{
	bool  is_pass = false;
	ap_cr4 = *(volatile uint32_t *)INIT_CR4_SAVE_ADDR;
	sse_debug("\nDump ap_cr4 = 0x%x\n", ap_cr4);
	is_pass = ((CR4_OSFXSR_BIT_MASK & ap_cr4) == 0);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: SSE instructions support_001
 *
 * Summary: ACRN hypervisor shall expose SSE to any VM in compliance with Chapter 10.1,Vol. 1,
 * SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 * If CPUID.01H:EDX.SSE[bit 25] = 1, SSE extensions are present.
 */
static __unused void sse_rqmid_27797_SSE_instructions_support_001(void)
{
	bool is_pass = false;
	is_pass = cpuid_sse_to_1();
	/*Check processor support SSE Feature*/
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: SSE2 instructions support_001
 *
 * Summary: ACRN hypervisor shall expose SSE to any VM in compliance with Chapter 10.1, Vol. 1,
 * SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 * If CPUID.01H:EDX.SSE2[bit 26] = 1, SSE2 extensions are present.
 */
static __unused void sse_rqmid_27800_SSE2_instructions_support_001(void)
{
	bool is_pass = false;
	is_pass = cpuid_sse2_to_1();
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: ACRN General support_001
 *
 * Summary: ACRN hypervisor shall expose MMX Capability existence to any VM,
 * in compliance with Chapter 9.6, Vol. 1, SDM.
 */
static __unused void sse_rqmid_27527_ACRN_General_support_001(void)
{
	bool is_pass = false;
	is_pass = cpuid_mmx_to_1();
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: SSE2 instructions support_004
 *
 * Summary: ACRN hypervisor shall expose SSE to any VM in compliance
 * with Chapter 10.1, Vol. 1,
 * SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 * Check that the processor supports the CPUID instruction.
 * Bit 21 of the EFLAGS register can be used to check
 * processor’s support the CPUID instruction.
 * Check that the processor supports the SSE and/or SSE2 extensions
 * (true if CPUID.01H:EDX.SSE[bit 25] = 1 and/or PUID.01H:EDX.SSE2[bit 26] = 1).
 */
static __unused void sse_rqmid_27813_SSE2_instructions_support_004(void)
{
	bool is_pass = false;
	unsigned long check_bit = 0UL;
	int cnt = 0;
	/*Get RFLAG.ID to check it.*/
	check_bit = read_rflags();

	/*eflag bit 21:The ability of a program to set or clear this flag indicates support for the CPUID instruction*/
	write_rflags(check_bit ^ (1UL << FEATURE_INFORMATION_21));
	if (check_bit !=  read_rflags()) {
		cnt++;
	} else {
		sse_debug("\nEFLAG.ID[bit21] should be modified \n");
	}

	if (cpuid_sse_to_1() == true) {
		cnt++;
	} else {
		sse_debug("\ncpuid_sse_to_1 false \n");
	}
	if (cpuid_sse2_to_1() == true) {
		cnt++;
	} else {
		sse_debug("\ncpuid_sse2_to_1 false \n");
	}
	is_pass = (cnt == 3) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name: SSE2 instructions support_64 bit Mode CVTPS2DQ #PF_001
 *
 * Summary: ACRN hypervisor shall expose SSE2 to any VM in compliance with
 * Chapter 11.1, Vol. 1, SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 */
static __unused void sse2_cvtps2dq_pf(void *add2)
{
	sse_union add1;
	add1.u[0] = 1;
	add1.u[1] = 2;
	add1.u[2] = 3;
	add1.u[3] = 4;
	asm volatile(ASM_TRY("1f")
							"cvtps2dq %1, %%xmm1\n"
							"1:"
							: "=m" (add1)
							: "m" (*(sse_union *)add2));
}

static __unused void sse_rqmid_30071_SSE2_instructions_support_64_bit_Mode_CVTPS2DQ_PF_001(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE2_INS, sse2_cvtps2dq_pf, sizeof(sse_union));
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE3 instructions support_001
 *
 * Summary:ACRN hypervisor shall expose SSE3 to any VM in compliance with
 * Chapter 12.4.2, Vol. 1.  **Check that the processor supports the SIMD
 * and x87 SSE3 extensions (if CPUID.01H:ECX.SSE3[bit 0] = 1).
 */
static __unused void sse_rqmid_27804_SSE3_instructions_support_001(void)
{
	bool is_pass = false;
	is_pass = cpuid_sse3_to_1();
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE_instructions_support_003.
 *
 * Summary: ACRN hypervisor shall expose SSE to any VM in compliance with
 * Chapter 10.1, Vol. 1, SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 * If CR4.OSFXSR = 0, FXSAVE and FXRSTOR may or may not operate on SSE state;
 * this behavior is implementation dependent. Moreover, SSE instructions
 * cannot be used unless CR4.OSFXSR = 1.
 */
static __unused void cvtps2dq_checking(void *param)
{
	sse_union add1;
	sse_union add2;

	add1.u[0] = 1; add1.u[1] = 2; add1.u[2] = 3; add1.u[3] = 4;
	asm volatile(ASM_TRY("1f")
		     "cvtps2dq %[add2], %%xmm1\n"
		     "1:"
		     : [add1] "=m" (add1)
		     : [add2] "m" (add2.u));
}

static __unused void sse_rqmid_27799_SSE_instructions_support_003(void)
{
	int cnt = 0;
	bool is_pass = false;
	bool ret = false;
	write_cr4(read_cr4() | X86_CR4_OSFXSR);
	write_cr0(read_cr0() & ~(X86_CR0_EM));
	cvtps2dq_checking(NULL);
	if (exception_vector() == NO_EXCEPTION) {
		cnt++;
	}
	write_cr4(read_cr4() & ~X86_CR4_OSFXSR);
	ret = test_for_exception(UD_VECTOR, cvtps2dq_checking, NULL);
	if (ret == true) {
		cnt++;
	}
	is_pass = (cnt == 2) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE4_1 instructions support_001
 *
 * Summary:ACRN hypervisor shall expose SSE4_1 to any VM in compliance with
 * Chapter 12.12.2, Vol. 1, SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 * Check that the processor supports SSE4.1 (if CPUID.01H:ECX.SSE4_1[bit 19] = 1),
 * SSE3 (if CPUID.01H:ECX.SSE3[bit 0] = 1), and SSSE3 (if CPUID.01H:ECX.SSSE3[bit 9] = 1).
 */
static __unused void sse_rqmid_27808_SSE4_1_instructions_support_001(void)
{
	bool is_pass = false;
	bool sse4_1_to_1 = false;
	bool sse3_to_1 = false;
	bool ssse3_to_1 = false;
	sse4_1_to_1 = cpuid_sse4_1_to_1();
	sse3_to_1 = cpuid_sse3_to_1();
	ssse3_to_1 = cpuid_ssse3_to_1();
	is_pass = (sse4_1_to_1 & sse3_to_1 & ssse3_to_1);
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE4_2 POPCNT instruction support_001
 *
 * Summary:ACRN hypervisor exposes POPCNT instruction to VM in compliance
 * with Chapter 12.12.3, Vol. 1, SDM.**Before an application attempts to
 * use the POPCNT instruction, it must check that the processor supports
 * SSE4.2 (if CPUID.01H:ECX.SSE4_2[bit 20] = 1) and POPCNT
 * (if CPUID.01H:ECX.POPCNT[bit 23] = 1).
 */
static __unused void sse_rqmid_27811_SSE4_2_POPCNT_instruction_support_001(void)
{
	int cnt = 0;
	int check_bit = 0;
	bool ret = false;
	bool is_pass = false;
	ret = cpuid_sse4_2_to_1();
	if (ret == true) {
		cnt++;
	}
	check_bit = cpuid_indexed(CPUID_BASIC_INFORMATION_01, EXTENDED_STATE_SUBLEAF_0).c;
	check_bit &= (1U << FEATURE_INFORMATION_23);
	if (check_bit == (1U << FEATURE_INFORMATION_23)) {
		cnt++;
	}
	is_pass = (cnt == 2) ? true : false;
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE instructions support_64 bit Mode RCPPS #PF_002
 *
 * Summary:ACRN hypervisor shall expose SSE to any VM in compliance with Chapter
 * 10.1, Vol. 1, SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 */
static __unused void sse_64bit_Mode_RCPPS_pf(void *add2)
{
	sse_union *padd2 = (sse_union *)add2;
	padd2->u[0] = 1;
	padd2->u[1] = 1;
	padd2->u[3] = 1;
	padd2->u[4] = 1;
	asm volatile(ASM_TRY("1f")
							"rcpps %[add2], %%xmm1\n\t"
							"1:"
							:
							: [add2] "m" (*(sse_union *)add2));
}

static __unused void sse_rqmid_30098_SSE_instructions_support_64bit_Mode_RCPPS_PF_002(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE_INS, sse_64bit_Mode_RCPPS_pf, sizeof(sse_union));
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE2 instructions support_64 bit Mode MOVNTDQ #PF_003
 *
 * Summary:ACRN hypervisor shall expose SSE2 to any VM in compliance with Chapter 11.1,
 * Vol. 1, SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 */
static __unused void sse2_64bit_Mode_MOVNTDQ_pf(void *add2)
{
	asm volatile(ASM_TRY("1f")
							"movntdq %%xmm1, %[add2]\n\t"
							"1:"
							:
							: [add2] "m" (*(sse_union *)add2));
}

static __unused void sse_rqmid_30341_SSE2_instructions_support_64bit_Mode_MOVNTDQ_PF_003(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE2_INS, sse2_64bit_Mode_MOVNTDQ_pf, sizeof(sse_union));
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE3 instructions support_64 bit Mode LDDQU #PF_005
 *
 * Summary:ACRN hypervisor shall expose SSE3 to any VM in compliance with
 * Chapter 12.4.2,Vol. 1, SDM, Chapter 12.8.2, Vol. 1,
 * SDM and Table 13-1, Vol.3, SDM.
 */
static __unused void sse3_64bit_Mode_LDDQU_pf(void *add2)
{
	asm volatile(ASM_TRY("1f")
							"lddqu %[add2], %%xmm1\n\t"
							"1:"
							:
							: [add2] "m" (*(sse_union *)add2));
}

static __unused void sse_rqmid_30305_SSE3_instructions_support_64bit_Mode_LDDQU_PF_005(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE3_INS, sse3_64bit_Mode_LDDQU_pf, sizeof(sse_union));
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE4_1 instructions support_64 bit Mode PMAXUW #PF_006
 *
 * Summary:ACRN hypervisor shall expose SSE4_1 to any VM in compliance with
 * Chapter 12.12.2, Vol. 1, SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 */
static __unused void sse4_1_64bit_Mode_PMAXUW_pf(void *add2)
{
	sse_union *padd2 = add2;
	padd2->u[0] = 1;
	padd2->u[1] = 2;
	padd2->u[2] = 2;
	padd2->u[3] = 1;
	asm volatile(ASM_TRY("1f")
							"pmaxuw %[add2], %%xmm1\n\t"
							"1:"
							:
							: [add2] "m" (*(sse_union *)add2));
}

static __unused void sse_rqmid_30543_SSE4_1_instructions_support_64bit_Mode_PMAXUW_PF_006(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE4_1_INS, sse4_1_64bit_Mode_PMAXUW_pf, sizeof(sse_union));
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE2 instructions support_64 bit Mode PSUBQ #PF_013
 *
 * Summary:ACRN hypervisor shall expose SSE2 to any VM in compliance
 * with Chapter 11.1, Vol. 1, SDM and table 13-1 in chapter 13.1.3
 * Vol.3, SDM.
 */
static __unused void sse2_64bit_mode_PSUBQ_pf(void *add2)
{
	uint64_t *padd2 = add2;
	*padd2 = 1;
	asm volatile(ASM_TRY("1f")
							"psubq %0, %%mm1\n"
							"1:"
							:
							: "m" (*(uint64_t *)add2));
}

static __unused void sse_rqmid_30179_SSE2_instructions_support_64bit_Mode_PSUBQ_PF_013(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE2_INS, sse2_64bit_mode_PSUBQ_pf, sizeof(uint64_t));
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE instructions support_64 bit Mode PMULHUW #PF_011
 *
 * Summary:ACRN hypervisor shall expose SSE to any VM in compliance with
 * Chapter 10.1, Vol. 1, SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 */
static __unused void sse_64bit_mode_PMULHUW_pf(void *add2)
{
	uint64_t *padd2 = add2;
	*padd2 = 1;
	asm volatile(ASM_TRY("1f")
							"pmulhuw %0, %%mm1\n"
							"1:"
							:
							: "m" (*(uint64_t *)add2));
}

static __unused void sse_rqmid_31001_SSE_instructions_support_64bit_Mode_PMULHUW_PF_011(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE_INS, sse_64bit_mode_PMULHUW_pf, sizeof(uint64_t));
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE3 instructions support_64 bit Mode PALIGNR #PF_009
 *
 * Summary:ACRN hypervisor shall expose SSSE3 to any VM in compliance
 * with Chapter 12.7.2,  Vol. 1, SDM and table 13-1 in chapter 13.1.3
 * Vol.3, SDM.
 */
static __unused void ssse3_64bit_mode_PALIGNR_pf(void *add2)
{
	uint64_t *padd2 = add2;
	*padd2 = 1;
	asm volatile(ASM_TRY("1f")
							"palignr $4, %0, %%mm1\n"
							"1:"
							:
							: "m" (*(uint64_t *)add2));
}

static __unused void sse_rqmid_30813_SSE3_instructions_support_64bit_Mode_PALIGNR_PF_009(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSSE3_INS, ssse3_64bit_mode_PALIGNR_pf, sizeof(uint64_t));
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE3 instructions support_64 bit Mode MOVDDUP #PF_008
 *
 * Summary:ACRN hypervisor shall expose SSE3 to any VM in compliance with
 * Chapter 12.4.2, Vol. 1, SDM, Chapter 12.8.2, Vol. 1,
 * SDM and Table 13-1, Vol.3, SDM.
 */
static __unused void sse3_64bit_mode_MOVDDUP_pf(void *add2)
{
	uint64_t *padd2 = add2;
	*padd2 = 1;
	asm volatile(ASM_TRY("1f")
							"movddup %0, %%xmm1\n"
							"1:"
							:
							: "m" (*(uint64_t *)add2));
}

static __unused void sse_rqmid_30637_SSE3_instructions_support_64bit_Mode_MOVDDUP_PF_008(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE3_INS, sse3_64bit_mode_MOVDDUP_pf, sizeof(uint64_t));
	report("%s \n", is_pass, __FUNCTION__);
}

#else
/*
 * @brief case name:SSE instructions support_Protected Mode MINPS #PF_002
 *
 * Summary:ACRN hypervisor shall expose SSE to any VM in compliance with
 * Chapter 10.1, Vol. 1, SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 */
static __unused void sse_proctect_mode_MINPS_pf(void *add2)
{
	sse_union *padd2 = (sse_union *)add2;
	padd2->u[0] = 1;
	padd2->u[1] = 2;
	padd2->u[3] = 1;
	padd2->u[4] = 2;
	asm volatile(ASM_TRY("1f")
							"minps %[add2], %%xmm1\n\t"
							"1:"
							:
							: [add2] "m" (*(sse_union *)add2));
	sse_debug("\n exception_vector = %d \n", exception_vector());
}

static __unused void sse_rqmid_30321_SSE_instructions_support_Protected_Mode_MINPS_PF_002(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE_INS, sse_proctect_mode_MINPS_pf, sizeof(sse_union));
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE instructions support_Protected Mode PINSRW #PF_011
 *
 * Summary:ACRN hypervisor shall expose SSE to any VM in compliance with
 * Chapter 10.1, Vol. 1, SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 */
static __unused void sse_proctect_mode_PINSRW_pf(void *add2)
{
	asm volatile(ASM_TRY("1f")
							"pinsrw $3, %0, %%xmm1\n"
							"1:"
							:
							: "m" (*(uint16_t *)add2));
}

static __unused void sse_rqmid_30567_SSE_instructions_support_Protected_Mode_PINSRW_PF_011(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE_INS, sse_proctect_mode_PINSRW_pf, sizeof(uint16_t));
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE2 instructions support_Protected Mode MOVLPD #PF_003
 *
 * Summary:ACRN hypervisor shall expose SSE2 to any VM in compliance with
 * Chapter 11.1, Vol. 1, SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 */
static __unused void sse2_proctect_mode_MOVLPD_pf(void *add2)
{
	asm volatile(ASM_TRY("1f")
							"movlpd %0, %%xmm1\n"
							"1:"
							:
							: "m" (*(uint64_t *)add2));
}

static __unused void sse_rqmid_30342_SSE2_instructions_support_Protected_Mode_MOVLPD_PF_003(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE2_INS, sse2_proctect_mode_MOVLPD_pf, sizeof(uint64_t));
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE3 instructions support_Protected Mode PMADDUBSW #PF_009
 *
 * Summary:ACRN hypervisor shall expose SSSE3 to any VM in compliance with
 * Chapter 12.7.2,  Vol. 1, SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 */
static __unused void sse3_proctect_mode_PMADDUBSW_pf(void *add2)
{
	uint64_t *padd2 = add2;
	*padd2 = 1;
	asm volatile(ASM_TRY("1f")
							"pmaddubsw %0, %%mm1\n"
							"1:"
							:
							: "m" (*(uint64_t *)add2));
}

static __unused void sse_rqmid_31054_SSE3_instructions_support_Protected_Mode_PMADDUBSW_PF_009(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE3_INS, sse3_proctect_mode_PMADDUBSW_pf, sizeof(uint64_t));
	report("%s \n", is_pass, __FUNCTION__);
}

/*
 * @brief case name:SSE2 instructions support_Protected Mode PSUBQ #PF_013
 *
 * Summary:ACRN hypervisor shall expose SSE2 to any VM in compliance with
 * Chapter 11.1, Vol. 1, SDM and table 13-1 in chapter 13.1.3 Vol.3, SDM.
 */
static __unused void sse2_proctect_mode_PSUBQ_pf(void *add2)
{
	uint64_t *padd2 = add2;
	*padd2 = 1;
	asm volatile(ASM_TRY("1f")
							"psubq %0, %%mm1\n"
							"1:"
							:
							: "m" (*(uint64_t *)add2));
}

static __unused void sse_rqmid_30916_SSE2_instructions_support_Protected_Mode_PSUBQ_PF_013(void)
{
	bool is_pass = false;
	is_pass = test_sse_instruction_PF_exception(SSE2_INS, sse2_proctect_mode_PSUBQ_pf, sizeof(uint64_t));
	report("%s \n", is_pass, __FUNCTION__);
}


#endif

static void sse_get_64bit_xsave_value_for_startup(void)
{
	volatile fxsave_64bit_t *sse_64bit_start;

	cr4_osfxsr_to_1();/* enable cr4.OSFXSR[9] for SSE, refer to sdm vol1 10.5.1.2*/
	sse_64bit_start = (volatile fxsave_64bit_t *)STARTUP_SSE64BIT_XSAVE_ADDR;
	asm volatile("fxsave %0" : "=m"(*sse_64bit_start));
	cr4_osfxsr_to_0();
}

static __unused void print_case_list(void)
{
	printf("\t\t SSE feature case list:\n\r");
#ifdef __x86_64__
#ifdef IN_NATIVE
	printf("\t\t Case ID:%d case name:%s\n\r", 27859u,
	"Physical SSE support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27868u,
	"Physical POPCNT instruction support_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 36247u,
	"Physical SSE2 support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 36245u,
	"Physical SSE3 support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 36244u,
	"Physical SSSE3 support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 36243u,
	"Physical SSE4_1 support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 36242u,
	"Physical SSE4_2 support_001");

#else
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 27797u,
	"SSE instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27800u,
	"SSE2 instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27527u,
	"ACRN General support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 23188u,
	"SSE CR4.OSFXSR initial state following startup_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 23189u,
	"SSE CR4.OSFXSR initial state following INIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27813u,
	"SSE2 instructions support_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 30071u,
	"SSE2 instructions support_64 bit Mode CVTPS2DQ #PF_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27804u,
	"SSE3 instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27799u,
	"SSE_instructions_support_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 27808u,
	"SSE4_1 instructions support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27811u,
	"SSE4_2 POPCNT instruction support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 30098u,
	"SSE instructions support_64 bit Mode RCPPS #PF_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 30341u,
	"SSE2 instructions support_64 bit Mode MOVNTDQ #PF_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 30305u,
	"SSE3 instructions support_64 bit Mode LDDQU #PF_005");
	printf("\t\t Case ID:%d case name:%s\n\r", 30543u,
	"SSE4_1 instructions support_64 bit Mode PMAXUW #PF_006");
	printf("\t\t Case ID:%d case name:%s\n\r", 30179u,
	"SSE2 instructions support_64 bit Mode PSUBQ #PF_013");
	printf("\t\t Case ID:%d case name:%s\n\r", 31001u,
	"SSE instructions support_64 bit Mode PMULHUW #PF_011");
	printf("\t\t Case ID:%d case name:%s\n\r", 30813u,
	"SSE3 instructions support_64 bit Mode PALIGNR #PF_009");
	printf("\t\t Case ID:%d case name:%s\n\r", 30637u,
	"SSE3 instructions support_64 bit Mode MOVDDUP #PF_008");
	printf("\t\t Case ID:%d case name:%s\n\r", 23197u,
	"SSE MXCSR initial state following INIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27437u,
	"SSE XMM0-XMM15 initial state following INIT");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 23207u,
	"SSE MXCSR initial state following start up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 23208u,
	"SSE XMM0-XMM15 initial state following start up_001");

#endif
#else
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 30321u,
	"SSE instructions support_Protected Mode MINPS #PF_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 30567u,
	"SSE instructions support_Protected Mode PINSRW #PF_011");
	printf("\t\t Case ID:%d case name:%s\n\r", 30342u,
	"SSE2 instructions support_Protected Mode MOVLPD #PF_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 31054u,
	"SSE3 instructions support_Protected Mode PMADDUBSW #PF_009");
	printf("\t\t Case ID:%d case name:%s\n\r", 30916u,
	"SSE2 instructions support_Protected Mode PSUBQ #PF_013");
#endif
#endif
}

static void sse_ap_unchanged_case_23197()
{
	volatile fxsave_64bit_t *sse_save;
	ulong cr4;

	cr4 = read_cr4();
	write_cr4(cr4 | (1<<9));/* enable cr4.OSFXSR[9] for SSE. */

	if (ap_start_count == 0) {
		sse_save = (volatile fxsave_64bit_t *)INIT_UNCHANGED_SSE_XSAVE_ADDR;
		asm volatile("fxsave %0" : "=m"(*sse_save));
		/* set new value to sse */
		sse_save->mxcsr = sse_save->mxcsr ^ MXCSR_OE_BIT;/*modify mxcsr's OE bit*/
		/*modified value restore to fpu*/
		asm volatile("fxrstor %0" : : "m"(*sse_save));
		/*save new values to gloabal var for case for  23197  */
		set_mxcsr1 = sse_save->mxcsr;
		ap_start_count++;
		wait_ap = 1;
	} else if (ap_start_count == 1) {
		/*run to here when ap receive SIPI  */
		/*save new values to gloabal var for case  23197 */
		sse_save = (volatile fxsave_64bit_t *)INIT_UNCHANGED_SSE_XSAVE_ADDR;
		asm volatile("fxsave %0" : "=m"(*sse_save));
		set_mxcsr2 = sse_save->mxcsr;
		ap_start_count++;
		wait_ap = 1;
	} else if (ap_start_count == 2) {
		sse_save = (volatile fxsave_64bit_t *)INIT_UNCHANGED_SSE_XSAVE_ADDR;
		asm volatile("fxsave %0" : "=m"(*sse_save));
		sse_save->mxcsr = sse_save->mxcsr ^ MXCSR_OE_BIT;/*mxcsr restore to init value*/
		asm volatile("fxrstor %0" : : "m"(*sse_save));
		wait_ap = 1;
	}
	/*restore cr4's value*/
	write_cr4(cr4);
}

static void sse_ap_unchanged_case_27437()
{
	ulong cr4;
	volatile fxsave_64bit_t *sse_save;
	/*HV flush fpu, need init fpu*/
	asm volatile("fninit");

	/*test only on the ap  2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	cr4 = read_cr4();
	write_cr4(cr4 | (1<<9));/* enable cr4.OSFXSR[9] for SSE. */

	if (ap_start_count == 0) {
		sse_save = (volatile fxsave_64bit_t *)INIT_UNCHANGED_SSE_XSAVE_ADDR;
		asm volatile("fxsave %0" : "=m"(*sse_save));

		/* set new value to sse */
		for (int i = 0; i < 16; i++) {
			sse_save->xmm_regs[i] = sse_save->xmm_regs[i] + 1; /*old value add one*/
		}

		/*modified value restore to fpu*/
		asm volatile("fxrstor %0" : : "m"(*sse_save));
		/*save new values to gloabal var for case for 27437  */
		for (int i = 0; i < 16; i++) {
			xmm_unchange_regs1[i] = sse_save->xmm_regs[i];
		}
		ap_start_count++;
		wait_ap = 1;
	} else if (ap_start_count == 1) {
		/*run to here when ap receive SIPI again */
		/*save new values to gloabal var for case 27437 */
		sse_save = (volatile fxsave_64bit_t *)INIT_UNCHANGED_SSE_XSAVE_ADDR;
		asm volatile("fxsave %0" : "=m"(*sse_save));
		for (int i = 0; i < 16; i++) {
			xmm_unchange_regs2[i] = sse_save->xmm_regs[i];
		}
		ap_start_count++;
		wait_ap = 1;
	} else if (ap_start_count == 2) {
		/*xmm_regs restore to init value*/
		sse_save = (volatile fxsave_64bit_t *)INIT_UNCHANGED_SSE_XSAVE_ADDR;
		asm volatile("fxsave %0" : "=m"(*sse_save));
		for (int i = 0; i < 16; i++) {
			sse_save->xmm_regs[i] = sse_save->xmm_regs[i] - 1;
		}
		asm volatile("fxrstor %0" : : "m"(*sse_save));
		wait_ap = 1;
	}
	/*restore cr4's value*/
	write_cr4(cr4);
}

void ap_main(void)
{
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	while (1) {
		switch (cur_case_id) {
		case 23197:
			sse_ap_unchanged_case_23197();
			cur_case_id = 0;
			break;
		case 27437:
			sse_ap_unchanged_case_27437();
			cur_case_id = 0;
			break;
		default:
			asm volatile ("nop\n\t" :::"memory");
		}
	}
}

int main(void)
{
	setup_idt();
	setup_vm();
	sse_get_64bit_xsave_value_for_startup();
	print_case_list();
#ifdef __x86_64__
#ifdef IN_NATIVE
	sse_rqmid_27859_physical_sse_support_001();
	sse_rqmid_27868_Physical_POPCNT_instruction_support_001();
	sse_rqmid_36247_physical_sse2_support_001();
	sse_rqmid_36245_physical_sse3_support_001();
	sse_rqmid_36244_physical_ssse3_support_001();
	sse_rqmid_36243_physical_sse4_support_001();
	sse_rqmid_36242_physical_sse4_2_support_001();
#else
#ifdef IN_NON_SAFETY_VM
	sse_rqmid_27797_SSE_instructions_support_001();
	sse_rqmid_27800_SSE2_instructions_support_001();
	sse_rqmid_27527_ACRN_General_support_001();
	sse_rqmid_23189_SSE_CR4_OSFXSR_initial_state_following_INIT_001();
	sse_rqmid_27813_SSE2_instructions_support_004();
	sse_rqmid_30071_SSE2_instructions_support_64_bit_Mode_CVTPS2DQ_PF_001();
	sse_rqmid_27804_SSE3_instructions_support_001();
	sse_rqmid_27799_SSE_instructions_support_003();
	sse_rqmid_27808_SSE4_1_instructions_support_001();
	sse_rqmid_27811_SSE4_2_POPCNT_instruction_support_001();
	sse_rqmid_30098_SSE_instructions_support_64bit_Mode_RCPPS_PF_002();
	sse_rqmid_30341_SSE2_instructions_support_64bit_Mode_MOVNTDQ_PF_003();
	sse_rqmid_30305_SSE3_instructions_support_64bit_Mode_LDDQU_PF_005();
	sse_rqmid_30543_SSE4_1_instructions_support_64bit_Mode_PMAXUW_PF_006();
	sse_rqmid_30179_SSE2_instructions_support_64bit_Mode_PSUBQ_PF_013();
	sse_rqmid_31001_SSE_instructions_support_64bit_Mode_PMULHUW_PF_011();
	sse_rqmid_30813_SSE3_instructions_support_64bit_Mode_PALIGNR_PF_009();
	sse_rqmid_30637_SSE3_instructions_support_64bit_Mode_MOVDDUP_PF_008();
	sse_rqmid_42027_SSE_MXCSR_initial_state_following_INIT_002();
	sse_rqmid_23197_SSE_MXCSR_unchanged_following_INIT_001();
	sse_rqmid_27437_SSE_XMM0_XMM15_unchanged_following_INIT_001();
#endif
	sse_rqmid_23207_SSE_MXCSR_initial_state_following_startup_001();
	sse_rqmid_23208_SSE_XMM0_XMM15_initial_state_following_startup_001();
	sse_rqmid_23188_SSE_CR4_OSFXSR_initial_state_following_startup_001();

#endif
#else
#ifdef IN_NON_SAFETY_VM
	sse_rqmid_30321_SSE_instructions_support_Protected_Mode_MINPS_PF_002();
	sse_rqmid_30567_SSE_instructions_support_Protected_Mode_PINSRW_PF_011();
	sse_rqmid_30342_SSE2_instructions_support_Protected_Mode_MOVLPD_PF_003();
	sse_rqmid_31054_SSE3_instructions_support_Protected_Mode_PMADDUBSW_PF_009();
	sse_rqmid_30916_SSE2_instructions_support_Protected_Mode_PSUBQ_PF_013();
#endif
#endif
	return report_summary();
}
