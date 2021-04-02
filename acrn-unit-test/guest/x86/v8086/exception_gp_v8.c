asm(".code16gcc");
#include "v8086_lib.h"
#include "v8_condition.h"
#include "v8_condition.c"

u16 execption_inc_len = 0;

u16 sp = 0;
u16 sp_bak = 0;
void test_pusha_gp(void)
{
	asm volatile("mov %%sp, %0\n" : "=m"(sp) : );
	sp_bak = sp;
	//sp -= 16;
	//sp &= ~0x0F;
	sp = 0x7;
	asm volatile("movw %0, %%sp\n" : : "r"(sp));
	asm volatile(ASM_TRY("1f")
				"pushaw\n"
				"1:\n"
				"movw %0, %%sp\n"
				: : "m"(sp_bak));
}

static jmp_buf jmpbuf;
static __unused void int_4_handler(void)
{
	u8 vector = OF_VECTOR;
	asm("movb %0, "xstr(EXCEPTION_ADDR) : : "r" (vector));
	longjmp(jmpbuf, 1);
}

static __unused void do_int_4(void)
{
	u16 ret = 0;
	u16 irqidx = request_irq(4, int_4_handler);
	if (RET_FAILURE == irqidx) {
		report_ex("request_irq() returns 0x%x", 0, irqidx);
		return;
	}
	ret = setjmp(jmpbuf);
	if (0 == ret) {
		call_int(4);
	} else {
		free_irq(irqidx);
	}
}

static __unused void gp_v8_instruction_0(const char *msg)
{
	u32 a = 0;
	asm volatile(ASM_TRY("1f")
				"mov %0, %%dr0\n"
				"1:\n" : : "r"(a));
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

//Modified manually: reconstruct the case totally.
#define CR4_UMIP (1 << 11)
static __unused void gp_v8_instruction_1(const char *msg)
{
	u16  value = 0;
	u32 cr4 = read_cr4() | CR4_UMIP;
	//Modified manually: cannot set CR4.UMIP to 1 for hypervisor issue, it'll crash.
	//write_cr4(cr4);
	if (cr4 & CR4_UMIP) {
		asm volatile(ASM_TRY("1f")
					"SMSW %0\n" "1:\n"
					: "=m"(value) : : "memory");
		u8 vector = exception_vector();
		report_ex("vector=%d", GP_VECTOR == vector,  vector);
	} else {
		report_ex("cr4=0x%x", 0, cr4);
	}
}

static __unused void gp_v8_instruction_2(const char *msg)
{
	test_pusha_gp();
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

static __unused void gp_v8_instruction_3(const char *msg)
{
	do_int_4();
	report("%s", (exception_vector() == OF_VECTOR), __FUNCTION__);
}

static __unused void gp_v8_0(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_DeReLoadRead_false();
	execption_inc_len = 3;
	do_at_ring0(gp_v8_instruction_0, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_v8_1(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_CR4_UMIP_1();
	do_at_ring0(gp_v8_instruction_1, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_v8_2(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_ESP_SP_hold();
	do_at_ring0(gp_v8_instruction_2, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}

static __unused void gp_v8_3(void)
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	condition_INTO_EXE_true();
	condition_OF_Flag_1();
	do_at_ring0(gp_v8_instruction_3, "");
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
}


void v8086_main(void)
{
	#ifdef PHASE_0
	gp_v8_0();
	gp_v8_1();
	gp_v8_2();
	gp_v8_3();
	#endif

	report_summary();
	send_cmd(FUNC_V8086_EXIT);
}
