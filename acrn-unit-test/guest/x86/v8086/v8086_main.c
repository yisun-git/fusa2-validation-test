asm(".code16gcc");

#include "v8086_lib.h"

#define TRY_SEGMENT(seg) do {\
	u8 vecter = NO_EXCEPTION;\
	asm volatile(\
		"movw %bx, %cx\n"\
		"movl $0x10001, %ebx\n"\
		ASM_TRY("1f")\
		"movw %" #seg ":(%ebx), %ax\n"\
		"1:"\
		"movw %cx, %bx\n"\
		);\
	vecter = exception_vector();\
	report_ex(#seg ":vector=%u", GP_VECTOR == vecter, vecter);\
} while (0)

__noinline void v8086_rqmid_40491_check_offset_out_of_range(void)
{
	TRY_SEGMENT(fs);
}

jmp_buf jmpbuf;
void int_demo_handler(void)
{
	longjmp(jmpbuf, 1);
}

__noinline void int_demo(void)
{
	u16 ret = 0;
	u16 irqidx = 0;

	printf("cr0 = 0x%x\n", read_cr0());
	write_cr0(0xc0000011);
	printf("cr0 = 0x%x\n", read_cr0());

	irqidx = request_irq(25, int_demo_handler);
	if (RET_FAILURE == irqidx) {
		report_ex("request_irq() returns 0x%x", 0, irqidx);
		return;
	}
	ret = setjmp(jmpbuf);
	if (0 == ret) {
		call_int(25);
	} else {
		free_irq(irqidx);
		report("%s()", ret == 1, __func__);
	}
}

const struct case_func v8086_cases[] = {
	{ 40491, v8086_rqmid_40491_check_offset_out_of_range },
	{ 12345, int_demo },
	{ 0, nullptr }
};

void  v8086_main(void)
{
	printf("## v8086_main() ##\n");
	run_cases(v8086_cases);
	report_summary();
	send_cmd(FUNC_V8086_EXIT);
}

