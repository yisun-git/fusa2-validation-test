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
struct addr_m16_16_type addr_m16_16;

//Modified manually: remove 'static'.
unsigned rdmsr_checking(u32 index, u64 *val)
{
	u32 eax = 0;
	u32 edx = 0;
	asm volatile(ASM_TRY("1f")
				"rdmsr \n\t"
				"1:"
				: "=a"(eax), "=d"(edx) : "c"(index));
	if (val) {
		*val = eax + ((u64)edx << 32);
	}
	return exception_vector();
}

//Modified manually: remove 'static'.
unsigned wrmsr_checking(u32 index, u64 val)
{
	u32 a = val, d = val >> 32;
	asm volatile (ASM_TRY("1f")
				"wrmsr\n\t"
				"1:"
				:
				: "a"(a), "d"(d), "c"(index)
				: "memory");
	return exception_vector();
}

void called_func(void)
{
	return;
}

void test_div_Quot_Large_DE(void *data)
{
	u8 a = 1;
	//Modified manually: ASM_TRY("1f")
	asm volatile(
				"mov $0xFFFF, %%ax\n"
				"div %0\n"
				: : "r"(a));
}

void test_exeed_64K_hold_jbe(void *data)
{
	u8 a = 0;
	asm volatile(ASM_TRY("1f")
				"mov $0xFF, %%ax\n"
				"cmp %0, %%ax\n"
				"jbe %%cs:0xFFFF\n"
				"1:\n"
				: : "m"(a));
}

void test_bound_ud_001(void *data)
{
	int t[2] = {0, 10};
	u32 r_a = 0xff;
	asm volatile (".byte 0x62, 0xD0\n"
				: [add]"=m"(t[0])
				: "r"(r_a)
				: "memory");
}

u16 sp = 0;
u16 sp_bak = 0;
void test_pusha_gp(void)
{
	asm volatile("mov %%sp, %0\n" : "=m"(sp) : );
	sp_bak = sp;
	//sp -= 16;
	//sp &= ~0x0F;
	//Modified manually: there is no way to restore sp, if it's changed.
	//sp = 0x7;
	asm volatile("movw %0, %%sp\n" : : "r"(sp));
	asm volatile(ASM_TRY("1f")
				"pushaw\n"
				"1:\n"
				"movw %0, %%sp\n"
				: : "m"(sp_bak));
}

//Modified manually: add the conditional compilation for the size limit.
#ifdef PHASE_0
static __unused void gp_ra_instruction_0(const char *msg)
{
	u32 check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_12));
	asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:\n" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_1(const char *msg)
{
	u32 check_bit = 0;
	check_bit = read_cr4();
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_15));
	asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:\n" : : "r"(check_bit));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_2(const char *msg)
{
	u32 r_b = 1;
	asm volatile (ASM_TRY("1f") "mov %0, %%cr5\n" "1:\n" : : "r" (r_b));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_3(const char *msg)
{
	u32 cr0 = read_cr0();
	//VM will exit,bug, refer to ACCB-81
	//cr0 |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_29));
	asm volatile (ASM_TRY("1f") "mov %0, %%cr0\n" "1:\n" : : "r"(cr0) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_4(const char *msg)
{
	u32 check_bit = 0;
	check_bit = read_cr4();
	asm volatile(ASM_TRY("1f") "mov %0, %%cr4\n" "1:\n" : : "r"(check_bit));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_5(const char *msg)
{
	u32 r_a = 1;
	u32 check_bit = 0;
	asm volatile(ASM_TRY("1f")
				"mov %%dr7, %0\n" "1:\n" : "=r"(check_bit) : : );
	check_bit |= (FEATURE_INFORMATION_BIT(FEATURE_INFORMATION_13));
	asm volatile(ASM_TRY("1f")
				"mov %0, %%dr7\n" "1:\n" :  : "r"(check_bit) : );
	asm volatile(ASM_TRY("1f")
				"mov %0, %%dr6\n" "1:\n" : : "r"(r_a) : );
	report("%s", (exception_vector() == DB_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_6(const char *msg)
{
	u32 a = 0;
	asm volatile (ASM_TRY("1f") "lock adc %1, %0\n" "1:\n" : "=m"(a) : "i" (0x1));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_7(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xC7, 0xC0 \n" "1:\n"
				: [output] "=m" (unsigned_64) : );
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_8(const char *msg)
{
	u64 m = 0;
	asm volatile(ASM_TRY("1f") "CMPXCHG8B %0 \n" "1:\n" : "=m"(m) : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_9(const char *msg)
{
	test_pusha_gp();
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_10(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"pushaw \n"
				"popaw \n"
				"1:\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_11(const char *msg)
{
	asm volatile(ASM_TRY("1f")
				"cmp $0x1, %%ax\n"
				"1:\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_12(const char *msg)
{
	u8 a = 0;
	asm volatile(ASM_TRY("1f")
				"idiv %0\n"
				"1:\n"
				: : "r"(a));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_13(const char *msg)
{
	test_for_r_exception(DE_VECTOR, test_div_Quot_Large_DE, NULL);
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_14(const char *msg)
{
	u8 a = 1;
	asm volatile(ASM_TRY("1f")
				"div %0\n"
				"1:\n"
				: : "r"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_15(const char *msg)
{
	u8 a = 0;
	asm volatile(ASM_TRY("1f")
				"idiv %0\n"
				"1:\n"
				: : "r"(a));
	report("%s", (exception_vector() == DE_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_16(const char *msg)
{
	u8 a = 1;
	asm volatile(ASM_TRY("1f")
				"idiv %0\n"
				"1:\n"
				: : "r"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_17(const char *msg)
{
	test_for_r_exception(GP_VECTOR, test_exeed_64K_hold_jbe, NULL);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_18(const char *msg)
{
	u8 a = 1;
	asm volatile(ASM_TRY("1f")
				"cmp %0, %%ax\n"
				"jb 1f\n"
				"1:\n"
				: : "m"(a));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_19(const char *msg)
{
	set_idt_limit(32);
	set_idt_entry(0x40, &called_func);
	asm volatile(ASM_TRY("1f")
				"int $0x40\n"
				"1:\n" : : );
	set_idt_limit(256);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_20(const char *msg)
{
	set_idt_entry(0x40, &called_func);
	asm volatile(ASM_TRY("1f")
				"int $0x40\n"
				"1:\n" : : );
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_21(const char *msg)
{
	unsigned_32 = 0;
	short r_a = 0xFF;
	asm volatile (ASM_TRY("1f") "bound %1, %[add]\n" "1:\n"
				: [add]"=m"(unsigned_32)
				: "r"(r_a)
				: "memory");
	report("%s", (exception_vector() == BR_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_22(const char *msg)
{
	test_for_r_exception(UD_VECTOR, test_bound_ud_001, NULL);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_23(const char *msg)
{
	unsigned_32 = 0;
	short r_a = 0x0;
	asm volatile (ASM_TRY("1f") "bound %1, %[add]\n" "1:\n"
				: [add]"=m"(unsigned_32)
				: "r"(r_a)
				: "memory");
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_24(const char *msg)
{
	asm volatile(ASM_TRY("1f") ".byte 0x0F, 0xB2, 0xD0 \n" "1:\n"
				: : "m" (addr_m16_16));
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_1
static __unused void gp_ra_instruction_25(const char *msg)
{
	addr_m16_16.selector = 0;
	addr_m16_16.offset = (u16)(u32)called_func;
	asm volatile(ASM_TRY("1f") "lss %0, %%ax \n" "1:\n" : : "m" (addr_m16_16));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_26(const char *msg)
{
	//u64 result = 0;//ACCB-81
	//rdmsr_checking(0xFFFF, &result);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_27(const char *msg)
{
	//u64 result = 0;//ACCB-81
	//rdmsr_checking(0xFFFF, &result);
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_28(const char *msg)
{
	u64 result = 0;
	rdmsr_checking(0x17, &result);
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_29(const char *msg)
{
	//wrmsr_checking(0xFFFF, 0x11);//ACCB-81
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_30(const char *msg)
{
	//wrmsr_checking(0xFFFF, 0x11);//ACCB-81
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_31(const char *msg)
{
	//wrmsr_checking(0xFFFF, 0x11);//ACCB-81
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_32(const char *msg)
{
	u64 result = 0;
	rdmsr_checking(0x17, &result);
	wrmsr_checking(0x17, (result | 0x1));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_33(const char *msg)
{
	u64 result = 0;
	rdmsr_checking(0x10, &result);
	wrmsr_checking(0x10, (result));
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}

static __unused void gp_ra_instruction_34(const char *msg)
{
	u16 a = 0x1100;
	asm volatile(ASM_TRY("1f")
				"tzcnt %%ds:0xFFFF, %%ax\n"
				"1:\n"
				: : "m"(a) : "memory");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_ra_instruction_35(const char *msg)
{
	u16 a = 0x1100;
	asm volatile(ASM_TRY("1f")
				"tzcnt %0, %%ax\n"
				"1:\n"
				: : "m"(a) : "memory");
	report("%s", (exception_vector() == NO_EXCEPTION), __FUNCTION__);
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_0
static __unused void gp_ra_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_1();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_0, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_1();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_1, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_AccessCR1567_true();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_2, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_true();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_3, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_4(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_CR4_R_W_0();
	condition_CR4_PCIDE_0();
	condition_AccessCR1567_false();
	condition_WCR0Invalid_false();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_4, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_5(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_DeAc_true();
	condition_DR7_GD_1();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_5, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_6(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_used();
	condition_Mem_Dest_hold();
	condition_D_segfault_not_occur();
	do_at_ring0(gp_ra_instruction_6, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_7(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	condition_MemLocat_false();
	do_at_ring0(gp_ra_instruction_7, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_8(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	condition_MemLocat_true();
	do_at_ring0(gp_ra_instruction_8, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_9(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_ESP_SP_hold();
	do_at_ring0(gp_ra_instruction_9, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_10(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_ESP_SP_not_hold();
	do_at_ring0(gp_ra_instruction_10, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_11(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	do_at_ring0(gp_ra_instruction_11, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_12(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_Divisor_0_true();
	do_at_ring0(gp_ra_instruction_12, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_13(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_Divisor_0_false();
	condition_Quot_large_true();
	do_at_ring0(gp_ra_instruction_13, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_14(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_Divisor_0_false();
	condition_Quot_large_false();
	do_at_ring0(gp_ra_instruction_14, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_15(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_true();
	do_at_ring0(gp_ra_instruction_15, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_16(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_Divisor_0_false();
	condition_Sign_Quot_Large_false();
	do_at_ring0(gp_ra_instruction_16, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_17(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_exceed_64K_hold();
	do_at_ring0(gp_ra_instruction_17, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_18(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_exceed_64K_not_hold();
	do_at_ring0(gp_ra_instruction_18, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_19(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_NumOutIDT_true();
	do_at_ring0(gp_ra_instruction_19, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_20(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_NumOutIDT_false();
	do_at_ring0(gp_ra_instruction_20, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_21(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_Bound_test_fail();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_21, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_22(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_22, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_23(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_Bound_test_pass();
	condition_Sec_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_23, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_24(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_Sou_Not_mem_location_true();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_24, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}
//Modified manually: add the conditional compilation for the size limit.
#endif

#ifdef PHASE_1
static __unused void gp_ra_25(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_D_segfault_not_occur();
	condition_Sou_Not_mem_location_false();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_25, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_26(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_ECX_MSR_Reserved_true();
	do_at_ring0(gp_ra_instruction_26, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_27(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_ECX_MSR_Reserved_false();
	condition_ECX_MSR_Unimplement_true();
	do_at_ring0(gp_ra_instruction_27, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_28(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_ECX_MSR_Reserved_false();
	condition_ECX_MSR_Unimplement_false();
	do_at_ring0(gp_ra_instruction_28, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_29(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_register_nc_hold();
	do_at_ring0(gp_ra_instruction_29, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_30(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_register_nc_not_hold();
	condition_ECX_MSR_Reserved_true();
	do_at_ring0(gp_ra_instruction_30, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_31(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_register_nc_not_hold();
	condition_ECX_MSR_Reserved_false();
	condition_ECX_MSR_Unimplement_true();
	do_at_ring0(gp_ra_instruction_31, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_32(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_register_nc_not_hold();
	condition_ECX_MSR_Reserved_false();
	condition_ECX_MSR_Unimplement_false();
	condition_EDX_EAX_set_reserved_true();
	do_at_ring0(gp_ra_instruction_32, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_33(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_LOCK_not_used();
	condition_register_nc_not_hold();
	condition_ECX_MSR_Reserved_false();
	condition_ECX_MSR_Unimplement_false();
	condition_EDX_EAX_set_reserved_false();
	do_at_ring0(gp_ra_instruction_33, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_34(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	condition_exceed_64K_hold();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_34, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_ra_35(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_D_segfault_not_occur();
	condition_exceed_64K_not_hold();
	execption_inc_len = 3;
	do_at_ring0(gp_ra_instruction_35, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}
//Modified manually: add the conditional compilation for the size limit.
#endif

int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	#ifdef PHASE_0
	gp_ra_0();
	gp_ra_1();
	gp_ra_2();
	gp_ra_3();
	gp_ra_4();
	gp_ra_5();
	gp_ra_6();
	gp_ra_7();
	gp_ra_8();
	gp_ra_9();
	gp_ra_10();
	gp_ra_11();
	gp_ra_12();
	gp_ra_13();
	gp_ra_14();
	gp_ra_15();
	gp_ra_16();
	gp_ra_17();
	gp_ra_18();
	gp_ra_19();
	gp_ra_20();
	gp_ra_21();
	gp_ra_22();
	gp_ra_23();
	gp_ra_24();
	#endif
	#ifdef PHASE_1
	gp_ra_25();
	gp_ra_26();
	gp_ra_27();
	gp_ra_28();
	gp_ra_29();
	gp_ra_30();
	gp_ra_31();
	gp_ra_32();
	gp_ra_33();
	gp_ra_34();
	gp_ra_35();
	#endif

	return report_summary();
}
