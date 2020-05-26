#include "processor.h"
#include "instruction_common.h"
#include "libcflat.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "avx.h"
#include "smp.h"
#include "fwcfg.h"
static void avx_save_ymm_register(void *data);
static void avx_set_ymm_register(void *data);


/**Function about XSAVE feature: **/
__unused static int xsetbv_checking(u32 index, u64 value)
{
	u32 eax = value;
	u32 edx = value >> 32;

	asm volatile(ASM_TRY("1f")
				 "xsetbv\n\t" /* xsetbv */
				 "1:"
				 : : "a" (eax), "d" (edx), "c" (index));
	return exception_vector();
}

__unused static int xgetbv_checking(u32 index, u64 *result)
{
	u32 eax, edx;

	asm volatile(ASM_TRY("1f")
				 ".byte 0x0f,0x01,0xd0\n\t" /* xgetbv */
				 "1:"
				 : "=a" (eax), "=d" (edx)
				 : "c" (index));
	*result = eax + ((u64)edx << 32);
	return exception_vector();
}
__unused static void avx_save_ymm_register(void *data)
{
	avx_union *avx;
	avx = (avx_union *)data;

	asm volatile("VMOVDQA %%ymm0, %0" : "=m"(avx[0]));
	asm volatile("VMOVDQA %%ymm1, %0" : "=m"(avx[1]));
	asm volatile("VMOVDQA %%ymm2, %0" : "=m"(avx[2]));
	asm volatile("VMOVDQA %%ymm3, %0" : "=m"(avx[3]));
	asm volatile("VMOVDQA %%ymm4, %0" : "=m"(avx[4]));
	asm volatile("VMOVDQA %%ymm5, %0" : "=m"(avx[5]));
	asm volatile("VMOVDQA %%ymm6, %0" : "=m"(avx[6]));
	asm volatile("VMOVDQA %%ymm7, %0" : "=m"(avx[7]));
	asm volatile("VMOVDQA %%ymm8, %0" : "=m"(avx[8]));
	asm volatile("VMOVDQA %%ymm9, %0" : "=m"(avx[9]));
	asm volatile("VMOVDQA %%ymm10, %0" : "=m"(avx[10]));
	asm volatile("VMOVDQA %%ymm11, %0" : "=m"(avx[11]));
	asm volatile("VMOVDQA %%ymm12, %0" : "=m"(avx[12]));
	asm volatile("VMOVDQA %%ymm13, %0" : "=m"(avx[13]));
	asm volatile("VMOVDQA %%ymm14, %0" : "=m"(avx[14]));
	asm volatile("VMOVDQA %%ymm15, %0" : "=m"(avx[15]));
}
__unused static void avx_set_ymm_register(void *data)
{
	avx_union *avx;
	avx = (avx_union *)data;

	asm volatile("VMOVDQA %0, %%ymm0" : : "m"(avx[0]));
	asm volatile("VMOVDQA %0, %%ymm1" : : "m"(avx[1]));
	asm volatile("VMOVDQA %0, %%ymm2" : : "m"(avx[2]));
	asm volatile("VMOVDQA %0, %%ymm3" : : "m"(avx[3]));
	asm volatile("VMOVDQA %0, %%ymm4" : : "m"(avx[4]));
	asm volatile("VMOVDQA %0, %%ymm5" : : "m"(avx[5]));
	asm volatile("VMOVDQA %0, %%ymm6" : : "m"(avx[6]));
	asm volatile("VMOVDQA %0, %%ymm7" : : "m"(avx[7]));
	asm volatile("VMOVDQA %0, %%ymm8" : : "m"(avx[8]));
	asm volatile("VMOVDQA %0, %%ymm9" : : "m"(avx[9]));
	asm volatile("VMOVDQA %0, %%ymm10" : : "m"(avx[10]));
	asm volatile("VMOVDQA %0, %%ymm11" : : "m"(avx[11]));
	asm volatile("VMOVDQA %0, %%ymm12" : : "m"(avx[12]));
	asm volatile("VMOVDQA %0, %%ymm13" : : "m"(avx[13]));
	asm volatile("VMOVDQA %0, %%ymm14" : : "m"(avx[14]));
	asm volatile("VMOVDQA %0, %%ymm15" : : "m"(avx[15]));
}

#ifdef __i386__
void save_unchanged_reg()
{
	asm volatile ("pause");
}
#elif __x86_64__
__attribute__ ((aligned(64)))	static avx_union init_ymm_ap[16];
__attribute__ ((aligned(64)))	static avx_union unchanged_ymm_ap[16];
void save_unchanged_reg()
{
	/*save AP ymm register*/
	avx_save_ymm_register((void *)init_ymm_ap);

	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 8; j++) {
			unchanged_ymm_ap[i].avx_u[j] = 2;
		}
	}
	/*change AP ymm register with unchanged_ymm_ap*/
	avx_set_ymm_register((void *)unchanged_ymm_ap);

}
#endif
/*--------------------------------Test case------------------------------------*/

#ifdef __x86_64__

/*
 * @brief case name:  23169:AVX initial YMM register following start-up_001.
 *
 * Summary:
 *  ACRN hypervisor shall set initial guest YMM0[bit 255:128]~YMM15[bit 255:128] to 0H following start-up.
 *
 */
static void  avx_rqmid_23169_AVX_initial_YMM_register_following_startup_001()
{
	bool ret = true;
	struct xsave_area_struct *xsave_startup = (struct xsave_area_struct *)XSAVE_STARTUP_ADDR;

	/*just check YMM0-YMM7*/
	for (int i = 0; i < 128; i++) {
		if (xsave_startup->ymm[i] != 0) {
			ret = false;
			break;
		}
	}

	__attribute__ ((aligned(64)))  avx_union avx[16];
	memset(avx, 0x0, sizeof(avx));
	avx_save_ymm_register(avx);
	for (int i = 0; i < 16; i++) {
		for (int j = 4; j < 8; j++) {
			if (avx[i].avx_u[j] != 0) {
				ret = false;
			}
		}
	}

	report("%s", ret, __FUNCTION__);
}

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:  23170:AVX initial YMM register following INIT_001.
 *
 * Summary:
 *  After AP receives?first INIT, dump YMM0[bit 255:128]~YMM15[bit 255~128] value;
 * The AP YMM0[bit 255:128]~YMM15[bit 255~128] should be equal with BP?YMM0[bit 255:128]~YMM15[bit 255~128]
 *
 */
static void  avx_rqmid_23170_AVX_initial_YMM_register_following_INIT_001()
{
	bool ret = true;
	__attribute__ ((aligned(64))) avx_union ymm_bp[16];
	struct xsave_area_struct *xsave_init;

	xsave_init = (struct xsave_area_struct *)XSAVE_INIT_ADDR;
	/*just check YMM0-YMM7*/
	for (int i = 0; i < 128; i++) {
		if (xsave_init->ymm[i] != 0) {
			ret = false;
		}
	}
	/*save BP YMM register*/
	avx_save_ymm_register(ymm_bp);
	if (memcmp(ymm_bp, init_ymm_ap, sizeof(ymm_bp))) {
		ret = false;
	}
	report("%s", ret, __FUNCTION__);
}
/*
 * @brief case name:  23170:AVX initial YMM register following INIT_001.
 *
 * Summary:
 *  After AP receives first INIT, dump YMM0[bit 255:128]~YMM15[bit 255~128] value;
 * The AP YMM0[bit 255:128]~YMM15[bit 255~128] should be equal with BP?YMM0[bit 255:128]~YMM15[bit 255~128]
 *
 */
static void  avx_rqmid_37029_AVX_initial_YMM_register_following_INIT_002()
{
	/*send second init to AP:
	 *
	 */
	send_sipi();
	/*
	 *init_ymm_ap is the second init AP value,it should be equal with unchanged_ymm_ap.
	 * the AP YMM register has been changed @save_unchanged_reg
	 */
	report("%s", memcmp(init_ymm_ap, unchanged_ymm_ap, sizeof(init_ymm_ap)) == 0, __FUNCTION__);
}

#endif

/*
 * @brief case name:  27412:AVX2 exposure_001.
 *
 * Summary: DNG_141811: AVX2 exposure.
 *  ACRN hypervisor shall expose AVX2 to any VM, in compliance with Chapter 14.7, Vol. 1, SDM.
 *
 *	CPUID.(EAX=07H, ECX=0H):EBX.AVX2[bit 5] should be 1
 */
static void  avx_rqmid_27412_avx2_exposure_001()
{
	report("%s", cpuid(0x7).b & SUPPORT_AVX2, __FUNCTION__);
}

/*------------------------------Test case End!----------------------------------*/
/*
 * @brief case name:  27413:AVX expose FMA support_001.
 *
 * Summary: DNG_132194: AVX expose FMA support.
 *  ACRN hypervisor shall expose fused-multiply-add extensions to any VM, in compliance with Chapter 14.5, Vol. 1, SDM.
 *
 *	CPUID.1:ECX.FMA[bit 12] should be 1
 */
static void  avx_rqmid_27413_fma_exposure_001()
{
	report("%s", cpuid(0x1).c & CPUID_1_ECX_FMA, __FUNCTION__);
}
/*
 * @brief case name:  27417:AVX expose F16C support_001.
 *
 * Summary: DNG_132195: AVX expose F16C support.
 *  ACRN hypervisor shall expose half-precision floating-point conversion to any VM,
 *  in compliance with Chapter 14.4, Vol. 1, SDM.
 *
 *	CPUID.1H:ECX.OSXSAVE[bit 27] should be 1;
 *	CPUID.01H:ECX.AVX[bit 28] should be 1
 *	CPUID.01H:ECX.F16C[bit 29] should be 1
 *  XCR0[2:1] should be 11b
 */
static void avx_rqmid_27417_f16c_exposure_001()
{
	u32 c;
	u64 xcr0;

	write_cr4(read_cr4() | X86_CR4_OSXSAVE);
	c = cpuid(1).c;

	xsetbv_checking(XCR0_MASK, STATE_X87 | STATE_SSE | STATE_AVX);
	xgetbv_checking(XCR0_MASK, &xcr0);

	report("%s", (c & SUPPORT_OSXSAVE) && ((xcr0 & 0x6) == 0x6)
		   && (c & CPUID_1_ECX_AVX) && (c & CPUID_1_ECX_F16C), __FUNCTION__);
}
/*
 * @brief case name:  27421:AVX expose execution environment_015.
 *
 * Summary: DNG_132162: AVX expose execution environment.
 *  ACRN hypervisor shall expose AVX execution environment to any VM,
 *  in compliance with Chapter 14.1 and 14.3, Vol. 1, SDM.
 */
static void  avx_rqmid_27421_avx_exposure_015()
{
	u32 c;
	u64 xcr0;
	ulong cr4;

	write_cr4(read_cr4() | X86_CR4_OSXSAVE);
	cr4 = read_cr4();
	c = cpuid(1).c;

	xsetbv_checking(XCR0_MASK, STATE_X87 | STATE_SSE | STATE_AVX);
	xgetbv_checking(XCR0_MASK, &xcr0);

	report("%s", (c & SUPPORT_OSXSAVE) && (cr4 & X86_CR4_OSXSAVE)
		   && ((xcr0 & 0x6) == 0x6) && (c & CPUID_1_ECX_AVX), __FUNCTION__);
}
/*
 * @brief case name: 27309:AVX-512 hiding_Additional 512-bit Instruction Extensions Of Intel AVX-512 Family_001.
 *
 * Summary: DNG_139022: AVX-512 hiding.
 *  ACRN hypervisor shall hide AVX-512 from any VM,
 *  in compliance with Chapter 15.2, 15.3, 15.4, Vol. 1,
 *  and Table 3-8 Information Returned by CPUID Instruction (Contd.), Vol.2, SDM.
 *
 *	CPUID.(EAX=01H):ECX[bit 27 shall be 1
 *	XCR0[7:5] should be 000b and XCR0[2:1] should be 11b
 *	CPUID.0x7.0:EBX.AVX512F[bit 16] should be 0
 *	CPUID.0x7.0:EBX.AVX512ER[bit 27] should be 0
 */
static void avx_rqmid_27309_avx512_hiding_additional_512bit_instruction_ext_001()
{
	u64 xcr0;

	write_cr4(read_cr4() | X86_CR4_OSXSAVE);
	xsetbv_checking(XCR0_MASK, STATE_X87 | STATE_SSE | STATE_AVX);
	xgetbv_checking(XCR0_MASK, &xcr0);

	report("%s", (!(xcr0 & 0xE0)) && ((xcr0 & 0x6) == 0x6) && (cpuid(1).c & SUPPORT_OSXSAVE)
		   && !(cpuid(0x7).b & SUPPORT_AVX512F) && !(cpuid(0x7).b & SUPPORT_AVX512ER), __FUNCTION__);
}
/*
 * @brief case name:  27311:AVX-512 hiding_DETECTION 512-bit INSTRUCTION GROUPSOF INTEL AVX-512 FAMILY_001.
 *
 * Summary: DNG_139022: AVX-512 hiding.
 *  ACRN hypervisor shall hide AVX-512 from any VM, in compliance with Chapter 15.2, 15.3, 15.4, Vol. 1,
 *  and Table 3-8 Information Returned by CPUID Instruction (Contd.), Vol.2, SDM.
 *
 * CPUID.(EAX=01H):ECX[bit 27] equal with 1.
 * XCR0[7:5] should be 000b and XCR0[2:1] should be 11b
 * CPUID.0x7.0:EBX.AVX512F[bit 16] should be 0
 * CPUID.0x7.0:EBX.AVX512CD[bit 28] also should be 0
 */
static void avx_rqmid_27311_avx512_hiding_detection_512bit_inst_group_001()
{
	u64 xcr0;

	write_cr4(read_cr4() | X86_CR4_OSXSAVE);
	xsetbv_checking(XCR0_MASK, STATE_X87 | STATE_SSE | STATE_AVX);
	xgetbv_checking(XCR0_MASK, &xcr0);

	report("%s", (!(xcr0 & 0xE0)) && ((xcr0 & 0x6) == 0x6) && (cpuid(1).c & SUPPORT_OSXSAVE)
		   && !(cpuid(0x7).b & SUPPORT_AVX512F) && !(cpuid(0x7).b & SUPPORT_AVX512CD), __FUNCTION__);
}

/**
 * @brief excute_vcmppd_instruction
 *
 * @param none
 *
 * @retval execute result
 * 0: execute successfully
 * other: occured exception ID
 */
u16 excute_vcmppd_instruction()
{
	__attribute__ ((aligned(64))) sse_union m128;
	u16 ret;
	m128.u[0] = 0;
	m128.u[1] = 0;
	m128.u[2] = 0;
	m128.u[3] = 0;

	asm volatile("vmovaps %0, %%xmm2"::"m"(m128));
	asm volatile(ASM_TRY("1f")
				 "vcmppd $0, %0, %%xmm2, %%xmm1\n\t"
				 "1:"
				 : : "m"(m128) : "memory");
	ret = exception_vector();
	return ret;
}
/*--------------------Jasson coding---------------------------------------------*/

/**
 * @brief case name: AVX expose execution environment_64 bit Mode_VCMPPD_normal_execution_001
 *
 * Summary: Under 64 bit Mode,  executing VCMPPD shall generate normal execution .
 *
 * @param None
 *
 * @retval None
 *
 */
void avx_rqmid_30000_64bit_mode_vcmppd_normal_execution_001()
{
	u16 ret;
	u32 mxcsr = 0x1f;/*enable report exception*/

	/*check support AVX or not firstly*/
	if (!(cpuid(1).c & CPUID_1_ECX_AVX)) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	write_cr0(read_cr0() & ~X86_CR0_TS);/*clear ts*/
	write_cr4(read_cr4() | X86_CR4_OSXSAVE);/*enable osxsave */
	write_cr4(read_cr4() | X86_CR4_OSFXSR);
	xsetbv_checking(0, STATE_X87 | STATE_SSE | STATE_AVX);
	asm volatile("ldmxcsr %0" : : "m"(mxcsr));

	ret = excute_vcmppd_instruction();
	report("%s", !ret, __FUNCTION__);
}
/**
 * @brief excute_vcvtsd2si_instruction
 *
 * @param none
 *
 * @retval excute result
 */
static u16 excute_vcvtsd2si_instruction(void)
{
	__attribute__ ((aligned(64))) double val = 12.0;
	u16 ret;

	asm volatile(ASM_TRY("1f")
				 "vcvtsd2si %0, %%rcx\n\t"
				 "1:"
				 : : "m"(val));

	ret = exception_vector();
	return ret;
}
/**
 * @brief vcvtsd2si_normal_execution_sub_step which run in ring3
 *
 * @param none
 *
 * @retval excute result
 */
static void test_vcvtsd2si_normal_execution_sub_step(const char *msg)
{
	u16 level;

	level = read_cs() & 0x3;
	report("%s", (excute_vcvtsd2si_instruction() == 0) && (level == 3), msg);
}
/**
 * @brief case name: AVX expose execution environment_64 bit Mode_VCVTSD2SI_normal_execution_006
 *
 * Summary: Under 64 bit Mode and CPL3, executing vcvtsd2si shall generate normal execution .
 *
 * @param None
 *
 * @retval None
 *
 */
static void avx_rqmid_30159_64bit_mode_vcvtsd2si_normal_execution_006()
{
	u32 mxcsr = 0x1f;

	/*check support AVX or not firstly*/
	if (!(cpuid(1).c & CPUID_1_ECX_AVX)) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	write_cr4(read_cr4() | X86_CR4_OSXSAVE);/*enable osxsave */
	write_cr4(read_cr4() | X86_CR4_OSFXSR);
	xsetbv_checking(0, STATE_X87 | STATE_SSE | STATE_AVX);
	write_cr0(read_cr0() & ~X86_CR0_TS);/*clear ts*/
	write_cr0(read_cr0() | X86_CR0_AM);/*set AM */
	asm volatile("ldmxcsr %0" : : "m"(mxcsr));

	do_at_ring3(test_vcvtsd2si_normal_execution_sub_step, __FUNCTION__);
}
/**
 * @brief case name: AVX exposure 64 bit mode Mode_VPUNPCKLBW_#PF_002
 *
 * Summary: Under 64 bit Mode, Rebulid the paging structure, to create the page fault(pgfault: occur),
 * executing VPUNPCKLBW shall generate #PF .
 *
 * @param None
 *
 * @retval None
 *
 */
static void avx_rqmid_30210_64bit_mode_vpunpcklbw_pf_002()
{
	avx256 *avx;

	/*check avx and avx2*/
	if (!(cpuid(1).c & CPUID_1_ECX_AVX) || !(cpuid(0x7).b & SUPPORT_AVX2)) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	write_cr4(read_cr4() | X86_CR4_OSXSAVE);/*enable osxsave */
	xsetbv_checking(0, STATE_X87 | STATE_SSE | STATE_AVX);

	avx = malloc(sizeof(avx256));
	set_page_control_bit((void *)avx, PAGE_PTE, PAGE_P_FLAG, 0, true);
	asm volatile(ASM_TRY("1f")
				 "vpunpcklbw %0, %%ymm1, %%ymm2\n"
				 "1:"
				 : : "m"(*avx));

	report("%s", (exception_vector() == PF_VECTOR) && (exception_error_code() == 0),  __FUNCTION__);
}
#endif
#ifdef __i386__
/**
 * @brief excute_vpbroadcastd_instruction
 *
 * @param none
 *
 * @retval excute result
 */
static int excute_vpbroadcastd_instruction()
{
	u32 m32 = 1;

	asm volatile(ASM_TRY("1f")
				 "vpbroadcastd %0, %%ymm1\n\t"
				 "1:"
				 : : "m"(m32));

	return exception_vector();;
}

/**
 * @brief test_vpbroadcastd_normal_execution_sub_step which run in special ring level
 *
 * @param none
 *
 * @retval excute result
 */
static void test_vpbroadcastd_normal_execution(const char *msg)
{
	u16 level;

	level = read_cs() & 0x3;
	report("%s", (excute_vpbroadcastd_instruction() == 0) && (level == 1), msg);
}

/**
 * @brief case name: AVX2 exposure_Protected Mode_VPBROADCASTD_normal_execution_017
 *
 * Summary: Under Protected Mode and CPL1 ,  executing VPBROADCASTD shall generate normal execution .
 *
 * CPUID.01H:ECX.AVX[bit 28] should be 1
 * CPUID.(EAX=07H,ECX=0):EBX[bit 5] should be 1
 * @param None
 *
 * @retval None
 *
 */
static void avx_rqmid_30909_protected_mode_vpbroadcastd_normal_execution_017()
{
	if (!(cpuid(1).c & CPUID_1_ECX_AVX) || !(cpuid(0x7).b & SUPPORT_AVX2_BIT)) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	write_cr4(read_cr4() | X86_CR4_OSXSAVE);/*enable osxsave */
	xsetbv_checking(0, STATE_X87 | STATE_SSE | STATE_AVX);
	write_cr0(read_cr0() & ~X86_CR0_TS);/*clear ts*/
	write_cr0(read_cr0() & ~X86_CR0_EM);

	do_at_ring1(test_vpbroadcastd_normal_execution, __FUNCTION__);
}
/**
 * @brief case name: AVX exposure F16C protected Mode_VCVTPH2PS_#PF_003
 *
 * Summary: Under Protected Mode,Rebulid the paging structure,
 * to create the page fault(pgfault: occur), executing VCVTPH2PS shall generate #PF(0).
 *
 * @param None
 *
 * @retval None
 *
 */
static void avx_rqmid_30929_protected_mode_vcvtph2ps_pf_003()
{
	u64 *op;
	/*check F16c feature*/
	if (!(cpuid(1).c & CPUID_1_ECX_F16C)) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	write_cr4(read_cr4() | X86_CR4_OSXSAVE);/*enable osxsave */
	xsetbv_checking(0, STATE_X87 | STATE_SSE | STATE_AVX);

	op = malloc(sizeof(u64));
	set_page_control_bit((void *)op, PAGE_PTE, PAGE_P_FLAG, 0, true);
	asm volatile(ASM_TRY("1f")
				 "vcvtph2ps %0,%%xmm1\n"
				 "1:"
				 : : "m"(*op));

	report("%s", (exception_vector() == PF_VECTOR) && exception_error_code() == 0, __FUNCTION__);
}
/*----------------------------Jasson coding------------------------------------*/
/**
 * @brief excute_vsqrtpd_instruction
 *
 * @param none
 *
 * @retval excute result
 */
static int excute_vsqrtpd_instruction(void)
{
	avx_union m256;

	memset(&m256, 0, sizeof(m256));
	asm volatile(ASM_TRY("1f")
				 "vsqrtpd %0, %%ymm1\n\t"
				 "1:"
				 : : "m"(m256) : "memory");

	return exception_vector();
}
/**
 * @brief case name: AVX expose execution environment_Protected Mode_VSQRTPD_normal_execution_001
 *
 * Summary: Under Protected Mode  ,   executing VSQRTPD shall generate normal execution .
 *
 * @param None
 *
 * @retval None
 *
 */
static void avx_rqmid_30441_protected_mode_vsqrtpd_normal_execution_001()
{
	u32 mxcsr = 0x1f;

	/*check support AVX or not firstly*/
	if (!(cpuid(1).c & CPUID_1_ECX_AVX)) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	write_cr4(read_cr4() | X86_CR4_OSXSAVE);/*enable osxsave */
	write_cr4(read_cr4() | X86_CR4_OSFXSR);
	xsetbv_checking(0, STATE_X87 | STATE_SSE | STATE_AVX);
	write_cr0(read_cr0() & ~X86_CR0_TS);/*clear ts*/

	asm volatile("ldmxcsr %0" : : "m"(mxcsr));

	report("%s", excute_vsqrtpd_instruction() == 0, __FUNCTION__);
}
/**
 * @brief case name: AVX exposure execution environment protected Mode_VSQRTPD_#PF_001
 *
 * Summary: Under Protected Mode,Rebulid the paging structure,
 * to create the page fault(pgfault: occur), executing VSQRTPD shall generate #PF.
 *
 * @param None
 *
 * @retval None
 *
 */
static void avx_rqmid_30425_protected_mode_vsqrtpd_pf_001()
{
	avx128 *avx;

	if (!(cpuid(1).c & CPUID_1_ECX_AVX)) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	write_cr4(read_cr4() | X86_CR4_OSXSAVE);/*enable osxsave */
	xsetbv_checking(0, STATE_X87 | STATE_SSE | STATE_AVX);

	avx = malloc(sizeof(avx128));
	set_page_control_bit((void *)avx, PAGE_PTE, PAGE_P_FLAG, 0, true);
	asm volatile(ASM_TRY("1f")
				 "vsqrtpd %0,%%xmm1\n"
				 "1:"
				 ::"m"(*avx));

	report("%s", (exception_vector() == PF_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}
#endif
static void print_case_list(void)
{
	printf("AVX feature case list:\n\r");
#ifdef __x86_64__
	printf("\t Case ID:%d case name:%s\n\r", 23169, "AVX initial YMM register following startup 001");
#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 23170, "AVX initial YMM register following INIT 001");
	printf("\t Case ID:%d case name:%s\n\r", 37029, "AVX initial YMM register following INIT 002");
#endif
	printf("\t Case ID:%d case name:%s\n\r", 27309, "avx512 hiding additional 512bit instruction ext 001");
	printf("\t Case ID:%d case name:%s\n\r", 27311, "avx512 hiding detection 512bit inst group 001");
	printf("\t Case ID:%d case name:%s\n\r", 27412, "avx2 exposure 001");
	printf("\t Case ID:%d case name:%s\n\r", 27413, "fma exposure 001");
	printf("\t Case ID:%d case name:%s\n\r", 27417, "f16c exposure 001");
	printf("\t Case ID:%d case name:%s\n\r", 27421, "avx exposure 015");
	printf("\t Case ID:%d case name:%s\n\r", 30000, "64bit mode vcmppd normal execution 001");
	printf("\t Case ID:%d case name:%s\n\r", 30159, "64bit mode vcvtsd2si normal execution 006");
	printf("\t Case ID:%d case name:%s\n\r", 30210, "64bit mode vpunpcklbw pf 002");
#endif
#ifdef __i386__
	printf("\t Case ID:%d case name:%s\n\r", 30909, "protected mode vpbroadcastd normal execution 017");
	printf("\t Case ID:%d case name:%s\n\r", 30929, "protected mode mode vcvtph2ps pf 003");
	printf("\t Case ID:%d case name:%s\n\r", 30441, "protected vsqrtpd normal execution 001");
	printf("\t Case ID:%d case name:%s\n\r", 30425, "protected vsqrtpd vsqrtpd pf 001");
#endif
}
int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	irq_enable();
	print_case_list();

#ifdef __x86_64__
	avx_rqmid_23169_AVX_initial_YMM_register_following_startup_001();
#ifdef IN_NON_SAFETY_VM
	avx_rqmid_23170_AVX_initial_YMM_register_following_INIT_001();
	avx_rqmid_37029_AVX_initial_YMM_register_following_INIT_002();
#endif
	avx_rqmid_27309_avx512_hiding_additional_512bit_instruction_ext_001();
	avx_rqmid_27311_avx512_hiding_detection_512bit_inst_group_001();
	avx_rqmid_27412_avx2_exposure_001();
	avx_rqmid_27413_fma_exposure_001();
	avx_rqmid_27417_f16c_exposure_001();
	avx_rqmid_27421_avx_exposure_015();
	avx_rqmid_30000_64bit_mode_vcmppd_normal_execution_001();
	avx_rqmid_30159_64bit_mode_vcvtsd2si_normal_execution_006();
	avx_rqmid_30210_64bit_mode_vpunpcklbw_pf_002();
#endif
#ifdef __i386__
	avx_rqmid_30909_protected_mode_vpbroadcastd_normal_execution_017();
	avx_rqmid_30929_protected_mode_vcvtph2ps_pf_003();
	avx_rqmid_30441_protected_mode_vsqrtpd_normal_execution_001();
	avx_rqmid_30425_protected_mode_vsqrtpd_pf_001();
#endif
	return report_summary();
}
