asm(".code16gcc");
#include "v8086_lib.h"
#include "v8_condition.h"
#include "v8_condition.c"

__unused __attribute__ ((aligned(64))) static union_unsigned_128 unsigned_128;
__unused __attribute__ ((aligned(64))) static union_unsigned_256 unsigned_256;
__attribute__ ((aligned(64))) __unused static u8 unsigned_8 = 0;
__attribute__ ((aligned(64))) __unused static u16 unsigned_16 = 0;
__attribute__ ((aligned(64))) __unused static u32 unsigned_32 = 0;
__attribute__ ((aligned(64))) __unused static u64 unsigned_64 = 0;
__attribute__ ((aligned(64))) __unused static u64 array_64[4] = {0};
u8 exception_vector_bak = 0xFF;
u16 execption_inc_len = 0;


static __unused u16 *unaligned_address_16(void)
{
	__attribute__ ((aligned(16))) u16 addr_unalign_16 = 0;
	u16 *aligned_addr = &addr_unalign_16;
	u16 *non_aligned_addr = (u16 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused u32 *unaligned_address_32(void)
{
	__attribute__ ((aligned(16))) u32 addr_unalign_32 = 0;
	u32 *aligned_addr = &addr_unalign_32;
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

//caused by parameter need 64 bit, but in 32 bit mode set it to 32 bit.
static __unused u32 *unaligned_address_64(void)
{
	__attribute__ ((aligned(16))) u32 addr_unalign_32 = 0;
	u32 *aligned_addr = &addr_unalign_32;
	u32 *non_aligned_addr = (u32 *)((u8 *)aligned_addr + 1);
	return non_aligned_addr;
}

static __unused union_unsigned_128 *unaligned_address_128(void)
{
	__attribute__ ((aligned(32))) union_unsigned_128 addr_unalign_128;
	addr_unalign_128.m[0] = 1;
	addr_unalign_128.m[1] = 1;
	addr_unalign_128.m[2] = 1;
	addr_unalign_128.m[3] = 1;
	union_unsigned_128 *addr = (union_unsigned_128 *)&addr_unalign_128;
	union_unsigned_128 *non_aligned_addr = (union_unsigned_128 *)((u8 *)addr + 1);
	return non_aligned_addr;
}

// trigger_pgfault
static __unused union_unsigned_128 *trigger_pgfault(void)
{
	union_unsigned_128 *add1 = (union_unsigned_128 *)(PAGE_FAULT_ADDR);
	return add1;
}

//Modified manually: add the conditional compilation for the size limit.
#ifdef PHASE_0
static __unused void sse_v8_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f") "HSUBPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_v8_instruction_1(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_v8_instruction_2(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_v8_instruction_3(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MPSADBW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_v8_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PHSUBD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_v8_instruction_5(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVZXBW %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_v8_instruction_6(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MINSS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_v8_instruction_7(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_v8_instruction_8(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVAPD %%xmm1, %[output]\n" "1:"
				: [output] "=m" (*(trigger_pgfault())) : );
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_v8_instruction_9(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_1
static __unused void sse_v8_instruction_10(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMAXUB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_v8_instruction_11(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PCMPESTRM $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_v8_instruction_12(const char *msg)
{
	asm volatile(ASM_TRY("1f") "DIVPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(trigger_pgfault())));
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

static __unused void sse_v8_instruction_13(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVSD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_v8_instruction_14(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSADBW %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_v8_instruction_15(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSUBQ %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_v8_instruction_16(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSHUFB %[input_1], %%mm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_v8_instruction_17(const char *msg)
{
	asm volatile(ASM_TRY("1f") "UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_32())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_v8_instruction_18(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}

static __unused void sse_v8_instruction_19(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PMOVSXWD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_64())));
	exception_vector_bak = exception_vector();
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_0
static __unused void sse_v8_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_0, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_1, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_2, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_3, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_4, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_5, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_6, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_7, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_8, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_9, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_1
static __unused void sse_v8_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_10, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_11, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_Alignment_aligned();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_occur();
	execption_inc_len = 3;
	do_at_ring0(sse_v8_instruction_12, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_v8_instruction_13, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_v8_instruction_14, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_v8_instruction_15, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_v8_instruction_16, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_v8_instruction_17, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_v8_instruction_18, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_v8_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_not_used();
	condition_VEX_not_used();
	condition_CR0_EM_0();
	condition_CR4_OSFXSR_1();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_CR0_AC_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sse_v8_instruction_19, "");
	execption_inc_len = 0;
	report("%s", (exception_vector_bak == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}
//Modified manually: add the conditional compilation for the size limit.
#endif

void v8086_main(void)
{
	//Modified manually: set page present first.
	set_page_present(PAGE_FAULT_ADDR, 0);

	asm volatile("fninit");
	#ifdef PHASE_0
	sse_v8_0();
	sse_v8_1();
	sse_v8_2();
	sse_v8_3();
	sse_v8_4();
	sse_v8_5();
	sse_v8_6();
	sse_v8_7();
	sse_v8_8();
	sse_v8_9();
	//Modified manually: add the conditional compilation for the size limit.
    #endif
    #ifdef PHASE_1
	sse_v8_10();
	sse_v8_11();
	sse_v8_12();
	sse_v8_13();
	sse_v8_14();
	sse_v8_15();
	sse_v8_16();
	sse_v8_17();
	sse_v8_18();
	sse_v8_19();
	#endif

	report_summary();
	send_cmd(FUNC_V8086_EXIT);
}
