asm(".code16gcc");
#include "v8086_lib.h"

#define MAGIC_DWORD 0xfeedbabe
#define MAGIC_WORD 0xcafe

#define TRY_SEGMENT(seg) do {\
	u8 vecter = NO_EXCEPTION;\
	u16 ecode = 0;\
	asm volatile(\
		"movl %ebx, %ecx\n"\
		"movl $0x10001, %ebx\n"\
		ASM_TRY("1f")\
		"movw %" #seg ":(%ebx), %ax\n"\
		"1:"\
		"movl %ecx, %ebx\n"\
		);\
	vecter = exception_vector();\
	ecode = exception_error_code();\
	report_ex(#seg "vector=%u, ecode=0x%x",\
		((GP_VECTOR == vecter) && (0 == ecode)), vecter, ecode);\
} while (0)

#define REQUEST_IRQ_EX(vec, handler, ecode) \
	if (RET_SUCCESS == request_irq_ex((vec), (handler), 0, __func__)) {\
		u8 vecter = exception_vector();\
		u16 err_code = exception_error_code();\
		report_ex("vector=%u, ecode=0x%x",\
			(((vec) == vecter) && ((ecode) == err_code)), vecter, err_code);\
	}

jmp_buf jmpbuf;
__noinline void irq_handler(void)
{
	longjmp(jmpbuf, MAGIC_WORD);
}

u16 request_irq_ex(u32 irq, irq_handler_ex_t handler,
	void *data, const char *func_name)
{
	u16 irqidx = request_irq(irq, &irq_handler);

	if (RET_FAILURE == irqidx) {
		if (func_name)
			inner_report(0, func_name, "request_irq() returns 0x%x", irqidx);
		return RET_FAILURE;
	}
	write_v8086_esp(); /* restore esp for longjmp */
	int result = setjmp(jmpbuf);

	if (0 == result) {
		asm("movl $"xstr(NO_EXCEPTION)", %gs:"xstr(EXCEPTION_ADDR));
		handler(data);
	}
	clear_v8086_esp();
	free_irq(irqidx);
	//printf("%s() = 0x%x\n", __func__, result);
	if (MAGIC_WORD != result) {
		if (func_name)
			inner_report(0, func_name, "no exception (setjmp result = 0x%x)", result);
		return RET_FAILURE;
	}

	return RET_SUCCESS;
}

/**
 * Table32 - row 1
 * In v8086 or real mode, move a memory operand effective address, which is outside the range 0
 * through 0xFFFF, to DS segment registers, will generate an #GP.
 */
__noinline void v8086_rqmid_46053_segmentation_table32_001(void)
{
	TRY_SEGMENT(ds);
}

/**
 * Table32 - row 1
 * In v8086 or real mode, move a memory operand effective address, which is outside the range 0
 * through 0xFFFF, to ES segment registers, will generate an #GP.
 */
__noinline void v8086_rqmid_46054_segmentation_table32_002()
{
	TRY_SEGMENT(es);
}

/**
 * Table32 - row 1
 * In v8086 or real mode, move a memory operand effective address, which is outside the range 0
 * through 0xFFFF, to FS segment registers, will generate an #GP.
 */
__noinline void v8086_rqmid_46055_segmentation_table32_003()
{

	TRY_SEGMENT(fs);
}

/**
 * Table32 - row 1
 * In v8086 or real mode, move a memory operand effective address, which is outside the range 0
 * through 0xFFFF, to GS segment registers, will generate an #GP.
 */
__noinline void v8086_rqmid_46056_segmentation_table32_004()
{
	TRY_SEGMENT(gs);
}

__noinline u16 func_w_switch_stack(void)
{
	return 0xbabe;
}

char small_stack[2] __attribute__((aligned(4)));
__noinline void no_room_call_wss_handler(void *data)
{
	u32 stack_top = ((u32)small_stack + sizeof(small_stack));
	u16 ss = stack_top >> 4;
	u16 sp = stack_top & 0xf;

	asm volatile (
		"mov %0, %%ss\n"
		"mov %1, %%sp\n"
		"call func_w_switch_stack\n"
		:
		: "rm"(ss), "rm"(sp)
		: "memory"
	);
}

/**
 * Table33 - row 1
 * In v8086 mode, use a stack does not have room for the return address, parameters, or stack segment pointer,
 * which is outside the range 0 through 0xFFFF, will generate an #SS.
 */
__noinline void v8086_rqmid_46066_segmentation_table33_001(void)
{
	REQUEST_IRQ_EX(SS_VECTOR, no_room_call_wss_handler, 0);
}

__noinline void beyond_ss_limit_call_wss_handler(void *data)
{
	asm volatile (
		"mov 0x5a, %%ss\n"
		"movl $0xfffff, %%esp\n"
		"call func_w_switch_stack\n"
		: : : "memory"
	);
}

/**
 * Table33- row 3
 * In v8086 mode, use a stack with a memory operand effective address beyond segment limit,
 * will generate an #SS.
 */
__noinline void v8086_rqmid_46077_segmentation_table33_003(void)
{
	REQUEST_IRQ_EX(SS_VECTOR, beyond_ss_limit_call_wss_handler, 0);
}

__noinline void outside_64k_handler(void *data)
{
	asm volatile (
		"movl $0xffffe, %%ebx\n"
		"movl $0xdead, (%%ebx)\n"
		: :	: "memory"
	);
}


/**
 * Table34 - row 1
 * In v8086 or real mode, move data to a memory operand effective address,
 * which is outside the range 0 through 0xFFFF, will generate an #GP.
 */
__noinline void v8086_rqmid_46061_segmentation_table34_001(void)
{
	REQUEST_IRQ_EX(GP_VECTOR, outside_64k_handler, 0);
}

__noinline void beyond_cs_limit_handler(void *data)
{
	struct far_pointer32 fptr = {
		.offset = 0x10001,
		.selector = 0
	};

	asm volatile (
		"lcall *%0\n"
		:
		: "m"(fptr)
		: "memory"
	);
}

/*
 * Table34 - row 2
 * In v8086 or real mode, move data to a memory operand effective address,
 * which is outside the range 0 through 0xFFFF, will generate an #GP.
 */
__noinline void v8086_rqmid_46065_segmentation_table34_002(void)
{
	REQUEST_IRQ_EX(GP_VECTOR, beyond_cs_limit_handler, 0);
}

void v8086_main(void)
{
	printf("## v8086_main() ##\n");
	v8086_rqmid_46053_segmentation_table32_001();
	v8086_rqmid_46054_segmentation_table32_002();
	v8086_rqmid_46055_segmentation_table32_003();
	v8086_rqmid_46056_segmentation_table32_004();
	v8086_rqmid_46066_segmentation_table33_001();
	v8086_rqmid_46077_segmentation_table33_003();
	v8086_rqmid_46061_segmentation_table34_001();
	v8086_rqmid_46065_segmentation_table34_002();
	report_summary();
	send_cmd(FUNC_V8086_EXIT);
}

