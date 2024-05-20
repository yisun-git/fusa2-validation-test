#define PT_B6
#ifdef PT_B6
#include "processor.h"
#include "condition.h"
#include "condition.c"
#include "libcflat.h"
#include "desc.h"
#include "desc.c"
#include "alloc.h"
#include "alloc.c"
#include "misc.h"
#include "fwcfg.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"
#else
asm(".code16gcc");
#include "rmode_lib.h"
#include "r_condition.h"
#include "r_condition.c"
#endif

__unused __attribute__ ((aligned(64))) static union_unsigned_128 unsigned_128;
__unused __attribute__ ((aligned(64))) static union_unsigned_256 unsigned_256;
__attribute__ ((aligned(64))) __unused static u8 unsigned_8 = 0;
__attribute__ ((aligned(64))) __unused static u16 unsigned_16 = 0;
__attribute__ ((aligned(64))) __unused static u32 unsigned_32 = 0;
__attribute__ ((aligned(64))) __unused static u64 unsigned_64 = 0;
__attribute__ ((aligned(64))) __unused static u64 array_64[4] = {0};
u8 exception_vector_bak = 0xFF;

#ifdef PT_B6
u16 execption_inc_len = 0;
#else
extern u16 execption_inc_len;
#endif

#ifdef PT_B6
void handled_exception(struct ex_regs *regs)
{
	struct ex_record *ex;
	unsigned ex_val;

	ex_val = regs->vector | (regs->error_code << 16) |
			(((regs->rflags >> 16) & 1) << 8);
	asm("mov %0, %%gs:"xstr(EXCEPTION_ADDR)"" : : "r"(ex_val));
	for (ex = &exception_table_start; ex != &exception_table_end; ++ex) {
		if (ex->rip == regs->rip) {
			regs->rip = ex->handler;
			return;
		}
	}
	if (execption_inc_len == 0) {
		unhandled_exception(regs, false);
	} else {
		regs->rip += execption_inc_len;
	}
}

void set_handle_exception(void)
{
	handle_exception(DE_VECTOR, &handled_exception);
	handle_exception(DB_VECTOR, &handled_exception);
	handle_exception(NMI_VECTOR, &handled_exception);
	handle_exception(BP_VECTOR, &handled_exception);
	handle_exception(OF_VECTOR, &handled_exception);
	handle_exception(BR_VECTOR, &handled_exception);
	handle_exception(UD_VECTOR, &handled_exception);
	handle_exception(NM_VECTOR, &handled_exception);
	handle_exception(DF_VECTOR, &handled_exception);
	handle_exception(TS_VECTOR, &handled_exception);
	handle_exception(NP_VECTOR, &handled_exception);
	handle_exception(SS_VECTOR, &handled_exception);
	handle_exception(GP_VECTOR, &handled_exception);
	handle_exception(PF_VECTOR, &handled_exception);
	handle_exception(MF_VECTOR, &handled_exception);
	handle_exception(AC_VECTOR, &handled_exception);
	handle_exception(MC_VECTOR, &handled_exception);
	handle_exception(XM_VECTOR, &handled_exception);
}
#endif

static __unused union_unsigned_128 *unaligned_address_128(void)
{
	static __attribute__ ((aligned(64))) union_unsigned_128 addr_unalign_128;
	addr_unalign_128.m[0] = 1;
	addr_unalign_128.m[1] = 1;
	addr_unalign_128.m[2] = 1;
	addr_unalign_128.m[3] = 1;
	union_unsigned_128 *addr = (union_unsigned_128 *)&addr_unalign_128;
	union_unsigned_128 *non_aligned_addr = (union_unsigned_128 *)((u8 *)addr + 1);
	return non_aligned_addr;
}

static __unused void test_MASKMOVDQU_0_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"MASKMOVDQU %%xmm2, %%xmm1\n" "1:"
				:  : );
}

static __unused void sse_pt_ra_b6_instruction_0(const char *msg)
{
	test_for_exception(UD_VECTOR, test_MASKMOVDQU_0_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_MAXPD_1_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"MAXPD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_1(const char *msg)
{
	test_for_exception(UD_VECTOR, test_MAXPD_1_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_ra_b6_instruction_2(const char *msg)
{
	write_cr0(read_cr0() & ~(X86_CR0_EM));
	write_cr4(read_cr4() | X86_CR4_OSFXSR);
	asm volatile(ASM_TRY("1f") "CVTTPS2DQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void test_CVTPS2DQ_3_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"CVTPS2DQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_3(const char *msg)
{
	test_for_exception(UD_VECTOR, test_CVTPS2DQ_3_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_CVTPD2PS_4_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"CVTPD2PS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_4(const char *msg)
{
	test_for_exception(UD_VECTOR, test_CVTPD2PS_4_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_RCPPS_5_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"RCPPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_5(const char *msg)
{
	test_for_exception(UD_VECTOR, test_RCPPS_5_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_RSQRTPS_6_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"RSQRTPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_6(const char *msg)
{
	test_for_exception(UD_VECTOR, test_RSQRTPS_6_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_ra_b6_instruction_7(const char *msg)
{
	write_cr0(read_cr0() & ~(X86_CR0_EM));
	write_cr4(read_cr4() | X86_CR4_OSFXSR);
	asm volatile(ASM_TRY("1f") "MINPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void test_ORPS_8_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"ORPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_8(const char *msg)
{
	test_for_exception(UD_VECTOR, test_ORPS_8_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_MOVUPS_9_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"MOVUPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_9(const char *msg)
{
	test_for_exception(UD_VECTOR, test_MOVUPS_9_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_PSRLDQ_10_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PSRLDQ $0x1, %%xmm1\n" "1:"
				:  : );
}

static __unused void sse_pt_ra_b6_instruction_10(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PSRLDQ_10_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_MOVDQA_11_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"MOVDQA %%xmm1, %[output]\n" "1:"
				: [output] "=m" (unsigned_128) : );
}

static __unused void sse_pt_ra_b6_instruction_11(const char *msg)
{
	test_for_exception(UD_VECTOR, test_MOVDQA_11_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_UCOMISS_12_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"UCOMISS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_32));
}

static __unused void sse_pt_ra_b6_instruction_12(const char *msg)
{
	test_for_exception(UD_VECTOR, test_UCOMISS_12_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_MOVMSKPS_13_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"MOVMSKPS %%xmm1, %[output]\n" "1:"
				: [output]"=r"(unsigned_32) : );
}

static __unused void sse_pt_ra_b6_instruction_13(const char *msg)
{
	test_for_exception(UD_VECTOR, test_MOVMSKPS_13_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_MOVSLDUP_14_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"MOVSLDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_14(const char *msg)
{
	test_for_exception(UD_VECTOR, test_MOVSLDUP_14_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_MOVSLDUP_15_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"MOVSLDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_15(const char *msg)
{
	test_for_exception(UD_VECTOR, test_MOVSLDUP_15_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_ra_b6_instruction_16(const char *msg)
{
	write_cr0(read_cr0() & ~(X86_CR0_EM));
	write_cr4(read_cr4() | X86_CR4_OSFXSR);
	asm volatile(ASM_TRY("1f") "MOVSLDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void test_HSUBPS_17_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"HSUBPS %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_17(const char *msg)
{
	test_for_exception(UD_VECTOR, test_HSUBPS_17_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_MOVSLDUP_18_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"MOVSLDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_18(const char *msg)
{
	test_for_exception(UD_VECTOR, test_MOVSLDUP_18_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_PBLENDW_19_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PBLENDW $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_19(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PBLENDW_19_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_PCMPEQQ_20_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"PCMPEQQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_20(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PCMPEQQ_20_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_ra_b6_instruction_21(const char *msg)
{
	write_cr0(read_cr0() & ~(X86_CR0_EM));
	write_cr4(read_cr4() | X86_CR4_OSFXSR);
	asm volatile(ASM_TRY("1f") "PBLENDVB %%xmm0, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (*(unaligned_address_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void test_PACKUSDW_22_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PACKUSDW %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_22(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PACKUSDW_22_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_BLENDVPS_23_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"BLENDVPS %%xmm0, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_23(const char *msg)
{
	test_for_exception(UD_VECTOR, test_BLENDVPS_23_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_PMOVZXDQ_24_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PMOVZXDQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
}

static __unused void sse_pt_ra_b6_instruction_24(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PMOVZXDQ_24_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_PMOVZXWD_25_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PMOVZXWD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
}

static __unused void sse_pt_ra_b6_instruction_25(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PMOVZXWD_25_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_MOVDDUP_26_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
}

static __unused void sse_pt_ra_b6_instruction_26(const char *msg)
{
	test_for_exception(UD_VECTOR, test_MOVDDUP_26_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_MOVDDUP_27_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"MOVDDUP %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_64));
}

static __unused void sse_pt_ra_b6_instruction_27(const char *msg)
{
	test_for_exception(UD_VECTOR, test_MOVDDUP_27_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_MOVLHPS_28_nm(void *data)
{
	asm volatile(ASM_TRY("1f")
				"MOVLHPS %%xmm2, %%xmm1\n" "1:"
				:  : );
}

static __unused void sse_pt_ra_b6_instruction_28(const char *msg)
{
	test_for_exception(NM_VECTOR, test_MOVLHPS_28_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_ra_b6_instruction_29(const char *msg)
{
	asm volatile(ASM_TRY("1f") "MOVHLPS %%xmm2, %%xmm1\n" "1:"
				:  : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void test_MOVMSKPD_30_nm(void *data)
{
	asm volatile(ASM_TRY("1f")
				"MOVMSKPD %%xmm1, %[output]\n" "1:"
				: [output]"=r"(unsigned_32) : );
}

static __unused void sse_pt_ra_b6_instruction_30(const char *msg)
{
	test_for_exception(NM_VECTOR, test_MOVMSKPD_30_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_ra_b6_instruction_31(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PSLLDQ $0x1, %%xmm1\n" "1:"
				:  : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void test_PHADDW_32_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PHADDW %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_32(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PHADDW_32_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_PHADDSW_33_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"PHADDSW %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_33(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PHADDSW_33_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_ra_b6_instruction_34(const char *msg)
{
	write_cr0(read_cr0() & ~(X86_CR0_EM));
	write_cr4(read_cr4() | X86_CR4_OSFXSR);
	asm volatile(ASM_TRY("1f") "PHSUBW %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void test_PALIGNR_35_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PALIGNR $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_35(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PALIGNR_35_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_PHADDD_36_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PHADDD %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_36(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PHADDD_36_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_PCMPISTRI_37_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PCMPISTRI $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_37(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PCMPISTRI_37_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_PCMPGTQ_38_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"PCMPGTQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_38(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PCMPGTQ_38_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_ra_b6_instruction_39(const char *msg)
{
	write_cr0(read_cr0() & ~(X86_CR0_EM));
	write_cr4(read_cr4() | X86_CR4_OSFXSR);
	asm volatile(ASM_TRY("1f") "PCMPGTQ %[input_1], %%xmm1\n" "1:"
				:  : [input_1] "m" (*(unaligned_address_128())));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void test_PCMPISTRM_40_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PCMPISTRM $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_40(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PCMPISTRM_40_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_PCMPISTRM_41_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PCMPISTRM $0x1, %[input_2], %%xmm1\n" "1:"
				:  : [input_2] "m" (unsigned_128));
}

static __unused void sse_pt_ra_b6_instruction_41(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PCMPISTRM_41_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_PMOVMSKB_42_mf(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PMOVMSKB %%mm1, %[output]\n" "1:"
				: [output]"=r"(unsigned_32) : );
}

static __unused void sse_pt_ra_b6_instruction_42(const char *msg)
{
	test_for_exception(MF_VECTOR, test_PMOVMSKB_42_mf, NULL);
	report("%s", (exception_vector() == MF_VECTOR), __FUNCTION__);
}

static __unused void test_PMOVMSKB_43_ud(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PMOVMSKB %%mm1, %[output]\n" "1:"
				: [output]"=r"(unsigned_32) : );
}

static __unused void sse_pt_ra_b6_instruction_43(const char *msg)
{
	test_for_exception(UD_VECTOR, test_PMOVMSKB_43_ud, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_PMOVMSKB_44_nm(void *data)
{
	asm volatile(ASM_TRY("1f")
				"PMOVMSKB %%mm1, %[output]\n" "1:"
				: [output]"=r"(unsigned_32) : );
}

static __unused void sse_pt_ra_b6_instruction_44(const char *msg)
{
	test_for_exception(NM_VECTOR, test_PMOVMSKB_44_nm, NULL);
	report("%s", (exception_vector() == NM_VECTOR), __FUNCTION__);
}

static __unused void sse_pt_ra_b6_instruction_45(const char *msg)
{
	asm volatile(ASM_TRY("1f") "PEXTRW $0x1, %%mm1, %[output]\n" "1:"
				: [output]"=r"(unsigned_32) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void sse_pt_ra_b6_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_0();
	do_at_ring0(sse_pt_ra_b6_instruction_0, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE2_1();
	condition_LOCK_used();
	do_at_ring0(sse_pt_ra_b6_instruction_1, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_2(void)
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
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_ra_b6_instruction_2, "");
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

static __unused void sse_pt_ra_b6_3(void)
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
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_ra_b6_instruction_3, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_4(void)
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
	condition_CR4_OSFXSR_0();
	do_at_ring0(sse_pt_ra_b6_instruction_4, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_0();
	do_at_ring0(sse_pt_ra_b6_instruction_5, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE_1();
	condition_LOCK_used();
	do_at_ring0(sse_pt_ra_b6_instruction_6, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_7(void)
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
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_ra_b6_instruction_7, "");
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

static __unused void sse_pt_ra_b6_8(void)
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
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_ra_b6_instruction_8, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_9(void)
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
	condition_CR4_OSFXSR_0();
	do_at_ring0(sse_pt_ra_b6_instruction_9, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_10(void)
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
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_ra_b6_instruction_10, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_11(void)
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
	condition_CR4_OSFXSR_0();
	do_at_ring0(sse_pt_ra_b6_instruction_11, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_12(void)
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
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_ra_b6_instruction_12, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_13(void)
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
	condition_CR4_OSFXSR_0();
	do_at_ring0(sse_pt_ra_b6_instruction_13, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_0();
	do_at_ring0(sse_pt_ra_b6_instruction_14, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE3_1();
	condition_LOCK_used();
	do_at_ring0(sse_pt_ra_b6_instruction_15, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_16(void)
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
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_ra_b6_instruction_16, "");
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

static __unused void sse_pt_ra_b6_17(void)
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
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_ra_b6_instruction_17, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_18(void)
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
	condition_CR4_OSFXSR_0();
	do_at_ring0(sse_pt_ra_b6_instruction_18, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_0();
	do_at_ring0(sse_pt_ra_b6_instruction_19, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_20(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_1_1();
	condition_LOCK_used();
	do_at_ring0(sse_pt_ra_b6_instruction_20, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_21(void)
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
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_ra_b6_instruction_21, "");
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

static __unused void sse_pt_ra_b6_22(void)
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
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_ra_b6_instruction_22, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_23(void)
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
	condition_CR4_OSFXSR_0();
	do_at_ring0(sse_pt_ra_b6_instruction_23, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_24(void)
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
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_ra_b6_instruction_24, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_25(void)
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
	condition_CR4_OSFXSR_0();
	do_at_ring0(sse_pt_ra_b6_instruction_25, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_26(void)
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
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_ra_b6_instruction_26, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_27(void)
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
	condition_CR4_OSFXSR_0();
	do_at_ring0(sse_pt_ra_b6_instruction_27, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_28(void)
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
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_ra_b6_instruction_28, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_29(void)
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
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_ra_b6_instruction_29, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_30(void)
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
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_ra_b6_instruction_30, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_31(void)
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
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_ra_b6_instruction_31, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_32(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_0();
	do_at_ring0(sse_pt_ra_b6_instruction_32, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_33(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSSE3_1();
	condition_LOCK_used();
	do_at_ring0(sse_pt_ra_b6_instruction_33, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_34(void)
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
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_ra_b6_instruction_34, "");
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

static __unused void sse_pt_ra_b6_35(void)
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
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_ra_b6_instruction_35, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_36(void)
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
	condition_CR4_OSFXSR_0();
	do_at_ring0(sse_pt_ra_b6_instruction_36, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_37(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_2_0();
	do_at_ring0(sse_pt_ra_b6_instruction_37, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_38(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_CPUID_SSE4_2_1();
	condition_LOCK_used();
	do_at_ring0(sse_pt_ra_b6_instruction_38, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_39(void)
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
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring0(sse_pt_ra_b6_instruction_39, "");
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

static __unused void sse_pt_ra_b6_40(void)
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
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_ra_b6_instruction_40, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_41(void)
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
	condition_CR4_OSFXSR_0();
	do_at_ring0(sse_pt_ra_b6_instruction_41, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_42(void)
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
	condition_CR0_NE_1();
	condition_FPU_excp_hold();
	do_at_ring0(sse_pt_ra_b6_instruction_42, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_43(void)
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
	condition_FPU_excp_not_hold();
	condition_CR0_EM_1();
	do_at_ring0(sse_pt_ra_b6_instruction_43, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_44(void)
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
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_1();
	do_at_ring0(sse_pt_ra_b6_instruction_44, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}

static __unused void sse_pt_ra_b6_45(void)
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
	condition_FPU_excp_not_hold();
	condition_CR0_EM_0();
	condition_CR0_TS_0();
	do_at_ring0(sse_pt_ra_b6_instruction_45, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	#ifdef __x86_64__
	write_cr8(cr8);
	#endif
	asm volatile("fninit");
}


int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	asm volatile("fninit");
	sse_pt_ra_b6_0();
	sse_pt_ra_b6_1();
	sse_pt_ra_b6_2();
	sse_pt_ra_b6_3();
	sse_pt_ra_b6_4();
	sse_pt_ra_b6_5();
	sse_pt_ra_b6_6();
	sse_pt_ra_b6_7();
	sse_pt_ra_b6_8();
	sse_pt_ra_b6_9();
	sse_pt_ra_b6_10();
	sse_pt_ra_b6_11();
	sse_pt_ra_b6_12();
	sse_pt_ra_b6_13();
	sse_pt_ra_b6_14();
	sse_pt_ra_b6_15();
	sse_pt_ra_b6_16();
	sse_pt_ra_b6_17();
	sse_pt_ra_b6_18();
	sse_pt_ra_b6_19();
	sse_pt_ra_b6_20();
	sse_pt_ra_b6_21();
	sse_pt_ra_b6_22();
	sse_pt_ra_b6_23();
	sse_pt_ra_b6_24();
	sse_pt_ra_b6_25();
	sse_pt_ra_b6_26();
	sse_pt_ra_b6_27();
	sse_pt_ra_b6_28();
	sse_pt_ra_b6_29();
	sse_pt_ra_b6_30();
	sse_pt_ra_b6_31();
	sse_pt_ra_b6_32();
	sse_pt_ra_b6_33();
	sse_pt_ra_b6_34();
	sse_pt_ra_b6_35();
	sse_pt_ra_b6_36();
	sse_pt_ra_b6_37();
	sse_pt_ra_b6_38();
	sse_pt_ra_b6_39();
	sse_pt_ra_b6_40();
	sse_pt_ra_b6_41();
	sse_pt_ra_b6_42();
	sse_pt_ra_b6_43();
	sse_pt_ra_b6_44();
	sse_pt_ra_b6_45();

	return report_summary();
}
