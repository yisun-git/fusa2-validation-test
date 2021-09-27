asm(".code16gcc");
#include "rmode_lib.h"
#include "r_condition.h"
#include "r_condition.c"
__attribute__ ((aligned(64))) __unused static u8 unsigned_8 = 0;
__attribute__ ((aligned(64))) __unused static u16 unsigned_16 = 0;
__attribute__ ((aligned(64))) __unused static u32 unsigned_32 = 0;
__attribute__ ((aligned(64))) __unused static u64 unsigned_64 = 0;
__attribute__ ((aligned(64))) __unused static u64 array_64[4] = {0};
u8 exception_vector_bak = 0xFF;
extern u16 execption_inc_len;

void mpx_precondition(void)
{
}

//Modified manually: add the conditional compilation for the size limit.
#ifdef PHASE_0
static __unused void mpx_ra_instruction_0(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = 0xAUL << 32;
	#else
	bound = 0xAUL << 16;
	#endif
	bound += 0x04UL;
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCL %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_1(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = 0xAUL << 32;
	#else
	bound = 0xAUL << 16;
	#endif
	bound += 0x04UL;
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCL %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_2(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_3(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				#ifdef __x86_64__
				".byte 0xf3, 0x0f, 0x1a, 0x25, 0x6d, 0x6b, 0x06, 0x00\n" "1:"
				#else
				".byte 0xf3, 0x0f, 0x1a, 0x25\n" "1:"
				#endif
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_4(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				#ifdef __x86_64__
				".byte 0xf2, 0x0f, 0x1a, 0x25, 0xd1, 0x6b, 0x06, 0x00\n" "1:"
				#else
				".byte 0xf2, 0x0f, 0x1a, 0x25\n" "1:"
				#endif
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_5(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = 0xAUL << 32;
	#else
	bound = 0xAUL << 16;
	#endif
	bound += 0x04UL;
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCL %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_6(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = 0xAUL << 32;
	#else
	bound = 0xAUL << 16;
	#endif
	bound += 0x04UL;
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCL %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_7(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_8(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_9(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_10(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_11(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				#ifdef __x86_64__
				".byte 0xf2, 0x0f, 0x1a, 0x25, 0xd1, 0x6b, 0x06, 0x00\n" "1:"
				#else
				".byte 0xf2, 0x0f, 0x1a, 0x25\n" "1:"
				#endif
				:  : [input_1] "m" (unsigned_64));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_12(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_13(const char *msg)
{
	u64 bound = 0;
	#ifdef __x86_64__
	bound = (~0xAUL + 1) << 32;
	#else
	bound = (~0xAUL + 1) << 16;
	#endif
	bound += (~0x00UL + 1);
	asm volatile(ASM_TRY("1f")
				"BNDMK %0, %%bnd0" : : "m"(bound));
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDCU %[input_1], %%bnd0\n" "1:"
				:  : [input_1] "m" (a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_14(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"bndmk %0, %%bnd0\n\t"
				"1:"
				: : "m"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_1
static __unused void mpx_ra_instruction_15(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0xf3, 0x0f, 0x1b, 0x65, 0xf8\n\t"
				"1:"
				: : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_16(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"bndmk %0, %%bnd0\n\t"
				"1:"
				: : "m"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_17(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"bndmk %0, %%bnd0\n\t"
				"1:"
				: : "m"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_18(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_19(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_ra_instruction_20(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_21(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"lock\n"
				".byte 0x66,0x0f,0x1b,0xc7 \n\t" "1:" : : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_ra_instruction_22(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_23(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void mpx_ra_instruction_24(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void mpx_ra_instruction_25(const char *msg)
{
	u64 a = 0;
	asm volatile(ASM_TRY("1f")
				"lock\n"
				"BNDMOV %%bnd2, %[output]\n" "1:"
				: [output] "=m" (a) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_0
static __unused void mpx_ra_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Bound_Lower();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_0, "");
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

static __unused void mpx_ra_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Bound_Upper();
	condition_16b_addressing_hold();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_1, "");
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

static __unused void mpx_ra_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_16b_addressing_hold();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_2, "");
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

static __unused void mpx_ra_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Bound_Upper();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_3, "");
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

static __unused void mpx_ra_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_4, "");
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

static __unused void mpx_ra_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Bound_Upper();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_5, "");
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

static __unused void mpx_ra_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Bound_Upper();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_6, "");
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

static __unused void mpx_ra_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_7, "");
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

static __unused void mpx_ra_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_8, "");
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

static __unused void mpx_ra_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Bound_Upper();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_9, "");
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

static __unused void mpx_ra_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Bound_Lower();
	condition_16b_addressing_hold();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_10, "");
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

static __unused void mpx_ra_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Bound_Lower();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_11, "");
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

static __unused void mpx_ra_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Bound_Lower();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_12, "");
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

static __unused void mpx_ra_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_Bound_Lower();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_13, "");
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

static __unused void mpx_ra_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_16b_addressing_hold();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_14, "");
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
static __unused void mpx_ra_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_15, "");
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

static __unused void mpx_ra_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_16, "");
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

static __unused void mpx_ra_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_17, "");
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

static __unused void mpx_ra_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_16b_addressing_hold();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_18, "");
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

static __unused void mpx_ra_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_16b_addressing_hold();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_19, "");
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

static __unused void mpx_ra_20(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_20, "");
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

static __unused void mpx_ra_21(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_21, "");
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

static __unused void mpx_ra_22(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_22, "");
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

static __unused void mpx_ra_23(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_23, "");
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

static __unused void mpx_ra_24(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_0();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_24, "");
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

static __unused void mpx_ra_25(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	#ifdef __x86_64__
	u32 cr8 = read_cr8();
	#endif
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	condition_16b_addressing_not_hold();
	condition_BNDCFGS_EN_1();
	condition_BND4_15_not_used();
	execption_inc_len = 3;
	do_at_ring0(mpx_ra_instruction_25, "");
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

int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	mpx_precondition();
	asm volatile("fninit");
	#ifdef PHASE_0
	mpx_ra_0();
	mpx_ra_1();
	mpx_ra_2();
	mpx_ra_3();
	mpx_ra_4();
	mpx_ra_5();
	mpx_ra_6();
	mpx_ra_7();
	mpx_ra_8();
	mpx_ra_9();
	mpx_ra_10();
	mpx_ra_11();
	mpx_ra_12();
	mpx_ra_13();
	mpx_ra_14();
	#endif
	#ifdef PHASE_1
	mpx_ra_15();
	mpx_ra_16();
	mpx_ra_17();
	mpx_ra_18();
	mpx_ra_19();
	mpx_ra_20();
	mpx_ra_21();
	mpx_ra_22();
	mpx_ra_23();
	mpx_ra_24();
	mpx_ra_25();
	#endif

	return report_summary();
}
