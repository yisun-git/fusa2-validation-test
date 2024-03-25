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
#include "delay.h"
#include "fwcfg.h"

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

static uint64_t xcr0_enter_1st_init, xcr1_enter_1st_init;

static uint64_t xcr0_init_write_7_to_xcr0, xcr1_init_after_sse_exe;
static uint64_t xcr0_enter_2nd_init, xcr1_enter_2nd_init;
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

__unused static void notify_ap_modify_and_read_init_value(int case_id)
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

#ifdef __i386__
void ap_main(void)
{
	asm volatile ("pause");
}

#elif __x86_64__

typedef void (*ap_init_value_modify)(void);
__unused static void ap_init_value_process(ap_init_value_modify modify_init_func)
{
	if (need_modify_init_value) {
		need_modify_init_value = 0;
		modify_init_func();
		wait_ap = 1;
	} else {
		if (cur_case_id == 23151) {
			xcr0_enter_2nd_init = asm_read_xcr(0);
		} else if (cur_case_id == 23635) {
			xcr1_enter_2nd_init = asm_read_xcr(1);
		}
		wait_ap = 1;
	}
}

__unused static void modify_cr4_osxsave_init_value()
{
	asm_write_cr4_osxsave(1);
}

__unused static void modify_xcr0_init_value()
{
	asm_write_xcr(0, STATE_X87 | STATE_SSE | STATE_AVX);
	xcr0_init_write_7_to_xcr0 = asm_read_xcr(0);
}

__unused static void modify_xinuse_bit2to0_initial_state()
{
	asm_write_xcr(0, STATE_X87 | STATE_SSE | STATE_AVX);
	asm volatile("movapd %xmm1, %xmm2");
	xcr1_init_after_sse_exe = asm_read_xcr(1);
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 23154:
		fp = modify_cr4_osxsave_init_value;
		ap_init_value_process(fp);
		break;
	case 23151:
		fp = modify_xcr0_init_value;
		ap_init_value_process(fp);
	case 23635:
		fp = modify_xinuse_bit2to0_initial_state;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif

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
	asm volatile("fldl (%%rdi)\n\t"	// use double extended float
		"fldl (%%rsi)\n\t"		// use double extended float
		"fadd %%st(0), %%st(1)\n\t"	// add st(0) and st(1)
		// automatically truncate to single-float, write to the first arg and pop the value
		"fstpl (%%rdi)\n\t"
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


static __unused uint32_t get_xsave_support(void)
{
	uint32_t xsave_support;
	xsave_support = cpuid(1).c;
	xsave_support &= CPUID_1_ECX_XSAVE;

	return xsave_support;
}

static __unused uint32_t get_osxsave_support(void)
{
	uint32_t osxsave_support;
	osxsave_support = cpuid(1).c;
	osxsave_support &= CPUID_1_ECX_OSXSAVE;

	return osxsave_support;
}

static __unused uint32_t get_xsaveopt_support(void)
{
	uint32_t xsaveopt_support;
	xsaveopt_support = cpuid_indexed(CPUID_XSAVE_FUC, EXTENDED_STATE_SUBLEAF_1).a;
	xsaveopt_support &= SUPPORT_XSAVEOPT;

	return xsaveopt_support;
}

static __unused uint32_t get_xsavec_support(void)
{
	uint32_t xsavec_support;
	xsavec_support = cpuid_indexed(CPUID_XSAVE_FUC, EXTENDED_STATE_SUBLEAF_1).a;
	xsavec_support &= SUPPORT_XSAVEC;

	return xsavec_support;
}

static __unused uint32_t get_xcr1_support(void)
{
	uint32_t read_xcr1_support;
	read_xcr1_support = cpuid_indexed(CPUID_XSAVE_FUC, EXTENDED_STATE_SUBLEAF_1).a;
	read_xcr1_support &= SUPPORT_XGETBV;

	return read_xcr1_support;
}

static __unused uint64_t get_supported_user_states(void)
{
	uint64_t supported_user_states_low = cpuid_indexed(CPUID_XSAVE_FUC, 0).a;
	uint64_t supported_user_states_hi = cpuid_indexed(CPUID_XSAVE_FUC, 0).d;
	uint64_t supported_user_states = (supported_user_states_hi << 32U) | supported_user_states_low;

	return supported_user_states;
}

static __unused uint64_t get_supported_supervisor_states(void)
{
	uint64_t supported_supervisor_states_low = cpuid_indexed(CPUID_XSAVE_FUC, 1).c;
	uint64_t supported_supervisor_states_hi = cpuid_indexed(CPUID_XSAVE_FUC, 1).d;
	uint64_t supported_supervisor_states = (supported_supervisor_states_hi << 32U) |
						supported_supervisor_states_low;

	return supported_supervisor_states;
}

static __unused struct xsave_exception xsaveopt_exception_checking(struct xsave_area *area_ptr, uint64_t mask)
{
	asm volatile(ASM_TRY("1f")
			"xsaveopt %[addr]\n\t"
			"1:"
			: : [addr]"m" (*(area_ptr)),
			"d" ((uint32_t)(mask >> 32U)),
			"a" ((uint32_t)mask) :
			"memory");

	struct xsave_exception ret;
	ret.vector = exception_vector();
	ret.error_code = exception_error_code();
	return ret;
}

static __unused struct xsave_exception xsave_exception_checking(struct xsave_area *area_ptr, uint64_t mask)
{
	asm volatile(ASM_TRY("1f")
			"xsave %[addr]\n\t"
			"1:"
			: : [addr]"m" (*(area_ptr)),
			"d" ((uint32_t)(mask >> 32U)),
			"a" ((uint32_t)mask) :
			"memory");

	struct xsave_exception ret;
	ret.vector = exception_vector();
	ret.error_code = exception_error_code();
	return ret;
}

static __unused struct xsave_exception xsavec_exception_checking(struct xsave_area *area_ptr, uint64_t mask)
{
	asm volatile(ASM_TRY("1f")
			"xsavec %[addr]\n\t"
			"1:"
			: : [addr]"m" (*(area_ptr)),
			"d" ((uint32_t)(mask >> 32U)),
			"a" ((uint32_t)mask) :
			"memory");

	struct xsave_exception ret;
	ret.vector = exception_vector();
	ret.error_code = exception_error_code();
	return ret;
}

static __unused struct xsave_exception xrstor_exception_checking(struct xsave_area *area_ptr, uint64_t mask)
{
	asm volatile(ASM_TRY("1f")
			"xrstor %[addr]\n\t"
			"1:"
			: : [addr]"m" (*(area_ptr)),
			"d" ((uint32_t)(mask >> 32U)),
			"a" ((uint32_t)mask) :
			"memory");

	struct xsave_exception ret;
	ret.vector = exception_vector();
	ret.error_code = exception_error_code();
	return ret;
}

static __unused struct xsave_exception xsetbv_exception_checking(uint32_t reg, uint64_t val)
{
	asm volatile(ASM_TRY("1f")
			"xsetbv\n\t"
			"1:"
			: : "c"(reg), "a"((uint32_t)val), "d"((uint32_t)(val >> 32U)));

	struct xsave_exception ret;
	ret.vector = exception_vector();
	ret.error_code = exception_error_code();
	return ret;
}

static __unused struct xsave_exception xgetbv_exception_checking(uint32_t reg)
{
	uint32_t xcrl, xcrh;

	asm volatile(ASM_TRY("1f")
			"xgetbv\n\t"
			"1:"
			: "=a"(xcrl), "=d"(xcrh)
			: "c"(reg));

	struct xsave_exception ret;
	ret.vector = exception_vector();
	ret.error_code = exception_error_code();
	return ret;
}

static __unused void xsave_reset_xinuse(void)
{
	/* Set XCR0 to 7 so that XINUSE[1] (SSE) and XINUSE[2] (AVX) can be cleared. */
	asm_write_xcr(0, STATE_X87 | STATE_SSE | STATE_AVX);

	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));
	area_test.xsave_hdr.hdr.xcomp_bv = XSAVE_COMPACTED_FORMAT;

	/* Execute XRSTOR instruction with EDX:EAX = 6 to clear XINUSE[2:1] */
	asm_xrstor(&area_test, STATE_SSE | STATE_AVX);
}

static __unused void xsave_clean_up_env(void)
{
	asm_write_cr4_osxsave(1);

	xsave_reset_xinuse();
	debug_print("Clear XINUSE[1] (SSE) and XINUSE[2] (AVX): XCR1 = 0x%lx \n", asm_read_xcr(1));

	asm_write_xcr(0, STATE_X87);
	debug_print("Set XCR0 to 1: XCR0 = 0x%lx \n", asm_read_xcr(0));

	asm_write_cr4_osxsave(0);
	debug_print("Clear CR4.OSXSAVE[18]: CR4.OSXSAVE[18] = 0x%lx \n", read_cr4() & X86_CR4_OSXSAVE);

	uint64_t cr0 = read_cr0();
	uint64_t clr_bits_mask = CR0_AM_MASK | CR0_TS_MASK;
	write_cr0(cr0 & (~clr_bits_mask));
	debug_print("Clear CR0.AM: CR0.AM = 0x%lx \n", read_cr0() & CR0_AM_MASK);
	debug_print("Clear CR0.TS: CR0.TS = 0x%lx \n", read_cr0() & CR0_TS_MASK);

	uint64_t eflags = read_rflags();
	write_rflags(eflags & (~X86_EFLAGS_AC));
	debug_print("Clear EFLAGS.AC: EFLAGS.AC = 0x%lx \n", read_rflags() & X86_EFLAGS_AC);
}

/*
 * @brief case name: XSAVE_CR4_initial_state_following_start-up_001
 *
 * Summary: ACRN hypervisor shall set initial guest CR4.OSXSAVE to 0 following start-up.
 */
static __unused void xsave_rqmid_23153_xsave_cr4_initial_state_following_startup_001(void)
{
	uint64_t volatile cr4_startup = *((uint64_t volatile *)XSAVE_STARTUP_CR4_ADDR);
	uint64_t cr4_osxsave_startup = cr4_startup & X86_CR4_OSXSAVE;

	debug_print("Check CR4.OSXSAVE initial state following start-up \n");
	debug_print("cr4_osxsave_startup = 0x%lx, XSAVE_EXPECTED_STARTUP_CR4_OSXSAVE = 0x%lx \n",
		cr4_osxsave_startup, XSAVE_EXPECTED_STARTUP_CR4_OSXSAVE);

	report("%s", (cr4_osxsave_startup == XSAVE_EXPECTED_STARTUP_CR4_OSXSAVE), __FUNCTION__);
}

/*
 * @brief case name: XSAVE_XCR0_initial_state_following_start-up_001
 *
 * Summary: ACRN hypervisor shall set initial guest XCR0 to 1H following start-up.
 */
static __unused void xsave_rqmid_22853_xsave_xcr0_initial_state_following_startup_001(void)
{
	uint32_t volatile xcr0_startup_low = *((uint32_t volatile *)XSAVE_STARTUP_XCR0_LOW_ADDR);
	uint32_t volatile xcr0_startup_hi = *((uint32_t volatile *)XSAVE_STARTUP_XCR0_HI_ADDR);
	uint64_t xcr0_startup = ((uint64_t)xcr0_startup_hi << 32U) | (uint64_t)xcr0_startup_low;

	debug_print("Check XCR0 initial state following start-up: \n");
	debug_print("xcr0_startup = 0x%lx, XSAVE_EXPECTED_STARTUP_XCR0 = 0x%lx \n",
		xcr0_startup, XSAVE_EXPECTED_STARTUP_XCR0);

	report("%s", (xcr0_startup == XSAVE_EXPECTED_STARTUP_XCR0), __FUNCTION__);
}

/*
 * @brief case name: XSAVE XINUSE[bit 2:0] initial state following start-up_001
 *
 * Summary: ACRN hypervisor shall set initial guest XINUSE[bit 2:0] to 1H following start-up. SDM Vol1, Chapter 13.6.
 *
 * As XCR1 is equal to XCR0 & XINUSE, XINUSE[2:0] is equal to XCR1 when XCR0 is set to 7.
 */
static __unused void xsave_rqmid_23634_xinuse_bit2to0_initial_state_following_startup_001(void)
{
	uint32_t volatile xcr1_startup_low = *((uint32_t volatile *)XSAVE_STARTUP_XCR1_LOW_ADDR);
	uint32_t volatile xcr1_startup_hi = *((uint32_t volatile *)XSAVE_STARTUP_XCR1_HI_ADDR);
	uint64_t xcr1_startup = ((uint64_t)xcr1_startup_hi << 32U) | (uint64_t)xcr1_startup_low;

	/* As XCR1 is equal to XCR0 & XINUSE, XINUSE[2:0] is equal to XCR1 when XCR0 is set to 7. */
	uint64_t xinuse_startup = xcr1_startup;
	debug_print("Check XINUSE initial state following start-up: \n");
	debug_print("xinuse_startup = 0x%lx, XSAVE_EXPECTED_STARTUP_XINUSE = 0x%lx \n",
		xinuse_startup, XSAVE_EXPECTED_STARTUP_XINUSE);

	report("%s", (xinuse_startup == XSAVE_EXPECTED_STARTUP_XINUSE), __FUNCTION__);
}

/*
 * @brief case name: XSAVE_CR4_initial_state_following_INIT_001
 *
 * Summary: ACRN hypervisor shall set initial guest CR4.OSXSAVE to 0 following INIT.
 */
static __unused void xsave_rqmid_23154_xsave_cr4_initial_state_following_init_001(void)
{
	uint64_t volatile cr4_init = *((uint64_t volatile *)XSAVE_INIT_CR4_ADDR);
	uint64_t cr4_osxsave_init = cr4_init & X86_CR4_OSXSAVE;
	bool is_pass = true;

	debug_print("Check CR4.OSXSAVE initial state following INIT \n");
	debug_print("cr4_osxsave_init = 0x%lx, XSAVE_EXPECTED_INIT_CR4_OSXSAVE = 0x%lx \n",
		cr4_osxsave_init, XSAVE_EXPECTED_INIT_CR4_OSXSAVE);
	if (cr4_osxsave_init != XSAVE_EXPECTED_INIT_CR4_OSXSAVE) {
		is_pass = false;
	}

	notify_ap_modify_and_read_init_value(23154);

	cr4_init = *((uint64_t volatile *)XSAVE_INIT_CR4_ADDR);
	cr4_osxsave_init = cr4_init & X86_CR4_OSXSAVE;

	if (cr4_osxsave_init != XSAVE_EXPECTED_INIT_CR4_OSXSAVE) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: XSAVE_XCR0_initial_state_following_INIT_002
 *
 * Summary: ACRN hypervisor shall keep guest XCR0 unchanged following INIT.
 *
 * This case verifies the XCR0 following the first INIT received by the AP.
 * It shall be equal to 1H since ACRN hypervisor shall set initial guest XCR0 to 1H following start-up.
 */
static __unused void xsave_rqmid_36768_xcr0_initial_state_following_init_002(void)
{
	xcr0_enter_1st_init = *(volatile uint64_t *)XSAVE_XCR0_LOW;

	printf("Check XCR0 following the first INIT received by the AP: \n");
	printf("XCR0 following the first INIT = 0x%lx, expected value = 0x%lx \n",
		xcr0_enter_1st_init, XSAVE_EXPECTED_STARTUP_XCR0);

	report("%s", (xcr0_enter_1st_init == XSAVE_EXPECTED_STARTUP_XCR0), __FUNCTION__);
}

/*
 * @brief case name: XSAVE XINUSE[bit 2:0] initial state following INIT_002
 *
 * Summary: ACRN hypervisor shall keep guest XINUSE[bit 2:0] unchanged following INIT. SDM Vol1, Chapter 13.6.
 *
 * This case verifies the XINUSE[bit 2:0] following the first INIT received by the AP.
 * It shall be equal to 1H since ACRN hypervisor shall set initial guest XINUSE[bit 2:0] to 1H following start-up.
 */
static __unused void xsave_rqmid_24108_xinuse_bit2to0_initial_state_following_init_002(void)
{
	xcr1_enter_1st_init = *(volatile uint64_t *)XSAVE_XCR1_LOW;
	debug_print("Check XINUSE[bit 2:0] following the first INIT received by the AP: \n");
	debug_print("XINUSE[bit 2:0] following the first INIT = 0x%lx, expected value = 0x%lx \n",
		xcr1_enter_1st_init, XSAVE_EXPECTED_INIT_XINUSE);

	report("%s", (xcr1_enter_1st_init == XSAVE_EXPECTED_INIT_XINUSE), __FUNCTION__);
}

/*
 * @brief case name: XSAVE_XCR0_initial_state_following_INIT_001
 *
 * Summary: ACRN hypervisor shall keep guest XCR0 unchanged following INIT.
 *
 * This case verifies XCR0 is unchanged following the INIT (when it has been set to 7H on the AP).
 */
static __unused void xsave_rqmid_23151_xcr0_initial_state_following_init_001(void)
{

	notify_ap_modify_and_read_init_value(23151);

	debug_print("Check XCR0 following the INIT when it has been set to 7H: \n");
	debug_print("XCR0 following the 2nd INIT = 0x%lx, expected value = 0x%lx \n",
		xcr0_enter_2nd_init, xcr0_init_write_7_to_xcr0);

	report("%s", (xcr0_enter_2nd_init == xcr0_init_write_7_to_xcr0), __FUNCTION__);
}

/*
 * @brief case name: XSAVE XINUSE[bit 2:0] initial state following INIT_001
 *
 * Summary: ACRN hypervisor shall keep guest XINUSE[bit 2:0] unchanged following INIT. SDM Vol1, Chapter 13.6.
 *
 * This case verifies XINUSE[bit 2:0] is unchanged following the INIT (when XINUSE[1] has been set to 1 on the AP).
 */
static __unused void xsave_rqmid_23635_XINUSE_bit2to0_initial_state_following_INIT_001(void)
{

	notify_ap_modify_and_read_init_value(23635);
	debug_print("Check XINUSE[bit 2:0] following the INIT when XINUSE[1] has been set to 1: \n");
	debug_print("XINUSE[bit 2:0] following the 2nd INIT = 0x%lx, expected value = 0x%lx \n",
		xcr1_enter_2nd_init, xcr1_init_after_sse_exe);

	bool cond_1 = ((xcr1_init_after_sse_exe & STATE_SSE) == STATE_SSE);
	bool cond_2 = (xcr1_enter_2nd_init == xcr1_init_after_sse_exe);

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

/*
 * @brief case name: XSAVE XSAVEOPT (RFBM[i] = 1, XINUSE[i] = 1) before XRSTOR following initializing event_X87
 *
 * Summary:
 * For state component i, when a vCPU executes XSAVEOPT, guest EDX:EAX[bit i] is 1,
 * guest XCR0[bit i] is 1, guest XINUSE[i] is 1, exception conditions are unsatisfied and
 * XRSTOR has never been executed since last initializing event, ACRN hypervisor shall
 * guarantee that state component i is saved to the XSAVE area.
 *
 * This case verifies state component 0, which is X87.
 */
static __unused void xsave_rqmid_36703_xsaveopt_before_xrstor_initial_state_following_initializing_event_x87(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[bit 18] to 1.\n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Write 1 to XCR0 to make sure XCR0[0] is 1 \n");
	asm_write_xcr(0, STATE_X87);

	debug_print("Step-3: Execute X87 instruction to make sure XINUSE[0] is 1 \n");
	asm volatile("fninit");

	debug_print("Step-4: Set the XSAVE area to all 0s \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-5: Execute XSAVEOPT instruction with EDX:EAX (instruction mask) is 1 to save X87 state \n");
	asm_xsaveopt(&area_test, STATE_X87);

	uint64_t expected_xstate_bv_x87 = STATE_X87;
	uint64_t xstate_bv_test_x87 = area_test.xsave_hdr.hdr.xstate_bv & STATE_X87;
	debug_print("Step-6: Check XSTATE_BV in the XSAVE area: \n");
	debug_print("xstate_bv_test_x87 = 0x%lx, expected_xstate_bv_x87 = 0x%lx \n",
		xstate_bv_test_x87, expected_xstate_bv_x87);

	report("%s", (xstate_bv_test_x87 == expected_xstate_bv_x87), __FUNCTION__);
}

/*
 * @brief case name: XSAVE XSAVEOPT (RFBM[i] = 1, XINUSE[i] = 1) before XRSTOR following initializing event_SSE
 *
 * Summary:
 * For state component i, when a vCPU executes XSAVEOPT, guest EDX:EAX[bit i] is 1,
 * guest XCR0[bit i] is 1, guest XINUSE[i] is 1, exception conditions are unsatisfied and
 * XRSTOR has never been executed since last initializing event, ACRN hypervisor shall
 * guarantee that state component i is saved to the XSAVE area.
 *
 * This case verifies state component 1, which is SSE.
 */
static __unused void xsave_rqmid_23636_xsaveopt_before_xrstor_initial_state_following_initializing_event_sse(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[bit 18] to 1.\n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Write 3 to XCR0 to make sure XCR0[1] is 1 \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE);

	debug_print("Step-3: Execute SSE instruction to let XINUSE[1] be 1 \n");
	asm volatile("movapd %xmm1, %xmm2");

	debug_print("Step-4: Set the XSAVE area to all 0s \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-5: Execute XSAVEOPT instruction with EDX:EAX (instruction mask) is 2 to save SSE state \n");
	asm_xsaveopt(&area_test, STATE_SSE);

	uint64_t expected_xstate_bv_sse = STATE_SSE;
	uint64_t xstate_bv_test_sse = area_test.xsave_hdr.hdr.xstate_bv & STATE_SSE;
	debug_print("Step-6: Check XSTATE_BV in the XSAVE area: \n");
	debug_print("xstate_bv_test_sse = 0x%lx, expected_xstate_bv_sse = 0x%lx \n",
		xstate_bv_test_sse, expected_xstate_bv_sse);

	report("%s", (xstate_bv_test_sse == expected_xstate_bv_sse), __FUNCTION__);
}

/*
 * @brief case name: XSAVE XSAVEOPT (RFBM[i] = 1, XINUSE[i] = 1) before XRSTOR following initializing event_AVX
 *
 * Summary:
 * For state component i, when a vCPU executes XSAVEOPT, guest EDX:EAX[bit i] is 1,
 * guest XCR0[bit i] is 1, guest XINUSE[i] is 1, exception conditions are unsatisfied and
 * XRSTOR has never been executed since last initializing event, ACRN hypervisor shall
 * guarantee that state component i is saved to the XSAVE area.
 *
 * This case verifies state component 2, which is AVX.
 */
static __unused void xsave_rqmid_23637_xsaveopt_before_xrstor_initial_state_following_initializing_event_avx(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[bit 18] to 1.\n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Write 7 to XCR0 to make sure XCR0[2] is 1 \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE | STATE_AVX);

	debug_print("Step-3: Execute AVX instruction to let XINUSE[2] be 1 \n");
	asm volatile("vmovapd %ymm1, %ymm2");

	debug_print("Step-4: Set the XSAVE area to all 0s \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-5: Execute XSAVEOPT instruction with EDX:EAX (instruction mask) is 4 to save AVX state \n");
	asm_xsaveopt(&area_test, STATE_AVX);

	uint64_t expected_xstate_bv_avx = STATE_AVX;
	uint64_t xstate_bv_test_avx = area_test.xsave_hdr.hdr.xstate_bv & STATE_AVX;
	debug_print("Step-6: Check XSTATE_BV in the XSAVE area: \n");
	debug_print("xstate_bv_test_avx = 0x%lx, expected_xstate_bv_avx = 0x%lx \n",
		xstate_bv_test_avx, expected_xstate_bv_avx);

	report("%s", (xstate_bv_test_avx == expected_xstate_bv_avx), __FUNCTION__);
}

/*
 * Case name: 22895:XSAVE init and modified optimizations_001.
 *
 * Summary: DNG_132105: XSAVE init and modified optimizations.
 *
 * ACRN hypervisor shall expose XSAVE init and modified optimizations to any VM,
 * in compliance with Chapter 13.2, 13.6 and 13.9, Vol. 1, SDM.
 *
 * CPUID.(EAX=DH, ECX=1H):EAX[2] enumerates support for execution of XGETBV with ECX = 1
 */
static __unused void xsave_rqmid_22895_init_and_modified_optimizations_001(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[bit 18] to 1.\n");
	asm_write_cr4_osxsave(1);

	uint32_t read_xcr1_support = get_xcr1_support();
	debug_print("Step-2: Check if CPUID.(EAX=DH, ECX=1H):EAX[2] is 1: read_xcr1_support = 0x%x \n",
		read_xcr1_support);

	debug_print("Step-3: Execute XGETBV with ECX=1 and no exception shall be generated. \n");
	struct xsave_exception xgetbv_ret = xgetbv_exception_checking(1);

	bool cond_1 = (read_xcr1_support != 0);
	bool cond_2 = (xgetbv_ret.vector == NO_EXCEPTION);

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

/*
 * Case name: 23638:XSAVE init and modified optimizations_002.
 *
 * Summary: DNG_132105: XSAVE init and modified optimizations.
 *
 * ACRN hypervisor shall expose XSAVE init and modified optimizations to any VM,
 * in compliance with Chapter 13.2, 13.6 and 13.9, Vol. 1, SDM.
 *
 * XSAVEOPT Instruction:
 * UD exception triggered.
 */
static __unused void xsave_rqmid_23638_init_and_modified_optimizations_002(void)
{
	xsave_clean_up_env();

	uint32_t xsaveopt_support = get_xsaveopt_support();
	debug_print("Step-1: Check if CPUID.(EAX=DH, ECX=1H):EAX[0] is 1: xsaveopt_support = 0x%x \n",
		xsaveopt_support);

	debug_print("Step-2: Clear CR4.OSXSAVE[18] to 0. \n");
	asm_write_cr4_osxsave(0);

	debug_print("Step-3: Set the XSAVE area to all 0s \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-4: Execute XSAVEOPT instruction with CR4.OSXSAVE[18]=0. \n");
	struct xsave_exception ret = xsaveopt_exception_checking(&area_test, 0);
	debug_print("Expect #UD exception (UD_VECTOR = %d). Actual exception: ret.vector = %d \n",
		UD_VECTOR, ret.vector);

	bool cond_1 = (xsaveopt_support != 0);
	bool cond_2 = (ret.vector == UD_VECTOR);

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

/*
 * Case name: 23639:XSAVE init and modified optimizations_003.
 *
 * Summary: DNG_132105: XSAVE init and modified optimizations.
 *
 * ACRN hypervisor shall expose XSAVE init and modified optimizations to any VM,
 * in compliance with Chapter 13.2, 13.6 and 13.9, Vol. 1, SDM.
 *
 * XSAVEOPT Instruction:
 * No exception triggered, Guest EDX:EAX[i] is 0.
 */
static __unused void xsave_rqmid_23639_init_and_modified_optimizations_003(void)
{
	xsave_clean_up_env();

	uint32_t xsaveopt_support = get_xsaveopt_support();
	debug_print("Step-1: Check if CPUID.(EAX=DH, ECX=1H):EAX[0] is 1: xsaveopt_support = 0x%x \n",
		xsaveopt_support);

	debug_print("Step-2: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-3: Set the XSAVE area to all 0s \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-4: Execute XSAVEOPT instruction with EDX:EAX = 0. \n");
	asm_xsaveopt(&area_test, 0);

	uint64_t expected_xstate_bv = 0;
	uint64_t xstate_bv_test = area_test.xsave_hdr.hdr.xstate_bv;
	debug_print("Step-5: Check XSTATE_BV in XSAVE area: xstate_bv_test = 0x%lx, expected_xstate_bv = 0x%lx \n",
		xstate_bv_test, expected_xstate_bv);

	bool cond_1 = (xsaveopt_support != 0);
	bool cond_2 = (xstate_bv_test == expected_xstate_bv);

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

/*
 * Case name: 23640:XSAVE init and modified optimizations_004.
 *
 * Summary: DNG_132105: XSAVE init and modified optimizations.
 *
 * ACRN hypervisor shall expose XSAVE init and modified optimizations to any VM,
 * in compliance with Chapter 13.2, 13.6 and 13.9, Vol. 1, SDM.
 *
 * XSAVEOPT Instruction:
 * No exception triggered, Guest EDX:EAX[i] is 1, Guest XCR0[i] is 0.
 *
 * This case utilizes the state component 1 (SSE) to test.
 * 1. Guest XCR0[1] is 0 by setting XCR0 to 1
 * 2. Guest EDX:EAX[1] is 1 by executing XSAVEOPT with EDX:EAX = 7
 */
static __unused void xsave_rqmid_23640_init_and_modified_optimizations_004(void)
{
	xsave_clean_up_env();

	uint32_t xsaveopt_support = get_xsaveopt_support();
	debug_print("Step-1: Check if CPUID.(EAX=DH, ECX=1H):EAX[0] is 1: xsaveopt_support = 0x%x \n",
		xsaveopt_support);

	debug_print("Step-2: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-3: Write 1 to XCR0 \n");
	asm_write_xcr(0, STATE_X87);

	debug_print("Step-4: Set the XSAVE area to all 0s \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-5: Execute XSAVEOPT instruction with EDX:EAX = 7. \n");
	asm_xsaveopt(&area_test, STATE_X87 | STATE_SSE | STATE_AVX);

	uint64_t expected_xstate_bv_sse = 0;
	uint64_t xstate_bv_test_sse = area_test.xsave_hdr.hdr.xstate_bv & STATE_SSE;
	debug_print("Step-6: Check XSTATE_BV in XSAVE area: \n");
	debug_print("xstate_bv_test_sse = 0x%lx, expected_xstate_bv_sse = 0x%lx \n",
		xstate_bv_test_sse, expected_xstate_bv_sse);

	bool cond_1 = (xsaveopt_support != 0);
	bool cond_2 = (xstate_bv_test_sse == expected_xstate_bv_sse);

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

/*
 * Case name: 23641:XSAVE init and modified optimizations_005.
 *
 * Summary: DNG_132105: XSAVE init and modified optimizations.
 *
 * ACRN hypervisor shall expose XSAVE init and modified optimizations to any VM,
 * in compliance with Chapter 13.2, 13.6 and 13.9, Vol. 1, SDM.
 *
 * XSAVEOPT Instruction:
 * No exception triggered, Guest EDX:EAX[i] is 1, Guest XCR0[i] is 1, XINUSE[i] is 0.
 *
 * This test case utilizes state component 2 (AVX) to test.
 * 1. Guest XCR0[2] is 1 by setting XCR0 to 7.
 * 2. XINUSE[2] is 0 by default environment set-up.
 * 3. Guest EDX:EAX[2] is 1 by executing XSAVEOPT with EDX:EAX = 4.
 */
static __unused void xsave_rqmid_23641_init_and_modified_optimizations_005(void)
{
	xsave_clean_up_env();

	uint32_t xsaveopt_support = get_xsaveopt_support();
	debug_print("Step-1: Check if CPUID.(EAX=DH, ECX=1H):EAX[0] is 1: xsaveopt_support = 0x%x \n",
		xsaveopt_support);

	debug_print("Step-2: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-3: Write 7 to XCR0 \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE | STATE_AVX);

	uint64_t xcr1 = asm_read_xcr(1);
	uint64_t xinuse_avx = xcr1 & STATE_AVX;
	debug_print("Step-4: Check if XINUSE[2] is 0: xinuse_avx = 0x%lx \n", xinuse_avx);

	debug_print("Step-5: Set the XSAVE area to all 0s \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-6: Execute XSAVEOPT instruction with EDX:EAX = 4 (set AVX bit in the instruction mask) \n");
	asm_xsaveopt(&area_test, STATE_AVX);

	uint64_t expected_xstate_bv_avx = 0;
	uint64_t xstate_bv_test_avx = area_test.xsave_hdr.hdr.xstate_bv & STATE_AVX;
	debug_print("Step-7: Check XSTATE_BV in XSAVE area: \n");
	debug_print("xstate_bv_test_avx = 0x%lx, expected_xstate_bv_avx = 0x%lx \n",
		xstate_bv_test_avx, expected_xstate_bv_avx);

	bool cond_1 = (xsaveopt_support != 0);
	bool cond_2 = (xinuse_avx == 0);
	bool cond_3 = (xstate_bv_test_avx == expected_xstate_bv_avx);

	report("%s", (cond_1 && cond_2 && cond_3), __FUNCTION__);
}

/*
 * Case name: 36805:XSAVE init and modified optimizations_007.
 *
 * Summary: DNG_132105: XSAVE init and modified optimizations.
 *
 * ACRN hypervisor shall expose XSAVE init and modified optimizations to any VM,
 * in compliance with Chapter 13.2, 13.6 and 13.9, Vol. 1, SDM.
 *
 * XSAVEOPT Instruction:
 * No exception triggered, Guest EDX:EAX[i] is 1, Guest XCR0[i] is 1, XINUSE[i] is 1, XRSTOR has been executed
 * since last initializing event, XMODIFIED[i] is 1.
 *
 * Utilize state component 1 (SSE) to test.
 *
 * This test case utilizes state component 1 (SSE) to test.
 * 1. Guest XCR0[1] is 1 by setting XCR0 to 3.
 * 2. XINUSE[1] is 1 by executing SSE instruction.
 * 3. XMODIFIED[1] is constructed as 1 through executing SSE instruction between XRSTOR instruction and XSAVEOPT
 *    instruction.
 * 4. Guest EDX:EAX[1] is 1 by executing XSAVEOPT with EDX:EAX = 2
 */
static __unused void xsave_rqmid_36805_init_and_modified_optimizations_007(void)
{
	xsave_clean_up_env();

	uint32_t xsaveopt_support = get_xsaveopt_support();
	debug_print("Step-1: Check if CPUID.(EAX=DH, ECX=1H):EAX[0] is 1: xsaveopt_support = 0x%x \n",
		xsaveopt_support);

	debug_print("Step-2: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-3: Write 3 to XCR0 \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE);

	debug_print("Step-4: Execute SSE instruction to let XINUSE[1] be 1 \n");
	asm volatile("movapd %xmm1, %xmm2");

	uint64_t xcr1 = asm_read_xcr(1);
	uint64_t xinuse_sse = xcr1 & STATE_SSE;
	debug_print("Step-5: Check if XINUSE[1] is 1: xinuse_sse = 0x%lx \n", xinuse_sse);

	debug_print("Step-6: Set the XSAVE area to all 0s \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-7: Execute XRSTOR instruction with EDX:EAX = 2 (set SSE bit in the instruction mask) \n");
	asm_xrstor(&area_test, STATE_SSE);

	debug_print("Step-8: Execute SSE instruction to let XMODIFIED[1] be 1 \n");
	asm volatile("movapd %xmm1, %xmm2");

	debug_print("Step-9: Execute XSAVEOPT instruction with EDX:EAX = 2 (set SSE bit in the instruction mask) \n");
	asm_xsaveopt(&area_test, STATE_SSE);

	uint64_t expected_xstate_bv_sse = STATE_SSE;
	uint64_t xstate_bv_test_sse = area_test.xsave_hdr.hdr.xstate_bv & STATE_SSE;
	debug_print("Step-10: Check XSTATE_BV in XSAVE area: \n");
	debug_print("xstate_bv_test_sse = 0x%lx, expected_xstate_bv_sse = 0x%lx \n",
		xstate_bv_test_sse, expected_xstate_bv_sse);

	bool cond_1 = (xsaveopt_support != 0);
	bool cond_2 = (xinuse_sse == STATE_SSE);
	bool cond_3 = (xstate_bv_test_sse == expected_xstate_bv_sse);

	report("%s", (cond_1 && cond_2 && cond_3), __FUNCTION__);
}

/*
 * Case name: 22840:XSAVE_general_support_001_CPUID_XSAVE.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * CPUID.1:ECX.XSAVE[bit 26] enumerates general support for the XSAVE feature set.
 * If this bit is 1, the processor supports the following instructions:
 * XGETBV, XRSTOR, XSAVE, and XSETBV.
 */
static __unused void xsave_rqmid_22840_general_support_001_cpuid_xsave(void)
{
	xsave_clean_up_env();

	uint32_t xsave_support = get_xsave_support();
	debug_print("Step-1: Check if CPUID.01H:ECX.XSAVE[bit 26] is 1: xsave_support = 0x%x \n", xsave_support);

	debug_print("Step-2: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-3: Execute XGETBV with ECX is 0. No exception shall be triggered. \n");
	struct xsave_exception xgetbv_ret = xgetbv_exception_checking(0);

	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-4: Execute XSAVE with 64-byte aligned memory. No exception shall be triggered. \n");
	struct xsave_exception xsave_ret = xsave_exception_checking(&area_test, 0);

	debug_print("Step-5: Execute XRSTOR with 64-byte aligned memory. No exception shall be triggered. \n");
	struct xsave_exception xrstor_ret = xrstor_exception_checking(&area_test, 0);

	debug_print("Step-6: Execute XSETBV with EDX:EAX is 1 and ECX is 3. No exception shall be triggered. \n");
	struct xsave_exception xsetbv_ret = xsetbv_exception_checking(0, STATE_X87 | STATE_SSE);

	bool cond_1 = (xsave_support != 0);
	bool cond_2 = (xgetbv_ret.vector == NO_EXCEPTION);
	bool cond_3 = (xsave_ret.vector == NO_EXCEPTION);
	bool cond_4 = (xrstor_ret.vector == NO_EXCEPTION);
	bool cond_5 = (xsetbv_ret.vector == NO_EXCEPTION);

	report("%s", (cond_1 && cond_2 && cond_3 && cond_4 && cond_5), __FUNCTION__);
}

/*
 * Case name: 22863:XSAVE_general_support_002_set_CR4_OSXSAVE.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * CR4.OSXSAVE can be set to 1 if and only if CPUID.1:ECX.XSAVE[bit 26] is enumerated as 1.
 */
static __unused void xsave_rqmid_22863_general_support_002_set_cr4_osxsave(void)
{
	xsave_clean_up_env();

	uint32_t xsave_support = get_xsave_support();
	debug_print("Step-1: Check if CPUID.01H:ECX.XSAVE[bit 26] is 1: xsave_support = 0x%x \n", xsave_support);

	debug_print("Step-2: Set CR4.OSXSAVE[18] to 1. No exception shall be triggered. \n");
	uint8_t cr4_osxsave_1_ret = write_cr4_osxsave(1);

	uint64_t cr4_osxsave_1 = asm_read_cr4_osxsave();
	debug_print("Step-3: Check if CR4.OSXSAVE[bit 18] is 1. cr4_osxsave_1 = 0x%lx \n", cr4_osxsave_1);

	debug_print("Step-4: Set CR4.OSXSAVE[18] to 0. \n");
	uint8_t cr4_osxsave_0_ret = write_cr4_osxsave(0);

	uint64_t cr4_osxsave_0 = asm_read_cr4_osxsave();
	debug_print("Step-5: Check if CR4.OSXSAVE[bit 18] is 0. cr4_osxsave_0 = 0x%lx \n", cr4_osxsave_0);

	bool cond_1 = (xsave_support != 0);
	bool cond_2 = (cr4_osxsave_1_ret == NO_EXCEPTION);
	bool cond_3 = (cr4_osxsave_1 != 0);
	bool cond_4 = (cr4_osxsave_0_ret == NO_EXCEPTION);
	bool cond_5 = (cr4_osxsave_0 == 0);

	report("%s", (cond_1 && cond_2 && cond_3 && cond_4 && cond_5), __FUNCTION__);
}

/*
 * Case name: 22793:XSAVE_general_support_003_set_XCR0.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * CPUID function 0DH, sub-function 0
 * EDX:EAX is a bitmap of all the user state components that can be managed using the XSAVE feature set.
 * A bit can be set in XCR0 if and only if the corresponding bit is set in this bitmap.
 */
static __unused void xsave_rqmid_22793_general_support_003_set_xcr0(void)
{
	xsave_clean_up_env();

	uint64_t supported_user_states = get_supported_user_states();
	debug_print("Step-1: Check CPUID.(EAX=DH, ECX=0H):(EDX:EAX): supported_user_states = 0x%lx \n",
		supported_user_states);
	bool cond_1 = (supported_user_states == XSAVE_SUPPORTED_USER_STATES);

	debug_print("Step-2: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-3: Execute XSETBV with ECX = 0 and EDX:EAX = 1, 3, 7. No exception shall be triggered. \n");
	struct xsave_exception xsetbv_ret_1 = xsetbv_exception_checking(0, STATE_X87);
	struct xsave_exception xsetbv_ret_3 = xsetbv_exception_checking(0, STATE_X87 | STATE_SSE);
	struct xsave_exception xsetbv_ret_7 = xsetbv_exception_checking(0, STATE_X87 | STATE_SSE | STATE_AVX);
	bool cond_2 = ((xsetbv_ret_1.vector == NO_EXCEPTION) &&
			(xsetbv_ret_3.vector == NO_EXCEPTION) &&
			(xsetbv_ret_7.vector == NO_EXCEPTION));

	debug_print("Step-3: For each i satisfying 3 <= i < 64, set XCR0[0] and XCR0[i] to 1. \n");
	uint32_t i;
	uint64_t val;
	struct xsave_exception ret;
	bool cond_3 = true;
	for (i = 3U; i < 64U; i++) {
		val = STATE_X87 | (1UL << i);
		ret = xsetbv_exception_checking(0, val);

		if ((ret.vector != GP_VECTOR) || (ret.error_code != 0)) {
			cond_3 = false;
			debug_print("Failure!!! Setting XCR0[%d] to 1. \n", i);
			debug_print("Expect #GP(0) exception (GP_VECTOR = %d). \n", GP_VECTOR);
			debug_print("Actual exception: ret.vector = %d, ret.error_code = %d \n",
				ret.vector, ret.error_code);
		}
	}

	report("%s", (cond_1 && cond_2 && cond_3), __FUNCTION__);
}

/*
 * Case name: 22794:XSAVE_general_support_004_supported_states_size.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * CPUID.(EAX=DH, ECX=0H):ECX enumerates the size (in bytes) required by the XSAVE instruction for an XSAVE area
 * containing all the user state components supported by this processor
 */
static __unused void xsave_rqmid_22794_general_support_004_supported_states_size(void)
{
	xsave_clean_up_env();

	uint64_t supported_user_states = get_supported_user_states();
	debug_print("Step-1: Check if CPUID.(EAX=DH, ECX=0H):(EDX:EAX) is 7: supported_user_states = 0x%lx \n",
		supported_user_states);

	uint32_t avx_area_size = cpuid_indexed(CPUID_XSAVE_FUC, 2).a;
	debug_print("Step-2: Get AVX area size (specified by CPUID.(EAX=DH, ECX=2H):EAX). avx_area_size = 0x%x \n",
		avx_area_size);

	uint32_t expected_size = XSAVE_LEGACY_AREA_SIZE + XSAVE_HEADER_AREA_SIZE + avx_area_size;
	uint32_t total_size = cpuid_indexed(CPUID_XSAVE_FUC, 0).c;
	debug_print("Step-3: Check CPUID.(EAX=DH, ECX=0H):ECX: total_size = 0x%x, expected_size = 0x%x \n",
		total_size, expected_size);

	bool cond_1 = (supported_user_states == XSAVE_SUPPORTED_USER_STATES);
	bool cond_2 = (total_size == expected_size);

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

/*
 * Case name: 22797:XSAVE_general_support_005_enabled_user_states_size.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * CPUID.(EAX=DH, ECX=0H):EBX enumerates the size (in bytes) required by the XSAVE instruction for an XSAVE area
 * containing all the user state components corresponding to bits currently set in XCR0.
 */
static __unused void xsave_rqmid_22797_general_support_005_enabled_user_states_size(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	uint64_t supported_user_states = get_supported_user_states();
	debug_print("Step-2: Check if CPUID.(EAX=DH, ECX=0H):(EDX:EAX) is 7: supported_user_states = 0x%lx \n",
		supported_user_states);

	debug_print("Step-3: Write 7 to XCR0. \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE | STATE_AVX);

	uint32_t avx_area_size = cpuid_indexed(CPUID_XSAVE_FUC, 2).a;
	debug_print("Step-4: Get AVX area size (specified by CPUID.(EAX=DH, ECX=2H):EAX). avx_area_size = 0x%x \n",
		avx_area_size);

	uint32_t expected_size_7 = XSAVE_LEGACY_AREA_SIZE + XSAVE_HEADER_AREA_SIZE + avx_area_size;
	uint32_t enabled_size_7 = cpuid_indexed(CPUID_XSAVE_FUC, 0).b;
	debug_print("Step-5: Check CPUID.(EAX=DH, ECX=0H):EBX: enabled_size_7 = 0x%x, expected_size_7 = 0x%x \n",
		enabled_size_7, expected_size_7);

	debug_print("Step-6: Write 3 to XCR0. \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE);

	uint32_t expected_size_3 = XSAVE_LEGACY_AREA_SIZE + XSAVE_HEADER_AREA_SIZE;
	uint32_t enabled_size_3 = cpuid_indexed(CPUID_XSAVE_FUC, 0).b;
	debug_print("Step-7: Check CPUID.(EAX=DH, ECX=0H):EBX: enabled_size_3 = 0x%x, expected_size_3 = 0x%x \n",
		enabled_size_3, expected_size_3);

	debug_print("Step-8: Write 1 to XCR0. \n");
	asm_write_xcr(0, STATE_X87);

	uint32_t expected_size_1 = XSAVE_LEGACY_AREA_SIZE + XSAVE_HEADER_AREA_SIZE;
	uint32_t enabled_size_1 = cpuid_indexed(CPUID_XSAVE_FUC, 0).b;
	debug_print("Step-9: Check CPUID.(EAX=DH, ECX=0H):EBX: enabled_size_1 = 0x%x, expected_size_1 = 0x%x \n",
		enabled_size_1, expected_size_1);

	bool cond_1 = (supported_user_states == XSAVE_SUPPORTED_USER_STATES);
	bool cond_2 = (enabled_size_7 == expected_size_7);
	bool cond_3 = (enabled_size_3 == expected_size_3);
	bool cond_4 = (enabled_size_1 == expected_size_1);

	report("%s", (cond_1 && cond_2 && cond_3 && cond_4), __FUNCTION__);
}

/*
 * Case name: 22798:XSAVE_general_support_006_CPUID_XSAVEOPT.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * CPUID.(EAX=DH, ECX=1H):EAX[0] enumerates support for the XSAVEOPT instruction.
 */
static __unused void xsave_rqmid_22798_general_support_006_cpuid_xsaveopt(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	uint32_t xsaveopt_support = get_xsaveopt_support();
	debug_print("Step-2: Check if CPUID.(EAX=DH, ECX=1H):EAX[0] is 1: xsaveopt_support = 0x%x \n",
		xsaveopt_support);

	debug_print("Step-3: Execute XSAVEOPT instruction. No exception shall be triggered. \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));
	struct xsave_exception xsaveopt_ret = xsaveopt_exception_checking(&area_test, 0);

	bool cond_1 = (xsaveopt_support != 0);
	bool cond_2 = (xsaveopt_ret.vector == NO_EXCEPTION);

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

/*
 * Case name: 22827:XSAVE_general_support_011_all_enabled_states_size.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * CPUID.(EAX=DH, ECX=1H):EBX enumerates the size (in bytes) required by the XSAVES instruction for an XSAVE area
 * containing all the state components corresponding to bits currently set in XCR0 | IA32_XSS.
 */
static __unused void xsave_rqmid_22827_general_support_011_all_enabled_states_size(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	uint64_t supported_user_states = get_supported_user_states();
	debug_print("Step-2: Check if CPUID.(EAX=DH, ECX=0H):(EDX:EAX) is 7: supported_user_states = 0x%lx \n",
		supported_user_states);

	uint64_t supported_supervisor_states = get_supported_supervisor_states();
	debug_print("Step-3: Check if CPUID.(EAX=DH, ECX=1H):(EDX:ECX) is 0: supported_supervisor_states = 0x%lx \n",
		supported_supervisor_states);

	debug_print("Step-4: Write 7 to XCR0. \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE | STATE_AVX);

	uint32_t avx_area_size = cpuid_indexed(CPUID_XSAVE_FUC, 2).a;
	debug_print("Step-5: Get AVX area size (specified by CPUID.(EAX=DH, ECX=2H):EAX). avx_area_size = 0x%x \n",
		avx_area_size);

	uint32_t expected_size_7 = XSAVE_LEGACY_AREA_SIZE + XSAVE_HEADER_AREA_SIZE + avx_area_size;
	uint32_t enabled_size_7 = cpuid_indexed(CPUID_XSAVE_FUC, 1).b;
	debug_print("Step-6: Check CPUID.(EAX=DH, ECX=1H):EBX: enabled_size_7 = 0x%x, expected_size_7 = 0x%x \n",
		enabled_size_7, expected_size_7);

	debug_print("Step-7: Write 3 to XCR0. \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE);

	uint32_t expected_size_3 = XSAVE_LEGACY_AREA_SIZE + XSAVE_HEADER_AREA_SIZE;
	uint32_t enabled_size_3 = cpuid_indexed(CPUID_XSAVE_FUC, 1).b;
	debug_print("Step-8: Check CPUID.(EAX=DH, ECX=1H):EBX: enabled_size_3 = 0x%x, expected_size_3 = 0x%x \n",
		enabled_size_3, expected_size_3);

	debug_print("Step-9: Write 1 to XCR0. \n");
	asm_write_xcr(0, STATE_X87);

	uint32_t expected_size_1 = XSAVE_LEGACY_AREA_SIZE + XSAVE_HEADER_AREA_SIZE;
	uint32_t enabled_size_1 = cpuid_indexed(CPUID_XSAVE_FUC, 1).b;
	debug_print("Step-10: Check CPUID.(EAX=DH, ECX=1H):EBX: enabled_size_1 = 0x%x, expected_size_1 = 0x%x \n",
		enabled_size_1, expected_size_1);

	bool cond_1 = (supported_user_states == XSAVE_SUPPORTED_USER_STATES);
	bool cond_2 = (supported_supervisor_states == XSAVE_SUPPORTED_SUPERVISOR_STATES);
	bool cond_3 = (enabled_size_7 == expected_size_7);
	bool cond_4 = (enabled_size_3 == expected_size_3);
	bool cond_5 = (enabled_size_1 == expected_size_1);

	report("%s", (cond_1 && cond_2 && cond_3 && cond_4 && cond_5), __FUNCTION__);
}

/*
 * Case name: 22829:XSAVE_general_support_013_state_size.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * CPUID function 0DH, sub-function i (i > 1).
 * EAX enumerates the size (in bytes) required for state component i.
 */
static __unused void xsave_rqmid_22829_general_support_013_state_size(void)
{
	xsave_clean_up_env();

	uint64_t supported_user_states = get_supported_user_states();
	debug_print("Step-1: Check if CPUID.(EAX=DH, ECX=0H):(EDX:EAX) is 7: supported_user_states = 0x%lx \n",
		supported_user_states);

	debug_print("Step-2: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-3: Write 7 to XCR0. \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE | STATE_AVX);

	uint32_t enabled_size = cpuid_indexed(CPUID_XSAVE_FUC, 0).b;
	debug_print("Step-4: Check CPUID.(EAX=DH, ECX=0H):EBX: enabled_size = 0x%x \n", enabled_size);

	uint32_t expected_avx_area_size = enabled_size - XSAVE_LEGACY_AREA_SIZE - XSAVE_HEADER_AREA_SIZE;
	uint32_t avx_area_size = cpuid_indexed(CPUID_XSAVE_FUC, 2).a;
	debug_print("Step-5: Get AVX area size (specified by CPUID.(EAX=DH, ECX=2H):EAX). \n");
	debug_print("avx_area_size = 0x%x expected_avx_area_size = 0x%x \n",
		avx_area_size, expected_avx_area_size);

	bool cond_1 = (supported_user_states == XSAVE_SUPPORTED_USER_STATES);
	bool cond_2 = (avx_area_size == expected_avx_area_size);

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

/*
 * Case name: 22831:XSAVE_general_support_015_user_state_ECX_bit0.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * CPUID function 0DH, sub-function i (i > 1).
 * If state component i is a user state component, ECX[0] return 0
 */
static __unused void xsave_rqmid_22831_general_support_015_user_state_ecx_bit0(void)
{
	xsave_clean_up_env();

	uint64_t supported_user_states = get_supported_user_states();
	debug_print("Step-1: Check if CPUID.(EAX=DH, ECX=0H):(EDX:EAX) is 7: supported_user_states = 0x%lx \n",
		supported_user_states);

	uint64_t supported_supervisor_states = get_supported_supervisor_states();
	debug_print("Step-2: Check if CPUID.(EAX=DH, ECX=1H):(EDX:ECX) is 0: supported_supervisor_states = 0x%lx \n",
		supported_supervisor_states);

	uint32_t avx_us_bit = cpuid_indexed(CPUID_XSAVE_FUC, 2).c;
	avx_us_bit &= XSAVE_USER_SUPERVISOR;
	debug_print("Step-3: Check if CPUID.(EAX=DH, ECX=2H):ECX[0] is 0: avx_us_bit = 0x%x \n", avx_us_bit);

	bool cond_1 = (supported_user_states == XSAVE_SUPPORTED_USER_STATES);
	bool cond_2 = (supported_supervisor_states == XSAVE_SUPPORTED_SUPERVISOR_STATES);
	bool cond_3 = (avx_us_bit == 0);

	report("%s", (cond_1 && cond_2 && cond_3), __FUNCTION__);
}

/*
 * Case name: 22865:XSAVE_general_support_016_state_offset.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * CPUID function 0DH, sub-function i (i > 1).
 * If ECX[1] returns 0, state component i is located immediately following the preceding state component
 */
static __unused void xsave_rqmid_22865_general_support_016_state_offset(void)
{
	xsave_clean_up_env();

	uint64_t supported_user_states = get_supported_user_states();
	debug_print("Step-1: Check if CPUID.(EAX=DH, ECX=0H):(EDX:EAX) is 7: supported_user_states = 0x%lx \n",
		supported_user_states);

	uint64_t supported_supervisor_states = get_supported_supervisor_states();
	debug_print("Step-2: Check if CPUID.(EAX=DH, ECX=1H):(EDX:ECX) is 0: supported_supervisor_states = 0x%lx \n",
		supported_supervisor_states);

	uint32_t avx_align_bit = cpuid_indexed(CPUID_XSAVE_FUC, 2).c;
	avx_align_bit &= XSAVE_ALIGNMENT;
	debug_print("Step-3: Check if CPUID.(EAX=DH, ECX=2H):ECX[1] is 0: avx_align_bit = 0x%x \n", avx_align_bit);

	uint32_t expected_avx_offset = XSAVE_LEGACY_AREA_SIZE + XSAVE_HEADER_AREA_SIZE;
	uint32_t avx_offset = cpuid_indexed(CPUID_XSAVE_FUC, 2).b;
	debug_print("Step-4: Check CPUID.(EAX=DH, ECX=2H):EBX: avx_offset = 0x%x expected_avx_offset = 0x%x \n",
		avx_offset, expected_avx_offset);

	bool cond_1 = (supported_user_states == XSAVE_SUPPORTED_USER_STATES);
	bool cond_2 = (supported_supervisor_states == XSAVE_SUPPORTED_SUPERVISOR_STATES);
	bool cond_3 = (avx_align_bit == 0);
	bool cond_4 = (avx_offset == expected_avx_offset);

	report("%s", (cond_1 && cond_2 && cond_3 && cond_4), __FUNCTION__);
}

/*
 * Case name: 22832:XSAVE_general_support_017_CPUID_ECX_EDX_rsvd_bits.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * CPUID function 0DH, sub-function i (i > 1).
 * ECX[31:2] and EDX return 0
 */
static __unused void xsave_rqmid_22832_general_support_017_cpuid_ecx_edx_rsvd_bits(void)
{
	xsave_clean_up_env();

	uint64_t supported_user_states = get_supported_user_states();
	debug_print("Step-1: Check if CPUID.(EAX=DH, ECX=0H):(EDX:EAX) is 7: supported_user_states = 0x%lx \n",
		supported_user_states);

	uint64_t supported_supervisor_states = get_supported_supervisor_states();
	debug_print("Step-2: Check if CPUID.(EAX=DH, ECX=1H):(EDX:ECX) is 0: supported_supervisor_states = 0x%lx \n",
		supported_supervisor_states);

	uint32_t avx_ecx_rsvd = cpuid_indexed(CPUID_XSAVE_FUC, 2).c;
	avx_ecx_rsvd = avx_ecx_rsvd >> 2U;
	debug_print("Step-3: Check if CPUID.(EAX=DH, ECX=2H):ECX[31:2] is 0: avx_ecx_rsvd = 0x%x \n", avx_ecx_rsvd);

	uint32_t avx_edx = cpuid_indexed(CPUID_XSAVE_FUC, 2).d;
	debug_print("Step-4: Check if CPUID.(EAX=DH, ECX=2H):EDX is 0: avx_edx = 0x%x \n", avx_edx);

	bool cond_1 = (supported_user_states == XSAVE_SUPPORTED_USER_STATES);
	bool cond_2 = (supported_supervisor_states == XSAVE_SUPPORTED_SUPERVISOR_STATES);
	bool cond_3 = (avx_ecx_rsvd == 0);
	bool cond_4 = (avx_edx == 0);

	report("%s", (cond_1 && cond_2 && cond_3 && cond_4), __FUNCTION__);
}

/*
 * Case name: 22835:XSAVE_general_support_018_CPUID_unsupported_states.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * CPUID function 0DH, sub-function i (i > 1).
 * If the XSAVE feature set does not support state component i, sub-function i returns 0 in EAX, EBX, ECX, and EDX.
 */
static __unused void xsave_rqmid_22835_general_support_018_cpuid_unsupported_states(void)
{
	xsave_clean_up_env();

	uint64_t supported_user_states = get_supported_user_states();
	debug_print("Step-1: Check if CPUID.(EAX=DH, ECX=0H):(EDX:EAX) is 7: supported_user_states = 0x%lx \n",
		supported_user_states);

	uint64_t supported_supervisor_states = get_supported_supervisor_states();
	debug_print("Step-2: Check if CPUID.(EAX=DH, ECX=1H):(EDX:ECX) is 0: supported_supervisor_states = 0x%lx \n",
		supported_supervisor_states);

	bool cond_1 = (supported_user_states == XSAVE_SUPPORTED_USER_STATES);
	bool cond_2 = (supported_supervisor_states == XSAVE_SUPPORTED_SUPERVISOR_STATES);

	debug_print("Step-3: For each i in the range [3, 63], check if CPUID.(EAX=DH, ECX=i) return all 0. \n");
	uint32_t i, eax, ebx, ecx, edx;
	bool cond_3 = true;
	for (i = 3U; i < 64U; i++) {
		eax = cpuid_indexed(CPUID_XSAVE_FUC, i).a;
		ebx = cpuid_indexed(CPUID_XSAVE_FUC, i).b;
		ecx = cpuid_indexed(CPUID_XSAVE_FUC, i).c;
		edx = cpuid_indexed(CPUID_XSAVE_FUC, i).d;

		if ((eax != 0U) || (ebx != 0U) || (ecx != 0U) || (edx != 0U)) {
			cond_3 = false;
			debug_print("Failure!!! CPUID.(EAX=DH, ECX=%dH): EAX=%xH, EBX=%xH, ECX=%xH, EDX=%xH \n",
				i, eax, ebx, ecx, edx);
		}
	}

	report("%s", (cond_1 && cond_2 && cond_3), __FUNCTION__);
}

/*
 * Case name: 21175:XSAVE_general_support_007_UD.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * If CR4.OSXSAVE[bit 18] is 0, execution of any of XGETBV, XRSTOR, XSAVE, XSAVEC, XSAVEOPT,
 * and XSETBV causes an invalid-opcode exception (#UD).
 */
static __unused void xsave_rqmid_21175_general_support_007_ud(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Clear CR4.OSXSAVE[18] to 0. \n");
	asm_write_cr4_osxsave(0);

	debug_print("Step-2: Execute XGETBV with ECX is 0. \n");
	struct xsave_exception xgetbv_ret = xgetbv_exception_checking(0);
	debug_print("Expect #UD exception (UD_VECTOR = %d). Actual exception: xgetbv_ret.vector = %d \n",
		UD_VECTOR, xgetbv_ret.vector);

	debug_print("Step-3: Execute XSETBV with EDX:EAX is 1 and ECX is 0. \n");
	struct xsave_exception xsetbv_ret = xsetbv_exception_checking(0, STATE_X87);
	debug_print("Expect #UD exception (UD_VECTOR = %d). Actual exception: xsetbv_ret.vector = %d \n",
		UD_VECTOR, xsetbv_ret.vector);

	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-4: Execute XSAVE with 64-byte aligned memory specified \n");
	struct xsave_exception xsave_ret = xsave_exception_checking(&area_test, 0);
	debug_print("Expect #UD exception (UD_VECTOR = %d). Actual exception: xsave_ret.vector = %d \n",
		UD_VECTOR, xsave_ret.vector);

	debug_print("Step-5: Execute XRSTOR with 64-byte aligned memory specified \n");
	struct xsave_exception xrstor_ret = xrstor_exception_checking(&area_test, 0);
	debug_print("Expect #UD exception (UD_VECTOR = %d). Actual exception: xrstor_ret.vector = %d \n",
		UD_VECTOR, xrstor_ret.vector);

	debug_print("Step-6: Execute XSAVEC with 64-byte aligned memory specified \n");
	struct xsave_exception xsavec_ret = xsavec_exception_checking(&area_test, 0);
	debug_print("Expect #UD exception (UD_VECTOR = %d). Actual exception: xsavec_ret.vector = %d \n",
		UD_VECTOR, xsavec_ret.vector);

	debug_print("Step-7: Execute XSAVEOPT with 64-byte aligned memory specified \n");
	struct xsave_exception xsaveopt_ret = xsaveopt_exception_checking(&area_test, 0);
	debug_print("Expect #UD exception (UD_VECTOR = %d). Actual exception: xsaveopt_ret.vector = %d \n",
		UD_VECTOR, xsaveopt_ret.vector);

	bool cond_1 = (xgetbv_ret.vector == UD_VECTOR);
	bool cond_2 = (xsetbv_ret.vector == UD_VECTOR);
	bool cond_3 = (xsave_ret.vector == UD_VECTOR);
	bool cond_4 = (xrstor_ret.vector == UD_VECTOR);
	bool cond_5 = (xsavec_ret.vector == UD_VECTOR);
	bool cond_6 = (xsaveopt_ret.vector == UD_VECTOR);

	report("%s", (cond_1 && cond_2 && cond_3 && cond_4 && cond_5 && cond_6), __FUNCTION__);
}

/*
 * Case name: 22854:XSAVE_general_support_024_XCR0_rsvd_bits.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * XCR0[63:10] and XCR0[8] are reserved. So these bits shall be 0.
 */
static __unused void xsave_rqmid_22854_general_support_024_xcr0_rsvd_bits(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Execute XGETBV with ECX = 0. \n");
	uint64_t xcr0 = asm_read_xcr(0);

	uint64_t xcr0_bit_8 = xcr0 & (1UL << 8U);
	debug_print("Step-3: Check if XCR0[8] is 0. xcr0_bit_8 = 0x%lx \n", xcr0_bit_8);

	uint64_t xcr0_bit_63_10 = xcr0 >> 10U;
	debug_print("Step-4: Check if XCR0[63:10] is 0. xcr0_bit_63_10 = 0x%lx \n", xcr0_bit_63_10);

	bool cond_1 = (xcr0_bit_8 == 0);
	bool cond_2 = (xcr0_bit_63_10 == 0);

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

static bool cond_ring3_helper_22861;
static uint64_t xcr0_set_val_helper_22861;

static __unused void helper_22861_read_xcr0_at_ring3(const char *msg)
{
	uint16_t cpl = read_cs() & 0x3;
	uint64_t xcr0 = asm_read_xcr(0);

	debug_print("Execute XGETBV with ECX = 0 when CPL is 3. CPL = %d, XCR0 = 0x%lx \n",
		cpl, xcr0);

	cond_ring3_helper_22861 = ((cpl == 3) && (xcr0 == xcr0_set_val_helper_22861));
}

/*
 * Case name: 22861:XSAVE_general_support_023_read_XCR0.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * Executing the XGETBV instruction with ECX = 0 returns the value of XCR0 in EDX:EAX.
 * XGETBV can be executed if CR4.OSXSAVE = 1 (if CPUID.1:ECX.OSXSAVE = 1), regardless of CPL.
 */
static __unused void xsave_rqmid_22861_general_support_023_read_xcr0(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	uint32_t osxsave_support = get_osxsave_support();
	debug_print("Step-2: Check if CPUID.1:ECX.OSXSAVE is 1. osxsave_support = 0x%x \n", osxsave_support);
	bool cond_1 = (osxsave_support != 0);

	debug_print("Step-3: Write 1 to XCR0. \n");
	xcr0_set_val_helper_22861 = STATE_X87;
	asm_write_xcr(0, xcr0_set_val_helper_22861);

	uint16_t cpl = read_cs() & 0x3;
	uint64_t xcr0 = asm_read_xcr(0);
	debug_print("Step-4: Execute XGETBV with ECX = 0 when CPL is 0. CPL = %d, XCR0 = 0x%lx \n",
		cpl, xcr0);
	bool cond_ring0_1 = ((cpl == 0) && (xcr0 == xcr0_set_val_helper_22861));

	debug_print("Step-6: Execute XGETBV with ECX = 0 when CPL is 3. \n");
	cond_ring3_helper_22861 = false;
	do_at_ring3(helper_22861_read_xcr0_at_ring3, "");
	bool cond_ring3_1 = cond_ring3_helper_22861;

	debug_print("Step-7: Write 3 to XCR0. \n");
	xcr0_set_val_helper_22861 = STATE_X87 | STATE_SSE;
	asm_write_xcr(0, xcr0_set_val_helper_22861);

	cpl = read_cs() & 0x3;
	xcr0 = asm_read_xcr(0);
	debug_print("Step-8: Execute XGETBV with ECX = 0 when CPL is 0. CPL = %d, XCR0 = 0x%lx \n",
		cpl, xcr0);
	bool cond_ring0_3 = ((cpl == 0) && (xcr0 == xcr0_set_val_helper_22861));

	debug_print("Step-10: Execute XGETBV with ECX = 0 when CPL is 3. \n");
	cond_ring3_helper_22861 = false;
	do_at_ring3(helper_22861_read_xcr0_at_ring3, "");
	bool cond_ring3_3 = cond_ring3_helper_22861;

	debug_print("Step-11: Write 7 to XCR0. \n");
	xcr0_set_val_helper_22861 = STATE_X87 | STATE_SSE | STATE_AVX;
	asm_write_xcr(0, xcr0_set_val_helper_22861);

	cpl = read_cs() & 0x3;
	xcr0 = asm_read_xcr(0);
	debug_print("Step-12: Execute XGETBV with ECX = 0 when CPL is 0. CPL = %d, XCR0 = 0x%lx \n",
		cpl, xcr0);
	bool cond_ring0_7 = ((cpl == 0) && (xcr0 == xcr0_set_val_helper_22861));

	debug_print("Step-14: Execute XGETBV with ECX = 0 when CPL is 3. \n");
	cond_ring3_helper_22861 = false;
	do_at_ring3(helper_22861_read_xcr0_at_ring3, "");
	bool cond_ring3_7 = cond_ring3_helper_22861;

	bool cond_ring0 = (cond_ring0_1 && cond_ring0_3 && cond_ring0_7);
	bool cond_ring3 = (cond_ring3_1 && cond_ring3_3 && cond_ring3_7);

	report("%s", (cond_1 && cond_ring0 && cond_ring3), __FUNCTION__);
}

/*
 * Case name: 22855:XSAVE_general_support_025_XCR0_rsvd_bits_gp.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * XCR0[63:10] and XCR0[8] are reserved.
 * Executing the XSETBV instruction causes a general-protection fault (#GP) if ECX = 0 and
 * any corresponding bit in EDX:EAX is not 0.
 */
static __unused void xsave_rqmid_22855_general_support_025_xcr0_rsvd_bits_gp(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Set XCR0[0] and XCR0[8] to 1. \n");
	uint64_t val = STATE_X87 | (1UL << 8U);
	struct xsave_exception ret_8 = xsetbv_exception_checking(0, val);
	debug_print("Expect #GP(0) exception (GP_VECTOR = %d). \n", GP_VECTOR);
	debug_print("Actual exception: ret_8.vector = %d, ret_8.error_code = %d \n",
		ret_8.vector, ret_8.error_code);

	bool cond_1 = ((ret_8.vector == GP_VECTOR) && (ret_8.error_code == 0));

	debug_print("Step-3: For each i satisfying 10 <= i < 64, set XCR0[0] and XCR0[i] to 1. \n");
	uint32_t i;
	struct xsave_exception ret;
	bool cond_2 = true;
	for (i = 10U; i < 64U; i++) {
		val = STATE_X87 | (1UL << i);
		ret = xsetbv_exception_checking(0, val);

		if ((ret.vector != GP_VECTOR) || (ret.error_code != 0)) {
			cond_2 = false;
			debug_print("Failure!!! Setting XCR0[%d] to 1. \n", i);
			debug_print("Expect #GP(0) exception (GP_VECTOR = %d). \n", GP_VECTOR);
			debug_print("Actual exception: ret.vector = %d, ret.error_code = %d \n",
				ret.vector, ret.error_code);
		}
	}

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

/*
 * Case name: 22856:XSAVE_general_support_026_CPUID_OSXSAVE.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * The value of CR4.OSXSAVE is returned in CPUID.1:ECX.OSXSAVE[bit 27].
 */
static __unused void xsave_rqmid_22856_general_support_026_cpuid_osxsave(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Clear CR4.OSXSAVE[18]. \n");
	asm_write_cr4_osxsave(0);

	uint32_t osxsave_support_0 = get_osxsave_support();
	debug_print("Step-2: Check if CPUID.1:ECX.OSXSAVE is 0. osxsave_support_0 = 0x%x \n", osxsave_support_0);

	debug_print("Step-3: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	uint32_t osxsave_support_1 = get_osxsave_support();
	debug_print("Step-4: Check if CPUID.1:ECX.OSXSAVE is 1. osxsave_support_1 = 0x%x \n", osxsave_support_1);

	bool cond_1 = (osxsave_support_0 == 0);
	bool cond_2 = (osxsave_support_1 != 0);

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

/*
 * Case name: 22862:XSAVE_general_support_019_NM.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * Execute XSAVE, XRSTOR, XSAVEC, XSAVEOPT.
 * If CR0.TS[bit 3] is 1, a device-not-available exception (#NM) occurs.
 */
static __unused void xsave_rqmid_22862_general_support_019_nm(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Set CR0.TS[bit 3] to 1. \n");
	uint64_t cr0 = read_cr0();
	uint64_t cr0_set_ts = cr0 | CR0_TS_MASK;
	write_cr0(cr0_set_ts);

	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-3: Execute XSAVE with 64-byte aligned memory specified \n");
	struct xsave_exception xsave_ret = xsave_exception_checking(&area_test, 0);
	debug_print("Expect #NM exception (NM_VECTOR = %d). Actual exception: xsave_ret.vector = %d \n",
		NM_VECTOR, xsave_ret.vector);

	debug_print("Step-4: Execute XRSTOR with 64-byte aligned memory specified \n");
	struct xsave_exception xrstor_ret = xrstor_exception_checking(&area_test, 0);
	debug_print("Expect #NM exception (NM_VECTOR = %d). Actual exception: xrstor_ret.vector = %d \n",
		NM_VECTOR, xrstor_ret.vector);

	debug_print("Step-5: Execute XSAVEC with 64-byte aligned memory specified \n");
	struct xsave_exception xsavec_ret = xsavec_exception_checking(&area_test, 0);
	debug_print("Expect #NM exception (NM_VECTOR = %d). Actual exception: xsavec_ret.vector = %d \n",
		NM_VECTOR, xsavec_ret.vector);

	debug_print("Step-6: Execute XSAVEOPT with 64-byte aligned memory specified \n");
	struct xsave_exception xsaveopt_ret = xsaveopt_exception_checking(&area_test, 0);
	debug_print("Expect #NM exception (NM_VECTOR = %d). Actual exception: xsaveopt_ret.vector = %d \n",
		NM_VECTOR, xsaveopt_ret.vector);

	/* Clear CR0.TS[bit 3] */
	write_cr0(cr0);

	bool cond_1 = (xsave_ret.vector == NM_VECTOR);
	bool cond_2 = (xrstor_ret.vector == NM_VECTOR);
	bool cond_3 = (xsavec_ret.vector == NM_VECTOR);
	bool cond_4 = (xsaveopt_ret.vector == NM_VECTOR);

	report("%s", (cond_1 && cond_2 && cond_3 && cond_4), __FUNCTION__);
}

/*
 * Case name: 28393:XSAVE_general_support_009_non_aligned_addr_GP.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * Execute XSAVE, XRSTOR, XSAVEC, XSAVEOPT.
 * If the address of the XSAVE area is not 64-byte aligned
 * (following conditions are not satisfied: CR0.AM = 1, CPL = 3, and EFLAGS.AC = 1),
 * a general-protection exception (#GP) occurs.
 */
static __unused void xsave_rqmid_28393_general_support_009_non_aligned_addr_gp(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	uint16_t cpl = read_cs() & 0x3;
	uint64_t cr0_am = read_cr0() & CR0_AM_MASK;
	uint64_t eflags_ac = read_rflags() & X86_EFLAGS_AC;
	debug_print("Step-2: Ensure following conditions are not all satisfied: CR0.AM = 1, CPL = 3, EFLAGS.AC = 1 \n");
	debug_print("cpl = %d, cr0_am = 0x%lx, eflags_ac = 0x%lx \n", cpl, cr0_am, eflags_ac);

	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	uint64_t non_aligned_addr = (uint64_t)&area_test + 1U;
	struct xsave_area *area_test_non_aligned = (struct xsave_area *)non_aligned_addr;
	debug_print("Construct a non-64-byte aligned address. non_aligned_addr = 0x%lx \n", non_aligned_addr);

	debug_print("Step-3: Execute XSAVE with non-64-byte aligned memory specified \n");
	struct xsave_exception xsave_ret = xsave_exception_checking(area_test_non_aligned, 0);
	debug_print("Expect #GP(0) exception (GP_VECTOR = %d). \n", GP_VECTOR);
	debug_print("Actual exception: xsave_ret.vector = %d, xsave_ret.error_code = %d \n",
		xsave_ret.vector, xsave_ret.error_code);

	debug_print("Step-4: Execute XRSTOR with non-64-byte aligned memory specified \n");
	struct xsave_exception xrstor_ret = xrstor_exception_checking(area_test_non_aligned, 0);
	debug_print("Expect #GP(0) exception (GP_VECTOR = %d). \n", GP_VECTOR);
	debug_print("Actual exception: xrstor_ret.vector = %d, xrstor_ret.error_code = %d \n",
		xrstor_ret.vector, xrstor_ret.error_code);

	debug_print("Step-5: Execute XSAVEC with non-64-byte aligned memory specified \n");
	struct xsave_exception xsavec_ret = xsavec_exception_checking(area_test_non_aligned, 0);
	debug_print("Expect #GP(0) exception (GP_VECTOR = %d). \n", GP_VECTOR);
	debug_print("Actual exception: xsavec_ret.vector = %d, xsavec_ret.error_code = %d \n",
		xsavec_ret.vector, xsavec_ret.error_code);

	debug_print("Step-6: Execute XSAVEOPT with non-64-byte aligned memory specified \n");
	struct xsave_exception xsaveopt_ret = xsaveopt_exception_checking(area_test_non_aligned, 0);
	debug_print("Expect #GP(0) exception (GP_VECTOR = %d). \n", GP_VECTOR);
	debug_print("Actual exception: xsaveopt_ret.vector = %d, xsaveopt_ret.error_code = %d \n",
		xsaveopt_ret.vector, xsaveopt_ret.error_code);

	bool cond_0 = !((cr0_am != 0) && (cpl == 3) && (eflags_ac != 0));
	bool cond_1 = ((xsave_ret.vector == GP_VECTOR) && (xsave_ret.error_code == 0));
	bool cond_2 = ((xrstor_ret.vector == GP_VECTOR) && (xrstor_ret.error_code == 0));
	bool cond_3 = ((xsavec_ret.vector == GP_VECTOR) && (xsavec_ret.error_code == 0));
	bool cond_4 = ((xsaveopt_ret.vector == GP_VECTOR) && (xsaveopt_ret.error_code == 0));

	report("%s", (cond_0 && cond_1 && cond_2 && cond_3 && cond_4), __FUNCTION__);
}

static uint64_t non_aligned_helper_28395;
static struct xsave_exception xsave_ret_helper_28395;
static struct xsave_exception xrstor_ret_helper_28395;
static struct xsave_exception xsavec_ret_helper_28395;
static struct xsave_exception xsaveopt_ret_helper_28395;

static __unused void helper_28395_at_ring3(const char *msg)
{
	/*
	 * As Alignment Check has been enabled, it's very easy to trigger an #AC exception.
	 * So, write the test code as pure as possible to avoid unintentional exceptions.
	 */

	/* Execute XSAVE with non-64-byte aligned memory specified */
	asm volatile(ASM_TRY("1f")
			"xsave %[addr]\n\t"
			"1:"
			: : [addr]"m" (*((struct xsave_area *)non_aligned_helper_28395)),
			"d" (0),
			"a" (0) :
			"memory");
	asm("movb %%gs:"xstr(EXCEPTION_VECTOR_ADDR)", %0" : "=q"(xsave_ret_helper_28395.vector));
	asm("mov %%gs:"xstr(EXCEPTION_ECODE_ADDR)", %0" : "=rm"(xsave_ret_helper_28395.error_code));

	/* Execute XRSTOR with non-64-byte aligned memory specified */
	asm volatile(ASM_TRY("1f")
			"xrstor %[addr]\n\t"
			"1:"
			: : [addr]"m" (*((struct xsave_area *)non_aligned_helper_28395)),
			"d" (0),
			"a" (0) :
			"memory");
	asm("movb %%gs:"xstr(EXCEPTION_VECTOR_ADDR)", %0" : "=q"(xrstor_ret_helper_28395.vector));
	asm("mov %%gs:"xstr(EXCEPTION_ECODE_ADDR)", %0" : "=rm"(xrstor_ret_helper_28395.error_code));

	/* Execute XSAVEC with non-64-byte aligned memory specified */
	asm volatile(ASM_TRY("1f")
			"xsavec %[addr]\n\t"
			"1:"
			: : [addr]"m" (*((struct xsave_area *)non_aligned_helper_28395)),
			"d" (0),
			"a" (0) :
			"memory");
	asm("movb %%gs:"xstr(EXCEPTION_VECTOR_ADDR)", %0" : "=q"(xsavec_ret_helper_28395.vector));
	asm("mov %%gs:"xstr(EXCEPTION_ECODE_ADDR)", %0" : "=rm"(xsavec_ret_helper_28395.error_code));

	/* Execute XSAVEOPT with non-64-byte aligned memory specified */
	asm volatile(ASM_TRY("1f")
			"xsaveopt %[addr]\n\t"
			"1:"
			: : [addr]"m" (*((struct xsave_area *)non_aligned_helper_28395)),
			"d" (0),
			"a" (0) :
			"memory");
	asm("movb %%gs:"xstr(EXCEPTION_VECTOR_ADDR)", %0" : "=q"(xsaveopt_ret_helper_28395.vector));
	asm("mov %%gs:"xstr(EXCEPTION_ECODE_ADDR)", %0" : "=rm"(xsaveopt_ret_helper_28395.error_code));
}

/*
 * Case name: 28395:XSAVE_general_support_012_non_aligned_addr_AC.
 *
 * Summary: DNG_132148: XSAVE general support.
 *
 * Execute XSAVE, XRSTOR, XSAVEC, XSAVEOPT.
 * If the address of the XSAVE area is not 64-byte aligned, CR0.AM = 1, CPL = 3, and EFLAGS.AC = 1,
 * an alignment-check exception (#AC) may occur instead of #GP.
 */
static __unused void xsave_rqmid_28395_general_support_012_non_aligned_addr_ac(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Set CR0.AM to 1. \n");
	uint64_t cr0 = read_cr0();
	write_cr0(cr0 | CR0_AM_MASK);

	debug_print("Step-3: Set EFLAGS.AC to 1. \n");
	uint64_t eflags = read_rflags();
	write_rflags(eflags | X86_EFLAGS_AC);

	debug_print("Check if CR0.AM = 1 and EFLAGS.AC = 1. cr0_am = 0x%lx, eflags_ac = 0x%lx \n",
		read_cr0() & CR0_AM_MASK, read_rflags() & X86_EFLAGS_AC);

	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	non_aligned_helper_28395 = (uint64_t)&area_test + 1U;
	debug_print("Construct a non-64-byte aligned address. non_aligned_helper_28395 = 0x%lx \n",
		non_aligned_helper_28395);

	debug_print("Step-4: Switch to ring3 (CPL = 3) \n");
	do_at_ring3(helper_28395_at_ring3, "");

	debug_print("Step-5: Execute XSAVE with non-64-byte aligned memory specified \n");
	debug_print("Expect #AC(0) exception (AC_VECTOR = %d). \n", AC_VECTOR);
	debug_print("Actual: xsave_ret_helper_28395.vector = %d, xsave_ret_helper_28395.error_code = %d \n\n",
		xsave_ret_helper_28395.vector, xsave_ret_helper_28395.error_code);

	debug_print("Step-6: Execute XRSTOR with non-64-byte aligned memory specified \n");
	debug_print("Expect #AC(0) exception (AC_VECTOR = %d). \n", AC_VECTOR);
	debug_print("Actual: xrstor_ret_helper_28395.vector = %d, xrstor_ret_helper_28395.error_code = %d \n\n",
		xrstor_ret_helper_28395.vector, xrstor_ret_helper_28395.error_code);

	debug_print("Step-7: Execute XSAVEC with non-64-byte aligned memory specified \n");
	debug_print("Expect #AC(0) exception (AC_VECTOR = %d). \n", AC_VECTOR);
	debug_print("Actual: xsavec_ret_helper_28395.vector = %d, xsavec_ret_helper_28395.error_code = %d \n\n",
		xsavec_ret_helper_28395.vector, xsavec_ret_helper_28395.error_code);

	debug_print("Step-8: Execute XSAVEOPT with non-64-byte aligned memory specified \n");
	debug_print("Expect #AC(0) exception (AC_VECTOR = %d). \n", AC_VECTOR);
	debug_print("Actual: xsaveopt_ret_helper_28395.vector = %d, xsaveopt_ret_helper_28395.error_code = %d \n\n",
		xsaveopt_ret_helper_28395.vector, xsaveopt_ret_helper_28395.error_code);

	bool cond_1 = ((xsave_ret_helper_28395.vector == AC_VECTOR) && (xsave_ret_helper_28395.error_code == 0));
	bool cond_2 = ((xrstor_ret_helper_28395.vector == AC_VECTOR) && (xrstor_ret_helper_28395.error_code == 0));
	bool cond_3 = ((xsavec_ret_helper_28395.vector == AC_VECTOR) && (xsavec_ret_helper_28395.error_code == 0));
	bool cond_4 = ((xsaveopt_ret_helper_28395.vector == AC_VECTOR) && (xsaveopt_ret_helper_28395.error_code == 0));

	report("%s", (cond_1 && cond_2 && cond_3 && cond_4), __FUNCTION__);

	xsave_clean_up_env();
}

/*
 * Case name: 22847:XSAVE_expose x87_support_001.
 *
 * Summary:
 * XCR0[0] is always 1.
 * Executing the XSETBV instruction causes a general-protection fault (#GP) if ECX = 0 and EAX[0] is 0.
 */
static __unused void xsave_rqmid_22847_expose_x87_support_001(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	uint64_t xcr0_x87 = asm_read_xcr(0) & STATE_X87;
	debug_print("Step-2: Check if XCR0[0] is 1. xcr0_x87 = 0x%lx \n", xcr0_x87);

	debug_print("Step-3: Execute XSETBV with EDX:EAX is 0 and ECX is 0. \n");
	struct xsave_exception xsetbv_ret = xsetbv_exception_checking(0, 0);
	debug_print("Expect #GP(0) exception (GP_VECTOR = %d). \n", GP_VECTOR);
	debug_print("Actual exception: xsetbv_ret.vector = %d, xsetbv_ret.error_code = %d \n",
		xsetbv_ret.vector, xsetbv_ret.error_code);

	bool cond_1 = (xcr0_x87 != 0);
	bool cond_2 = ((xsetbv_ret.vector == GP_VECTOR) && (xsetbv_ret.error_code == 0));

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

static __unused struct xsave_exception x87_exception_checking(void)
{
	float f_val = 2.5;

	asm volatile(ASM_TRY("1f")
			"flds %0\n\t"
			"1:"
			: : "m"(f_val) : "memory");

	struct xsave_exception ret;
	ret.vector = exception_vector();
	ret.error_code = exception_error_code();
	return ret;
}

/*
 * Case name: 27758:XSAVE_expose x87_support_003.
 *
 * Summary:
 * The XSAVE feature set can operate on x87 state only if the feature set is enabled (CR4.OSXSAVE = 1).
 */
static __unused void xsave_rqmid_27758_expose_x87_support_003(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Execute X87 instruction. \n");
	float f_val = 2.5;
	asm volatile("fninit");
	asm volatile("flds %0\n" : : "m"(f_val) : "memory");

	debug_print("Step-3: Set the FXSAVE area to all 0s \n");
	ALIGNED(16) struct xsave_fxsave_struct fxsave_test;
	fxsave_test.fcw = 0;
	memset(&fxsave_test, 0, sizeof(struct xsave_fxsave_struct));

	debug_print("Step-4: Execute FXSAVE instruction to save ST0 register. \n");
	asm_fxsave(&fxsave_test);
	uint64_t expected_st0_low64 = fxsave_test.fpregs[0];
	uint64_t expected_st0_hi16 = fxsave_test.fpregs[1] & XSAVE_ST0_BITS_79_64_MASK;

	debug_print("Step-5: Set the XSAVE area to all 0s \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-6: Execute XSAVE instruction with EDX:EAX = 1. \n");
	asm_xsave(&area_test, 1);
	uint64_t st0_low64 = area_test.legacy_region[XSAVE_ST0_OFFSET];
	uint64_t st0_hi16 = area_test.legacy_region[XSAVE_ST0_OFFSET + 1U] & XSAVE_ST0_BITS_79_64_MASK;

	debug_print("Step-7: Check ST0 register value. \n");
	debug_print("st0_low64 = 0x%lx expected_st0_low64 = 0x%lx \n", st0_low64, expected_st0_low64);
	debug_print("st0_hi16 = 0x%lx expected_st0_hi16 = 0x%lx \n", st0_hi16, expected_st0_hi16);

	debug_print("Step-8: Clear CR4.OSXSAVE[18]. \n");
	asm_write_cr4_osxsave(0);

	debug_print("Step-9: Set the XSAVE area to all 0s \n");
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-10: Execute X87 instruction. No exception shall be triggered. \n");
	struct xsave_exception x87_ret = x87_exception_checking();

	debug_print("Step-11: Execute XSAVE with with EDX:EAX = 1 \n");
	struct xsave_exception xsave_ret = xsave_exception_checking(&area_test, 1);
	debug_print("Expect #UD exception (UD_VECTOR = %d). Actual exception: xsave_ret.vector = %d \n",
		UD_VECTOR, xsave_ret.vector);

	bool cond_1 = ((st0_low64 == expected_st0_low64) && (st0_hi16 == expected_st0_hi16));
	bool cond_2 = (x87_ret.vector == NO_EXCEPTION);
	bool cond_3 = (xsave_ret.vector == UD_VECTOR);

	report("%s", (cond_1 && cond_2 && cond_3), __FUNCTION__);
}

static __unused struct xsave_exception sse_exception_checking(void)
{
	sse_union sse_test;
	sse_test.sse_u64[0] = 0xAAUL;
	sse_test.sse_u64[1] = 0xBBUL;

	asm volatile(ASM_TRY("1f")
			"movapd %0, %%xmm0\n\t"
			"1:"
			: : "m"(sse_test));

	struct xsave_exception ret;
	ret.vector = exception_vector();
	ret.error_code = exception_error_code();
	return ret;
}

/*
 * Case name: 27760:XSAVE_expose_SSE_support_002.
 *
 * Summary:
 * The XSAVE feature set can operate on SSE state only if the feature set is enabled (CR4.OSXSAVE = 1) and
 * has been configured to manage SSE state (XCR0[1] = 1).
 */
static __unused void xsave_rqmid_27760_expose_SSE_support_002(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Execute SSE instruction. \n");
	sse_union sse_test;
	sse_test.sse_u64[0] = 0xAAUL;
	sse_test.sse_u64[1] = 0xBBUL;
	asm volatile("movapd %0, %%xmm0" : : "m"(sse_test));

	debug_print("Step-3: Set the FXSAVE area to all 0s \n");
	ALIGNED(16) struct xsave_fxsave_struct fxsave_test;
	fxsave_test.fcw = 0;
	memset(&fxsave_test, 0, sizeof(struct xsave_fxsave_struct));

	debug_print("Step-4: Execute FXSAVE instruction to save XMM0 register. \n");
	asm_fxsave(&fxsave_test);
	uint64_t expected_xmm0_low64 = fxsave_test.xmm_regs[0];
	uint64_t expected_xmm0_hi64 = fxsave_test.xmm_regs[1];

	debug_print("Step-5: Execute XSETBV with ECX = 0 and EDX:EAX = 3. \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE);

	debug_print("Step-6: Set the XSAVE area to all 0s \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-7: Execute XSAVE instruction with EDX:EAX = 2. \n");
	asm_xsave(&area_test, STATE_SSE);
	uint64_t xmm0_low64 = area_test.legacy_region[XSAVE_XMM0_OFFSET];
	uint64_t xmm0_hi64 = area_test.legacy_region[XSAVE_XMM0_OFFSET + 1U];

	debug_print("Step-8: Check XMM0 register value in the XSAVE area. \n");
	debug_print("xmm0_low64 = 0x%lx expected_xmm0_low64 = 0x%lx \n", xmm0_low64, expected_xmm0_low64);
	debug_print("xmm0_hi64 = 0x%lx expected_xmm0_hi64 = 0x%lx \n", xmm0_hi64, expected_xmm0_hi64);

	debug_print("Step-9: Execute XSETBV with ECX = 0 and EDX:EAX = 1. \n");
	asm_write_xcr(0, STATE_X87);

	debug_print("Step-10: Set the XSAVE area to all 0s \n");
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-11: Execute XSAVE instruction with EDX:EAX = 2. \n");
	asm_xsave(&area_test, STATE_SSE);
	uint64_t xmm0_2nd_low64 = area_test.legacy_region[XSAVE_XMM0_OFFSET];
	uint64_t xmm0_2nd_hi64 = area_test.legacy_region[XSAVE_XMM0_OFFSET + 1U];

	debug_print("Step-12: Check XMM0 register value in the XSAVE area. \n");
	debug_print("xmm0_2nd_low64 = 0x%lx expected value = 0x%lx \n", xmm0_2nd_low64, 0UL);
	debug_print("xmm0_2nd_hi64 = 0x%lx expected value = 0x%lx \n", xmm0_2nd_hi64, 0UL);

	debug_print("Step-13: Clear CR4.OSXSAVE[18]. \n");
	asm_write_cr4_osxsave(0);

	debug_print("Step-14: Set the XSAVE area to all 0s \n");
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-15: Execute SSE instruction. No exception shall be triggered. \n");
	struct xsave_exception sse_ret = sse_exception_checking();

	debug_print("Step-16: Execute XSAVE with EDX:EAX = 2 \n");
	struct xsave_exception xsave_ret = xsave_exception_checking(&area_test, STATE_SSE);
	debug_print("Expect #UD exception (UD_VECTOR = %d). Actual exception: xsave_ret.vector = %d \n",
		UD_VECTOR, xsave_ret.vector);

	bool cond_1 = ((xmm0_low64 == expected_xmm0_low64) && (xmm0_hi64 == expected_xmm0_hi64));
	bool cond_2 = ((xmm0_2nd_low64 == 0UL) && (xmm0_2nd_hi64 == 0UL));
	bool cond_3 = (sse_ret.vector == NO_EXCEPTION);
	bool cond_4 = (xsave_ret.vector == UD_VECTOR);

	report("%s", (cond_1 && cond_2 && cond_3 && cond_4), __FUNCTION__);
}

/*
 * Case name: 22864:XSAVE_expose_AVX_support_002.
 *
 * Summary:
 * Executing the XSETBV instruction causes a general-protection fault (#GP) if ECX = 0 and EAX[2:1] has the value 10b.
 */
static __unused void xsave_rqmid_22864_expose_AVX_support_002(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Execute XSETBV with EDX:EAX is 5 and ECX is 0. \n");
	struct xsave_exception xsetbv_ret = xsetbv_exception_checking(0, STATE_X87 | STATE_AVX);
	debug_print("Expect #GP(0) exception (GP_VECTOR = %d). \n", GP_VECTOR);
	debug_print("Actual exception: xsetbv_ret.vector = %d, xsetbv_ret.error_code = %d \n",
		xsetbv_ret.vector, xsetbv_ret.error_code);

	report("%s", ((xsetbv_ret.vector == GP_VECTOR) && (xsetbv_ret.error_code == 0)), __FUNCTION__);
}

#define AVX_TEST_BITS		(~0UL)

/*
 * Case name: 22640:XSAVE_expose_AVX_support_003.
 *
 * Summary:
 * Software can use the XSAVE feature set to manage AVX state only if XCR0[2] = 1.
 *
 * This test case verifies that the value of YMM0 register can be saved to memory when XCR0[2] is 1.
 */
static __unused void xsave_rqmid_22640_expose_AVX_support_003(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Write 7 to XCR0. \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE | STATE_AVX);

	debug_print("Step-3: Execute AVX instruction to let YMM0 be non-zero. \n");
	ALIGNED(32) avx_union avx_test;
	uint16_t i;
	for (i = 0U; i < 4U; i++) {
		avx_test.avx_u64[i] = AVX_TEST_BITS;
	}
	asm volatile("vmovapd %0, %%ymm0" : : "m"(avx_test));

	debug_print("Step-4: Set the XSAVE area to all 0s. \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-5: Execute XSAVE instruction with EDX:EAX = 6. \n");
	asm_xsave(&area_test, STATE_SSE | STATE_AVX);
	/* low 128 bits saved in XMM0 in the legacy region */
	uint64_t ymm0_low_128_bits[2];
	ymm0_low_128_bits[0] = area_test.legacy_region[XSAVE_XMM0_OFFSET];
	ymm0_low_128_bits[1] = area_test.legacy_region[XSAVE_XMM0_OFFSET + 1U];
	/* high 128 bits saved in YMM0 in the extended region */
	uint64_t ymm0_hi_128_bits[2];
	ymm0_hi_128_bits[0] = area_test.extend_region[XSAVE_YMM0_OFFSET];
	ymm0_hi_128_bits[1] = area_test.extend_region[XSAVE_YMM0_OFFSET + 1U];

	debug_print("Step-6: Check YMM0 register value in the XSAVE area after executing XSAVE instruction. \n");
	uint64_t expected_ymm0_low_128_bits[2];
	expected_ymm0_low_128_bits[0] = AVX_TEST_BITS;
	expected_ymm0_low_128_bits[1] = AVX_TEST_BITS;

	uint64_t expected_ymm0_hi_128_bits[2];
	expected_ymm0_hi_128_bits[0] = AVX_TEST_BITS;
	expected_ymm0_hi_128_bits[1] = AVX_TEST_BITS;

	/* low 128 bits */
	bool cond_1 = true;
	if (memcmp(ymm0_low_128_bits, expected_ymm0_low_128_bits, 16U)) {
		cond_1 = false;
	}

	/* high 128 bits */
	bool cond_2 = true;
	if (memcmp(ymm0_hi_128_bits, expected_ymm0_hi_128_bits, 16U)) {
		cond_2 = false;
	}

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

static __unused void xsave_avx_ymm0_ymm15(void)
{
	ALIGNED(32) avx_union avx_test;
	uint16_t i;
	for (i = 0U; i < 4U; i++) {
		avx_test.avx_u64[i] = AVX_TEST_BITS;
	}

	asm volatile("vmovapd %0, %%ymm0" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm1" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm2" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm3" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm4" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm5" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm6" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm7" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm8" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm9" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm10" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm11" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm12" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm13" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm14" : : "m"(avx_test));
	asm volatile("vmovapd %0, %%ymm15" : : "m"(avx_test));
}

/*
 * Case name: 37219:XSAVE_expose_AVX_support_64_bit_mode.
 *
 * Summary:
 * Bytes 127:0 of the AVX-state section are used for YMM0_H–YMM7_H.
 * Bytes 255:128 are used for YMM8_H–YMM15_H, but they are used only in 64-bit mode.
 */
static __unused void xsave_rqmid_37219_expose_AVX_support_64_bit_mode(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Write 7 to XCR0. \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE | STATE_AVX);

	debug_print("Step-3: Execute AVX instruction to YMM0~YMM15 be non-zero. \n");
	xsave_avx_ymm0_ymm15();

	debug_print("Step-4: Set the XSAVE area to all 0s. n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-5: Execute XSAVE instruction with EDX:EAX = 4. \n");
	asm_xsave(&area_test, STATE_AVX);

	uint16_t arr_size = XSAVE_AVX_AREA_SIZE / sizeof(uint64_t);
	uint16_t i;

	uint64_t test_avx_region[arr_size];
	for (i = 0U; i < arr_size; i++) {
		test_avx_region[i] = area_test.extend_region[i];
	}

	uint64_t expected_avx_region[arr_size];
	for (i = 0U; i < arr_size; i++) {
		expected_avx_region[i] = AVX_TEST_BITS;
	}

	debug_print("Step-6: Check bytes 255:0 of the AVX-state section in the XSAVE area. \n");
	bool cond = true;
	if (memcmp(test_avx_region, expected_avx_region, sizeof(XSAVE_AVX_AREA_SIZE))) {
		cond = false;
	}

	report("%s", cond, __FUNCTION__);
}

/*
 * Case name: 27761:XSAVE_expose_AVX_support_004.
 *
 * Summary:
 * The XSAVE feature set can operate on AVX state only if the feature set is enabled (CR4.OSXSAVE = 1) and
 * has been configured to manage AVX state (XCR0[2] = 1).
 */
static __unused void xsave_rqmid_27761_expose_AVX_support_004(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Write 1 to XCR0. \n");
	asm_write_xcr(0, STATE_X87);

	debug_print("Step-3: Execute AVX instruction \n");
	ALIGNED(32) avx_union avx_test;
	avx_test.m[0] = 2.5f;
	int avx_ret = vmovapd_check(&avx_test);
	debug_print("Expect #UD exception (UD_VECTOR = %d). Actual exception: avx_ret = %d \n",
		UD_VECTOR, avx_ret);

	report("%s", (avx_ret == UD_VECTOR), __FUNCTION__);
}

/*
 * Case name: 23631:XSAVE hide PKRU support_001.
 *
 * Summary: DNG_129928: XSAVE hide PKRU support.
 * ACRN hypervisor shall hide XSAVE-managed PKRU states from any VM, in compliance with Chapter 13.2 and 13.3,
 * Vol. 1, SDM.
 *
 * Check if CPUID.(EAX=DH, ECX=0H):EAX[9] is 0
 */
static __unused void xsave_rqmid_23631_hide_pkru_support_001(void)
{
	xsave_clean_up_env();

	uint32_t pkru_support = (cpuid_indexed(CPUID_XSAVE_FUC, 0).a) & STATE_PKRU;
	debug_print("Check if CPUID.(EAX=DH, ECX=0H):EAX[9] is 0: pkru_support = 0x%x \n", pkru_support);

	report("%s", (pkru_support == 0), __FUNCTION__);
}

/*
 * Case name: 23632:XSAVE hide MPX support_001.
 *
 * Summary: DNG_129926: XSAVE hide MPX support.
 * ACRN hypervisor shall hide XSAVE-managed MPX states from any VM, in compliance with Chapter 13.2 and 13.3,
 * Vol. 1, SDM.
 *
 * Check if CPUID.(EAX=DH, ECX=0H):EAX[4:3] is 0
 */
static __unused void xsave_rqmid_23632_hide_mpx_support_001(void)
{
	xsave_clean_up_env();

	uint32_t mpx_bndregs_support = (cpuid_indexed(CPUID_XSAVE_FUC, 0).a) & STATE_MPX_BNDREGS;
	debug_print("Check if CPUID.(EAX=DH, ECX=0H):EAX[3] is 0: mpx_bndregs_support = 0x%x \n", mpx_bndregs_support);

	uint32_t mpx_bndcsr_support = (cpuid_indexed(CPUID_XSAVE_FUC, 0).a) & STATE_MPX_BNDCSR;
	debug_print("Check if CPUID.(EAX=DH, ECX=0H):EAX[4] is 0: mpx_bndcsr_support = 0x%x \n", mpx_bndcsr_support);

	bool cond_1 = (mpx_bndregs_support == 0);
	bool cond_2 = (mpx_bndcsr_support == 0);

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

/*
 * Case name: 22828:XSAVE_supervisor_state_components_002
 *
 * Summary:
 * CPUID function 0DH, sub-function 1:
 * EDX:ECX is a bitmap of all the supervisor state components that can be managed by XSAVES and XRSTORS.
 */
static __unused void xsave_rqmid_22828_supervisor_state_components_002(void)
{
	xsave_clean_up_env();

	uint64_t supported_supervisor_states = get_supported_supervisor_states();
	debug_print("Check if CPUID.(EAX=DH, ECX=1H):(EDX:ECX) is 0: supported_supervisor_states = 0x%lx \n",
		supported_supervisor_states);

	report("%s", (supported_supervisor_states == XSAVE_SUPPORTED_SUPERVISOR_STATES), __FUNCTION__);
}

/*
 * Case name: 22799:XSAVE_compaction_extensions_001.
 *
 * Summary:
 * CPUID.(EAX=DH, ECX=1H):EAX[1] enumerates support for the XSAVEC instruction.
 */
static __unused void xsave_rqmid_22799_compaction_extensions_001(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	uint32_t xsavec_support = get_xsavec_support();
	debug_print("Step-2: Check if CPUID.(EAX=DH, ECX=1H):EAX[1] is 1: xsavec_support = 0x%x \n",
		xsavec_support);

	debug_print("Step-3: Execute XSAVEC instruction. No exception shall be triggered. \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));
	struct xsave_exception xsavec_ret = xsavec_exception_checking(&area_test, 0);

	bool cond_1 = (xsavec_support != 0);
	bool cond_2 = (xsavec_ret.vector == NO_EXCEPTION);

	report("%s", (cond_1 && cond_2), __FUNCTION__);
}

/*
 * Case name: 37042:XSAVE_compaction_extensions_002.
 *
 * Summary:
 * Verify XSAVEC instruction behaviors.
 */
static __unused void xsave_rqmid_37042_compaction_extensions_002(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Write 7 to XCR0. \n");
	asm_write_xcr(0, STATE_X87 | STATE_SSE | STATE_AVX);

	debug_print("Step-3: Execute AVX instruction to let YMM0 be non-zero. \n");
	ALIGNED(32) avx_union avx_test;
	avx_test.avx_u64[2] = AVX_TEST_BITS;
	avx_test.avx_u64[3] = AVX_TEST_BITS;
	asm volatile("vmovapd %0, %%ymm0" : : "m"(avx_test));

	debug_print("Step-4: Set the XSAVE area to all 0s. \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-5: Execute XSAVEC instruction with EDX:EAX is 4. \n");
	asm_xsavec(&area_test, STATE_AVX);

	debug_print("Step-6: Check XSTATE_BV in the XSAVE area (XSTATE_BV[bit 2] shall be 1): \n");
	uint64_t expected_xstate_bv_avx = STATE_AVX;
	uint64_t xstate_bv_test_avx = area_test.xsave_hdr.hdr.xstate_bv & STATE_AVX;
	debug_print("xstate_bv_test_avx = 0x%lx, expected_xstate_bv_avx = 0x%lx \n",
		xstate_bv_test_avx, expected_xstate_bv_avx);

	debug_print("Step-6: Check XCOMP_BV in the XSAVE area (XCOMP_BV[bit 2] and XCOMP_BV[bit 63] shall be 1): \n");
	uint64_t expected_xcomp_bv = STATE_AVX | XSAVE_COMPACTED_FORMAT;
	uint64_t xcomp_bv_test = area_test.xsave_hdr.hdr.xcomp_bv & (STATE_AVX | XSAVE_COMPACTED_FORMAT);
	debug_print("xcomp_bv_test = 0x%lx, expected_xcomp_bv = 0x%lx \n",
		xcomp_bv_test, expected_xcomp_bv);

	debug_print("Step-7: Check YMM0 in the XSAVE area after executing XSAVE instruction. \n");
	/* high 128 bits saved in YMM0 in the extended region */
	uint64_t ymm0_hi_128_bits[2];
	ymm0_hi_128_bits[0] = area_test.extend_region[XSAVE_YMM0_OFFSET];
	ymm0_hi_128_bits[1] = area_test.extend_region[XSAVE_YMM0_OFFSET + 1U];

	uint64_t expected_ymm0_hi_128_bits[2];
	expected_ymm0_hi_128_bits[0] = AVX_TEST_BITS;
	expected_ymm0_hi_128_bits[1] = AVX_TEST_BITS;

	bool cond_1 = (xstate_bv_test_avx == expected_xstate_bv_avx);
	bool cond_2 = (xcomp_bv_test == expected_xcomp_bv);

	bool cond_3 = true;
	if (memcmp(ymm0_hi_128_bits, expected_ymm0_hi_128_bits, 16U)) {
		cond_3 = false;
	}

	report("%s", (cond_1 && cond_2 && cond_3), __FUNCTION__);
}

/*
 * Case name: 28387:XSAVE_Compacted_Form_of_XRSTOR_001.
 *
 * Summary:
 *
 * Vol1_13.8.2:
 * The compacted from of XRSTOR performs additional fault checking. Any of the following conditions causes a #GP:
 * The XCOMP_BV field of the XSAVE header sets a bit in the range 62:0 that is not set in XCR0.
 */
static __unused void xsave_rqmid_28397_compacted_form_of_xrstor_001(void)
{
	xsave_clean_up_env();

	debug_print("Step-1: Set CR4.OSXSAVE[18] to 1. \n");
	asm_write_cr4_osxsave(1);

	debug_print("Step-2: Write 1 to XCR0. \n");
	asm_write_xcr(0, STATE_X87);

	debug_print("Step-3: Set the XSAVE area to all 0s \n");
	ALIGNED(64) struct xsave_area area_test;
	area_test.legacy_region[0] = 0;
	memset(&area_test, 0, sizeof(struct xsave_area));

	debug_print("Step-4: Set XCOMP_BV[63] and XCOMP_BV[2] to 1 \n");
	area_test.xsave_hdr.hdr.xcomp_bv = XSAVE_COMPACTED_FORMAT | STATE_AVX;
	debug_print("area_test.xsave_hdr.hdr.xcomp_bv = 0x%lx \n", area_test.xsave_hdr.hdr.xcomp_bv);

	debug_print("Step-5: Execute XRSTOR with 64-byte aligned memory specified \n");
	struct xsave_exception xrstor_ret = xrstor_exception_checking(&area_test, 1);
	debug_print("Expect #GP(0) exception (GP_VECTOR = %d). \n", GP_VECTOR);
	debug_print("Actual exception: xrstor_ret.vector = %d, xrstor_ret.error_code = %d \n",
		xrstor_ret.vector, xrstor_ret.error_code);

	report("%s", ((xrstor_ret.vector == GP_VECTOR) && (xrstor_ret.error_code == 0)), __FUNCTION__);
}

/*
 * Case name: 40367: Physical platform doesn't support XSAVE-managed PKRU states (252410, Application Constraint)
 *
 * Summary:
 * Physical platform doesn't support XSAVE-managed PKRU states (252410, Application Constraint)
 *
 * Check if CPUID.(EAX=DH, ECX=0H):EAX[9] is 0
 */
static __unused void xsave_rqmid_40367_physical_pkru_support_001(void)
{
	uint32_t pkru_support = (cpuid_indexed(CPUID_XSAVE_FUC, 0).a) & STATE_PKRU;
	debug_print("Check if CPUID.(EAX=DH, ECX=0H):EAX[9] is 0: pkru_support = 0x%x \n", pkru_support);

	report("%s", (pkru_support == 0), __FUNCTION__);
}

#define NUM_NATIVE_CASES		7U
struct xsave_case native_cases[NUM_NATIVE_CASES] = {
	{28468u, "XSAVE physical compaction extensions_001"},
	{28386u, "XSAVE physical general support_001"},
	{28385u, "XSAVE physical x87 support_001"},
	{28388u, "XSAVE physical SSE support_001"},
	{28390u, "XSAVE physical AVX support_001"},
	{28392u, "XSAVE physical init and modified optimizations_001"},
	{40367u, "XSAVE physical PKRU support_001"},
};

#define NUM_NON_SAFETY_VM_CASES		5U
struct xsave_case non_safety_vm_cases[NUM_NON_SAFETY_VM_CASES] = {
	{23154u, "XSAVE_CR4_initial_state_following_INIT_001"},
	{36768u, "XSAVE_XCR0_initial_state_following_INIT_002"},
	{24108u, "XSAVE_XINUSE[bit 2:0]_initial_state_following_INIT_002"},
	{23151u, "XSAVE_XCR0_initial_state_following_INIT_001"},
	{23635u, "XSAVE XINUSE[bit 2:0] initial state following INIT_001"},
};

#define NUM_COMMON_CASES		55U
struct xsave_case common_cases[NUM_COMMON_CASES] = {
	{23633u, "XSAVE hide AVX-512 support_001"},
	{22826u, "XSAVE general support_010"},
	{22867u, "XSAVE expose AVX support_001"},
	{22844u, "XSAVE general support_020"},
	{22866u, "XSAVE expose SSE support_001"},
	{22911u, "XSAVE general support_020"},
	{22846u, "XSAVE expose x87 support_002"},
	{22830u, "XSAVE general support_014"},
	{22825u, "XSAVE supervisor state components_001"},

	{23153u, "XSAVE_CR4_initial_state_following_start-up_001"},
	{22853u, "XSAVE_XCR0_initial_state_following_start-up_001"},
	{23634u, "XSAVE XINUSE[bit 2:0] initial state following start-up_001"},

	{36703u, "XSAVE XSAVEOPT (RFBM[i] = 1, XINUSE[i] = 1) before XRSTOR following initializing event_X87"},
	{23636u, "XSAVE XSAVEOPT (RFBM[i] = 1, XINUSE[i] = 1) before XRSTOR following initializing event_SSE"},
	{23637u, "XSAVE XSAVEOPT (RFBM[i] = 1, XINUSE[i] = 1) before XRSTOR following initializing event_AVX"},

	{22895u, "XSAVE_init_and_modified_optimizations_001"},
	{23638u, "XSAVE init and modified optimizations_002"},
	{23639u, "XSAVE init and modified optimizations_003"},
	{23640u, "XSAVE init and modified optimizations_004"},
	{23641u, "XSAVE init and modified optimizations_005"},
	{23642u, "XSAVE init and modified optimizations_006"},
	{36805u, "XSAVE init and modified optimizations_007"},

	{22840u, "XSAVE_general_support_001_CPUID_XSAVE"},
	{22863u, "XSAVE_general_support_002_set_CR4_OSXSAVE"},
	{22793u, "XSAVE_general_support_003_set_XCR0"},
	{22794u, "XSAVE_general_support_004_supported_states_size"},
	{22797u, "XSAVE_general_support_005_enabled_user_states_size"},
	{22798u, "XSAVE_general_support_006_CPUID_XSAVEOPT"},
	{21175u, "XSAVE_general_support_007_UD"},
	{28393u, "XSAVE_general_support_009_non_aligned_addr_GP"},
	{22827u, "XSAVE_general_support_011_all_enabled_states_size"},
	{28395u, "XSAVE_general_support_012_non_aligned_addr_AC"},
	{22829u, "XSAVE_general_support_013_state_size"},
	{22831u, "XSAVE_general_support_015_user_state_ECX_bit0"},
	{22865u, "XSAVE_general_support_016_state_offset"},
	{22832u, "XSAVE_general_support_017_CPUID_ECX_EDX_rsvd_bits"},
	{22835u, "XSAVE_general_support_018_CPUID_unsupported_states"},
	{22862u, "XSAVE_general_support_019_NM"},
	{22861u, "XSAVE_general_support_023_read_XCR0"},
	{22854u, "XSAVE_general_support_024_XCR0_rsvd_bits"},
	{22855u, "XSAVE_general_support_025_XCR0_rsvd_bits_gp"},
	{22856u, "XSAVE_general_support_026_CPUID_OSXSAVE"},

	{23631u, "XSAVE hide PKRU support_001"},
	{23632u, "XSAVE hide MPX support_001"},
	{22828u, "XSAVE_supervisor_state_components_002"},

	{22847u, "XSAVE_expose x87_support_001"},
	{27758u, "XSAVE_expose x87_support_003"},

	{27760u, "XSAVE_expose_SSE_support_002"},

	{22864u, "XSAVE_expose_AVX_support_002"},
	{22640u, "XSAVE_expose_AVX_support_003"},
	{27761u, "XSAVE_expose_AVX_support_004"},
	{37219u, "XSAVE_expose_AVX_support_64_bit_mode"},

	{22799u, "XSAVE_compaction_extensions_001"},
	{37042u, "XSAVE_compaction_extensions_002"},
	{28397u, "XSAVE_Compacted_Form_of_XRSTOR_001"},
};

static void print_case_list(void)
{
	uint32_t i;

	/*_x86_64__*/
	printf("\t\t XSAVE feature case list:\n\r");
#ifdef IN_NATIVE
	for (i = 0U; i < NUM_NATIVE_CASES; i++) {
		printf("\t\t Case ID:%d Case Name:%s\n\r", native_cases[i].case_id, native_cases[i].case_name);
	}
#else
#ifdef IN_NON_SAFETY_VM
	for (i = 0U; i < NUM_NON_SAFETY_VM_CASES; i++) {
		printf("\t\t Case ID:%d Case Name:%s\n\r",
			non_safety_vm_cases[i].case_id, non_safety_vm_cases[i].case_name);
	}
#endif
	for (i = 0U; i < NUM_COMMON_CASES; i++) {
		printf("\t\t Case ID:%d Case Name:%s\n\r",
			common_cases[i].case_id, common_cases[i].case_name);
	}
#endif
	printf("\t\t \n\r \n\r");
}

int main(void)
{
	setup_idt();
	setup_ring_env();

	print_case_list();

	/* following start-up cases */
	xsave_rqmid_23153_xsave_cr4_initial_state_following_startup_001();
	xsave_rqmid_22853_xsave_xcr0_initial_state_following_startup_001();
	xsave_rqmid_23634_xinuse_bit2to0_initial_state_following_startup_001();

#ifdef IN_NATIVE
	xsave_rqmid_28468_physical_compaction_extensions_AC_001();
	xsave_rqmid_28386_physical_general_support_AC_001();
	xsave_rqmid_28385_physical_x87_support_AC_001();
	xsave_rqmid_28388_physical_sse_support_AC_001();
	xsave_rqmid_28390_physical_avx_support_AC_001();
	xsave_rqmid_28392_physical_init_and_modified_optimizations_AC_001();
	xsave_rqmid_40367_physical_pkru_support_001();
#else
#ifdef IN_NON_SAFETY_VM
	/* following INIT cases */
	xsave_rqmid_23154_xsave_cr4_initial_state_following_init_001();

	/* This case verifies the XCR0 following the first INIT received by the AP. */
	xsave_rqmid_36768_xcr0_initial_state_following_init_002();
	/* This case verifies the XINUSE[bit 2:0] following the first INIT received by the AP. */
	xsave_rqmid_24108_xinuse_bit2to0_initial_state_following_init_002();

	/* Using 2nd INIT to verify case 23151 and 23635 as the environment is very similar. */
	send_sipi();
	/* This case verifies XCR0 is unchanged following INIT (when it has been set to 7H on the AP). */
	xsave_rqmid_23151_xcr0_initial_state_following_init_001();
	/* This case verifies XINUSE[2:0] is unchanged following INIT (when XINUSE[1] has been set to 1 on the AP). */
	xsave_rqmid_23635_XINUSE_bit2to0_initial_state_following_INIT_001();
#endif
	/*
	 * NOTE:
	 * Following three test cases has to be executed before XRSTOR instruction has been executed.
	 * 36703, 23636, 23637.
	 */
	xsave_rqmid_36703_xsaveopt_before_xrstor_initial_state_following_initializing_event_x87();
	xsave_rqmid_23636_xsaveopt_before_xrstor_initial_state_following_initializing_event_sse();
	xsave_rqmid_23637_xsaveopt_before_xrstor_initial_state_following_initializing_event_avx();

	xsave_rqmid_22895_init_and_modified_optimizations_001();
	xsave_rqmid_23638_init_and_modified_optimizations_002();
	xsave_rqmid_23639_init_and_modified_optimizations_003();
	xsave_rqmid_23640_init_and_modified_optimizations_004();
	xsave_rqmid_23641_init_and_modified_optimizations_005();
	/* NOTE: XRSTOR instruction will be executed in 36805. */
	xsave_rqmid_36805_init_and_modified_optimizations_007();

	xsave_rqmid_22840_general_support_001_cpuid_xsave();
	xsave_rqmid_22863_general_support_002_set_cr4_osxsave();
	xsave_rqmid_22856_general_support_026_cpuid_osxsave();
	xsave_rqmid_22798_general_support_006_cpuid_xsaveopt();
	xsave_rqmid_22835_general_support_018_cpuid_unsupported_states();

	xsave_rqmid_22794_general_support_004_supported_states_size();
	xsave_rqmid_22797_general_support_005_enabled_user_states_size();
	xsave_rqmid_22827_general_support_011_all_enabled_states_size();

	xsave_rqmid_22829_general_support_013_state_size();
	xsave_rqmid_22865_general_support_016_state_offset();
	xsave_rqmid_22831_general_support_015_user_state_ecx_bit0();

	xsave_rqmid_22832_general_support_017_cpuid_ecx_edx_rsvd_bits();

	xsave_rqmid_22861_general_support_023_read_xcr0();
	xsave_rqmid_22854_general_support_024_xcr0_rsvd_bits();
	xsave_rqmid_22855_general_support_025_xcr0_rsvd_bits_gp();

	xsave_rqmid_21175_general_support_007_ud();
	xsave_rqmid_22862_general_support_019_nm();
	xsave_rqmid_28393_general_support_009_non_aligned_addr_gp();
	xsave_rqmid_28395_general_support_012_non_aligned_addr_ac();

	xsave_rqmid_23631_hide_pkru_support_001();
	xsave_rqmid_23632_hide_mpx_support_001();
	xsave_rqmid_22828_supervisor_state_components_002();

	xsave_rqmid_22847_expose_x87_support_001();
	xsave_rqmid_27758_expose_x87_support_003();

	xsave_rqmid_27760_expose_SSE_support_002();

	xsave_rqmid_22864_expose_AVX_support_002();
	xsave_rqmid_22640_expose_AVX_support_003();
	xsave_rqmid_27761_expose_AVX_support_004();
	xsave_rqmid_37219_expose_AVX_support_64_bit_mode();

	xsave_rqmid_22799_compaction_extensions_001();
	xsave_rqmid_37042_compaction_extensions_002();
	xsave_rqmid_28397_compacted_form_of_xrstor_001();

	xsave_rqmid_23633_hide_avx_512_support_001();
	/* NOTE: XRSTOR instruction will be executed in 23642. */
	xsave_rqmid_23642_init_and_modified_optimizations_006();
	xsave_rqmid_22826_check_reserved_bit();
	xsave_rqmid_22867_expose_avx_support();
	xsave_rqmid_22844_xsetbv_at_ring3();
	xsave_rqmid_22866_expose_sse_support();
	xsave_rqmid_22911_check_xsave_head_size();
	xsave_rqmid_22846_x87_support();
	xsave_rqmid_22830_check_xsave_area_offset();
	xsave_rqmid_22825_supervisor_state_components_001();
	xsave_rqmid_22793_general_support_003_set_xcr0();
#endif

	return report_summary();
}
