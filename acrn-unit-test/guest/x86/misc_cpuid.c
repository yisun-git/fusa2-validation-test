#include "libcflat.h"
#include "processor.h"
#include "desc.h"
#include "register_op.h"
#include "regdump.h"
#include "xsave.h"
#include "instruction_common.h"
#include "misc_cpuid.h"
#include "misc.h"
#include "fwcfg.h"
#include "delay.h"

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;

#define VALUE_TO_WRITE_MSR			0x12

typedef unsigned __attribute__((vector_size(16))) sse128;

typedef union {
	sse128 sse;
	u64 u[2];
} sse_un;

typedef struct {
		u32 check_value;
		u32 start_bit;
		u32 end_bit;
	} t_rvdbits;

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
		wait_ap = 1;
	}
}

__unused static void modify_misc_enable_bit23_init_value()
{
	if ((cpuid(1).c  & (1ul << 14))) {
		wrmsr(MSR_IA32_MISC_ENABLE, rdmsr(MSR_IA32_MISC_ENABLE) & (~(1ull << 23)));
	}
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_cpu_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 38369:
		fp = modify_misc_enable_bit23_init_value;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif

#ifdef IN_NATIVE
/*
 * @brief case name:The physical platform supports enhanced bitmap instructions_001
 *
 * Summary: The physical platform supports enhanced bitmap instructions
 *
 */
static void misc_cpuid_rqmid_38251_cpuid_eax07_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x07, 0).b;
	report("%s", (result >> 3 & 0x1) && (result >> 8 & 0x1), __FUNCTION__);
}

/*
 * @brief case name:The physical platform supports AESNI instruction extensions_001
 *
 * Summary: The physical platform supports AESNI instruction extensions
 *
 */
static void misc_cpuid_rqmid_38308_cpuid_eax01_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x01, 0).c;
	report("%s", (result >> 25 & 0x1) == 0x1, __FUNCTION__);
}

/*
 * @brief case name:The physical platform doesn't support RDPID instruction_001
 *
 * Summary: The physical platform doesn't support RDPID instruction
 *
 */
static void misc_cpuid_rqmid_38309_cpuid_eax07_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x07, 0).c;
	report("%s", (result  & (1UL << 22)) == 0x0, __FUNCTION__);
}

/*
 * @brief case name:The physical platform doesn't support PREFETCHWT1 instruction_001
 *
 * Summary: PREFETCHWT1 instruction shall be unavailable on the physical platform.
 *
 */
static void misc_cpuid_rqmid_38310_cpuid_eax07_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x07, 0).c;
	report("%s", (result & 0x1) == 0x0, __FUNCTION__);
}

/*
 * @brief case name:The physical platform doesn't support SHA extensions_001
 *
 * Summary: The physical platform doesn't support SHA extensions
 *
 */
static void misc_cpuid_rqmid_38311_cpuid_eax07_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x07, 0).c;
	report("%s", (result & (1UL << 29)) == 0x0, __FUNCTION__);
}

/*
 * @brief case name:The physical platform supports PCLMULQDQ instruction_001
 *
 * Summary: The physical platform supports PCLMULQDQ instruction
 *
 */
static void misc_cpuid_rqmid_38312_cpuid_eax01_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x01, 0).c;
	report("%s", (result >> 1 & 0x1) == 0x1, __FUNCTION__);
}

/*
 * @brief case name:The physical platform doesn't support user-mode instruction prevention_001
 *
 * Summary:The physical platform doesn't support user-mode instruction prevention
 *
 */
static void misc_cpuid_rqmid_38313_cpuid_eax07_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x07, 0).c;
	report("%s", (result  & (1UL << 2)) == 0x0, __FUNCTION__);
}

/*
 * @brief case name:The physical platform doesn't support Direct Cache Access_001
 *
 * Summary:The physical platform doesn't support Direct Cache Access
 *
 */
static void misc_cpuid_rqmid_38314_cpuid_eax01_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x01, 0).c;
	report("%s", (result  & (1UL << 18)) == 0x0, __FUNCTION__);
}

/*
 * @brief case name:The physical platform doesn't support  additional features of side channel mitigations_001
 *
 * Summary:The physical platform doesn't support  additional features of side channel mitigations
 *
 */
static void misc_cpuid_rqmid_38315_cpuid_eax07_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x07, 0).d;
	report("%s", (result & (1UL << 29)) == 0x0, __FUNCTION__);
}

/*
 * @brief case name:The physical platform shall guarantee supported CPUID leaf 7 sub-leaves is 0H_001
 *
 * Summary:The physical platform shall guarantee supported CPUID leaf 7 sub-leaves is 0H
 *
 */
static void misc_cpuid_rqmid_38316_cpuid_eax07_ecx0_001()
{
	int result = 1;
	cpuid_indexed(0x07, 0);
	report("%s", result, __FUNCTION__);
}

/*
 * @brief case name:The physical platform supports self snoop_001
 *
 * Summary:The physical platform supports self snoop
 *
 */
static void misc_cpuid_rqmid_38317_cpuid_eax01_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x01, 0).d;
	report("%s", (result >> 27 & 0x1) == 0x1, __FUNCTION__);
}

/*
 * @brief case name:The physical platform doesn't support PSN_001
 *
 * Summary:The physical platform doesn't support PSN
 *
 */
static void misc_cpuid_rqmid_38318_cpuid_eax01_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x01, 0).d;
	report("%s", (result  & (1UL << 18)) == 0x0, __FUNCTION__);
}

/*
 * @brief case name:The physical platform supports multiple cores_001
 *
 * Summary:The physical platform supports multiple cores
 *
 */
static void misc_cpuid_rqmid_38319_cpuid_eax01_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0x01, 0).d;
	report("%s", (result >> 28 & 0x1) == 0x1, __FUNCTION__);
}

#else

static	void set_sse_env()
{
	write_cr4(read_cr4() | X86_CR4_OSXSAVE);/*enable osxsave */
	write_cr4(read_cr4() | X86_CR4_OSFXSR);
	xsetbv_checking(0, STATE_X87 | STATE_SSE | STATE_AVX);
	write_cr0(read_cr0() & ~X86_CR0_TS);/*clear ts*/
	write_cr0(read_cr0() | X86_CR0_AM);/*set AM */
}

static unsigned rdpid_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"rdpid %%rax\n\t"
		"1:"
		:
		:
		:);

	return exception_vector();
}


static unsigned sha1msg1_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"sha1msg1 %%xmm1, %%xmm2\n\t"
		"1:"
		:
		:
		:);

	return exception_vector();
}

static unsigned sha1msg2_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"sha1msg2 %%xmm1, %%xmm2\n\t"
		"1:"
		:
		:
		:);

	return exception_vector();
}

static unsigned sha1nexte_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"sha1nexte %%xmm1, %%xmm2\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

static unsigned sha1rnds4_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"sha1rnds4 $1, %%xmm1, %%xmm2\n\t"
		"1:"
		:
		:
		:
		);
	return exception_vector();
}

static unsigned sha256msg1_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"sha256msg1 %%xmm1, %%xmm2\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

static unsigned sha256msg2_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"sha256msg2 %%xmm1, %%xmm2\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

static unsigned sha256rnds2_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"sha256rnds2 %%xmm1 ,%%xmm2\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

static unsigned aesenc_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"aesenc %%xmm1 ,%%xmm2\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

static unsigned aesenclast_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"aesenclast %%xmm1 ,%%xmm2\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

static unsigned aesdec_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"aesdec %%xmm1 ,%%xmm2\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

static unsigned aesdeclast_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"aesdeclast %%xmm1 ,%%xmm2\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

static unsigned aesimc_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"aesimc %%xmm1 ,%%xmm2\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

static unsigned aeskeygen_checking(void)
{
	asm volatile(ASM_TRY("1f")
		"aeskeygenassist $10, %%xmm2,%%xmm1\n\t"
		"1:"
		:
		:
		:);
	return exception_vector();
}

/*
 * @brief case name: Write "Genu" in ASCII encoding to guest EBX when a vCPU
 * attempts to read CPUID.0H_001
 *
 * Summary: When a vCPU attempts to read CPUID.0H, ACRN hypervisor shall write "Genu" in ASCII
 * encoding to guest EBX.
 *
 */
static void misc_cpuid_rqmid_38181_cpuid_eax00_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0, 0).b;
	report("%s", (*(char *)&result == 'G') && (*((char *)&result + 1) == 'e') &&
	(*((char *)&result + 2) == 'n') && (*((char *)&result + 3) == 'u'),  __FUNCTION__);
}

/*
 * @brief case name: Write "ntel" in ASCII encoding to guest ECX When a vCPU
 * attempts to read CPUID.0H_001
 *
 * Summary: When a vCPU attempts to read CPUID.0H, ACRN hypervisor shall write "ntel" in ASCII
 * encoding to guest ECX.
 *
 */
static void misc_cpuid_rqmid_38182_cpuid_eax00_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0, 0).c;
	report("%s", (*(char *)&result == 'n') && (*((char *)&result + 1) == 't') &&
	(*((char *)&result + 2) == 'e') && (*((char *)&result + 3) == 'l'),  __FUNCTION__);
}

/*
 * @brief case name: Write "inel" in ASCII encoding to guest EDX when a vCPU
 * attempts to read CPUID.0H
 *
 * Summary: When a vCPU attempts to read CPUID.0H, ACRN hypervisor shall write "ineI" in ASCII encoding to guest EDX.
 *
 */
static void misc_cpuid_rqmid_38183_cpuid_eax00_ecx0_001()
{
	u32 result;
	result = cpuid_indexed(0, 0).d;
	report("%s", (*(char *)&result == 'i') && (*((char *)&result + 1) == 'n') &&
	(*((char *)&result + 2) == 'e') && (*((char *)&result + 3) == 'I'),  __FUNCTION__);

}

/*
 * @brief case name: Write 806eaH to guest EAX when a vCPU attempts to read CPUID.1H_001
 *
 * Summary: When a vCPU attempts to read CPUID.1H, ACRN hypervisor shall write 806eaH to guest EAX.
 *
 */
static void misc_cpuid_rqmid_38184_cpuid_eax01_ecx0_001()
{
	report("%s", (cpuid_indexed(1, 0).a) == 0x806ea, __FUNCTION__);
}

/*
 * @brief case name: Write 0H to guest EBX [bit 7:0] when a vCPU attempts to read CPUID.1H_001
 *
 * Summary: When a vCPU attempts to read CPUID.1H, ACRN hypervisor shall write 0H to guest EBX [bit 7:0].
 *
 */
static void misc_cpuid_rqmid_38185_cpuid_eax01_ecx0_001()
{
	report("%s", (cpuid_indexed(1, 0).b & 0xFF) == 0, __FUNCTION__);
}

/*
 * @brief case name: Write 1H to guest ECX [bit 1] when a vCPU attempts to read CPUID.1H_001
 *
 * Summary: When a vCPU attempts to read CPUID.1H, ACRN hypervisor shall write 1H to guest ECX [bit 1].
 *
 */
static void misc_cpuid_rqmid_38186_cpuid_eax01_ecx0_001()
{
	report("%s", ((cpuid_indexed(1, 0).c & (1 << 1)) >> 1) == 1, __FUNCTION__);
}

/*
 * @brief case name: Write 0H to guest ECX [bit 11] when a vCPU attempts to read CPUID.1H_001
 *
 * Summary: When a vCPU attempts to read CPUID.1H, ACRN hypervisor shall write 0H to guest ECX [bit 11].
 *
 */
static void misc_cpuid_rqmid_38187_cpuid_eax01_ecx0_001()
{
	report("%s", (cpuid_indexed(1, 0).c & (1UL << 11)) == 0, __FUNCTION__);
}

/*
 * @brief case name: Write 0H to guest ECX [bit 14] when a vCPU attempts to read CPUID.1H_001
 *
 * Summary: Write 0 to ECX[bit 14] when a vCPU attempts to read CPUID.1H
 *
 */
static void misc_cpuid_rqmid_38199_cpuid_eax01_ecx0_001()
{
	printf("cpuid_indexed(1, 0).c = 0x%x\n", cpuid_indexed(1, 0).c);
	report("%s", (cpuid_indexed(1, 0).c  & (1UL << 14)) == 0, __FUNCTION__);
}

/*
 * @brief case name: Write 1H to guest ECX [bit 25] when a vCPU attempts to read CPUID.1H_001
 *
 * Summary: Write 1 to ECX[bit 25] when a vCPU attempts to read CPUID.1H
 *
 */
static void misc_cpuid_rqmid_38200_cpuid_eax01_ecx0_001()
{
	report("%s", ((cpuid_indexed(1, 0).c >> 25) & 0x1) == 1, __FUNCTION__);
}

/*
 * @brief case name: Write 0H to guest EDX [bit 18] when a vCPU attempts to read CPUID.1H_001
 *
 * Summary: Write 0 to EDX[bit 18] when a vCPU attempts to read CPUID.1H
 *
 */
static void misc_cpuid_rqmid_38202_cpuid_eax01_ecx0_001()
{
	report("%s", (cpuid_indexed(1, 0).d  & (1UL << 18)) == 0, __FUNCTION__);
}

/*
 * @brief case name: Write 1H to guest EDX [bit 27] when a vCPU attempts to read CPUID.1H_001
 *
 * Summary: Write 1 to EDX[bit 27] when a vCPU attempts to read CPUID.1H
 *
 */
static void misc_cpuid_rqmid_38203_cpuid_eax01_ecx0_001()
{
	report("%s", ((cpuid_indexed(1, 0).d >> 27) & 0x1) == 1, __FUNCTION__);
}

/*
 * @brief case name: Write 0H to guest EDX [bit 31] when a vCPU attempts to read CPUID.1H_001
 *
 * Summary: Write 0 to EDX[bit 31] when a vCPU attempts to read CPUID.1H
 *
 */
static void misc_cpuid_rqmid_38205_cpuid_eax01_ecx0_001()
{
	report("%s", (cpuid_indexed(1, 0).d  & (1UL << 31)) == 0, __FUNCTION__);
}

/*
 * @brief case name: Write 0H to guest EAX, guest EBX, guest ECX, and guest EDX when a vCPU attempts
 * to read CPUID.3H_001
 *
 * Summary: When a vCPU attempts to read CPUID.3H, ACRN hypervisor shall write 0H to guest EAX,
 * guest EBX, guest ECX, and guest EDX.
 *
 */
static void misc_cpuid_rqmid_38206_cpuid_eax03_ecx0_001()
{
	struct cpuid cpuid03h = {0};
	cpuid03h = cpuid_indexed(3, 0);
	report("%s", ((cpuid03h.a == 0) && (cpuid03h.b == 0) && (cpuid03h.c == 0) && (cpuid03h.d == 0)), __FUNCTION__);
}

/*
 * @brief case name: Write 0H to guest EAX when a vCPU attempts to read CPUID.(EAX=7H, ECX=0H)_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=7H, ECX=0H), ACRN hypervisor shall write 0H to guest EAX.
 *
 */
static void misc_cpuid_rqmid_38207_cpuid_eax07_ecx0_001()
{
	report("%s", (cpuid_indexed(7, 0).a  == 0), __FUNCTION__);
}

/*
 * @brief case name: Write 1H to guest EBX [bit 3] when a vCPU attempts to read CPUID.(EAX=7H, ECX=0H)_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=7H, ECX=0H), ACRN hypervisor shall write 1H to guest EBX [bit 3].
 *
 */
static void misc_cpuid_rqmid_38208_cpuid_eax07_ecx0_001()
{
	report("%s", ((cpuid_indexed(7, 0).b >> 3) & 0x1) == 1, __FUNCTION__);
}

/*
 * @brief case name: Read CPUID.(EAX=7H, ECX=0H):EBX [bit 8]_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=7H, ECX=0H),
 * ACRN hypervisor shall write 1H to guest EBX [bit 8].
 *
 */
static void misc_cpuid_rqmid_38209_cpuid_eax07_ecx0_001()
{
	report("%s", ((cpuid_indexed(7, 0).b >> 8) & 0x1) == 1, __FUNCTION__);
}

/*
 * @brief case name: Write 0H to guest EBX [bit 29] when a vCPU attempts to read CPUID.(EAX=7H, ECX=0H)_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=7H, ECX=0H), ACRN hypervisor shall write 0H to guest EBX [bit 29].
 *
 */
static void misc_cpuid_rqmid_38210_cpuid_eax07_ecx0_001()
{
	report("%s", (cpuid_indexed(7, 0).b  & (1UL << 29)) == 0, __FUNCTION__);
}

/*
 * @brief case name: Write 0H to guest ECX [bit 0] when a vCPU attempts to read CPUID.(EAX=7H, ECX=0H)_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=7H, ECX=0H), ACRN hypervisor shall write 0H to guest ECX [bit 0].
 *
 */
static void misc_cpuid_rqmid_38211_cpuid_eax07_ecx0_001()
{
	report("%s", (cpuid_indexed(7, 0).c  & 0x1) == 0, __FUNCTION__);
}

/*
 * @brief case name: Write 0H to guest ECX [bit 2] when a vCPU attempts to read CPUID.(EAX=7H, ECX=0H)_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=7H, ECX=0H), ACRN hypervisor shall write 0H to guest ECX [bit 2].
 *
 */
static void misc_cpuid_rqmid_38212_cpuid_eax07_ecx0_001()
{
	report("%s", (cpuid_indexed(7, 0).c & (1UL << 2)) == 0, __FUNCTION__);
}

/*
 * @brief case name: Write 0H to guest ECX [bit 22] when a vCPU attempts to read CPUID.(EAX=7H, ECX=0H)_001
 *
 * Summary: When a vCPU attempts to read CPUID.(EAX=7H, ECX=0H), ACRN hypervisor shall write 0H to guest ECX [bit 22].
 *
 */
static void misc_cpuid_rqmid_38213_cpuid_eax07_ecx0_001()
{
	report("%s", (cpuid_indexed(7, 0).c & (1UL << 22)) == 0, __FUNCTION__);
}

/*
 * @brief case name: Read CPUID.9H:EAX_001
 *
 * Summary: When a vCPU attempts to read CPUID.9H, ACRN hypervisor shall write 0H to guest EAX.
 *
 */
static void misc_cpuid_rqmid_38215_cpuid_eax09_ecx0_001()
{
	report("%s", cpuid_indexed(9, 0).a == 0, __FUNCTION__);
}

/*
 * @brief case name: Write 834H to guest EAX when a vCPU attempts to read CPUID.16H_001
 *
 * Summary: Write 834H to guest EAX when a vCPU attempts to read CPUID.16H.
 *
 */
static void misc_cpuid_rqmid_38218_cpuid_eax16h_ecx0_001()
{
	report("%s", cpuid_indexed(0x16, 0).a == 0x834, __FUNCTION__);
}

/*
 * @brief case name: Write 1068H to guest EBX when a vCPU attempts to read CPUID.16H_001
 *
 * Summary: Write 1068H to guest EBX when a vCPU attempts to read CPUID.16H.
 *
 */
static void misc_cpuid_rqmid_38219_cpuid_eax16h_ecx0_001()
{
	report("%s", cpuid_indexed(0x16, 0).b == 0x1068, __FUNCTION__);
}

/*
 * @brief case name: Write 64H to guest ECX when a vCPU attempts to read CPUID.16H_001
 *
 * Summary: Write 64H to guest ECX when a vCPU attempts to read CPUID.16H.
 *
 */
static void misc_cpuid_rqmid_38220_cpuid_eax16h_ecx0_001()
{
	report("%s", cpuid_indexed(0x16, 0).c == 0x64, __FUNCTION__);
}

/*
 * @brief case name: Write 80000008H to guest EAX when a vCPU attempts to read CPUID.80000000H_001
 *
 * Summary: Write 80000008H to guest EAX when a vCPU attempts to read CPUID.80000000H.
 *
 */
static void misc_cpuid_rqmid_38221_cpuid_eax80000000h_ecx0_001()
{
	report("%s", cpuid_indexed(0x80000000, 0).a == 0x80000008, __FUNCTION__);
}

/*
 * @brief case name: Write 0H to guest EAX when a vCPU attempts to read CPUID.80000001H_001
 *
 * Summary: Write 0H to guest EAX when a vCPU attempts to read CPUID.80000001H_001.
 *
 */
static void misc_cpuid_rqmid_38223_cpuid_eax80000001h_ecx0_001()
{
	report("%s", cpuid_indexed(0x80000001, 0).a == 0x0, __FUNCTION__);
}

/*
 * @brief case name: Write 1H to guest ECX [bit 5] when a vCPU attempts to read CPUID.80000001H_001
 *
 * Summary: Write 1H to guest ECX [bit 5] when a vCPU attempts to read CPUID.80000001H_001.
 *
 */
static void misc_cpuid_rqmid_38224_cpuid_eax80000001h_ecx0_001()
{
	report("%s", ((cpuid_indexed(0x80000001, 0).c >> 5) & 0x1) == 0x1, __FUNCTION__);
}

/*
 * @brief case name: Write "Inte" in ASCII encoding to guest EAX when a vCPU attempts to
 * read CPUID.80000002H_001
 *
 * Summary: Write "Inte" in ASCII encoding to guest EAX when a vCPU attempts to
 * read CPUID.80000002H
 *
 */
static void misc_cpuid_rqmid_38227_cpuid_eax80000002h_ecx0_001()
{
	int result;
	result = cpuid_indexed(0x80000002, 0).a;
	report("%s", (*(char *)&result == 'I') && (*((char *)&result + 1) == 'n') &&
	(*((char *)&result + 2) == 't') && (*((char *)&result + 3) == 'e'), __FUNCTION__);
}

/*
 * @brief case name: Write "l(R)" in ASCII encoding to guest EBX when a vCPU
 * attempts to read CPUID.80000002H_001
 *
 * Summary: Write "l(R)" in ASCII encoding to guest EAX when a vCPU attempts to
 * read CPUID.80000002H
 *
 */
static void misc_cpuid_rqmid_38229_cpuid_eax80000002h_ecx0_001()
{
	int result;
	result = cpuid_indexed(0x80000002, 0).b;
	report("%s", (*(char *)&result == 'l') && (*((char *)&result + 1) == '(') &&
	(*((char *)&result + 2) == 'R') && (*((char *)&result + 3) == ')'),  __FUNCTION__);

}

/*
 * @brief case name: Write "Cor" in ASCII encoding to guest ECX when a vCPU
 * attempts to read CPUID.80000002H_001
 *
 * Summary: Write "Cor" in ASCII encoding to guest ECX when a vCPU attempts to
 * read CPUID.80000002H
 *
 */
static void misc_cpuid_rqmid_38231_cpuid_eax80000002h_ecx0_001()
{
	int result;
	result = cpuid_indexed(0x80000002, 0).c;
	//printf("1 %x\n", *(char*)&result);
	//printf("2 %c\n", *((char*)&result + 1));
	//printf("3 %c\n", *((char*)&result + 2));
	//printf("4 %c\n", *((char*)&result + 3));

	report("%s", (*(char *)&result == ' ') && (*((char *)&result + 1) == 'C') &&
	(*((char *)&result + 2) == 'o') && (*((char *)&result + 3) == 'r'),  __FUNCTION__);

}

/*
 * @brief case name: Write "e(TM" in ASCII encoding to guest EDX when a vCPU
 * attempts to read CPUID.80000002H_001
 *
 * Summary: Write "e(TM" in ASCII encoding to guest EDX when a vCPU attempts
 * to read CPUID.80000002H_001
 *
 */
static void misc_cpuid_rqmid_38232_cpuid_eax80000002h_ecx0_001()
{
	int result;
	result = cpuid_indexed(0x80000002, 0).d;
	report("%s", (*(char *)&result == 'e') && (*((char *)&result + 1) == '(') &&
	(*((char *)&result + 2) == 'T') && (*((char *)&result + 3) == 'M'),  __FUNCTION__);

}

/*
 * @brief case name: Write ")i7" in ASCII encoding to guest EAX when a vCPU attempts to
 * read CPUID.80000003H_001
 *
 * Summary: Write ") i7" in ASCII encoding to guest EAX when a vCPU attempts to read CPUID.80000003H_001
 *
 */
static void misc_cpuid_rqmid_38233_cpuid_eax80000003h_ecx0_001()
{
	int result;
	result = cpuid_indexed(0x80000003, 0).a;
	report("%s", (*(char *)&result == ')') && (*((char *)&result + 1) == ' ') &&
	(*((char *)&result + 2) == 'i') && (*((char *)&result + 3) == '7'),  __FUNCTION__);
}

/*
 * @brief case name: Write "-865" in ASCII encoding to guest EBX when a vCPU attempts to read CPUID.80000003H_001
 *
 * Summary: Write "-865" in ASCII encoding to guest EBX when a vCPU attempts to read CPUID.80000003H
 *
 */
static void misc_cpuid_rqmid_38234_cpuid_eax80000003h_ecx0_001()
{
	int result;
	result = cpuid_indexed(0x80000003, 0).b;
	report("%s", (*(char *)&result == '-') && (*((char *)&result + 1) == '8') &&
	(*((char *)&result + 2) == '6') && (*((char *)&result + 3) == '5'),  __FUNCTION__);
}

/*
 * @brief case name: Write "0U C" in ASCII encoding to guest ECX when a vCPU attempts to read CPUID.80000003H_001
 *
 * Summary: Write "0U C" in ASCII encoding to guest ECX when a vCPU attempts to read CPUID.80000003H
 *
 */
static void misc_cpuid_rqmid_38235_cpuid_eax80000003h_ecx0_001()
{
	int result;
	result = cpuid_indexed(0x80000003, 0).c;
	report("%s", (*(char *)&result == '0') && (*((char *)&result + 1) == 'U') &&
	(*((char *)&result + 2) == ' ') && (*((char *)&result + 3) == 'C'),  __FUNCTION__);
}

/*
 * @brief case name: Write "PU @" in ASCII encoding to guest EDX when a vCPU attempts to read CPUID.80000003H_001
 *
 * Summary: Write "PU @" in ASCII encoding to guest EDX when a vCPU attempts to read CPUID.80000003H
 *
 */
static void misc_cpuid_rqmid_38236_cpuid_eax80000003h_ecx0_001()
{
	int result;
	result = cpuid_indexed(0x80000003, 0).d;
	report("%s", (*(char *)&result == 'P') && (*((char *)&result + 1) == 'U') &&
	(*((char *)&result + 2) == ' ') && (*((char *)&result + 3) == '@'),  __FUNCTION__);
}

/*
 * @brief case name:Write " 1.9" in ASCII encoding to guest EAX when a vCPU attempts to read CPUID.80000004H_001
 *
 * Summary:Write " 1.9" in ASCII encoding to guest EAX when a vCPU attempts to read CPUID.80000004H
 *
 */
static void misc_cpuid_rqmid_38237_cpuid_eax80000004h_ecx0_001()
{
	int result;
	result = cpuid_indexed(0x80000004, 0).a;
	report("%s", (*(char *)&result == ' ') && (*((char *)&result + 1) == '1') &&
	(*((char *)&result + 2) == '.') && (*((char *)&result + 3) == '9'),  __FUNCTION__);
}

/*
 * @brief case name:Write "0GHz" in ASCII encoding to guest EBX when a vCPU attempts to read CPUID.80000004H_001
 *
 * Summary:Write "0GHz" in ASCII encoding to guest EBX when a vCPU attempts to read CPUID.80000004H
 *
 */
static void misc_cpuid_rqmid_38238_cpuid_eax80000004h_ecx0_001()
{
	int result;
	result = cpuid_indexed(0x80000004, 0).b;
	report("%s", (*(char *)&result == '0') && (*((char *)&result + 1) == 'G') &&
	(*((char *)&result + 2) == 'H') && (*((char *)&result + 3) == 'z'),  __FUNCTION__);
}

/*
 * @brief case name:Write 0H to guest ECX and guest EDX when a vCPU attempts to read CPUID.80000004H_001
 *
 * Summary:Write 0H to guest ECX and guest EDX when a vCPU attempts to read CPUID.80000004H
 *
 */
static void misc_cpuid_rqmid_38239_cpuid_eax80000004h_ecx0_001()
{
	struct cpuid cpuid;
	cpuid = cpuid_indexed(0x80000004, 0);
	report("%s", (cpuid.c == 0) && (cpuid.d == 0),  __FUNCTION__);
}

/*
 * @brief case name:Write 40H to guest ECX [bit 7:0] when a vCPU attempts to read CPUID.80000006H_001
 *
 * Summary:Write 40H to guest ECX [bit 7:0] when a vCPU attempts to read CPUID.80000006H
 *
 */
static void misc_cpuid_rqmid_38240_cpuid_eax80000006h_ecx0_001()
{
	struct cpuid cpuid;
	cpuid = cpuid_indexed(0x80000006, 0);
	report("%s", (cpuid.c & 0xFF) == 0x40,  __FUNCTION__);
}

/*
 * @brief case name:Write 4H to guest ECX [bit 15:12] when a vCPU attempts to read CPUID.80000006H_001
 *
 * Summary:Write 4H to guest ECX [bit 15:12] when a vCPU attempts to read CPUID.80000006H
 *
 */
static void misc_cpuid_rqmid_38247_cpuid_eax80000006h_ecx0_001()
{
	struct cpuid cpuid;
	cpuid = cpuid_indexed(0x80000006, 0);
	report("%s", ((cpuid.c >> 12) & 0xF) == 0x4,  __FUNCTION__);
}

/*
 * @brief case name:Write 100H to guest ECX [bit 31:16] when a vCPU attempts to read CPUID.80000006H_001
 *
 * Summary:Write 100H to guest ECX [bit 31:16] when a vCPU attempts to read CPUID.80000006H
 *
 */
static void misc_cpuid_rqmid_38248_cpuid_eax80000006h_ecx0_001()
{
	struct cpuid cpuid;
	cpuid = cpuid_indexed(0x80000006, 0);
	report("%s", ((cpuid.c >> 16) & 0xFFFF) == 0x100,  __FUNCTION__);
}

/*
 * @brief case name:Write 0H to guest ECX [bit 18] when a vCPU attempts to read CPUID.1H_001
 *
 * Summary:Write 0H to guest ECX [bit 18] when a vCPU attempts to read CPUID.1H
 *
 */
static void misc_cpuid_rqmid_38250_cpuid_eax01h_ecx0_001()
{
	struct cpuid cpuid;
	cpuid = cpuid_indexed(0x01, 0);
	report("%s", (cpuid.c & (1UL << 18)) == 0,  __FUNCTION__);
}

/*
 * @brief case name:Write 0H to guest EAX, guest EBX, guest ECX, and guest EDX when a vCPU
 * attempts to read CPUID_001
 *
 * Summary:When a vCPU attempts to read CPUID, the leaf value in guest EAX is less than or
 * equal to the maximum input value and the leaf is unavailable on virtual platform, ACRN
 * hypervisor shall write 0H to guest EAX, guest EBX, guest ECX and guest EDX.
 *
 */
static void misc_cpuid_rqmid_38283_cpuid_eax12h_ecx0_001()
{
	struct cpuid cpuid;

	wrmsr(IA32_MISC_ENABLE, rdmsr(IA32_MISC_ENABLE) & (~(1 << 22)));
	cpuid = cpuid_indexed(0x12, 0);
	report("%s", (cpuid.a == 0) && (cpuid.b == 0) && (cpuid.c == 0) && (cpuid.d == 0),  __FUNCTION__);
}

/*
 * @brief case name:Write 2H to EAX when a vCPU attempts to read CPUID.0H_001
 *
 * Summary:Write 2H to EAX when a vCPU attempts to read CPUID.0H
 *
 */
static void misc_cpuid_rqmid_38284_cpuid_eax0h_ecx0_001()
{
	struct cpuid cpuid;
	wrmsr(IA32_MISC_ENABLE, rdmsr(IA32_MISC_ENABLE) | (1 << 22));
	cpuid = cpuid_indexed(0x0, 0);
	report("%s", cpuid.a == 0x2,  __FUNCTION__);
	/*restore IA32_MISC_ENABLE's original value,or execute cpuid above 2 will return 0 in eax ebx exc edx*/
	wrmsr(IA32_MISC_ENABLE, rdmsr(IA32_MISC_ENABLE) ^ (1 << 22));
}

/*
 * @brief case name:Guarantee the ratio of the TSC frequency and the core crystal clock frequency is 58H_001
 *
 * Summary:Guarantee the ratio of the TSC frequency and the core crystal clock frequency is 58H
 *
 */
static void misc_cpuid_rqmid_38285_cpuid_eax15h_ecx0_001()
{
	struct cpuid cpuid;
	u32 b;
	u32 a;

	cpuid = cpuid_indexed(0x15, 0);
	b = cpuid.b;
	a = cpuid.a;

	if ((a != 0) && (b != 0)) {
		report("%s", (b/a) == 0x58, __FUNCTION__);
	} else {
		report("%s", false, __FUNCTION__);
	}
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access IA32_DEBUG_INTERFACE_001
 *
 * Summary: write from MSR register IA32_DEBUG_INTERFACE shall generate #GP
 */
static void misc_cpuid_rqmid_38291_write_msr_debug_interface()
{
	u64 msr_debug_interface = VALUE_TO_WRITE_MSR;
	report("%s",
		wrmsr_checking(IA32_DEBUG_INTERFACE, msr_debug_interface)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access IA32_DEBUG_INTERFACE_002
 *
 * Summary: read from guest MSR register IA32_DEBUG_INTERFACE shall generate #GP
 */
static void misc_cpuid_rqmid_38292_read_msr_debug_interface()
{
	u64 msr_debug_interface;
	report("%s",
		rdmsr_checking(IA32_DEBUG_INTERFACE, &msr_debug_interface)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access IA32_PLATFORM_DCA_CAP_001
 *
 * Summary: write from MSR register IA32_PLATFORM_DCA_CAP shall generate #GP
 */
static void misc_cpuid_rqmid_38293_write_msr_platform_dca_cap()
{
	u64 msr_platform_dca_cap = VALUE_TO_WRITE_MSR;
	report("%s",
		wrmsr_checking(IA32_PLATFORM_DCA_CAP, msr_platform_dca_cap)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access IA32_PLATFORM_DCA_CAP_002
 *
 * Summary: read from guest MSR register IA32_PLATFORM_DCA_CAP shall generate #GP
 */
static void misc_cpuid_rqmid_38294_read_msr_platform_dca_cap()
{
	u64 msr_platform_dca_cap;
	report("%s",
		rdmsr_checking(IA32_PLATFORM_DCA_CAP, &msr_platform_dca_cap)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name: Guarantee that the vCPU receives #GP(0) when a vCPU attempts to write guest IA32_MISC_ENABLE_001
 *
 * Summary: write some value to MSR register IA32_MISC_ENABLE shall generate #GP
 */
static void misc_cpuid_rqmid_38295_write_msr_ia32_misc_enable()
{
	u64 msr_ia32_misc_enable;

	msr_ia32_misc_enable = rdmsr(IA32_MISC_ENABLE);
	msr_ia32_misc_enable = msr_ia32_misc_enable ^ MSR_IA32_MISC_ENABLE_XTPR_DISABLE;

	report("%s",
		wrmsr_checking(IA32_MISC_ENABLE, msr_ia32_misc_enable)
		== GP_VECTOR, __FUNCTION__);
}

/**
 * @brief case name:Guarantee that the vCPU receives #UD when a vCPU attempts to execute RDPID_001
 *
 * Summary:Guarantee that the vCPU receives #UD when a vCPU attempts to execute RDPID
 */
static void misc_cpuid_rqmid_38299_execute_rdpid()
{
	report("%s", rdpid_checking() == UD_VECTOR, __FUNCTION__);
}

/**
 * @brief case name:No operation when a vCPU attempts to execute PREFETCHWT1_001
 *
 * Summary:When a vCPU attempts to execute PREFETCHWT1, ACRN hypervisor shall guarantee that the instruction
 * performs no operation
 */
static void misc_cpuid_rqmid_38306_execute_prefetchwt1()
{
	bool ret = false;
	u8 line = 1;

	enable_xsave();
	ret = CHECK_INSTRUCTION_REGS(asm volatile("prefetchwt1 %0\n\t"
		:
		: "m"(line)
		:));
	report("%s", ret, __FUNCTION__);
}

/**
 * @brief case name: Set the initial guest IA32_MISC_ENABLE [bit 23] to 1H following start-up_001
 * Summary: Read the initial value of IA32_MISC_ENABLE[bit 23] at BP start-up, the result shall be 1H.
 */
static void misc_cpuid_rqmid_38370_ia32_misc_enable_following_start_up_001(void)
{
	volatile u32 *ptr = (volatile u32 *)IA32_MISC_ENABLE_STARTUP_LOW_ADDR;
	u64 ia32_init_first;

	ia32_init_first = *ptr + ((u64)(*(ptr + 1)) << 32);
	report("%s", ((ia32_init_first >> 23) & 0x1) == 1UL,
		   __FUNCTION__);
}

/**
 * @brief case name: Set the initial guest IA32_MISC_ENABLE [bit 23] to 1H following INIT_001
 * Summary: Check the initial value of IA32_MISC_ENABLE[bit 23] at AP entry, the result shall be 1H.
 */
static void misc_cpuid_rqmid_38369_ia32_misc_enable_following_init_001(void)
{
	bool is_pass = true;

	if (((*(volatile uint64_t *)IA32_MISC_ENABLE_INIT_LOW_ADDR) & (1ull << 23)) == 0) {
		is_pass = false;
	}

	notify_ap_modify_and_read_init_value(38369);

	if (((*(volatile uint64_t *)IA32_MISC_ENABLE_INIT_LOW_ADDR) & (1ull << 23)) == 0) {
		is_pass = false;
	}

	report("%s", is_pass, __FUNCTION__);
}

/**
 * @brief case name:Guarantee that the vCPU receives #UD when a vCPU attempts to execute SHA1MSG1/SHA1MSG2/
 * SHA1NEXTE/SHA1RNDS4/SHA256MSG1/SHA256MSG2/SHA256RNDS2_001
 *
 * Summary:Guarantee that the vCPU receives #UD when a vCPU attempts to execute related SHA.
 */
static void misc_cpuid_rqmid_38372_execute_sha_related_instruction()
{
	report("%s", (sha1msg1_checking() == UD_VECTOR) && (sha1msg2_checking() == UD_VECTOR)
		&& (sha1nexte_checking() == UD_VECTOR) && (sha256msg1_checking() == UD_VECTOR)
		&& (sha256msg2_checking() == UD_VECTOR) && (sha256rnds2_checking() == UD_VECTOR)
		&& (sha1rnds4_checking() == UD_VECTOR), __FUNCTION__);
}

/*
 * @brief case name:Expose enhanced bitmap instructions to any VM_001
 *
 * Summary: Expose enhanced bitmap instructions to any VM.
 *
 */
static void misc_cpuid_rqmid_38414_cpuid_eax07_ecx0_001()
{
	u32 result;
	u64 result1;

	result = cpuid_indexed(0x07, 0).b;
	if (!((result >> 3) & 0x1)) {
		report("%s", false, __FUNCTION__);
		return;
	}

	if (!((result >> 8) & 0x1)) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movl $0xaaaaaaaa, %%eax\n"
				"movl $0x55555555, %%ebx\n"
				"andn %%ebx, %%eax, %%edx\n"
				 "movl %%edx, %0\n"
				 : "=m"(result)
				: : "memory");
	if (result != 0x55555555) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movl $0xaaaaaaaa, %%eax\n"
				"movl $0x800, %%ebx\n"
				"bextr %%ebx, %%eax, %%edx\n"
				"movl %%edx, %0\n"
				 : "=m"(result)
				: : "memory");
	if (result != 0xaa) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movl $0x0, %%eax\n"
				"movl $0x2, %%ebx\n"
				"blsi %%ebx, %%eax\n"
				"movl %%eax, %0\n"
				: "=m"(result)
				: : "memory");
	if (result != 0x2) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movl $0x0, %%eax\n"
				"movl $0x10, %%ebx\n"
				"blsmsk %%ebx, %%eax\n"
				"movl %%eax, %0\n"
				: "=m"(result)
				: : "memory");
	if (result != 0x1f) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movl $0x0, %%eax\n"
				"movl $0x18, %%ebx\n"
				"blsr %%ebx, %%eax\n"
				"movl %%eax, %0\n"
				: "=m"(result)
				: : "memory");
	if (result != 0x10) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movl $0x0, %%eax\n"
				"movl $0xF0, %%ebx\n"
				"tzcnt %%ebx, %%eax\n"
				"movl %%eax, %0\n"
				: "=m"(result)
				: : "memory");
	if (result != 4) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movl $0xffffffaa, %%eax\n"
				"movl $0x9, %%ebx\n"
				"bzhi %%ebx, %%eax, %%edx\n"
				"movl %%edx, %0\n"
				 : "=m"(result)
				: : "memory");
	if (result != 0x1aa) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movl $0x2, %%edx\n"
				"movl $0x5, %%ebx\n"
				"mulx %%ebx, %%eax, %%ecx\n"
				"movl %%eax, %0\n"
				 : "=m"(result)
				: : "memory");

	if (result != 0xa) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movl $0xf, %%eax\n"
				"movl $0x8888, %%ebx\n"
				"pdep %%ebx, %%eax, %%edx\n"
				"movl %%edx, %0\n"
				 : "=m"(result)
				: : "memory");
	if (result != 0x8888) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movl $0xfffffff7, %%eax\n"
				"movl $0x8888, %%ebx\n"
				"pext %%ebx, %%eax, %%edx\n"
				"movl %%edx, %0\n"
				 : "=m"(result)
				: : "memory");
	if (result != 0xe) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movq $0xf, %%rax\n"
				"rorxq $1, %%rax, %%rdx\n"
				"movq %%rdx, %0\n"
				 : "=m"(result1)
				: : "memory");
	if (result1 != 0x8000000000000007) {
		report("%s", false, __FUNCTION__);
		return;
	}


	asm volatile ("movl $0xf, %%eax\n"
				"movl $2, %%ebx\n"
				"sarx %%ebx, %%eax, %%edx\n"
				"movl %%edx, %0\n"
				 : "=m"(result)
				: : "memory");
	if (result != 0x3) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movl $0xf, %%eax\n"
					"movl $2, %%ebx\n"
					"shlx %%ebx, %%eax, %%edx\n"
					"movl %%edx, %0\n"
					 : "=m"(result)
					: : "memory");
	if (result != 0x3c) {
		report("%s", false, __FUNCTION__);
		return;
	}

	asm volatile ("movl $0xf, %%eax\n"
					"movl $2, %%ebx\n"
					"shrx %%ebx, %%eax, %%edx\n"
					"movl %%edx, %0\n"
					 : "=m"(result)
					: : "memory");
	if (result != 0x3) {
		report("%s", false, __FUNCTION__);
		return;
	}

	report("%s", true, __FUNCTION__);

}

/*
 * @brief case name:Expose AESNI instruction extensions feature to any VM_001
 *
 * Summary: Expose AESNI instruction extensions feature to any VM.
 *
 */
static void misc_cpuid_rqmid_38415_cpuid_eax01_ecx0_001()
{
	u32 result = 0;
	u32 count = 0;

	set_sse_env();
	result = cpuid_indexed(0x01, 0).d;
	if (((result >> 25) & 0x1) || ((result >> 26) & 0x1)) {
		count++;
	}

	result = cpuid_indexed(0x01, 0).c;
	if (((result >> 25) & 0x1)) {
		count++;
	}
	if (count != 2) {
		report("%s", false, __FUNCTION__);
		return;
	}

	if ((aesenc_checking() == NO_EXCEPTION) && (aesenclast_checking() == NO_EXCEPTION) &&
		(aesdec_checking() == NO_EXCEPTION) && (aesdeclast_checking() == NO_EXCEPTION) &&
		(aesimc_checking() == NO_EXCEPTION) && (aeskeygen_checking() == NO_EXCEPTION)) {
		report("%s", true, __FUNCTION__);
	} else {
		report("%s", false, __FUNCTION__);
	}
}

/*
 * @brief case name:Expose PCLMULQDQ instruction to any VM_001
 *
 * Summary: Expose PCLMULQDQ instruction to any VM.
 *
 */
static void misc_cpuid_rqmid_38416_expose_pclmulqdq_instruction_001()
{
	ALIGNED(64) sse_un mul1;
	ALIGNED(64) sse_un mul2;
	ALIGNED(64) sse_un mul3;

	set_sse_env();
	mul1.u[0] = 2;
	mul2.u[0] = 25;
	asm volatile(
			"vmovaps %1,%%xmm1\n\t"
			"vmovaps %2,%%xmm2\n\t"
			"pclmulqdq $0,%%xmm2, %%xmm1\n\t"
			"vmovaps %%xmm1,%0\n\t"
			: "=m"(mul3.sse)
			: "m"(mul1.sse), "m"(mul2.sse)
			: "memory");
	report("%s", mul3.u[0] == 50, __FUNCTION__);
}

int check_bits_is_set(u32 value, u32 start, u32 end)
{
	u32 check_value = value;
	u32 start_bit;
	u32 end_bit = end;

	for (start_bit = start; start_bit <= end_bit; start_bit++) {
		if ((check_value & (1UL << start_bit)) != 0) {
			printf("not zero reserve bit:%d\n", start_bit);
			return 1;
		}
	}
	return  0;
}

/*
 * @brief case name:Write 0H to reserved bits when a vCPU attempts to read CPUID_001
 *
 * Summary: Write 0H to reserved bits when a vCPU attempts to read CPUID
 *
 */
static void misc_cpuid_rqmid_38417_check_reserved_bits_001()
{
	struct cpuid cpuid_value;
	u32 count;
	u32 index;
	t_rvdbits	rvdbits[] = {
		{cpuid(0x3).a, 0, 31},
		{cpuid(0x3).b, 0, 31},
		{cpuid(0x4).a, 4, 4},
		{cpuid(0x4).a, 10, 13},
		{cpuid(0x4).d, 3, 31},
		{cpuid(0x5).a, 16, 31},
		{cpuid(0x5).b, 16, 31},
		{cpuid(0x5).c, 2, 31},
		{cpuid(0x6).a, 3, 3},
		{cpuid(0x6).a, 12, 12},
		{cpuid(0x6).a, 19, 19},
		{cpuid(0x6).a, 21, 31},
		{cpuid(0x6).b, 4, 31},
		{cpuid(0x6).c, 1, 2},
		{cpuid(0x6).c, 4, 31},
		{cpuid(0x6).d, 0, 31},
		{cpuid(0x7).b, 22, 22},
		{cpuid(0x7).c, 5, 16},
		{cpuid(0x7).c, 23, 29},
		{cpuid(0x7).c, 31, 31},
		{cpuid(0x7).d, 0, 12},
		{cpuid(0x7).d, 14, 31},
		{cpuid(0x9).b, 0, 31},
		{cpuid(0x9).c, 0, 31},
		{cpuid(0x9).d, 0, 31},
		{cpuid(0x0A).b, 7, 31},
		{cpuid(0x0A).d, 16, 31},
		{cpuid(0x0A).c, 0, 31},
		{cpuid(0x0B).a, 5, 31},
		{cpuid(0x0B).c, 16, 31},
		{cpuid(0x0B).b, 16, 31},
		{cpuid(0x0D).a, 14, 31},
		{cpuid_indexed(0x0D, 0x01).a, 4, 31},
		{cpuid_indexed(0x0D, 0x01).c, 10, 12},
		{cpuid_indexed(0x0D, 0x01).c, 14, 31},
		{cpuid_indexed(0x0D, 0x02).c, 2, 31},
		{cpuid(0xF).a, 0, 31},
		{cpuid(0xF).c, 0, 31},
		{cpuid(0xF).d, 0, 0},
		{cpuid(0xF).d, 2, 31},
		{cpuid_indexed(0x0F, 0x01).a, 0, 31},
		{cpuid_indexed(0x0F, 0x01).d, 3, 31},
		{cpuid(0x10).a, 0, 31},
		{cpuid(0x10).c, 0, 31},
		{cpuid(0x10).d, 0, 31},
		{cpuid(0x10).b, 0, 0},
		{cpuid(0x10).b, 4, 31},
		{cpuid_indexed(0x10, 0x01).a, 5, 31},
		{cpuid_indexed(0x10, 0x01).c, 0, 1},
		{cpuid_indexed(0x10, 0x01).c, 3, 31},
		{cpuid_indexed(0x10, 0x01).d, 16, 31},
		{cpuid_indexed(0x10, 0x02).a, 5, 31},
		{cpuid_indexed(0x10, 0x02).c, 0, 31},
		{cpuid_indexed(0x10, 0x02).d, 16, 31},
		{cpuid_indexed(0x10, 0x03).a, 2, 31},
		{cpuid_indexed(0x10, 0x03).b, 0, 31},
		{cpuid_indexed(0x10, 0x03).c, 0, 1},
		{cpuid_indexed(0x10, 0x03).c, 3, 31},
		{cpuid_indexed(0x10, 0x03).d, 16, 31},
		{cpuid_indexed(0x12, 0x00).a, 2, 4},
		{cpuid_indexed(0x12, 0x00).a, 2, 4},
		{cpuid_indexed(0x12, 0x00).c, 0, 31},
		{cpuid_indexed(0x12, 0x00).d, 16, 31},
		{cpuid_indexed(0x14, 0x00).b, 6, 31},
		{cpuid_indexed(0x14, 0x00).c, 4, 31},
		{cpuid_indexed(0x14, 0x00).d, 0, 31},
		{cpuid_indexed(0x14, 0x01).a, 3, 15},
		{cpuid_indexed(0x14, 0x01).c, 0, 31},
		{cpuid_indexed(0x14, 0x01).d, 0, 31},
		{cpuid_indexed(0x15, 0x00).d, 0, 31},
		{cpuid_indexed(0x16, 0x00).a, 16, 31},
		{cpuid_indexed(0x16, 0x00).b, 16, 31},
		{cpuid_indexed(0x16, 0x00).c, 16, 31},
		{cpuid_indexed(0x16, 0x00).d, 0, 31},
		{cpuid_indexed(0x17, 0x00).b, 17, 31},
		{cpuid_indexed(0x18, 0x00).d, 9, 13},
		{cpuid_indexed(0x18, 0x00).d, 26, 31},
		{cpuid_indexed(0x18, 0x01).a, 0, 31},
		{cpuid_indexed(0x18, 0x01).d, 9, 13},
		{cpuid_indexed(0x18, 0x01).d, 26, 31},
		{cpuid_indexed(0x80000000, 0x00).b, 0, 31},
		{cpuid_indexed(0x80000000, 0x00).c, 0, 31},
		{cpuid_indexed(0x80000000, 0x00).d, 0, 31},
		{cpuid_indexed(0x80000001, 0x00).b, 0, 31},
		{cpuid_indexed(0x80000001, 0x00).c, 1, 4},
		{cpuid_indexed(0x80000001, 0x00).c, 6, 7},
		{cpuid_indexed(0x80000001, 0x00).c, 9, 31},
		{cpuid_indexed(0x80000001, 0x00).d, 0, 10},
		{cpuid_indexed(0x80000001, 0x00).d, 12, 19},
		{cpuid_indexed(0x80000001, 0x00).d, 21, 25},
		{cpuid_indexed(0x80000001, 0x00).d, 28, 28},
		{cpuid_indexed(0x80000001, 0x00).d, 30, 31},
		{cpuid_indexed(0x80000006, 0x00).a, 0, 31},
		{cpuid_indexed(0x80000006, 0x00).b, 0, 31},
		{cpuid_indexed(0x80000006, 0x00).d, 0, 31},
		{cpuid_indexed(0x80000006, 0x00).c, 8, 11},
		{cpuid_indexed(0x80000007, 0x00).a, 0, 31},
		{cpuid_indexed(0x80000007, 0x00).b, 0, 31},
		{cpuid_indexed(0x80000007, 0x00).c, 0, 31},
		{cpuid_indexed(0x80000007, 0x00).d, 0, 7},
		{cpuid_indexed(0x80000007, 0x00).d, 9, 31},
		{cpuid_indexed(0x80000008, 0x00).a, 16, 31},
		{cpuid_indexed(0x80000008, 0x00).b, 0, 31},
		{cpuid_indexed(0x80000008, 0x00).c, 0, 31},
		{cpuid_indexed(0x80000008, 0x00).d, 0, 31},
	};

	count = sizeof(rvdbits)/sizeof(t_rvdbits);
	for (index = 0; index < count; index++) {
		if (check_bits_is_set(rvdbits[index].check_value,
			rvdbits[index].start_bit, rvdbits[index].end_bit)) {
			report("%s(%d) check_value:%x", false, __FUNCTION__, index, rvdbits[index].check_value);
			return;
		}
	}

	cpuid_value = cpuid_indexed(0x12, 0x02);
	if ((cpuid_value.a & 0xF) == 1) {
		if (check_bits_is_set(cpuid_value.a, 4, 11)) {
			report("%s(%d)", false, __FUNCTION__, __LINE__);
			return;
		}

		if (check_bits_is_set(cpuid_value.b, 20, 31)) {
			report("%s(%d)", false, __FUNCTION__, __LINE__);
			return;
		}

		if (check_bits_is_set(cpuid_value.c, 4, 11)) {
			report("%s(%d)", false, __FUNCTION__, __LINE__);
			return;
		}

		if (check_bits_is_set(cpuid_value.d, 20, 31)) {
			report("%s(%d)", false, __FUNCTION__, __LINE__);
			return;
		}
	}

	if (check_bits_is_set(cpuid_indexed(0x12, 0x00).a, 6, 6)) {
		if (check_bits_is_set(cpuid_indexed(0x12, 0x00).a, 2, 31)) {
			report("%s(%d)", false, __FUNCTION__, __LINE__);
			return;
		}
	}

	int maxsocid_index;
	maxsocid_index = cpuid_indexed(0x17, 0x00).a;

	if ((cpuid_indexed(0x17, maxsocid_index+1).a != 0) || (cpuid_indexed(0x17, maxsocid_index+1).b != 0) ||
		(cpuid_indexed(0x17, maxsocid_index+1).c != 0) || (cpuid_indexed(0x17, maxsocid_index+1).d != 0)) {

		report("%s(%d)", false, __FUNCTION__, __LINE__);
		return;
	}
	report("%s", true, __FUNCTION__);
}

/*
 * @brief case name:Expose LZCNT instruction to any VM_001
 *
 * Summary: ACRN hypervisor shall expose LZCNT instruction to any VM, in compliance with LZCNT—
 * Count the Number of Leading Zero Bits, Vol. 2, SDM.
 *
 */
static void misc_cpuid_rqmid_40331_expose_lzcnt_instruction_001()
{
	u32 result;
	u32 test_data = 0x1;
	asm volatile(
			"lzcnt %1, %%eax\n\t"
			"movl %%eax, %0\n\t"
			: "=rm"(result)
			: "r"(test_data)
			: "memory");

	report("%s", result == 31, __FUNCTION__);
}

#endif

static void print_case_list(void)
{
	printf("\t\t misc cpuid feature case list:\n\r");
#ifdef IN_NATIVE
	printf("\t\t Case ID:%d case name:%s\n\r", 38251u,
	"The physical platform supports enhanced bitmap instructions_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38308u,
	"The physical platform supports AESNI instruction extensions_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38309u,
	"The physical platform doesn't support RDPID instruction_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38310u,
	"The physical platform doesn't support PREFETCHWT1 instruction_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38311u,
	"The physical platform doesn't support SHA extensions_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38312u,
	"The physical platform supports PCLMULQDQ instruction_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38313u,
	"The physical platform doesn't support user-mode instruction prevention_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38314u,
	"The physical platform doesn't support Direct Cache Access_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38315u,
	"The physical platform doesn't support  additional features of side channel mitigations_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38316u,
	"The physical platform shall guarantee supported CPUID leaf 7 sub-leaves is 0H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38317u,
	"The physical platform supports self snoop_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38318u,
	"The physical platform doesn't support PSN_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38319u,
	"The physical platform supports multiple cores_001");
#else
	printf("\t\t Case ID:%d case name:%s\n\r", 38181u,
	"Write Genu in ASCII encoding to guest EBX when a vCPU attempts to read CPUID.0H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38182u,
	"Write ntel in ASCII encoding to guest ECX When a vCPU attempts to read CPUID.0H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38183u,
	"Write inel in ASCII encoding to guest EDX when a vCPU attempts to read CPUID.0H");
	printf("\t\t Case ID:%d case name:%s\n\r", 38184u,
	"Write 806eaH to guest EAX when a vCPU attempts to read CPUID.1H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38185u,
	"Write 0H to guest EBX [bit 7:0] when a vCPU attempts to read CPUID.1H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38186u,
	"Write 1H to guest ECX [bit 1] when a vCPU attempts to read CPUID.1H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38187u,
	"Write 0H to guest ECX [bit 11] when a vCPU attempts to read CPUID.1H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38199u,
	"Write 0H to guest ECX [bit 14] when a vCPU attempts to read CPUID.1H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38200u,
	"Write 1H to guest ECX [bit 25] when a vCPU attempts to read CPUID.1H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38202u,
	"Write 0H to guest EDX [bit 25] when a vCPU attempts to read CPUID.1H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38203u,
	"Write 1H to guest EDX [bit 27] when a vCPU attempts to read CPUID.1H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38205u,
	"Write 0H to guest EDX [bit 31] when a vCPU attempts to read CPUID.1H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38206u,
	"Write 0H to guest EAX, guest EBX, guest ECX, and guest EDX when a vCPU attempts_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38207u,
	"Write 0H to guest EAX when a vCPU attempts to read CPUID.(EAX=7H, ECX=0H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38208u,
	"Write 1H to guest EBX [bit 3] when a vCPU attempts to read CPUID.(EAX=7H, ECX=0H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38209u,
	"Read CPUID.(EAX=7H, ECX=0H):EBX [bit 8]_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38210u,
	"Write 0H to guest EBX [bit 29] when a vCPU attempts to read CPUID.(EAX=7H, ECX=0H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38211u,
	"Write 0H to guest ECX [bit 0] when a vCPU attempts to read CPUID.(EAX=7H, ECX=0H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38212u,
	"Write 0H to guest ECX [bit 2] when a vCPU attempts to read CPUID.(EAX=7H, ECX=0H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38213u,
	"Write 0H to guest ECX [bit 22] when a vCPU attempts to read CPUID.(EAX=7H, ECX=0H)_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38215u, "Read CPUID.9H:EAX_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38218u,
	"Write 834H to guest EAX when a vCPU attempts to read CPUID.16H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38219u,
	"Write 1068H to guest EBX when a vCPU attempts to read CPUID.16H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38220u,
	"Write 64H to guest ECX when a vCPU attempts to read CPUID.16H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38221u,
	"Write 80000008H to guest ECX when a vCPU attempts to read CPUID.80000000H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38223u,
	"Write 0H to guest EAX when a vCPU attempts to read CPUID.80000001H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38224u,
	"Write 1H to guest ECX [bit 5] when a vCPU attempts to read CPUID.80000001H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38227u,
	"Write Inte in ASCII encoding to guest EAX when a vCPU attempts to read CPUID.80000002H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38229u,
	"Write \"l(R)\" in ASCII encoding to guest ECX when a vCPU attempts to read CPUID.80000002H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38231u,
	"Write \"Cor\" in ASCII encoding to guest EBX when a vCPU attempts to read CPUID.80000002H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38232u,
	"Write \"e(TM\" in ASCII encoding to guest EDX when a vCPU attempts to read CPUID.80000002H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38233u,
	"Write \")i7\" in ASCII encoding to guest EAX when a vCPU attempts to read CPUID.80000003H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38234u,
	"Write \"-865\" in ASCII encoding to guest EBX when a vCPU attempts to read CPUID.80000003H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38235u,
	"Write \"0U C\" in ASCII encoding to guest ECX when a vCPU attempts to read CPUID.80000003H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38236u,
	"Write \"PU @\" in ASCII encoding to guest EDX when a vCPU attempts to read CPUID.80000003H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38237u,
	"Write \" 1.9\" in ASCII encoding to guest EAX when a vCPU attempts to read CPUID.80000004H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38238u,
	"Write \"0GHz\" in ASCII encoding to guest EBX when a vCPU attempts to read CPUID.80000004H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38239u,
	"Write 0H to guest ECX and guest EDX when a vCPU attempts to read CPUID.80000004H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38240u,
	"Write 40H to guest ECX [bit 7:0] when a vCPU attempts to read CPUID.80000006H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38247u,
	"Write 4H to guest ECX [bit 15:12] when a vCPU attempts to read CPUID.80000006H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38248u,
	"Write 100H to guest ECX [bit 31:16] when a vCPU attempts to read CPUID.80000006H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38250u,
	"Write 0H to guest ECX [bit 18] when a vCPU attempts to read CPUID.1H_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38283u,
	"Write 0H to guest EAX, guest EBX, guest ECX, and guest EDX when a vCPU attempts to read CPUID");
	printf("\t\t Case ID:%d case name:%s\n\r", 38284u,
	"Write 2H to EAX when a vCPU attempts to read CPUID.0H");
	printf("\t\t Case ID:%d case name:%s\n\r", 38285u,
	"Guarantee the ratio of the TSC frequency and the core crystal clock frequency is 58H");
	printf("\t\t Case ID:%d case name:%s\n\r", 38291u,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access IA32_DEBUG_INTERFACE_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38292u,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access IA32_DEBUG_INTERFACE_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 38293u,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access IA32_PLATFORM_DCA_CAP_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38294u,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempts to access IA32_PLATFORM_DCA_CAP_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 38295u,
	"Guarantee that the vCPU receives #GP(0) when a vCPU attempts to write guest IA32_MISC_ENABLE_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38299u,
	"Guarantee that the vCPU receives #UD when a vCPU attempts to execute RDPID_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38306u,
	"No operation when a vCPU attempts to execute PREFETCHWT1_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 38369u,
	"Set the initial guest IA32_MISC_ENABLE [bit 23] to 1H following INIT_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38370u,
	"Set the initial guest IA32_MISC_ENABLE [bit 23] to 1H following start-up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38372u,
	"Guarantee that the vCPU receives #UD when a vCPU attempts to execute SHA1MSG1/SHA1MSG2/\
	SHA1NEXTE/SHA1RNDS4/SHA256MSG1/SHA256MSG2/SHA256RNDS2_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38414u,
	"Expose enhanced bitmap instructions to any VM_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38415u,
	"Expose AESNI instruction extensions feature to any VM_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38416u,
	"Expose PCLMULQDQ instruction to any VM_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38417u,
	"Write 0H to reserved bits when a vCPU attempts to read CPUID_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 40331u, "Expose LZCNT instruction to any VM_001");
#endif
	printf("\t\t \n\r \n\r");

}

int main(void)
{
	setup_idt();
	print_case_list();
#ifdef IN_NATIVE
	misc_cpuid_rqmid_38251_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38308_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38309_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38310_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38311_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38312_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38313_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38314_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38315_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38316_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38317_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38318_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38319_cpuid_eax01_ecx0_001();
#else
	misc_cpuid_rqmid_38285_cpuid_eax15h_ecx0_001();
	misc_cpuid_rqmid_38181_cpuid_eax00_ecx0_001();
	misc_cpuid_rqmid_38182_cpuid_eax00_ecx0_001();
	misc_cpuid_rqmid_38183_cpuid_eax00_ecx0_001();
	misc_cpuid_rqmid_38184_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38185_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38186_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38187_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38199_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38200_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38202_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38203_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38205_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38206_cpuid_eax03_ecx0_001();
	misc_cpuid_rqmid_38207_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38414_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38208_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38209_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38210_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38211_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38212_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38213_cpuid_eax07_ecx0_001();
	misc_cpuid_rqmid_38215_cpuid_eax09_ecx0_001();
	misc_cpuid_rqmid_38218_cpuid_eax16h_ecx0_001();
	misc_cpuid_rqmid_38219_cpuid_eax16h_ecx0_001();
	misc_cpuid_rqmid_38220_cpuid_eax16h_ecx0_001();
	misc_cpuid_rqmid_38221_cpuid_eax80000000h_ecx0_001();
	misc_cpuid_rqmid_38223_cpuid_eax80000001h_ecx0_001();
	misc_cpuid_rqmid_38224_cpuid_eax80000001h_ecx0_001();
	misc_cpuid_rqmid_38227_cpuid_eax80000002h_ecx0_001();
	misc_cpuid_rqmid_38229_cpuid_eax80000002h_ecx0_001();
	misc_cpuid_rqmid_38231_cpuid_eax80000002h_ecx0_001();
	misc_cpuid_rqmid_38232_cpuid_eax80000002h_ecx0_001();
	misc_cpuid_rqmid_38233_cpuid_eax80000003h_ecx0_001();
	misc_cpuid_rqmid_38234_cpuid_eax80000003h_ecx0_001();
	misc_cpuid_rqmid_38235_cpuid_eax80000003h_ecx0_001();
	misc_cpuid_rqmid_38236_cpuid_eax80000003h_ecx0_001();
	misc_cpuid_rqmid_38237_cpuid_eax80000004h_ecx0_001();
	misc_cpuid_rqmid_38238_cpuid_eax80000004h_ecx0_001();
	misc_cpuid_rqmid_38239_cpuid_eax80000004h_ecx0_001();
	misc_cpuid_rqmid_38240_cpuid_eax80000006h_ecx0_001();
	misc_cpuid_rqmid_38247_cpuid_eax80000006h_ecx0_001();
	misc_cpuid_rqmid_38248_cpuid_eax80000006h_ecx0_001();
	misc_cpuid_rqmid_38250_cpuid_eax01h_ecx0_001();
	misc_cpuid_rqmid_38283_cpuid_eax12h_ecx0_001();
	misc_cpuid_rqmid_38284_cpuid_eax0h_ecx0_001();
	misc_cpuid_rqmid_38291_write_msr_debug_interface();
	misc_cpuid_rqmid_38292_read_msr_debug_interface();
	misc_cpuid_rqmid_38293_write_msr_platform_dca_cap();
	misc_cpuid_rqmid_38294_read_msr_platform_dca_cap();
	misc_cpuid_rqmid_38295_write_msr_ia32_misc_enable();
	misc_cpuid_rqmid_38299_execute_rdpid();
	misc_cpuid_rqmid_38306_execute_prefetchwt1();

	misc_cpuid_rqmid_38369_ia32_misc_enable_following_init_001();
	misc_cpuid_rqmid_38370_ia32_misc_enable_following_start_up_001();
	misc_cpuid_rqmid_38372_execute_sha_related_instruction();
	misc_cpuid_rqmid_38415_cpuid_eax01_ecx0_001();
	misc_cpuid_rqmid_38416_expose_pclmulqdq_instruction_001();
	misc_cpuid_rqmid_38417_check_reserved_bits_001();
	misc_cpuid_rqmid_40331_expose_lzcnt_instruction_001();
#endif
	return report_summary();
}
