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
#include "misc.c"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"
#include "segmentation.h"
#include "interrupt.h"
#include "delay.h"
#include "isr.h"
#else
asm(".code16gcc");
#include "rmode_lib.h"
#include "r_condition.h"
#include "r_condition.c"
#endif
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

#ifdef PT_B6
#ifdef __x86_64__
static __unused u64 *non_canon_align_mem(u64 addr)
{
	u64 address = 0;
	if ((addr >> 63) & 1) {
		address = (addr & (~(1ull << 63)));
	} else {
		address = (addr | (1UL<<63));
	}
	return (u64 *)address;
}
#else
#undef u64
#define u64 size_t
#define non_canon_align_mem(x) ((u32 *)(size_t)x)
#endif
#else
#undef u64
#define u64 size_t
#define non_canon_align_mem(x) ((u16 *)(size_t)x)
#endif

static __unused void test_SUB_instruction_3(void *data)
{
	asm volatile(ASM_TRY("1f")
		"lock\n"
	#ifdef PT_B6
	#ifdef __x86_64__
		"SUB $1, %[output]\n"
	#else
		"SUB $1, %%ds:0xFFFF0000\n"
	#endif
	#else
		"SUB $1, %%ds:0xFFFF\n"
	#endif
		"1:\n" : [output] "=a" (*(non_canon_align_mem((u64)&unsigned_8))) : );
}


static __unused void gp_pt_ra_b6_instruction_0(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BT $1, %[output]\n" "1:\n"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_gp_pt_ra_b6_instruction_1(void *data)
{
	asm volatile(ASM_TRY("1f")
	#ifdef PT_B6
	#ifdef __x86_64__
		"CMOVNA %[input_1], %[output]\n"
	#else
		"CMOVNA %%ds:0xFFFF0000, %[output]\n"
	#endif
	#else
		"CMOVNA %%ds:0xFFFF, %[output]\n"
	#endif
		"1:\n" : [output] "=r" (*(non_canon_align_mem((u64)&unsigned_16)))
		: [input_1] "r" (*(non_canon_align_mem((u64)&unsigned_16))));
}

static __unused void gp_pt_ra_b6_instruction_1(const char *msg)
{
	test_for_exception(GP_VECTOR, test_gp_pt_ra_b6_instruction_1, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_2(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BTS $1, %[output]\n" "1:\n"
				: [output] "=r" (unsigned_16) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void test_gp_pt_ra_b6_instruction_3(void *data)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
	#ifdef PT_B6
	#ifdef __x86_64__
				"SUB $1, %[output]\n"
	#else
				"SUB %%ds:0xFFFF0000, %[output]\n"
	#endif
	#else
				"SUB %%ds:0xFFFF, %[output]\n"
	#endif
				"1:\n" : [output] "=a" (*(non_canon_align_mem((u64)&unsigned_8))) : );
}

static __unused void gp_pt_ra_b6_instruction_3(const char *msg)
{
	test_for_exception(GP_VECTOR, test_SUB_instruction_3, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"CLD \n"
				"1:\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void test_gp_pt_ra_b6_instruction_5(void *data)
{
	asm volatile(ASM_TRY("1f")
	#ifdef PT_B6
	#ifdef __x86_64__
				"CMP $1, %[output]\n"
	#else
				"CMP %%ds:0xFFFF0000, %[output]\n"
	#endif
	#else
				"CMP %%ds:0xFFFF, %[output]\n"
	#endif
				"1:\n" : [output] "=a" (*(non_canon_align_mem((u64)&unsigned_8))) : );
}

static __unused void gp_pt_ra_b6_instruction_5(const char *msg)
{
	test_for_exception(GP_VECTOR, test_gp_pt_ra_b6_instruction_5, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_6(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"ADC $1, %[output]\n" "1:\n"
				: [output] "=a" (unsigned_8) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_7(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x8d, 0xd0\n"
				"1:\n" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_8(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"LEA %[input_1], %[output]\n" "1:\n"
				: [output] "=r" (unsigned_16) : [input_1] "m" (unsigned_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_9(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"RDRAND %[output]\n" "1:\n"
				: [output] "=r" (unsigned_16) :  );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_10(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"RDRAND %[output]\n" "1:\n"
				: [output] "=r" (unsigned_16) :  );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_11(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0xF2 \n"
				"RDRAND %[output]\n" "1:\n"
				: [output] "=r" (unsigned_16) :  );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_12(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"RDRAND %[output]\n" "1:\n"
				: [output] "=r" (unsigned_16) :  );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_13(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"RDSEED %[output]\n" "1:\n"
				: [output] "=r" (unsigned_16) :  );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_14(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"RDSEED %[output]\n" "1:\n"
				: [output] "=r" (unsigned_16) :  );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_15(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0xF2 \n"
				"RDSEED %[output]\n" "1:\n"
				: [output] "=r" (unsigned_16) :  );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_16(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"RDSEED %[output]\n" "1:\n"
				: [output] "=r" (unsigned_16) :  );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_17(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"UD1 %[input_1], %[output]\n" "1:\n"
				: [output] "=r" (unsigned_32) : [input_1] "m" (unsigned_32));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_instruction_18(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"nop \n"
				"1:\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_pt_ra_b6_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_used();
	do_at_ring0(gp_pt_ra_b6_instruction_0, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_occur();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_ra_b6_instruction_1, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_used();
	condition_Mem_Dest_not_hold();
	do_at_ring0(gp_pt_ra_b6_instruction_2, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_D_segfault_occur();
	do_at_ring0(gp_pt_ra_b6_instruction_3, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	do_at_ring0(gp_pt_ra_b6_instruction_4, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_occur();
	do_at_ring0(gp_pt_ra_b6_instruction_5, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	do_at_ring0(gp_pt_ra_b6_instruction_6, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_ra_b6_instruction_7, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_ra_b6_instruction_8, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_RDRAND_0();
	do_at_ring0(gp_pt_ra_b6_instruction_9, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_RDRAND_1();
	condition_LOCK_used();
	do_at_ring0(gp_pt_ra_b6_instruction_10, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_RDRAND_1();
	condition_LOCK_not_used();
	condition_F2HorF3H_hold();
	do_at_ring0(gp_pt_ra_b6_instruction_11, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_RDRAND_1();
	condition_LOCK_not_used();
	condition_F2HorF3H_not_hold();
	do_at_ring0(gp_pt_ra_b6_instruction_12, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_RDSEED_0();
	do_at_ring0(gp_pt_ra_b6_instruction_13, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_RDSEED_1();
	condition_LOCK_used();
	do_at_ring0(gp_pt_ra_b6_instruction_14, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_RDSEED_1();
	condition_LOCK_not_used();
	condition_F2HorF3H_hold();
	do_at_ring0(gp_pt_ra_b6_instruction_15, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CPUID_RDSEED_1();
	condition_LOCK_not_used();
	condition_F2HorF3H_not_hold();
	do_at_ring0(gp_pt_ra_b6_instruction_16, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_OpcodeExcp_true();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_ra_b6_instruction_17, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_pt_ra_b6_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_OpcodeExcp_false();
	execption_inc_len = 3;
	do_at_ring0(gp_pt_ra_b6_instruction_18, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}


int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	gp_pt_ra_b6_0();
	gp_pt_ra_b6_1();
	gp_pt_ra_b6_2();
	gp_pt_ra_b6_3();
	gp_pt_ra_b6_4();
	gp_pt_ra_b6_5();
	gp_pt_ra_b6_6();
	gp_pt_ra_b6_7();
	gp_pt_ra_b6_8();
	gp_pt_ra_b6_9();
	gp_pt_ra_b6_10();
	gp_pt_ra_b6_11();
	gp_pt_ra_b6_12();
	gp_pt_ra_b6_13();
	gp_pt_ra_b6_14();
	gp_pt_ra_b6_15();
	gp_pt_ra_b6_16();
	gp_pt_ra_b6_17();
	gp_pt_ra_b6_18();

	return report_summary();
}