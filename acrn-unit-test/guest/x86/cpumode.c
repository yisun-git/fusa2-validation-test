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
#include "xsave.h"
#include "misc.h"
#include "fwcfg.h"
#include "delay.h"
#include "cpumode.h"
#include "segmentation.h"

#define X86_EFLAGS_VM  0x00020000
static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;

__unused void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}

__unused static void notify_modify_and_read_init_value(int case_id)
{
	cur_case_id = case_id;
	need_modify_init_value = 1;
	/* will change INIT value after AP reboot */
	send_sipi();
	wait_ap_ready();
	/* Will check INIT value after AP reboot again */
	send_sipi();
	wait_ap_ready();
}

typedef void (*ap_init_value_modify)(void);
__unused static void ap_init_value_process(ap_init_value_modify modify_init_func)
{
	if (need_modify_init_value) {
		need_modify_init_value = 0;
		modify_init_func();
		wait_ap = 1;
	} else {
		wait_ap = 1;
	}
}

__unused static void modify_cr0_pe_init_init_value()
{
	write_cr0(read_cr0() | X86_CR0_PE);
}

__unused static void modify_efer_lme_init_value()
{
	wrmsr(X86_IA32_EFER, rdmsr(X86_IA32_EFER) | (1ull << 8));
}

__unused static void modify_efer_lma_init_value()
{
	wrmsr(X86_IA32_EFER, rdmsr(X86_IA32_EFER) | (1ull << 10));
}

#ifdef __x86_64__
void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 27814:
		fp = modify_cr0_pe_init_init_value;
		ap_init_value_process(fp);
		break;
	case 27816:
		fp = modify_efer_lme_init_value;
		ap_init_value_process(fp);
		break;
	case 27818:
		fp = modify_efer_lma_init_value;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#else
void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	while (1) {
		switch (cur_case_id) {
		case 27814:
			fp = modify_cr0_pe_init_init_value;
			ap_init_value_process(fp);
			break;
		default:
			asm volatile ("nop\n\t" :::"memory");
		}
	}
}
#endif

#ifdef __x86_64__
/**
 * @brief Case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempt to write CR0_001.
 *
 * Summary: In 64-bit mode, The vCPU receives #GP(0) when a vCPU attempt to clear CR0.PG.
 *
 */
void cpumode_rqmid_37685_receives_GP_when_clear_CR0_PG_001(void)
{
	asm volatile(
		"mov %cr0, %rax\n"
		"btc $31, %rax\n");

	asm volatile(ASM_TRY("1f")
		"mov %%rax, %%cr0\n\t"
		"1:":::);

	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}

/**
 * @brief Case name: Ignore this access when a vCPU attempts to write IA32_EFER_002.
 *
 * Summary: In 64-bit mode, ignore this access when a vCPU attempts to clear IA32_EFER.LMA.
 *
 */
void cpumode_rqmid_37688_Ignore_this_access_when_write_IA32_EFER_LMA_002(void)
{
	asm volatile(
		"movq $0xc0000080, %rcx\n" /* EFER */
		"rdmsr\n"
		"btcq $10, %rax\n" /* clear LMA */
		"wrmsr\n"
		);

	u64 ia32_efer = rdmsr(X86_IA32_EFER);
	report("%s", (ia32_efer & 0x400) != 0, __FUNCTION__);
}

#else

extern u32 pt_root;
u32 old_pt;
void mode_switch_to_compat(void)
{
	/* switch to compatibility */
	asm volatile(
		"movl %%cr0, %%eax\n"
		"btcl $31, %%eax\n" /* clear PG */
		"movl %%eax, %%cr0\n"

		"movl %%cr4, %%eax\n"
		"btsl $5, %%eax\n" /* set PAE */
		"movl %%eax, %%cr4\n"

		"movl %0, %%eax\n" /* set 4 level page table */
		"movl %%eax, %%cr3\n"

		"movl $0xc0000080, %%ecx\n" /* EFER */
		"rdmsr\n"
		"btsl $8, %%eax\n" /* set LME */
		"wrmsr\n"

		"movl %%cr0, %%eax\n"
		"btsl  $31, %%eax\n" /* set PG */
		"movl %%eax, %%cr0\n"

		"ljmpl $0x8, $1f\n"    /* switch to compatibility mode */
		"1:\n"
		:: "m"(pt_root)
		:);
}

void mode_switch_to_protect(void)
{
	/*back to protect*/
	asm volatile(
		"movl %%cr0, %%eax\n"
		"btcl  $31, %%eax\n" /* clear PG */
		"movl %%eax, %%cr0\n"

		"movl %0, %%eax\n" /* set 3 level page table */
		"movl %%eax, %%cr3\n"

		"movl $0xc0000080, %%ecx\n" /* EFER */
		"rdmsr\n"
		"btcl $8, %%eax\n" /* clear LME */
		"wrmsr\n"

		"movl %%cr4, %%eax\n"
		"btcl $5, %%eax\n" /* clear PAE */
		"movl %%eax, %%cr4\n"

		"movl %%cr0, %%eax\n"
		"btsl  $31, %%eax\n" /* set PG */
		"movl %%eax, %%cr0\n"

		"ljmpl $0x8, $1f\n"    /* switch to protect mode */
		"1:\n"
		: : "m"(old_pt)
		:);
}

/**
 * @brief Case name: Guarantee that the IA32_EFER.LMA [bit 10] is 0H when a vCPU attempt to write CR0_001.
 *
 * Summary: In compatibility mode, the IA32_EFER.LMA [bit 10] is 0H when a vCPU attempt to celar CR0.PG.
 *
 */
void cpumode_rqmid_37684_IA32_EFER_LMA_is_0H_when_clear_CR0_PG_001(void)
{
	mode_switch_to_compat();

	u64 ia32_efer = rdmsr(X86_IA32_EFER);
	if ((ia32_efer & 0x400) == 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	u32 cr0 = read_cr0();
	if ((cr0 & 0x80000000) == 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	asm volatile(
		"movl %cr0, %eax\n"
		"btcl  $31, %eax\n" /* clear PG */
		"movl %eax, %cr0\n");

	ia32_efer = rdmsr(X86_IA32_EFER);
	report("%s", (ia32_efer & 0x400) == 0, __FUNCTION__);

	asm volatile(
		"movl %0, %%eax\n" /* set 3 level page table */
		"movl %%eax, %%cr3\n"

		"movl $0xc0000080, %%ecx\n" /* EFER */
		"rdmsr\n"
		"btcl $8, %%eax\n" /* clear LME */
		"wrmsr\n"

		"movl %%cr4, %%eax\n"
		"btcl $5, %%eax\n" /* clear PAE */
		"movl %%eax, %%cr4\n"

		"movl %%cr0, %%eax\n"
		"btsl  $31, %%eax\n" /* set PG */
		"movl %%eax, %%cr0\n"

		"ljmpl $0x8, $1f\n"    /* switch to protect mode */
		"1:\n"
		: : "m"(old_pt)
		:);
}

/**
 * @brief Case name: Ignore this access when a vCPU attempts to write IA32_EFER_001.
 *
 * Summary: In compatibility mode, ignore this access when a vCPU attempts to clear IA32_EFER.LMA.
 *
 */
void cpumode_rqmid_37687_Ignore_this_access_when_write_IA32_EFER_LMA_001(void)
{
	mode_switch_to_compat();

	asm volatile(
		"mov $0xc0000080, %ecx\n" /* EFER */
		"rdmsr\n"
		"btc $10, %eax\n" /* clear LMA */
		"wrmsr\n"
		);

	u64 ia32_efer = rdmsr(X86_IA32_EFER);
	report("%s", (ia32_efer & 0x400) != 0, __FUNCTION__);
	mode_switch_to_protect();
}


/**
 * @brief Case name: Ignore this access when a vCPU attempts to write IA32_EFER_003.
 *
 * Summary: In protect mode, ignore this access when a vCPU attempts to set IA32_EFER.LMA to 0x1.
 *
 */
void cpumode_rqmid_37690_Ignore_this_access_when_write_IA32_EFER_LMA_003(void)
{
	asm volatile(
		"mov $0xc0000080, %ecx\n" /* EFER */
		"rdmsr\n"
		"bts $10, %eax\n" /* set LMA */
		"wrmsr\n"
		);

	u64 ia32_efer = rdmsr(X86_IA32_EFER);
	report("%s", (ia32_efer & 0x400) == 0, __FUNCTION__);
}

#endif

/**
 * @brief Case name: CPU Mode of Operation Forbidden switch back to real address mode_001.
 *
 * Summary: When a vCPU attempts to write CR0.PE, the old CR0.PE is 1 and the new CR0.PE is 0,
 * ACRN hypervisor shall guarantee that the vCPU receives #GP(0). 
 *
 */
void cpumode_rqmid_28141_CPU_Mode_of_Operation_Forbidden_switch_back_to_real_address_mode_001(void)
{
	ulong cr0 = read_cr0();
	if (cr0 & X86_CR0_PE) {
		ulong cur_cr0 = cr0 & ~X86_CR0_PE;

		asm volatile(ASM_TRY("1f")
			"mov %0, %%cr0\n\t"
			"1:"
			::"r"(cur_cr0):"memory");
	} else {
		report("\t\t %s", 0, __FUNCTION__);
		return;
	}

	report("\t\t %s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);

	return;
}


/**
 * @brief Case name: CPU Mode of Operation Initial guest CR0.PE following start-up_001.
 *
 * Summary: dump CR0.PE at  BP start-up, the value shall be 0. 
 *
 */

void cpumode_rqmid_28143_CPU_Mode_of_Operation_INITIAL_GUEST_CR0_PE_STARTUP_001(void)
{
	u32 cr0 = *((u32 *)STARTUP_CR0_ADDR);

	report("\t\t %s", (cr0 & 0x01) == 1, __FUNCTION__);
}

/**
 * @brief Case name: Initial guest EFLAGS.VM [bit 17] following startup_001.
 *
 * Summary: dump EFLAGS.VM at BP start-up, the value shall be 0. 
 *
 */
void cpumode_rqmid_46084_CPU_Mode_of_Operation_INITIAL_EFLAGS_VM_STARTUP_001(void)
{
	u32 eflags = *((u32 *)STARTUP_EFLAGS_ADDR);
	//printf("eflags=0x%x\n", eflags);
	report("\t\t %s", (eflags & X86_EFLAGS_VM) == 0, __FUNCTION__);
}

/**
 * @brief Case name: CPU Mode of Operation_INITIAL GUEST IA32 EFER LMA FOLLOWING STARTUP_001.
 *
 * Summary: dump IA32_EFER.LMA at BP start-up, the value shall be 0. 
 *
 */
void cpumode_rqmid_27817_CPU_Mode_of_Operation_INITIAL_GUEST_IA32_EFER_LMA_FOLLOWING_STARTUP_001(void)
{
	u32 efer = *((u32 *)STARTUP_IA32_EFER_ADDR);

	report("\t\t %s", (((efer >> 10) & 0x1) == 0), __FUNCTION__);

	return;
}

/**
 * @brief Case name: CPU Mode of Operation_INITIAL GUEST IA32 EFER LME FOLLOWING STARTUP_001.
 *
 * Summary: ACRN hypervisor shall set initial guest IA32_EFER.LME [bit 8] to 0H following startup. 
 *
 */
void cpumode_rqmid_27815_CPU_Mode_of_Operation_INITIAL_GUEST_IA32_EFER_LME_FOLLOWING_STARTUP_001(void)
{
	u32 efer = *((u32 *)STARTUP_IA32_EFER_ADDR);

	report("\t\t %s", (((efer >> 8) & 0x1) == 0), __FUNCTION__);

	return;
}

#ifdef IN_NON_SAFETY_VM
/**
 * @brief Case name: CPU Mode of Operation INITIAL GUEST MODE FOLLOWING INIT_001.
 *
 * Summary: ACRN hypervisor shall set initial guest CR0.PE to 0H following INIT. 
 *
 */

void cpumode_rqmid_27814_CPU_Mode_of_Operation_INITIAL_GUEST_MODE_FOLLOWING_INIT_001(void)
{
	u32 cr0 = *((u32 *)INIT_CR0_ADDR);
	bool is_pass = true;

	if ((cr0 & X86_CR0_PE) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(27814);
	cr0 = *((u32 *)INIT_CR0_ADDR);

	if ((cr0 & X86_CR0_PE) != 0) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/**
 * @brief Case name: Initial guest IA32_EFER.LME [bit 8] following INIT_001.
 *
 * Summary: ACRN hypervisor shall set initial guest IA32_EFER.LME [bit 8] to 0H following INIT.
 *
 */

__unused void cpumode_rqmid_27816_INITIAL_GUEST_IA32_EFER_LME_FOLLOWING_INIT_001(void)
{

	bool is_pass = true;
	volatile u64 INIT_IA32_EFER = *(volatile uint64_t *)INIT_IA32_EFER_LOW_ADDR;

	if ((INIT_IA32_EFER & (1ULL << 8)) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(27816);
	INIT_IA32_EFER = *(volatile uint64_t *)INIT_IA32_EFER_LOW_ADDR;
	if ((INIT_IA32_EFER & (1ULL << 8)) != 0) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}


/**
 * @brief Case name: Initial guest IA32_EFER.LMA [bit 10] following INIT_001.
 *
 * Summary: ACRN hypervisor shall set initial guest IA32_EFER.LMA[bit 10] to 0H following INIT.
 *
 */

__unused void cpumode_rqmid_27818_INITIAL_GUEST_IA32_EFER_LMA_FOLLOWING_INIT_001(void)
{

	bool is_pass = true;
	volatile u64 INIT_IA32_EFER = *(volatile uint64_t *)INIT_IA32_EFER_LOW_ADDR;

	if ((INIT_IA32_EFER & (1ULL << 10)) != 0) {
		is_pass = false;
	}

	notify_modify_and_read_init_value(27818);
	INIT_IA32_EFER = *(volatile uint64_t *)INIT_IA32_EFER_LOW_ADDR;
	if ((INIT_IA32_EFER & (1ULL << 10)) != 0) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/**
 * @brief Case name: INITIAL GUEST EFLAGS.VM [bit 17] FOLLOWING INIT_001
 *
 * Summary: dump EFLAGS.VM [bit 17] at AP protected mode, the value shall be 0. 
 *
 */
void cpumode_rqmid_46085_CPU_Mode_of_Operation_INITIAL_EFLAGS_VM_INIT_001(void)
{
	u32 eflags = *((u32 *)INIT_EFLAGS_ADDR);
	//printf("eflags=0x%x\n", eflags);
	report("\t\t %s", (eflags & X86_EFLAGS_VM) == 0, __FUNCTION__);
}
#endif

static void print_case_list(void)
{
	printf("cpumode feature case list:\n\r");

#ifdef __x86_64__
	printf("\t\t Case ID:%d case name:%s\n\r", 37685u, "the vCPU receives #GP(0) when a vCPU write CR0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37688u, "Ignore when a vCPU attempts to write IA32_EFER_002");

	printf("\t\t Case ID:%d case name:%s\n\r", 37716u,
		"Preserve the value of any YMM register when a vCPU attempts to switch to compatibility mode_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37713u,
		"Preserve the value of any XMM register when a vCPU attempts to switch to compatibility mode_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37717u,
		"Preserve the value of any general register when attempts to switch to compatibility mode_001");
#else
	printf("\t\t Case ID:%d case name:%s\n\r", 37684u, "the IA32_EFER.LMA is 0H when a vCPU write CR0_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37687u, "Ignore when a vCPU attempts to write IA32_EFER_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37690u, "Ignore when a vCPU attempts to write IA32_EFER_003");

	printf("\t\t Case ID:%d case name:%s\n\r", 37712u,
		"Preserve the value of any general register when attempts to switch to protected-mode then back_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37714u,
		"Preserve the value of any XMM register when attempts to switch to protected-mode then back_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37715u,
		"Preserve the value of any YMM register when attempts to switch to protected-mode then back_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 40496u,
		"CPU Mode of Operation Exposure of CPU modes of operation_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 40500u,
		"CPU Mode of Operation Exposure of CPU modes of operation_003");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 28141u, "Forbidden switch back to real address mode_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27815u, "INITIAL GUEST IA32 EFER LME FOLLOWING STARTUP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27817u, "INITIAL GUEST IA32 EFER LMA FOLLOWING STARTUP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28143u, "Initial guest CR0.PE following start-up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 46084u, "Initial guest EFLAGS.VM [bit 17] following startup_001");

#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 27814u, "INITIAL GUEST MODE FOLLOWING INIT_001");
#ifdef __x86_64__
	printf("\t\t Case ID:%d case name:%s\n\r", 27816u, " Initial guest IA32_EFER.LME [bit 8] following INIT_001.");
	printf("\t\t Case ID:%d case name:%s\n\r", 27818u, " Initial guest IA32_EFER.LME [bit 8] following INIT_001.");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 46085u, "INITIAL GUEST EFLAGS.VM [bit 17] FOLLOWING INIT_001");
#endif
}

#ifdef __x86_64__
#define AVX_ARRAY_NUM_1		16
#define AVX_ARRAY_NUM_2		8
#define XMM_ARRAY_NUM_1		16
#define XMM_ARRAY_NUM_2		4
#define REG_ARRAY_NUM		16
#else
#define AVX_ARRAY_NUM_1		8
#define AVX_ARRAY_NUM_2		8
#define XMM_ARRAY_NUM_1		8
#define XMM_ARRAY_NUM_2		4
#define REG_ARRAY_NUM		16
#endif

u64 get_xcr0(void)
{
	struct cpuid r;
	r = cpuid_indexed(0xd, 0);
	return r.a + ((u64)r.d << 32);
}

int xsave_setbv(u32 index, u64 value)
{
	u32 eax = value;
	u32 edx = value >> 32;

	asm volatile("xsetbv\n\t" /* xsetbv */
				 : : "a" (eax), "d" (edx), "c" (index));
	return 0;
}

int xsave_getbv(u32 index, u64 *result)
{
	u32 eax, edx;

	asm volatile("xgetbv\n" /* xgetbv */
				 : "=a" (eax), "=d" (edx)
				 : "c" (index));
	*result = eax + ((u64)edx << 32);
	return 0;
}

ALIGNED(32) avx_union avx1[16];
ALIGNED(32) avx_union avx2[16];

void mode_sw_ymm(avx_union avx[])
{
	write_cr0(read_cr0() & ~6);
	write_cr4(read_cr4() | 1 << 18);

	u64 xcr0;
	u64 supported_xcr0;
	supported_xcr0 = get_xcr0();
	/*enable all xsave bitmap 0x3--we support until now!!
	 *MPX component is hidden,so we add it ?
	 */
	xsave_getbv(0, &xcr0);
	xsave_setbv(0, supported_xcr0);

	asm volatile("vmovapd %%ymm0, %0\n\t" : "=m" (avx[0]) : );
	asm volatile("vmovapd %%ymm1, %0\n\t" : "=m" (avx[1]) : );
	asm volatile("vmovapd %%ymm2, %0\n\t" : "=m" (avx[2]) : );
	asm volatile("vmovapd %%ymm3, %0\n\t" : "=m" (avx[3]) : );
	asm volatile("vmovapd %%ymm4, %0\n\t" : "=m" (avx[4]) : );
	asm volatile("vmovapd %%ymm5, %0\n\t" : "=m" (avx[5]) : );
	asm volatile("vmovapd %%ymm6, %0\n\t" : "=m" (avx[6]) : );
	asm volatile("vmovapd %%ymm7, %0\n\t" : "=m" (avx[7]) : );
#ifdef __x86_64__
	asm volatile("vmovapd %%ymm8, %0\n\t" : "=m" (avx[8]) : );
	asm volatile("vmovapd %%ymm9, %0\n\t" : "=m" (avx[9]) : );
	asm volatile("vmovapd %%ymm10, %0\n\t" : "=m" (avx[10]) : );
	asm volatile("vmovapd %%ymm11, %0\n\t" : "=m" (avx[11]) : );
	asm volatile("vmovapd %%ymm12, %0\n\t" : "=m" (avx[12]) : );
	asm volatile("vmovapd %%ymm13, %0\n\t" : "=m" (avx[13]) : );
	asm volatile("vmovapd %%ymm14, %0\n\t" : "=m" (avx[14]) : );
	asm volatile("vmovapd %%ymm15, %0\n\t" : "=m" (avx[15]) : );
#endif
}

bool mode_ymm_diff(void)
{
	for (int i = 0; i < AVX_ARRAY_NUM_1; i++) {
		for (int j = 0; j < AVX_ARRAY_NUM_2; j++) {
			if (avx1[i].avx_u[j] != avx2[i].avx_u[j]) {
				return false;
			}
		}
	}

	return true;
}

ALIGNED(16) sse_union sse1[16];
ALIGNED(16) sse_union sse2[16];

void mode_sw_xmm(sse_union sse[])
{
	write_cr0(read_cr0() & ~6);
	write_cr4(read_cr4() | 0x200);

	asm volatile("movapd %%xmm0, %0" : "=m"(sse[0]) : :);
	asm volatile("movapd %%xmm1, %0" : "=m"(sse[1]) : :);
	asm volatile("movapd %%xmm2, %0" : "=m"(sse[2]) : :);
	asm volatile("movapd %%xmm3, %0" : "=m"(sse[3]) : :);
	asm volatile("movapd %%xmm4, %0" : "=m"(sse[4]) : :);
	asm volatile("movapd %%xmm5, %0" : "=m"(sse[5]) : :);
	asm volatile("movapd %%xmm6, %0" : "=m"(sse[6]) : :);
	asm volatile("movapd %%xmm7, %0" : "=m"(sse[7]) : :);
#ifdef __x86_64__
	asm volatile("movapd %%xmm8, %0" : "=m"(sse[8]) : :);
	asm volatile("movapd %%xmm9, %0" : "=m"(sse[9]) : :);
	asm volatile("movapd %%xmm10, %0" : "=m"(sse[10]) : :);
	asm volatile("movapd %%xmm11, %0" : "=m"(sse[11]) : :);
	asm volatile("movapd %%xmm12, %0" : "=m"(sse[12]) : :);
	asm volatile("movapd %%xmm13, %0" : "=m"(sse[13]) : :);
	asm volatile("movapd %%xmm14, %0" : "=m"(sse[14]) : :);
	asm volatile("movapd %%xmm15, %0" : "=m"(sse[15]) : :);
#endif
}

bool mode_xmm_diff(void)
{
	for (int i = 0; i < XMM_ARRAY_NUM_1; i++) {
		for (int j = 0; j < XMM_ARRAY_NUM_2; j++) {
			if (sse1[i].u[j] != sse2[i].u[j]) {
				return false;
			}
		}
	}

	return true;
}

ALIGNED(8) u64 reg1[16];
ALIGNED(8) u64 reg2[16];

void mode_sw_reg(u64 reg[])
{
#ifdef __x86_64__
	asm volatile("movq %%rax, %0" : "=m"(reg[0]) : :);
	asm volatile("movq %%rbx, %0" : "=m"(reg[1]) : :);
	asm volatile("movq %%rcx, %0" : "=m"(reg[2]) : :);
	asm volatile("movq %%rdx, %0" : "=m"(reg[3]) : :);
	asm volatile("movq %%rdi, %0" : "=m"(reg[4]) : :);
	asm volatile("movq %%rsi, %0" : "=m"(reg[5]) : :);
	asm volatile("movq %%rbp, %0" : "=m"(reg[6]) : :);
	asm volatile("movq %%rsp, %0" : "=m"(reg[7]) : :);
	asm volatile("movq %%r8, %0" : "=m"(reg[8]) : :);
	asm volatile("movq %%r9, %0" : "=m"(reg[9]) : :);
	asm volatile("movq %%r10, %0" : "=m"(reg[10]) : :);
	asm volatile("movq %%r11, %0" : "=m"(reg[11]) : :);
	asm volatile("movq %%r12, %0" : "=m"(reg[12]) : :);
	asm volatile("movq %%r13, %0" : "=m"(reg[13]) : :);
	asm volatile("movq %%r14, %0" : "=m"(reg[14]) : :);
	asm volatile("movq %%r15, %0" : "=m"(reg[15]) : :);
#else
	asm volatile("mov %%eax, %0" : "=m"(reg[0]) : :);
	asm volatile("mov %%ebx, %0" : "=m"(reg[1]) : :);
	asm volatile("mov %%ecx, %0" : "=m"(reg[2]) : :);
	asm volatile("mov %%edx, %0" : "=m"(reg[3]) : :);
	asm volatile("mov %%edi, %0" : "=m"(reg[4]) : :);
	asm volatile("mov %%esi, %0" : "=m"(reg[5]) : :);
	asm volatile("mov %%ebp, %0" : "=m"(reg[6]) : :);
	asm volatile("mov %%esp, %0" : "=m"(reg[7]) : :);

#endif
}

bool mode_reg_diff(void)
{
	for (int i = 0; i < REG_ARRAY_NUM; i++)	{
		if (i < 8) {
			reg1[i]	 = reg1[i] >> 32;
			reg2[i]	 = reg2[i] >> 32;
		}

		if (reg1[i] != reg2[i]) {
			return false;
		}
	}

	return true;
}


#ifdef __x86_64__
void mode_switch(void)
{
	asm volatile("	cli\n"
		 "	ljmp *1f\n" /* switch to compatibility */
		 "1:\n"
		 "	.long 2f\n"
		 "	.long " xstr(KERNEL_CS32) "\n"
		 ".code32\n"
		 "2:\n"
		 "	movl $0xab, %%eax\n"
		 "	ljmpl %[cs64], $3f\n"    /* back to long mode */
		 ".code64\n"
		 "3:\n"
		 :: [cs16] "i"(KERNEL_CS16), [ds16] "i"(KERNEL_DS16),
		    [cs32] "i"(KERNEL_CS32), [cs64] "i"(KERNEL_CS64)
		 : "rax", "rbx", "rcx", "rdx", "memory");
}
#else
void mode_switch(void)
{
	mode_switch_to_protect();
	mode_switch_to_compat();
}
#endif

#ifdef __x86_64__
/**
 * @brief Case name:Preserve the value of any YMM register
 * when a vCPU attempts to switch to compatibility mode_001
 *
 * Summary: In 64-bit mode, to switch to compatibility mode
 * then back into 64-bit mode, value of YMM0~15 should be no changed.
 *
 */
void cpumode_rqmid_37716_preserve_ymm015_64bit_switch_001(void)
{
	report("%s", mode_ymm_diff(), __FUNCTION__);
}

/**
 * @brief Case name:Preserve the value of any XMM register
 * when a vCPU attempts to switch to compatibility mode_001
 *
 * Summary: In 64-bit mode, to switch to compatibility mode
 * then back into 64-bit mode, value of XMM0~15 should be no changed.
 *
 */
void cpumode_rqmid_37713_preserve_xmm015_64bit_switch_001(void)
{
	report("%s", mode_xmm_diff(), __FUNCTION__);
}

/**
 * @brief Case name: Preserve the value of any general-purpose register
 * when a vCPU attempts to switch to compatibility mode_001
 *
 * Summary: In 64-bit mode, to switch to compatibility mode then back into 64-bit mode,
 *  value of general-purpose register and R8-15 should be no changed.
 *
 */
void cpumode_rqmid_37717_preserve_general_reg_64bit_switch_001(void)
{
	report("%s", mode_reg_diff(), __FUNCTION__);
}
#else
/**
 * @brief Case name: Preserve the value of any general-purpose register when a vCPU
 * switch to protected-mode then back into compatibility mode_001.
 *
 * Summary: In compatibility mode, to switch to protected-mode then back into compatibility mode,
 *  the value of any general-purpose register should be no changed.
 *
 */
void cpumode_rqmid_37712_preserve_general_reg_compat_switch_001(void)
{
	report("%s", mode_reg_diff(), __FUNCTION__);
}

/**
 * @brief Case name: Preserve the value of any XMM register when a vCPU attempts to
 * switch to protected-mode then back into compatibility mode_001
 *
 * Summary:In compatibility mode, to switch to protected-mode then back into
 * compatibility mode, value of XMM0~7 should be no changed.
 *
 */
void cpumode_rqmid_37714_preserve_xmm07_compat_switch_001(void)
{
	report("%s", mode_xmm_diff(), __FUNCTION__);
}

/**
 * @brief Case name: Preserve the value of any YMM register when a vCPU attempts to
 * switch to protected-mode then back into compatibility mode_001
 *
 * Summary:In compatibility mode, to switch to protected-mode then back into
 * compatibility mode, value of YMM0~7 should be no changed.
 *
 */
void cpumode_rqmid_37715_preserve_ymm07_compat_switch_001(void)
{
	report("%s", mode_ymm_diff(), __FUNCTION__);
}
#endif

void mode_switch_case(void)
{

#ifndef __x86_64__
	old_pt = read_cr3();
	mode_switch_to_compat();
#endif

	mode_sw_xmm(sse1);
	mode_sw_ymm(avx1);
	mode_sw_reg(reg1);

	mode_switch();

	mode_sw_xmm(sse2);
	mode_sw_ymm(avx2);
	mode_sw_reg(reg2);

#ifdef __x86_64__
	cpumode_rqmid_37716_preserve_ymm015_64bit_switch_001();
	cpumode_rqmid_37713_preserve_xmm015_64bit_switch_001();
	cpumode_rqmid_37717_preserve_general_reg_64bit_switch_001();
#else
	cpumode_rqmid_37712_preserve_general_reg_compat_switch_001();
	cpumode_rqmid_37714_preserve_xmm07_compat_switch_001();
	cpumode_rqmid_37715_preserve_ymm07_compat_switch_001();
#endif
}

#ifndef __x86_64__

#define AVL_SET       0x1
#define TSS_TYPE_BUSY 0x2
#define TEST_SEL      (0x0F << 3)

#define ASM_TRY_JUMP(selector) do { \
	asm volatile ( \
		ASM_TRY("1f\n") \
		"ljmpl %0, $1f\n" \
		"1:\n" \
		: : "i"(selector) \
		: "memory" \
	); \
} while (0)

/* Vol3: 7.6 16-BIT TASK-STATE SEGMENT (TSS) */
typedef struct tss16 {
	u16 prev;  /* Previous Task Link */
	u16 sp0;
	u16 ss0;
	u16 sp1;
	u16 ss1;
	u16 sp2;
	u16 ss2;
	u16 ip;  /* IP (Entry Point) */
	u16 flags;
	u16 ax, cx, dx, bx, sp, bp, si, di;
	u16 es;
	u16 cs;
	u16 ss;
	u16 ds;
	u16 ldt; /* Task LDT Selector */
} tss16_t;

tss16_t tss16;
void setup_tss16(u16 tr)
{
	tss16.prev = 0;
	tss16.ss0 = tss16.ss1 = tss16.ss2 = KERNEL_DS;
	tss16.sp = tss16.sp0 = tss16.sp1 = tss16.sp2 = 0;
	tss16.ip = 0;
	tss16.flags = 0;
	tss16.ax = tss16.cx = tss16.dx = tss16.bx = 0;
	tss16.bp = tss16.si = tss16.di = 0;
	tss16.cs = KERNEL_CS;
	tss16.ds = tss16.es = tss16.ss = KERNEL_DS;
	set_gdt_entry(tr, (ulong)&tss16, sizeof(tss16_t) - 1,
		DESCRIPTOR_TYPE_SYS | SYS_SEGMENT_AND_GATE_DESCRIPTOR_16BIT_TSSA |
		DESCRIPTOR_PRIVILEGE_LEVEL_0 | SEGMENT_PRESENT_SET,
		GRANULARITY_CLEAR | AVL_SET);
	ltr(tr);
}

static u8 set_cr0_pg(void)
{
	asm	volatile (
		ASM_TRY("2f\n")
		"mov %0, %%cr0\n"
		"2:\n"
		: : "r"(read_cr0() | X86_CR0_PG)
		: "memory"
		);
	return exception_vector();
}

static void report_gp(u8 vector, const char *case_name)
{
	if (NO_EXCEPTION == vector) {
		report("%s test - no exception occurs when setting  the CR0.PG",
			false,  case_name);
	} else if (GP_VECTOR == vector) {
		report("%s test", true,  case_name);
	} else {
		report("%s test - exception(%d) occurs when setting  the CR0.PG",
			false,  case_name, vector);
	}
}

extern gdt_entry_t gdt32[];

static ulong old_cr0;
static ulong old_cr3;
static ulong old_cr4;
static u64 old_efer;

static void prepare_ia32e_mode(void)
{
	old_cr0 = read_cr0();
	old_cr3	= read_cr3();
	old_cr4	= read_cr4();
	old_efer = rdmsr(MSR_EFER);

	/* Enter the protected mode and disable paging by setting CR0.PG = 0 */
	write_cr0(old_cr0 &	(~X86_CR0_PG));

	/* Enable physical-address extensions (PAE) by setting CR4.PAE = 1. */
	write_cr4(old_cr4 |	X86_CR4_PAE);

	/* Load CR3 with the physical base address of the Level 4 page map table (PML4). */
	write_cr3(pt_root);

    /* Set IA32_EFER.LME = 1. */
	wrmsr(MSR_EFER, old_efer | EFER_LME);
}

static void restore_ia32e_mode(void)
{
	wrmsr(MSR_EFER, old_efer);
	write_cr3(old_cr3);
	write_cr4(old_cr4);
	write_cr0(old_cr0);
}

/**
 * @brief Case name: CPU Mode of Operation Exposure of CPU modes of operation_002
 *
 * Summary: The processor performs 64-bit mode consistency checks whenever
 * software attempts to modify any of the enable bits directly involved in
 * activating IA-32e mode (IA32_EFER.LME, CR0.PG, and CR4.PAE). It will generate
 * a general protection fault (#GP) if consistency checks fail.
 *
 * Inject #GP(0) to the vCPU when a vCPU attempt to write CR0 (Req ID: 239782).
 * This case check if the TR contains a 16-bit TSS on an attempt to activate
 * IA-32e mode, a #GP will be generated.
 */
void cpumode_rqmid_40496_vcpu_attempt_to_write_cr0_with_16bit_tss(void)
{
	u8 vector = NO_EXCEPTION;

	/*Backup TSS*/
	u16 old_tr = str();
	u16 selector = old_tr >> 3;
	gdt_entry_t old_gdt = gdt32[selector];
	prepare_ia32e_mode();

	/* Set Guest TR to contain a 16-bit TSS. */
	setup_tss16(old_tr);
	vector = set_cr0_pg();
	restore_ia32e_mode();

	/*Restore TSS*/
	gdt32[selector] = old_gdt;
	gdt32[selector].access &= ~TSS_TYPE_BUSY;
	ltr(old_tr);
	report_gp(vector, __func__);
}

static u8 set_cs_l(void)
{
	set_gdt_entry(TEST_SEL, 0, SEGMENT_LIMIT_ALL,
		SEGMENT_PRESENT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_0 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_CODE_EXE_READ_ACCESSED,
		GRANULARITY_SET | DEFAULT_OPERATION_SIZE_32BIT_SEGMENT | L_64_BIT_CODE_SEGMENT);
	ASM_TRY_JUMP(TEST_SEL);
	return exception_vector();
}

/**
 * @brief Case name: CPU Mode of Operation Exposure of CPU modes of operation_003
 *
 * Summary: The processor performs 64-bit mode consistency checks whenever
 * software attempts to modify any of the enable bits directly involved in
 * activating IA-32e mode (IA32_EFER.LME, CR0.PG, and CR4.PAE). It will generate
 * a general protection fault (#GP) if consistency checks fail.
 *
 * Inject #GP(0) to the vCPU when a vCPU attempt to write CR0 (Req ID: 239781).
 * This case check if the current CS has the L-bit set on an attempt to activate
 * IA-32e mode, a #GP will be generated.
 */
void cpumode_rqmid_40500_vcpu_attempt_to_write_cr0_with_16bit_cs(void)
{
	/* Set CS.L = 1 */
	u8 vector = set_cs_l();
	if (NO_EXCEPTION != vector) {
		report("%s test - exception(%d) occurs when setting CS.L",
			false,  __func__, vector);
		return;
	}
	prepare_ia32e_mode();
	vector = set_cr0_pg();

	/* Restore CS */
	ASM_TRY_JUMP(KERNEL_CS);
	restore_ia32e_mode();

	if (NO_EXCEPTION != exception_vector()) {
		report("%s test - exception(%d) occurs when restoring CS",
			false, __func__, vector);
	} else {
		report_gp(vector, __func__);
	}
}
#endif

int main(int ac, char **av)
{
	setup_idt();
	print_case_list();

	mode_switch_case();
#ifdef __x86_64__
	cpumode_rqmid_37685_receives_GP_when_clear_CR0_PG_001();
	cpumode_rqmid_37688_Ignore_this_access_when_write_IA32_EFER_LMA_002();
#else
	cpumode_rqmid_37684_IA32_EFER_LMA_is_0H_when_clear_CR0_PG_001();
	cpumode_rqmid_37687_Ignore_this_access_when_write_IA32_EFER_LMA_001();
	cpumode_rqmid_37690_Ignore_this_access_when_write_IA32_EFER_LMA_003();
	cpumode_rqmid_40496_vcpu_attempt_to_write_cr0_with_16bit_tss();
	cpumode_rqmid_40500_vcpu_attempt_to_write_cr0_with_16bit_cs();
#endif
	cpumode_rqmid_28141_CPU_Mode_of_Operation_Forbidden_switch_back_to_real_address_mode_001();
	cpumode_rqmid_27815_CPU_Mode_of_Operation_INITIAL_GUEST_IA32_EFER_LME_FOLLOWING_STARTUP_001();
	cpumode_rqmid_27817_CPU_Mode_of_Operation_INITIAL_GUEST_IA32_EFER_LMA_FOLLOWING_STARTUP_001();
	cpumode_rqmid_28143_CPU_Mode_of_Operation_INITIAL_GUEST_CR0_PE_STARTUP_001();
	cpumode_rqmid_46084_CPU_Mode_of_Operation_INITIAL_EFLAGS_VM_STARTUP_001();
#ifdef IN_NON_SAFETY_VM
	cpumode_rqmid_27814_CPU_Mode_of_Operation_INITIAL_GUEST_MODE_FOLLOWING_INIT_001();
#ifdef __x86_64__
	cpumode_rqmid_27816_INITIAL_GUEST_IA32_EFER_LME_FOLLOWING_INIT_001();
	cpumode_rqmid_27818_INITIAL_GUEST_IA32_EFER_LMA_FOLLOWING_INIT_001();
#endif
	cpumode_rqmid_46085_CPU_Mode_of_Operation_INITIAL_EFLAGS_VM_INIT_001();
#endif

	return report_summary();
}
