#include "processor.h"
#include "xsave.h"
#include "libcflat.h"
#include "desc.h"
#include "desc.c"
#include "alloc.h"
#include "alloc.c"
#include "misc.h"
#include "delay.h"
#include "fwcfg.h"
#include "instruction_common.h"
#include "register_op.h"
#include "xsave.h"
#include "debug_print.h"
/**CPUID Function:**/
/**Some common function **/
u16 *creat_non_aligned_add(void)
{
	__attribute__ ((aligned(16))) u64 addr;
	u64 *aligned_addr = &addr;
	u16 *non_aligned_addr = (u16 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

int ap_start_count = 0;
u64 init_xinuse = 0;
u64 set_xinuse = 0;
void save_unchanged_reg(void)
{
	u32 volatile v1, v2;
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	/* enable cr4.OSFXSR[9] for SSE. */
	write_cr4_osxsave(1);

	if (ap_start_count == 0) {
		/* XINUSE[bit 2:0] init value */
		asm volatile(
			"mov $0, %eax\n\t"
			"mov $0, %edx\n\t"
			"mov $1, %ecx\n\t"
			"xgetbv\n\t"
		);

		/*store XCR0 & XINUSE to init_xinuse.*/
		asm volatile (
			"mov %%eax, %0\r\n"
			"mov %%edx, %1\r\n"
			: "=m"(v1), "=m"(v2));
		init_xinuse = v1 | ((u64)v2 << 32UL);

		/* exec SSE instrction. */
		asm volatile("movapd %xmm1, %xmm2");

		/* xsetbv XCR0 to X87|SSE|AVX. */
		asm volatile(
			"mov $0, %ecx\n\t"
			"mov $0, %edx\n\t"
			"mov $7, %eax\n\t"
			"xsetbv\n\t"
		);

		ap_start_count++;
	}

	/* xgetbv XCR0 & XINUSE. */
	asm volatile(
		"mov $0, %eax\n\t"
		"mov $0, %edx\n\t"
		"mov $1, %ecx\n\t"
		"xgetbv\n\t"
	);

	/*store XCR0 & XINUSE to set_xinuse.*/
	asm volatile (
		"mov %%eax, %0\r\n"
		"mov %%edx, %1\r\n"
		: "=m"(v1), "=m"(v2));
	set_xinuse = v1 | ((u64)v2 << 32UL);
}

/*
 * Case name: 23631:XSAVE hide PKRU support_001.
 *
 * Summary: DNG_129928: XSAVE hide PKRU support.
 *	    ACRN hypervisor shall hide XSAVE-managed PKRU states from any VM,
 *	    in compliance with Chapter 13.2 and 13.3, Vol. 1, SDM.
 */
static __unused void xsave_rqmid_23631_hide_pkru_support_001(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID that is the processor defult-support.******\n");
	uint64_t supported_xcr0;
	supported_xcr0 = get_supported_xcr0();
	i++;
#ifdef USE_DEBUG
	debug_print("The supported CPUID = %#lx\n", supported_xcr0);
#endif
	debug_print("******Step2: Get EAX[9], and compare with 0b.******\n");
	int r_eax;
	r_eax = (u32)(supported_xcr0>>STATE_PKRU_BIT);
	r_eax = 0x0001 & r_eax;
	if (r_eax == 0) {
		i++;
		debug_print("The value of EAX[9] = %#x \n", r_eax);
		debug_print("******23631:XSAVE_hide_PKRU_support_001, test case Passed.******\n");
	} else {
		debug_print("The value of EAX[9] = %#x \n", r_eax);
		debug_print("******23631:XSAVE_hide_PKRU_support_001, test case Failed.******\n");
	}
	report("%s", (i == 2), __FUNCTION__);
}


/*
 * Case name: 23632:XSAVE hide MPX support_001.
 *
 * Summary: DNG_129926: XSAVE hide MPX support.
 *	    ACRN hypervisor shall hide XSAVE-managed MPX states from any VM,
 *	    in compliance with Chapter 13.2 and 13.3, Vol. 1, SDM.
 */
static __unused void xsave_rqmid_23632_hide_mpx_support_001(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID that is the processor defult-support******\n");
	uint64_t supported_xcr0;
	supported_xcr0 = get_supported_xcr0();
	i++;
#ifdef USE_DEBUG
	debug_print("The supported CPUID = %#lx\n", supported_xcr0);
#endif
	debug_print("******Step2: Get EAX[4:3], and compare with 00b.******\n");
	int r_eax;
	r_eax = (u32)(supported_xcr0>>STATE_MPX_BNDREGS_BIT);
	r_eax = 0x0003 & r_eax;
	if (r_eax == 0) {
		i++;
		debug_print("The value of EAX[4:3] = %#x\n", r_eax);
		debug_print("******23632:XSAVE_hide_MPX_support_001, test case Passed.******\n");
	} else {
		debug_print("The value of EAX[4:3] = %#x\n", r_eax);
		debug_print("******23632:XSAVE_hide_MPX_support_001, test case Failed.******\n");
	}
	report("%s", (i == 2), __FUNCTION__);
}


/*
 * Case name: 23633:XSAVE hide AVX-512 support_001.
 *
 * Summary: DNG_ 129925:XSAVE hide AVX-512 support.
 *	    ACRN hypervisor shall hide XSAVE-managed AVX-512 states from any VM,
 *	    in compliance with Chapter 13.2 and 13.3, Vol. 1, SDM.
 */
static __unused void xsave_rqmid_23633_hide_avx_512_support_001(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID that is the processor defult-support******\n");
	uint64_t supported_xcr0;
	supported_xcr0 = get_supported_xcr0();
	i++;
#ifdef USE_DEBUG
	debug_print("The supported CPUID = %lu\nThe supported CPUID = %#lx\n", supported_xcr0, supported_xcr0);
#endif
	debug_print("******Step2: Get EAX[7:5], and compare with 000b.******\n");
	int r_eax;
	r_eax = (u32)(supported_xcr0>>STATE_AVX_512_OPMASK);
	r_eax = 0x0007 & r_eax;
	if (r_eax == 0) {
		i++;
		debug_print("The value of EAX[7:5] = %#x \n", r_eax);
		debug_print("******23633:XSAVE_hide_AVX_512_support_001, test case Passed.******\n");
	} else {
		debug_print("The value of EAX[7:5] = %#x \n", r_eax);
		debug_print("******23633:XSAVE_hide_AVX_512_support_001, test case Failed.******\n");
	}
	report("%s", (i == 2), __FUNCTION__);
}


/*
 * Case name: 28385:XSAVE_physical_x87_support_AC_001.
 *
 * Summary: DNG_ 129931: XSAVE physical x87 support
 *	    XSAVE-managed x87 states shall be available on the physical platform.
 */
static __unused void xsave_rqmid_28385_physical_x87_support_AC_001(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID that is the processor defult-support******\n");
	uint64_t supported_xcr0;
	supported_xcr0 = get_supported_xcr0();
	i++;
#ifdef USE_DEBUG
	debug_print("The supported CPUID = 0x%lx\n", supported_xcr0);
#endif
	debug_print("******Step2: Get EAX[0], and compare with 0b******\n");
	int r_eax;
	r_eax = (u32)(supported_xcr0>>STATE_X87_BIT);
	r_eax = 0x0001 & r_eax;
	if (r_eax == 1) {
		i++;
		debug_print("The value of EAX[0] = %#x \n", r_eax);
		debug_print("******28385:XSAVE_physical_x87_support_001, test case Passed******\n");
	} else {
		debug_print("The value of EAX[0] = %#x \n", r_eax);
		debug_print("******28385:XSAVE_physical_x87_support_001, test case Failed******\n");
	}
	report("%s", (i == 2), __FUNCTION__);
}


/*
 * Case name: 28386:XSAVE_physical_general_support_AC_001.
 *
 * Summary: DNG_ 132053: XSAVE physical general support.
 *	    XSAVE general support shall be available on the physical platform.
 */
static __unused void xsave_rqmid_28386_physical_general_support_AC_001(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID.1:ECX.XSAVE[bit 26], and compare with 0b******\n");
	int r_ecx;
	r_ecx = check_cpuid_1_ecx(CPUID_1_ECX_XSAVE);
	i++;
	if (r_ecx == 1) {
		i++;
		debug_print("The value of CPUID.1:ECX.XSAVE[bit 26] = %#x \n", r_ecx);
		debug_print("******28386:XSAVE_physical_general_support_001, test case Passed******\n");
	} else {
		debug_print("The value of CPUID.1:ECX.XSAVE[bit 26] = %#x \n", r_ecx);
		debug_print("******28386:XSAVE_physical_general_support_001, test case Failed******\n");
	}
	report("%s", (i == 2), __FUNCTION__);
}


/*
 * Case name: 28388:XSAVE_physical_SSE_support_AC_001.
 *
 * Summary: DNG_132150: XSAVE physical SSE support.
 *	    XSAVE-managed SSE states shall be available on the physical platform.
 */
static __unused void xsave_rqmid_28388_physical_sse_support_AC_001(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID.0DH:EAX.SSE[bit 1], and compare with 1b******\n");
	uint64_t supported_xcr0;
	supported_xcr0 = get_supported_xcr0();
	i++;
#ifdef USE_DEBUG
	debug_print("The supported CPUID = %lu\nThe supported CPUID = %#lx\n", supported_xcr0, supported_xcr0);
#endif
	int r_eax;
	r_eax = (u32)(supported_xcr0>>STATE_SSE_BIT);
	r_eax = 0x0001 & r_eax;
	if (r_eax == 1) {
		i++;
		debug_print("The value of CPUID.0DH:EAX.SSE[bit 1] = %#x \n", r_eax);
		debug_print("******28388:XSAVE_physical_SSE_support_001, test case Passed******\n");
	} else {
		debug_print("The value of CPUID.0DH:EAX.SSE[bit 1] = %#x \n", r_eax);
		debug_print("******28388:XSAVE_physical_SSE_support_001, test case Failed******\n");
	}
	report("%s", (i == 2), __FUNCTION__);
}


/*
 * Case name: 28390:XSAVE_physical_AVX_support_AC_001.
 *
 * Summary: DNG_132151: XSAVE physical AVX support.
 *	    XSAVE-managed AVX states shall be available on the physical platform.
 */
static __unused void xsave_rqmid_28390_physical_avx_support_AC_001(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID.0DH:EAX.AVX[bit 2], and compare with 1b******\n");
	uint64_t supported_xcr0;
	supported_xcr0 = get_supported_xcr0();
	i++;
#ifdef USE_DEBUG
	debug_print("The supported CPUID = %lu\nThe supported CPUID = 0x%lx\n", supported_xcr0, supported_xcr0);
#endif
	int r_eax;
	r_eax = (u32)(supported_xcr0>>STATE_AVX_BIT);
	r_eax = 0x0001 & r_eax;
	if (r_eax == 1) {
		i++;
		debug_print("The value of CPUID.0DH:EAX.AVX[bit 2] = %#x \n", r_eax);
		debug_print("******28390:XSAVE_physical_AVX_support_001, test case Passed******\n");
	} else {
		debug_print("The value of CPUID.0DH:EAX.AVX[bit 2] = %#x \n", r_eax);
		debug_print("******28390:XSAVE_physical_AVX_support_001, test case Failed******\n");
	}
	report("%s", (i == 2), __FUNCTION__);
}


/*
 * Case name: 28468:XSAVE_physical_compaction_extensions_AC_001.
 *
 * Summary: DNG_132152: XSAVE physical compaction extensions.
 *	    XSAVE compaction extensions shall be available on the physical platform.
 */
static __unused void xsave_rqmid_28468_physical_compaction_extensions_AC_001(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID.(EAX=0DH,ECX=1H):EAX[bit1], and compare with 1b******\n");
	int bit_support_compaction;
	bit_support_compaction = cpuid_indexed(CPUID_XSAVE_FUC, EXTENDED_STATE_SUBLEAF_1).a;
	bit_support_compaction = 0x0002 & bit_support_compaction;
	if (bit_support_compaction != 0) {
		i++;
		debug_print("The value of CPUID.(EAX=0DH,ECX=1H):EAX[bit1] = %#x \n", bit_support_compaction);
		debug_print("******28468:XSAVE_physical_compaction_extensions_001, test case Passed******\n");
	} else {
		debug_print("The value of CPUID.(EAX=0DH,ECX=1H):EAX[bit1] = %#x \n", bit_support_compaction);
		debug_print("******28468:XSAVE_physical_compaction_extensions_001, test case Failed******\n");
	}
	report("%s", (i == 1), __FUNCTION__);
}


/*
 * Case name: 28392:XSAVE_physical_init_and_modified_optimizations_AC_001.
 *
 * Summary: DNG_132153: XSAVE physical init and modified optimizations.
 *	    XSAVE init and modified optimizations shall be available on the physical platform.
 */
static __unused void xsave_rqmid_28392_physical_init_and_modified_optimizations_AC_001(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID that is the processor defult-support******\n");
	u32 bit_support_init_modified;

	bit_support_init_modified = cpuid_indexed(CPUID_XSAVE_FUC, EXTENDED_STATE_SUBLEAF_1).a;
	bit_support_init_modified = 0x5 & bit_support_init_modified;
	if (bit_support_init_modified == 0x5) {
		i++;
	}

	report("%s", (i == 1), __FUNCTION__);
}


/*
 * Case name: 28393:XSAVE_general_support_009.
 *
 * Summary: DNG_132148: XSAVE general support
 *	    Vol1_13.7/13.8: XSAVE/XRSTOR instruction:
 *	    If the address of the XSAVE area is not 64-byte aligned, a general-protection exception (#GP) occurs.
 */
static __unused void xsave_rqmid_28393_general_support_009(void)
{
	u32 i = 0;
	debug_print("******Step1: Set CR4.osxsave[bit18] to 1.******\n");
	write_cr4_osxsave(1);
	i++;

	debug_print("******Step2: Creat non-aligned address.******\n");
	__attribute__((aligned(64))) u64 addr;
	u64 *aligned_addr = &addr;
	u64 *non_aligned_addr = (u64 *)((u8 *)aligned_addr + 1);
	i++;
	debug_print("aligned_addr = %#lx\n", *aligned_addr);
	debug_print("non_aligned_addr = %#lx\n", *non_aligned_addr);

	debug_print("Step3: Rand to execute XSAVE/XSAVEC/XSAVOPT/XRSTOR instruciton with the non-aligned address.\n");
	debug_print("Excepted Result: Generate #GP exception, error_code=0000.\n");

	u32 r_eax = 0;
	u32 r_edx = 0;
	switch (get_random_value()%4) {
	case 0:
		debug_print("******Step4: Execute XSAVE instruciton with the non-aligned address.******\n");
		debug_print("******Excepted Result: Generate #GP exception, error_code=0000.******\n");
		asm volatile("xsave %[addr]\n\t"
			: : [addr]"m"(non_aligned_addr), "a"(r_eax), "d"(r_edx)
			: "memory");
		i++;
	case 1:
		debug_print("******Step4: Execute XSAVEC instruciton with the non-aligned address.******\n");
		debug_print("******Excepted Result: Generate #GP exception, error_code=0000.******\n");
		i++;
		asm volatile("xsavec %[addr]\n\t"
			: : [addr]"m"(non_aligned_addr), "a"(r_eax), "d"(r_edx)
			: "memory");
		i++;
	case 2:
		debug_print("******Step4: Execute XSAVEOPT instruciton with the non-aligned address.******\n");
		debug_print("******Excepted Result: Generate #GP exception, error_code=0000.******\n");
		i++;
		asm volatile("xsaveopt %[addr]\n\t"
			: : [addr]"m"(non_aligned_addr), "a"(r_eax), "d"(r_edx)
			: "memory");
		i++;
	case 3:
		debug_print("******Step4: Execute XRSTOR instruciton with the non-aligned address.******\n");
		debug_print("******Excepted Result: Generate #GP exception, error_code=0000.******\n");
		i++;
		asm volatile("xrstor %[addr]\n\t"
			: : [addr]"m"(non_aligned_addr), "a"(r_eax), "d"(r_edx)
			: "memory");
		i++;
	}
	report("%s", (i == 2), __FUNCTION__);
}


/*
 * Case name: 28395:XSAVE_general_support_012.
 *
 * Summary: DNG_132148: XSAVE general support.
 *	    Vol1_13.7/13.8: XSAVE/XRSTOR instruction:
 *	    If the address of the XSAVE area is not 64-byte aligned, a general-protection exception (#GP) occurs;
 *	    If CR0.AM = 1, CPL = 3, and EFLAGS.AC =1, an alignment-check exception (#AC) may occur instead of #GP.
 */
static __unused void xsave_rqmid_28395_general_support_012_subfun(const char *msg)
{
	u32 r_eax = 0;
	u32 r_edx = 0;
	switch (get_random_value()%4) {
	case 0:
		debug_print("******Step4: Execute XSAVE instruciton with the non-aligned address.******\n");
		debug_print("******Excepted Result: Generate #AC exception, error_code=0000.******\n");
		asm volatile("xsave %[addr]\n\t"
			: : [addr]"m"(*(creat_non_aligned_add())), "a"(r_eax), "d"(r_edx)
			: "memory");
		break;
	case 1:
		debug_print("******Step4: Execute XSAVEC instruciton with the non-aligned address.******\n");
		debug_print("******Excepted Result: Generate #AC exception, error_code=0000.******\n");
		asm volatile("xsavec %[addr]\n\t"
			: : [addr]"m"(*(creat_non_aligned_add())), "a"(r_eax), "d"(r_edx)
			: "memory");
		break;
	case 2:
		debug_print("******Step4: Execute XSAVEOPT instruciton with the non-aligned address.******\n");
		debug_print("******Excepted Result: Generate #AC exception, error_code=0000.******\n");
		asm volatile("xsaveopt %[addr]\n\t"
			: : [addr]"m"(*(creat_non_aligned_add())), "a"(r_eax), "d"(r_edx)
			: "memory");
		break;
	case 3:
		debug_print("******Step4: Execute XRSTOR instruciton with the non-aligned address.******\n");
		debug_print("******Excepted Result: Generate #AC exception, error_code=0000.******\n");
		asm volatile("xrstor %[addr]\n\t"
			: : [addr]"m"(*(creat_non_aligned_add())), "a"(r_eax), "d"(r_edx)
			: "memory");
		break;
	}
}

static __unused void xsave_rqmid_28395_general_support_012(void)
{
	u32 i = 0;
	debug_print("******Step1: Set CR4.osxsave[bit18] to 1.******\n");
	write_cr4_osxsave(1);
	i++;
	debug_print("******Step2: Set CR0.AM[bit18] and EFLAG.AC[bit18] to 1.******\n");
	set_cr0_AM(1);
	set_eflag_ac(1);
	i++;

	debug_print("Step3: Rand to execute XSAVE/XSAVEC/XSAVOPT/XRSTOR instruciton with the non-aligned address.\n");
	debug_print("Excepted Result: Generate #AC exception, error_code=0000.\n");
	do_at_ring3(xsave_rqmid_28395_general_support_012_subfun, "");
	i++;
	report("%s", (i == 2), __FUNCTION__);
}


/*
 * Case name: 28387:XSAVE_physical_x87_support_001.
 *
 * Summary: DNG_132148: XSAVE general support.
 *	    Vol1_13.8.2: The XCOMP_BV field of the XSAVE header sets a bit in the range 62:0 that is not set in XCR0.
 */
static __unused void xsave_rqmid_28397_general_support_027(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID.1:ECX.XSAVE[bit 26], and compare with 0b******\n");
	int r_ecx;
	r_ecx = check_cpuid_1_ecx(CPUID_1_ECX_XSAVE);
	if (r_ecx == 1) {
		i++;
		debug_print("The value of CPUID.1:ECX.XSAVE[bit 26] = %#x, so XSAVE feature.\n", r_ecx);
	} else {
		debug_print("The value of CPUID.1:ECX.XSAVE[bit 26] = %#x, "
			"XSAVE feature not support or disable.\n", r_ecx);
		debug_print("******28386:XSAVE_rqmid_28397_general_support_027, test case Failed******\n");
		return;
	}

	debug_print("******Step2: Set CR4.osxsave[bit18] to 1.******\n");
	write_cr4_osxsave(1);
	i++;

	debug_print("******Step3: Check XGETBV to make sure XSAVE feature can be used to manage which state.******\n");
	uint64_t xcr0;
	uint64_t test_bits;
	test_bits = STATE_X87 | STATE_SSE | STATE_AVX | STATE_MPX_BNDREGS | STATE_MPX_BNDCSR;
	xgetbv_checking(XCR0_MASK, &xcr0);
	if (xcr0 == test_bits) {
		debug_print("The value of xcr0 = 0x%lx, XSAVE feature ONLY can manage X87/SSE/AVX/MPX STATE.\n", xcr0);
	} else {
		debug_print("The value of xcr0 = 0x%lx, if xcr0==0x1 XSAVE feature ONLY can manage X87.\n", xcr0);
	}

	debug_print("******Step4: Execute XSAVE instruciton with [xsave_area_struct].******\n");
	u32 r_eax = 0;
	u32 r_edx = 0;
	__attribute__((aligned(64)))xsave_area_t xsave_area_created;
	asm volatile("xsave %[addr]\n\t"
		: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
		: "memory");
	i++;
	debug_print("******Step5: Set xcomp_bv[bit13] to 1.******\n ");
	xsave_area_created.xsave_hdr.xcomp_bv  |= STATE_HDC;
	i++;

	debug_print("******Step6: Execute XRSTOR instruciton with [xsave_area_struct].******\n");
	debug_print("******Excepted Result: Generate #GP exception, error_code=0000.******\n");
	asm volatile("xrstor %[addr]\n\t"
		: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
		: "memory");
	i++;
	report("%s", (i == 4), __FUNCTION__);
}


/*
 * Case name: 24444:XSAVE_general_support_021.
 *
 * Summary: DNG_132148: XSAVE general support.
 *	    Vol1_13.3: XCR0[1] is 0 coming out of RESET.
 *	    (Software can use the XSAVE feature set to manage SSE state only if XCR0[1] = 1.)
 */
static __unused void xsave_rqmid_24444_general_support_021(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID.1:ECX.XSAVE[bit 26], and compare with 0b.******\n ");
	int r_ecx;
	r_ecx = check_cpuid_1_ecx(CPUID_1_ECX_XSAVE);
	if (r_ecx == 1) {
		i++;
		debug_print("The value of CPUID.1:ECX.XSAVE[bit 26] = %#x, so XSAVE feature.\n", r_ecx);
	} else {
		debug_print("The value of CPUID.1:ECX.XSAVE[bit 26] = %#x, "
			"XSAVE feature not support or disable.\n", r_ecx);
		debug_print("******28386:XSAVE_rqmid_28397_general_support_027, test case Failed******\n ");
		return;
	}

	debug_print("******Step2: Set CR4.osxsave[bit18] to 1.******\n ");
	write_cr4_osxsave(1);
	i++;

	debug_print("******Step3: Check XGETBV to make sure XSAVE feature can be used to manage which state.******\n ");
	uint64_t xcr0;
	uint64_t test_bits;
	test_bits = STATE_X87 | STATE_SSE | STATE_AVX | STATE_MPX_BNDREGS | STATE_MPX_BNDCSR;
	xgetbv_checking(XCR0_MASK, &xcr0);
	if (xcr0 == test_bits) {
		debug_print("The value of xcr0 = %#lx, XSAVE feature ONLY can manage X87/SSE/AVX/MPX STATE\n", xcr0);
	} else {
		debug_print("The value of xcr0 = %#lx, if xcr0==0x1 XSAVE feature ONLY can manage X87.\n", xcr0);
	}

	debug_print("******Step4: Execute XSETBV with EDX:EAX=0x3/ECX=0.******\n");
	test_bits = STATE_X87 | STATE_SSE;
	xsetbv_checking(XCR0_MASK, test_bits);
	i++;

	debug_print("******Step5: Check XGETBV to make sure XSAVE feature "
		"can be used to manage X87&SSE state.******\n");
	test_bits = STATE_X87 | STATE_SSE;
	debug_print("******TEST_BITS is %#lx.******\n", test_bits);
	xcr0 = 0;
	xgetbv_checking(XCR0_MASK, &xcr0);
	if (xcr0 == test_bits) {
		i++;
		debug_print("******The value of xcr0 = 0x%lx, XSAVE feature "
			"ONLY can manage X87&SSE STATE.******\n", xcr0);
		debug_print("******24444:XSAVE_general_support_021, test case Passed.******\n");
	} else {
		debug_print("******The value of xcr0 = 0x%lx, if xcr0==0x1 XSAVE feature ONLY can manage X87.******\n",
			xcr0);
		debug_print("******24444:XSAVE_general_support_021, test case Failed.******\n");
	}
	report("%s", (i == 4), __FUNCTION__);
}


/*
 * Case name: 24418:XSAVE_general_support_022.
 *
 * Summary: DNG_132148: XSAVE general support.
 *	    Vol1_13.3: Software enables the XSAVE feature set by setting CR4.OSXSAVE[bit 18] to 1
 *	    (e.g., with the MOV to CR4 instruction). If this bit is 0,
 *	    execution of any of XGETBV, XRSTOR, XRSTORS, XSAVE, XSAVEC, XSAVEOPT, XSAVES, and XSETBV
 *	    causes an invalid-opcode exception (#UD).
 */
static __unused void xsave_rqmid_24418_general_support_022(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID.1:ECX.XSAVE[bit 26], and compare with 0b******\n");
	int r_ecx;
	r_ecx = check_cpuid_1_ecx(CPUID_1_ECX_XSAVE);
	if (r_ecx == 1) {
		i++;
		debug_print("The value of CPUID.1:ECX.XSAVE[bit 26] = %#x, so XSAVE feature SUPPORT.\n", r_ecx);
	} else {
		debug_print("The value of CPUID.1:ECX.XSAVE[bit 26] = %#x, "
			"XSAVE feature not support or disable.\n", r_ecx);
		debug_print("******28386:XSAVE_rqmid_28397_general_support_027, test case Failed******\n");
		return;
	}

	debug_print("******Step2: Clean CR4.osxsave[bit18] to 0.******\n");
	write_cr4_osxsave(0);
	i++;

	debug_print("******Step3:Rand to execute /XGETBV/XSAVE/XSAVEC/XSAVES/XSAVOPT/XRSTOR instruciton.******\n");
	int r_eax = 0;
	int r_edx = 0;
	uint64_t xcr0;
	__attribute__((aligned(64)))xsave_area_t xsave_area_created;
	switch (get_random_value()%7) {
	case 0:
		xgetbv_checking(XCR0_MASK, &xcr0);
	case 1:
		debug_print("******Step4: Execute XSAVE instruciton with CR4.osxsave[bit18]=0.******\n");
		debug_print("******Excepted Result: Generate #UD exception, error_code=0000.******\n");
		asm volatile("xsave %[addr]\n\t"
			: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
			: "memory");
		i++;
	case 2:
		debug_print("******Step4: Execute XRSTOR instruciton with CR4.osxsave[bit18]=0.******\n");
		debug_print("******Excepted Result: Generate #UD exception, error_code=0000.******\n");
		asm volatile("xrstor %[addr]\n\t"
			: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
			: "memory");
		i++;
	case 3:
		debug_print("******Step4: Execute XSAVEC instruciton with CR4.osxsave[bit18]=0.******\n");
		debug_print("******Excepted Result: Generate #UD exception, error_code=0000.******\n");
		asm volatile("xsavec %[addr]\n\t"
			: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
			: "memory");
		i++;
	case 4:
		debug_print("******Step4: Execute XSAVES instruciton with CR4.osxsave[bit18]=0.******\n");
		debug_print("******Excepted Result: Generate #UD exception, error_code=0000.******\n");
		asm volatile("xsaves %[addr]\n\t"
			: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
			: "memory");
		i++;
	case 5:
		debug_print("******Step4: Execute XRSTORS instruciton with CR4.osxsave[bit18]=0.******\n");
		debug_print("******Excepted Result: Generate #UD exception, error_code=0000.******\n");
		asm volatile("xrstors %[addr]\n\t"
			: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
			: "memory");
		i++;
	case 6:
		debug_print("******Step4: Execute XSAVEOPT instruciton with CR4.osxsave[bit18]=0.******\n");
		debug_print("******Excepted Result: Generate #UD exception, error_code=0000.******\n ");
		asm volatile("xsaveopt %[addr]\n\t"
			: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
			: "memory");
		i++;
	}
	report("%s", (i == 2), __FUNCTION__);
}


/*
 * Case name: 23638:XSAVE init and modified optimizations_002.
 *
 * Summary: DNG_132105: XSAVE init and modified optimizations.
 *	    ACRN hypervisor shall expose XSAVE init and modified optimizations to any VM,
 *	    in compliance with Chapter 13.2, 13.6 and 13.9, Vol. 1, SDM.
 */
static __unused void xsave_rqmid_23638_init_and_modified_optimizations_002(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID.1:ECX.XSAVE[bit 26], and compare with 0b******\n");
	int r_ecx;
	r_ecx = check_cpuid_1_ecx(CPUID_1_ECX_XSAVE);
	if (r_ecx == 1) {
		i++;
		debug_print("The value of CPUID.1:ECX.XSAVE[bit 26] = %#x, so XSAVE feature SUPPORT.\n", r_ecx);
	} else {
		debug_print("The value of CPUID.1:ECX.XSAVE[bit 26] = %#x, "
			"XSAVE feature not support or disable.\n", r_ecx);
		debug_print("******23638:XSAVE_rqmid_23638_init_and_modified_optimizations_002, "
			"test case Failed******\n");
		return;
	}

	debug_print("******Step2: Clean CR4.osxsave[bit18] to 0.******\n");
	write_cr4_osxsave(0);
	i++;

	debug_print("******Step4: Execute XSAVEOPT64 instruciton with CR4.osxsave[bit18]=0.******\n");
	debug_print("******Excepted Result: Generate #UD exception, error_code=0000.******\n");
	int r_eax = 0;
	int r_edx = 0;
	__attribute__((aligned(64)))xsave_area_t xsave_area_created;
	asm volatile("XSAVEOPT64 %[addr]\n\t"
		: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
		: "memory");
	i++;
	report("%s", (i == 2), __FUNCTION__);
}


/*
 * Case name: 23639:XSAVE init and modified optimizations_003.
 *
 * Summary: DNG_132105: XSAVE init and modified optimizations.
 *	    ACRN hypervisor shall expose XSAVE init and modified optimizations to any VM,
 *	    in compliance with Chapter 13.2, 13.6 and 13.9, Vol. 1, SDM.
 */
static __unused void xsave_rqmid_23639_init_and_modified_optimizations_003(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID.1:ECX.XSAVE[bit 26], and compare with 0b******\n");
	int bit_support;
	bit_support = cpuid_indexed(CPUID_XSAVE_FUC, EXTENDED_STATE_SUBLEAF_1).a;
	bit_support &= SUPPORT_XSAVEOPT;
	if (bit_support == SUPPORT_XSAVEOPT) {
		i++;
		debug_print("The value of CPUID.(EAX=0DH,ECX=0x1):EAX[0] = %#x, so XSAVEOPT Supported.\n", bit_support);
	} else {
		debug_print("The value of CPUID.(EAX=0DH,ECX=0x1):EAX[0] = %#x, "
			"XSAVEOPT not support or disable.", bit_support);
		debug_print("******23639:XSAVE_rqmid_23639_init_and_modified_optimizations_003, "
			"test case Failed******\n");
		return;
	}

	debug_print("******Step2: Set CR4.osxsave[bit18] to 1.******\n");
	write_cr4_osxsave(1);
	i++;

	debug_print("******Step3: Execute XSAVEOPT64 instruciton.******\n");
	int r_eax = 0, r_edx = 0;
	__attribute__((aligned(64)))xsave_area_t xsave_area_created = {0};
	asm volatile("XSAVEOPT64 %[addr]\n\t"
		: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
		: "memory");
	i++;

	debug_print("******Step4: Check the value of xstate_bv[4:0].******\n ");
	u64 state_bv = xsave_area_created.xsave_hdr.xstate_bv;
	state_bv &= (STATE_X87 | STATE_SSE | STATE_AVX | STATE_MPX_BNDREGS | STATE_MPX_BNDCSR);
	if (state_bv == 0) {
		i++;
		debug_print("The value of xstate_bv = %#lx.\n", state_bv);
		debug_print("******23639:XSAVE init and modified optimizations_003, test case Passed.******\n");
	} else {
		debug_print("The value of xstate_bv = %#lx.\n", state_bv);
		debug_print("******23639:XSAVE init and modified optimizations_003, test case Failed.******\n");
	}
	report("%s", (i == 4), __FUNCTION__);
}


/*
 * Case name: 23640:XSAVE init and modified optimizations_004.
 *
 * Summary: DNG_132105: XSAVE init and modified optimizations.
 *          ACRN hypervisor shall expose XSAVE init and modified optimizations to any VM,
 *          in compliance with Chapter 13.2, 13.6 and 13.9, Vol. 1, SDM.
 */
static __unused void xsave_rqmid_23640_init_and_modified_optimizations_004(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID.1:ECX.XSAVEOPT, and compare with 0b******\n");
	int bit_support;
	bit_support = cpuid_indexed(CPUID_XSAVE_FUC, EXTENDED_STATE_SUBLEAF_1).a;
	bit_support &= SUPPORT_XSAVEOPT;
	if (bit_support == SUPPORT_XSAVEOPT) {
		i++;
		debug_print("The value of CPUID.(EAX=0DH,ECX=0x1):EAX[0] = %#x, so XSAVEOPT Supported.\n", bit_support);
	} else {
		debug_print("The value of CPUID.(EAX=0DH,ECX=0x1):EAX[0] = %#x, "
			"XSAVEOPT not support or disable.", bit_support);
		debug_print("******23640:XSAVE_rqmid_23640_init_and_modified_optimizations_004, "
			"test case Failed******\n");
		return;
	}

	debug_print("******Step2: To XSETBV with ECX = 0 and EDX:EAX = 0x0.******\n");
	xsetbv_checking(0, STATE_X87);
	i++;

	debug_print("******Step3: Set CR4.osxsave[bit18] to 1.******\n");
	write_cr4_osxsave(1);
	i++;

	debug_print("******Step4: Execute XSAVEOPT64 instruciton.******\n");
	int r_eax = STATE_X87 | STATE_SSE | STATE_AVX;
	int r_edx = 0;
	__attribute__((aligned(64)))xsave_area_t xsave_area_created = {0};
	asm volatile("XSAVEOPT64 %[addr]\n\t"
		: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
		: "memory");
	i++;

	debug_print("******Step5: Check the value of xstate_bv[2:0].******\n ");
	u64 state_bv = xsave_area_created.xsave_hdr.xstate_bv;
	state_bv &= (STATE_X87 | STATE_SSE | STATE_AVX);
	if (state_bv == STATE_X87) {
		i++;
		debug_print("The value of xstate_bv = %#lx.\n", state_bv);
		debug_print("******23640:XSAVE init and modified optimizations_004, test case Passed.******\n");
	} else {
		debug_print("The value of xstate_bv = %#lx.\n", state_bv);
		debug_print("******23640:XSAVE init and modified optimizations_004, test case Failed.******\n");
	}
	report("%s", (i == 5), __FUNCTION__);
}


/*
 * Case name: 23641:XSAVE init and modified optimizations_005.
 *
 * Summary: DNG_132105: XSAVE init and modified optimizations.
 *          ACRN hypervisor shall expose XSAVE init and modified optimizations to any VM,
 *          in compliance with Chapter 13.2, 13.6 and 13.9, Vol. 1, SDM.
 */
static __unused void xsave_rqmid_23641_init_and_modified_optimizations_005(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID.1:ECX.XSAVE[bit 26], and compare with 0b******\n");
	int bit_support;
	bit_support = cpuid_indexed(CPUID_XSAVE_FUC, EXTENDED_STATE_SUBLEAF_1).a;
	bit_support &= SUPPORT_XSAVEOPT;
	if (bit_support == SUPPORT_XSAVEOPT) {
		i++;
		debug_print("The value of CPUID.(EAX=0DH,ECX=0x1):EAX[0] = %#x, so XSAVEOPT Supported.\n", bit_support);
	} else {
		debug_print("The value of CPUID.(EAX=0DH,ECX=0x1):EAX[0] = %#x, "
			"XSAVEOPT not support or disable.", bit_support);
		debug_print("******23641:XSAVE_rqmid_23641_init_and_modified_optimizations_005, "
			"test case Failed******\n");
		return;
	}

	debug_print("******Step2: Check XGETBV to make sure XSAVE feature can be used to manage which state.******\n");
	uint64_t xcr0;
	xgetbv_checking(XCR0_MASK, &xcr0);
	if (xcr0 == STATE_X87) {
		debug_print("The value of xcr0 = 0x%lx, XSAVE feature ONLY can manage X87.\n", xcr0);
	} else {
		debug_print("The value of xcr0 = 0x%lx, if xcr0==0x1 XSAVE feature can manage more state component.\n",
			xcr0);
	}

	debug_print("******Step3: Set CR4.osxsave[bit18] to 1.******\n");
	write_cr4_osxsave(1);
	i++;

	debug_print("******Step4: To XSETBV with ECX = 0 and EDX:EAX = 0x3.******\n");
	xsetbv_checking(0, STATE_X87);
	i++;

	debug_print("******Step5: Execute XSAVEOPT64 instruciton.******\n");
	int r_eax = STATE_X87 | STATE_SSE;
	int r_edx = 0;
	__attribute__((aligned(64)))xsave_area_t xsave_area_created = {0};
	asm volatile("XSAVEOPT64 %[addr]\n\t"
		: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
		: "memory");
	i++;

	debug_print("******Step6: Check the value of xstate_bv[2:0].******\n ");
	u64 state_bv = xsave_area_created.xsave_hdr.xstate_bv;
	state_bv &= (STATE_X87 | STATE_SSE | STATE_AVX);
	if (state_bv == STATE_X87) {
		i++;
		debug_print("The value of xstate_bv = %#lx.\n", state_bv);
		debug_print("******23641:XSAVE init and modified optimizations_005, test case Passed.******\n");
	} else {
		debug_print("The value of xstate_bv = %#lx.\n", state_bv);
		debug_print("******23641:XSAVE init and modified optimizations_005, test case Failed.******\n");
	}
	report("%s", (i == 5), __FUNCTION__);
}

/*
 * Case name: 23642:XSAVE init and modified optimizations_006.
 *
 * Summary: DNG_132105: XSAVE init and modified optimizations.
 *          ACRN hypervisor shall expose XSAVE init and modified optimizations to any VM,
 *          in compliance with Chapter 13.2, 13.6 and 13.9, Vol. 1, SDM.
 */
static __unused void xsave_rqmid_23642_init_and_modified_optimizations_006(void)
{
	u32 i = 0;
	debug_print("******Step1: Get CPUID.1:ECX.XSAVE[bit 26], and compare with 0b******\n");
	int bit_support;
	bit_support = cpuid_indexed(CPUID_XSAVE_FUC, EXTENDED_STATE_SUBLEAF_1).a;
	bit_support &= SUPPORT_XSAVEOPT;
	if (bit_support == SUPPORT_XSAVEOPT) {
		i++;
		debug_print("The value of CPUID.(EAX=0DH,ECX=0x1):EAX[0] = %#x, so XSAVEOPT Supported.\n", bit_support);
	} else {
		debug_print("The value of CPUID.(EAX=0DH,ECX=0x1):EAX[0] = %#x, "
			"XSAVEOPT not support or disable.", bit_support);
		debug_print("******23642:XSAVE_rqmid_23641_init_and_modified_optimizations_006, "
			"test case Failed******\n");
		return;
	}

	debug_print("******Step2 :Execute XGETBV with ECX = 0.******\n");
	uint64_t xcr0;
	xgetbv_checking(XCR0_MASK, &xcr0);

	debug_print("******Step3 :Compare EDX:EAX with 0x01.******\n");
	if (xcr0 == STATE_X87) {
		debug_print("The value of xcr0 = 0x%lx, XSAVE feature ONLY can manage X87.\n", xcr0);
	} else {
		debug_print("The value of xcr0 = 0x%lx, if xcr0==0x1 XSAVE feature can manage more state component.\n",
			xcr0);
	}

	debug_print("******Step4: Set CR4.osxsave[bit18] to 1.******\n");
	write_cr4_osxsave(1);
	i++;

	debug_print("******Step5: To XSETBV with ECX = 0 and EDX:EAX = 0x7.******\n");
	xsetbv_checking(0, STATE_X87 | STATE_SSE | STATE_AVX);
	i++;

	xgetbv_checking(XCR0_MASK, &xcr0);
	if (xcr0 != (STATE_X87 | STATE_SSE | STATE_AVX)) {
		debug_print("Set xcr0 (STATE_X87 | STATE_SSE | STATE_AVX) Failed. xcr0=0x%lx failed.\n ", xcr0);
	}

	debug_print("******Step6 : Malloc memory area to XSAVE Area and set it to 0x0.******\n");
	u32 r_eax = STATE_X87 | STATE_SSE | STATE_AVX;
	u32 r_edx = 0;
	__attribute__((aligned(64)))xsave_area_t xsave_area_created = {0};

	debug_print("******Step7 : Execute  XRSTOR64 mem.******\n");
	asm volatile("XRSTOR64 %[addr]\n\t"
		: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
		: "memory");
	i++;

	u64 state_bv = xsave_area_created.xsave_hdr.xstate_bv;
	state_bv &= (STATE_X87 | STATE_SSE | STATE_AVX);
	debug_print("The value of xstate_bv = %#lx.\n", state_bv);

	debug_print("******Step8: Execute SSE instruciton, e.g. PAVGB.******\n");
	write_cr0(read_cr0() & ~(X86_CR0_EM | X86_CR0_TS));
	write_cr4(read_cr4() | X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT);
	//__attribute__((target("sse"))) sse_union add1;
	//__attribute__((target("sse"))) sse_union add2;
	sse_union add1;
	sse_union add2;
	asm volatile("PAVGB %[add1], %%xmm1" : "=r"(add2.sse) : [add1]"m"(add1) : );
	i++;
	state_bv &= (STATE_X87 | STATE_SSE | STATE_AVX);
	debug_print("The value of xstate_bv = %#lx.\n", state_bv);

	debug_print("******Step9: Set xstate_bv[2:0] to 000b.******\n ");
	state_bv &= 0xfffffffffffffff8;
	debug_print("The value of xstate_bv = %#lx.\n", state_bv);

	debug_print("******Step10: Execute XSAVEOPT64 instruciton.******\n");
	r_eax = STATE_X87 | STATE_SSE | STATE_AVX;
	r_edx = 0;
	asm volatile("XSAVEOPT64 %[addr]\n\t"
		: : [addr]"m"(xsave_area_created), "a"(r_eax), "d"(r_edx)
		: "memory");
	i++;

	debug_print("******Step11: Compare xstate_bv[2:0] with ox2H.******\n");
	state_bv = xsave_area_created.xsave_hdr.xstate_bv;
	if (state_bv == STATE_SSE) {
		i++;
		debug_print("The value of xstate_bv = %#lx.\n", state_bv);
		debug_print("******23642:XSAVE init and modified optimizations_006, test case Passed.******\n");
	} else {
		debug_print("The value of xstate_bv = %#lx.\n", state_bv);
		debug_print("******23642:XSAVE init and modified optimizations_006, test case Failed.******\n");
	}
	report("%s", (i == 7), __FUNCTION__);
}



/**Coding by Li,hexiang.**/

__attribute__((aligned(64))) xsave_area_t st_xsave_area;
static __unused int xsave_checking(xsave_area_t *xsave_array, u64 xcr0)
{
	u32 eax = xcr0;
	u32 edx = xcr0 >> 32;

	asm volatile(ASM_TRY("1f")
		"xsave %[addr]\n\t"
		"1:"
		: : [addr]"m"(*xsave_array), "a"(eax), "d"(edx)
		: "memory");

	return exception_vector();
}

static __unused int xsaves_checking(xsave_area_t *xsave_array, u64 xcr0)
{
	u32 eax = xcr0;
	u32 edx = xcr0 >> 32;

	asm volatile(ASM_TRY("1f")
		"xsaves %[addr]\n\t"
		"1:"
		: : [addr]"m"(*xsave_array), "a"(eax), "d"(edx)
		: "memory");

	return exception_vector();
}

static __unused int xrstors_checking(xsave_area_t *xsave_array, u64 xcr0)
{
	u32 eax = xcr0;
	u32 edx = xcr0 >> 32;

	asm volatile(ASM_TRY("1f")
		"xrstors %[addr]\n\t"
		"1:"
		: : [addr]"m"(*xsave_array), "a"(eax), "d"(edx)
		: "memory");

	return exception_vector();
}



int vmovapd_check(avx_union *avx_data)
{
	asm volatile(ASM_TRY("1f")
		"vmovapd %0, %%ymm0\n\t"
		"1:" : : "m" (*avx_data));
	return exception_vector();
}

static __unused __attribute__((target("avx"))) int execute_avx_test()
{
	ALIGNED(32) avx_union avx1;

	/* EM, TS */
	write_cr0(read_cr0() & ~6);
	/* OSFXSR */
	write_cr4(read_cr4() | 0x600);

	avx1.m[0] = 1.0f;
	avx1.m[1] = 2.0f;
	avx1.m[2] = 3.0f;
	avx1.m[3] = 4.0f;
	avx1.m[4] = 5.0f;
	avx1.m[5] = 6.0f;
	avx1.m[6] = 7.0f;
	avx1.m[7] = 8.0f;

	return vmovapd_check(&avx1);
}

static __unused __attribute__((target("sse")))
void execute_sse_test(unsigned sse_data)
{
	sse_union sse;

	sse.u[0] = sse_data;
	sse.u[1] = sse_data + 1;
	sse.u[2] = sse_data + 2;
	sse.u[3] = sse_data + 3;

	write_cr0(read_cr0() & ~6);
	write_cr4(read_cr4() | 0x200);

	asm volatile("movapd %0, %%xmm0" : : "m"(sse));
}

static __unused __attribute__((target("sse")))
void execute_sse_test1(void)
{
	write_cr0(read_cr0() & ~6);
	write_cr4(read_cr4() | 0x200);

	asm volatile("movapd %%xmm1, %%xmm2" : :);
}


/*
 * @brief case name: XSAVE general support_010
 *
 * Summary: Check CPUID(EAX=0xd, ECX=1),the return result EAX[31:4] are reserved, so is must be 0.
 */
static __unused void xsave_rqmid_22826_check_reserved_bit()
{
	u32 eax = cpuid_indexed(CPUID_XSAVE_FUC, 1).a;
	u32 i = 0;

	if ((cpuid(1).c & CPUID_1_ECX_XSAVE) != 0) {
		i++;
	}
	if ((eax & ~(0xf)) == 0) {
		i++;
	}
	report("%s", (i == 2), __FUNCTION__);
}


/*
 * Case name:XSAVE expose AVX support_001
 *
 * Summary:Disable AVX state ,clear XCR0[bit 2], then execute AVX instruction,
 *         this cause #UD exception; Enable AVX state ,set XCR0[bit 2],
 *         then execute AVX instruction, check AVX data register value has changed.
 */
static __unused void xsave_rqmid_22867_expose_avx_support()
{
	u32 i = 0;
	u64 xcr0;
	u32 j;

	write_cr4_osxsave(1);

	xcr0 = STATE_X87 | STATE_SSE;
	if (xsetbv_checking(XCR0_MASK, xcr0) == NO_EXCEPTION) {
		i++;
	}

	if (execute_avx_test() == UD_VECTOR) {
		i++;
	}

	memset(&st_xsave_area, 0, sizeof(st_xsave_area));
	if (xsave_checking(&st_xsave_area, STATE_X87 | STATE_SSE | STATE_AVX) == NO_EXCEPTION) {
		i++;
	}

	/* check ymm0 is 0 */
	u32 k = 0;
	for (j = 0; j < 16; j++) {
		if (st_xsave_area.ymm[j] == 0) {
			k++;
		}
	}
	if (k == 16) {
		i++;
	}

	xcr0 = STATE_X87 | STATE_SSE | STATE_AVX;
	if (xsetbv_checking(XCR0_MASK, xcr0) == NO_EXCEPTION) {
		i++;
	}

	if (execute_avx_test() == NO_EXCEPTION) {
		i++;
	}

	memset(&st_xsave_area, 0, sizeof(st_xsave_area));
	if (xsave_checking(&st_xsave_area, STATE_X87 | STATE_SSE | STATE_AVX) == NO_EXCEPTION) {
		i++;
	}


	/* check ymm0 is not 0 */
	for (j = 0; j < 16; j++) {
		if (st_xsave_area.ymm[j] != 0) {
			i++;
			break;
		}
	}

	report("%s", (i == 8), __FUNCTION__);
}


static __unused void test_xsetbv_at_ring3(const char *msg)
{
	uint64_t test_bits;
	test_bits = STATE_X87 | STATE_SSE;
	report("xsave_rqmid_22844_xsetbv_at_ring3",
		xsetbv_checking(XCR0_MASK, test_bits) == GP_VECTOR);
}

/*
 * @brief case name: XSAVE general support_020
 *
 * Summary:Use XGETBV set XCR0= 0x3 at ring0 mode, can set succes;
 *         Use XGETBV set XCR0= 0x3 at ring3 mode, can not set ,cause #GP exception.
 */
static __unused void xsave_rqmid_22844_xsetbv_at_ring3(void)
{
	uint64_t test_bits;
	u32 i = 0;

	write_cr4_osxsave(1);

	test_bits = STATE_X87 | STATE_SSE;
	if (xsetbv_checking(XCR0_MASK, test_bits) == NO_EXCEPTION) {
		i++;
	}

	u64 xcr0;
	if (xgetbv_checking(XCR0_MASK, &xcr0) == NO_EXCEPTION) {
		if (test_bits == xcr0) {
			i++;
		}
	}

	if (i == 2) {
		do_at_ring3(test_xsetbv_at_ring3, "UMIP=0, CPL=3\n");
	} else {
		report("%s", false, __FUNCTION__);
	}
}

/*
 * @brief case name: XSAVE expose SSE support_001
 *
 * Summary:Under 64bit Mode, Disable SSE state ,clear XCR0[bit 1], then execute SSE instruction,
 *         check SSE data register value do not change; Enable SSE state ,set XCR0[bit 1],
 *         then execute SSE instruction, check SSE data register value has changed.
 */
static __unused void xsave_rqmid_22866_expose_sse_support()
{
#define XSAVE_AREA_XMM0_POS 160
	u32 i = 0;
	u64 xcr0;
	u32 j;

	write_cr4_osxsave(1);

	xcr0 = STATE_X87;
	if (xsetbv_checking(XCR0_MASK, xcr0) == NO_EXCEPTION) {
		i++;
	}

	execute_sse_test(1);

	memset(&st_xsave_area, 0, sizeof(st_xsave_area));
	if (xsave_checking(&st_xsave_area, STATE_X87 | STATE_SSE) == NO_EXCEPTION) {
		i++;
	}

	/* check xmm0 is 0 */
	u32 k = 0;
	for (j = XSAVE_AREA_XMM0_POS; j < (XSAVE_AREA_XMM0_POS + 16); j++) {
		if (st_xsave_area.fpu_sse[j] == 0) {
			k++;
		}
	}
	if (k == 16) {
		i++;
	}

	xcr0 = STATE_X87 | STATE_SSE;
	if (xsetbv_checking(XCR0_MASK, xcr0) == NO_EXCEPTION) {
		i++;
	}

	execute_sse_test(1);

	memset(&st_xsave_area, 0, sizeof(st_xsave_area));
	if (xsave_checking(&st_xsave_area, STATE_X87 | STATE_SSE) == NO_EXCEPTION) {
		i++;
	}

	/* check xmm0 is not 0 */
	for (j = XSAVE_AREA_XMM0_POS; j < (XSAVE_AREA_XMM0_POS + 16); j++) {
		if (st_xsave_area.fpu_sse[j] != 0) {
			i++;
			break;
		}
	}

	report("%s", (i == 6), __FUNCTION__);
}

static __unused void add_fpu(double *p, double *q)
{
	asm volatile("fld (%%rdi)\n\t"	// use double extended float
		"fld (%%rsi)\n\t"		// use double extended float
		"fadd %%st(0), %%st(1)\n\t"	// add st(0) and st(1)
		// automatically truncate to single-float, write to the first arg and pop the value
		"fstp (%%rdi)\n\t"
		:::);
}

static __unused bool fpu_probe_without_cpuid(void)
{
	unsigned long cr0;
	u16 fsw, fcw;
	fsw = fcw = 0xffff;
	cr0 = read_cr0();
	cr0 &= ~(X86_CR0_TS | X86_CR0_EM);
	write_cr0(cr0);
	asm volatile("fninit; fnstsw %0;fnstcw %1":"+m"(fsw), "+m"(fcw));
	return ((fsw == 0) && ((fcw & 0x103f) == 0x003f));
}

static __unused void exec_x87_fpu(void)
{
	double f = 30.00f;
	double f1 = 60.00f;
	add_fpu(&f, &f1);
}
/*
 * @brief case name: XSAVE general support_008
 *
 * Summary: Enable SSE,X87 state,check CPUID(EAX=0xd, ECX=0).EBX, get the xsave area size,
 *          this is the sum of legacy region area size and head area size.
 *          Because XSTATE_BV is at the start of Head area,
 *          then find XSTATE_BV can confirm the legacy region area size,
 *          so the Head area size = total size - legacy area size.
 */
static __unused void xsave_rqmid_22911_check_xsave_head_size()
{
	u32 head_size = 64;
	uint64_t test_bits;
	u32 total_size;
	u32 i = 0;
	uint64_t ret = 0x0UL;

	write_cr4_osxsave(1);

	if (check_cpuid_1_ecx(CPUID_1_ECX_XSAVE) == 0) {
		debug_print("CPUID_1_ECX_XSAVE failed. \n");
	}

	if (check_cpuid_1_ecx(CPUID_1_ECX_OSXSAVE) == 0) {
		debug_print("CPUID_1_ECX_OSXSAVE failed. \n");
	}

	ret = get_supported_xcr0();
	if (ret == SUPPORT_XCR0) {
		i++;
	} else {
		debug_print("Step1: SUPPORT_XCR0 != get_supported_xcr0()=0x%lx failed. \n", ret);
	}

	test_bits = STATE_X87 | STATE_SSE;
	ret = xsetbv_checking(XCR0_MASK, test_bits);
	if (ret == NO_EXCEPTION) {
		i++;
	} else {
		debug_print("Step2: xsetbv_checking failed, ret=0x%lx\n", ret);
	}

	total_size = cpuid_indexed(CPUID_XSAVE_FUC, 0).b;
	if (total_size == 576) {
		i++;
	} else {
		debug_print("Step3: total_size failed, total_size=0x%x\n", total_size);
	}

	exec_x87_fpu();

	int ret1 = 0;
	memset(&st_xsave_area, 0, sizeof(st_xsave_area));
	ret1 = xsave_checking(&st_xsave_area, (STATE_X87 | STATE_SSE));
	if (ret1 == NO_EXCEPTION) {
		i++;
	} else {
		debug_print("xsave_checking STATE_X87 | STATE_SSE failed.ret1=0x%x \n", ret1);
	}

	ret = get_supported_xcr0();
	debug_print("after test_bits, xcr0=0x%lx RFBM =0x%lx \n", ret, (ret & (STATE_X87 | STATE_SSE)));

	if (st_xsave_area.xsave_hdr.xstate_bv == 0x3) {
		i++;
	} else {
		debug_print("st_xsave_area.xsave_hdr.xstate_bv=0x%lx failed. \n", st_xsave_area.xsave_hdr.xstate_bv);
	}
	debug_print("i=%d, total_size=%d,  sizeof(st_xsave_area.fpu_sse)=%ld, head_size=%d \n",
		i, total_size, sizeof(st_xsave_area.fpu_sse), head_size);
	report("%s",
		((i == 5) && (head_size == (total_size - sizeof(st_xsave_area.fpu_sse)))), __FUNCTION__);
}

/*
 * @brief case name: XSAVE expose x87 support_002
 *
 * Summary:Under 64bit Mode, execute CPUID instruction to get user supported state components,
 *         and confirm SSE and X87 state supported.
 */
static __unused void xsave_rqmid_22846_x87_support()
{
	u64 xcr0 = get_supported_xcr0();

	report("%s",
		(xcr0 & (STATE_X87 | STATE_SSE)) == (STATE_X87 | STATE_SSE), __FUNCTION__);
}

/*
 * @brief case name: XSAVE general support_014
 *
 * Summary: Under 64bit Modd, execute CPUID.(EAX=0DH,ECX=0x2):EBX,
 *          the return result EBX enumerates the offset of the section used for state component 2 ,
 *          so the offset = total size - the user state component 2 size.
 */
static __unused void xsave_rqmid_22830_check_xsave_area_offset()
{
	__unused u32 head_size = 64;
	uint64_t test_bits;
	u32 avx_area_size;
	u32 total_size;
	u32 i = 0;

	write_cr4_osxsave(1);

	if (get_supported_xcr0() == SUPPORT_XCR0) {
		i++;
	}

	test_bits = SUPPORT_XCR0;
	if (xsetbv_checking(XCR0_MASK, test_bits) == NO_EXCEPTION) {
		i++;
	}

	total_size = cpuid_indexed(CPUID_XSAVE_FUC, 0).b;
	avx_area_size = cpuid_indexed(CPUID_XSAVE_FUC, 2).a;
	if ((total_size == 0x340) && (avx_area_size == 0x100)) {
		i++;
	}

	u32 avx_area_offset = cpuid_indexed(CPUID_XSAVE_FUC, 2).b;

	report("%s",	\
		((i == 3) && (avx_area_offset == (total_size - avx_area_size))), __FUNCTION__);
}


/*
 * @brief case name: XSAVE supervisor state components_001
 *
 * Summary: EAX[3] enumerates support for XSAVES, XRSTORS,
 * and the IA32_XSS MSR. If EAX[3] = 0, execution of XSAVES or XRSTORS causes a #UD;
 * an attempt to access the IA32_XSS MSR using RDMSR or WRMSR causes a general-protection exception (#GP).
 */
static __unused void xsave_rqmid_22825_supervisor_state_components_001(void)
{
	u64 val = 0x100;
	u32 index = 0xDA0;
	u32 a = val, d = val >> 32;
	int i = 0;
	int ret1 = 0;

	/*Step1: enable xsave feature set.*/
	write_cr4_osxsave(1);
	i++;

	/*Step2:Check CPU supported for XSAVES, XRSTORS instructions.*/
	u32 bit_support = cpuid_indexed(CPUID_XSAVE_FUC, EXTENDED_STATE_SUBLEAF_1).a;
	if ((bit_support & SUPPORT_XSAVES_XRTORS) == 0) {
		debug_print("Step2: SUPPORT_XSAVES_XRTORS ok.\n");
		i++;
	} else {
		debug_print("Step2: SUPPORT_XSAVES_XRTORS failed.\n");
	}

	/*Step3:Confirm hide enable supervisor state component, write MSR register IA32_XSS*/
	val = 0x100;
	index = 0xDA0;
	a = val;
	d = val >> 32;
	asm volatile(ASM_TRY("1f")
		"wrmsr\n\t"
		"1:"
		: : "a"(a), "d"(d), "c"(index) : "memory");
	ret1 = exception_vector();
	if (ret1 == 13) {
		i++;
		debug_print("\n Step3: wrmsr(0xDA0, 0x100) ok. exception_vector()=%d \n", ret1);
	} else {
		debug_print("\n Step3: wrmsr(0xDA0, 0x100) fail. exception_vector()=%d \n", ret1);
	}

	/*Step4:Comfirm processor unsupported read MSR register IA32_XSS*/
	index = 0xDA0;
	asm volatile(ASM_TRY("1f")
		"rdmsr\n\t"
		"1:"
		: "=a"(a), "=d"(d) : "c"(index) : "memory");
	ret1 = exception_vector();
	if (ret1 == 13) {
		i++;
		debug_print("\n Step4: rdmsr(0xDA0) ok. exception_vector()=%d \n", ret1);
	} else {
		debug_print("\n Step4: rdmsr(0xDA0) fail. exception_vector()=%d\n", ret1);
	}

	/*Step5:Check processor unsupported for XSAVES*/
	memset(&st_xsave_area, 0, sizeof(st_xsave_area));
	ret1 = xsaves_checking(&st_xsave_area, (STATE_X87 | STATE_SSE));
	if (ret1 == UD_VECTOR) {
		i++;
		debug_print("Step5:xsaves_checking STATE_X87 | STATE_SSE ok.exception_vector()=0x%x \n", ret1);
	} else {
		debug_print("Step5:xsaves_checking STATE_X87 | STATE_SSE failed.exception_vector()=0x%x \n", ret1);
	}

	/*Step6:Check processor unsupported for XRESTORS*/
	ret1 = xrstors_checking(&st_xsave_area, (STATE_X87 | STATE_SSE));
	if (ret1 == UD_VECTOR) {
		i++;
		debug_print("Step6:xrstors_checking STATE_X87 | STATE_SSE ok.exception_vector()=0x%x \n", ret1);
	} else {
		debug_print("Step6:xrstors_checking STATE_X87 | STATE_SSE failed.exception_vector()=0x%x \n", ret1);
	}
	report("%s", (i == 6), __FUNCTION__);
}


static __unused u64 get_init_xinuse(u64 eax_addr, u64 edx_addr)
{
	u64 xinuse = 0;
	u32 volatile v, v1;
	u64 v3, v4;
	v = *((u32 volatile *)eax_addr);
	v1 = *((u32 volatile *)edx_addr);
	v3 = v;
	v4 = v1;
	xinuse = v3 | (v4 << 32UL);
	return xinuse;
}

/*
 * @brief case name: XSAVE XINUSE[bit 2:0] initial state following INIT_001
 *
 * Summary: After AP receives first INIT, set the value of XINUSE with executing SSE
 * instruction; executing XGETBV shall get the same XINUSE value after second INIT.
 * And the test case pass when bit 1 of (XINUSE & XCR0) equals with 1.
 * This case constructs the environment with XINUSE[1] is 1.
 */
static __unused void xsave_rqmid_23635_XINUSE_bit2to0_initial_state_following_INIT_01(void)
{
	u32 chk = 0;
	u64 xinuse = 0;
	u64 xinuse1 = 0;
	xinuse = set_xinuse;
	debug_print("\n --->bp: before send sipi, XINUSE=%lx %d\n", xinuse, ap_start_count);

	/*send sipi to ap*/
	send_sipi();

	xinuse1 = set_xinuse;
	debug_print("\n --->ap: after send sipi, XINUSE=%lx %d\n", xinuse1, ap_start_count);

	if (xinuse == xinuse1) {
		chk++;
	}
	if ((xinuse & STATE_SSE) == STATE_SSE) {
		chk++;
	}

	report("%s", (chk == 2), __FUNCTION__);
}

/*
 * @brief case name: XSAVE XINUSE[bit 2:0] initial state following INIT_002
 *
 * Summary: At BP executing 1st instruction and AP receives the first init,
 * set the value of XINUSE with executing SSE instruction;
 * execute XGETBV to get XINUSE value, and the two XINUSE values should be equal.
 */
static __unused void xsave_rqmid_37093_XINUSE_bit2to0_initial_state_following_INIT_02(void)
{
	u32 chk = 0;
	u64 xinuse_bp = 0;
	u64 xinuse_ap = 0;
	xinuse_bp = get_init_xinuse(0x6000, 0x6004);
	debug_print("\n --->bp: XINUSE=%lx\n", xinuse_bp);

	xinuse_ap = init_xinuse;
	debug_print("\n --->ap: XINUSE=%lx\n", xinuse_ap);

	if (xinuse_bp == xinuse_ap) {
		chk++;
	}

	report("%s", (chk == 1), __FUNCTION__);
}

static void print_case_list(void)
{
	/*_x86_64__*/
	printf("\t\t XSAVE feature case list:\n\r");
#ifdef IN_NATIVE
	printf("\t\t Case ID:%d case name:%s\n\r", 28468u, "XSAVE physical compaction extensions_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28386u, "XSAVE physical general support_001 ");
	printf("\t\t Case ID:%d case name:%s\n\r", 28385u, "XSAVE physical x87 support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28388u, "XSAVE physical SSE support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28390u, "XSAVE physical AVX support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28392u, "XSAVE physical init and modified optimizations_001");
#else
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 23635u, "XSAVE XINUSE[bit 2:0] initial state following INIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37093u, "XSAVE XINUSE[bit 2:0] initial state following INIT_002");
#endif
	printf("\t\t Case ID:%d case name:%s\n\r", 23633u, "XSAVE hide AVX-512 support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 23642u, "XSAVE init and modified optimizations_006");
	printf("\t\t Case ID:%d case name:%s\n\r", 22826u, "XSAVE general support_010");
	printf("\t\t Case ID:%d case name:%s\n\r", 22867u, "XSAVE expose AVX support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 22844u, "XSAVE general support_020");
	printf("\t\t Case ID:%d case name:%s\n\r", 22866u, "XSAVE expose SSE support_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 22911u, "XSAVE general support_020");
	printf("\t\t Case ID:%d case name:%s\n\r", 22846u, "XSAVE expose x87 support_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 22830u, "XSAVE general support_014");
	printf("\t\t Case ID:%d case name:%s\n\r", 22825u, "XSAVE supervisor state components_001");
#endif
	printf("\t\t \n\r \n\r");
}

int main(void)
{
	setup_idt();
	setup_ring_env();
	asm volatile("fninit");

	print_case_list();

#ifdef IN_NATIVE
	xsave_rqmid_28468_physical_compaction_extensions_AC_001();
	xsave_rqmid_28386_physical_general_support_AC_001();
	xsave_rqmid_28385_physical_x87_support_AC_001();
	xsave_rqmid_28388_physical_sse_support_AC_001();
	xsave_rqmid_28390_physical_avx_support_AC_001();
	xsave_rqmid_28392_physical_init_and_modified_optimizations_AC_001();
#else
#ifdef IN_NON_SAFETY_VM
	xsave_rqmid_23635_XINUSE_bit2to0_initial_state_following_INIT_01();
	xsave_rqmid_37093_XINUSE_bit2to0_initial_state_following_INIT_02();
#endif
	xsave_rqmid_23633_hide_avx_512_support_001();
	xsave_rqmid_23642_init_and_modified_optimizations_006();
	xsave_rqmid_22826_check_reserved_bit();
	xsave_rqmid_22867_expose_avx_support();
	xsave_rqmid_22844_xsetbv_at_ring3();
	xsave_rqmid_22866_expose_sse_support();
	xsave_rqmid_22911_check_xsave_head_size();
	xsave_rqmid_22846_x87_support();
	xsave_rqmid_22830_check_xsave_area_offset();
	xsave_rqmid_22825_supervisor_state_components_001();
#endif

	return report_summary();
}
