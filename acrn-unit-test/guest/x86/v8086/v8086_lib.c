#ifndef V8086_LIB
#define V8086_LIB

asm(".code16gcc");

asm(".equ REGSZ,4\n"
	".globl setjmp\n"
"setjmp:\n"
	"mov 0*REGSZ(%esp), %eax\n" // get return EIP
	"mov 1*REGSZ(%esp), %ecx\n" // get jmp_buf
	"mov %eax, 0*REGSZ(%ecx)\n" // save eip
	"mov %esp, 1*REGSZ(%ecx)\n" // save esp
	"mov %ebp, 2*REGSZ(%ecx)\n" // save ebp
	"mov %ebx, 3*REGSZ(%ecx)\n" // save ebx
	"mov %esi, 4*REGSZ(%ecx)\n" // save esi
	"mov %edi, 5*REGSZ(%ecx)\n" // save edi
	"xor %eax, %eax\n"          // clear eax
	"ret\n"
	".globl longjmp\n"
"longjmp:\n"
	"mov 2*REGSZ(%esp), %eax\n" // get return value
	"mov 1*REGSZ(%esp), %ecx\n" // get jmp_buf
	"mov 5*REGSZ(%ecx), %edi\n" // restore edi
	"mov 4*REGSZ(%ecx), %esi\n" // restore esi
	"mov 3*REGSZ(%ecx), %ebx\n" // restore ebx
	"mov 2*REGSZ(%ecx), %ebp\n" // restore ebp
	"mov 1*REGSZ(%ecx), %esp\n" // restore esp
	"mov 0*REGSZ(%ecx), %ecx\n" // restore eip
	"mov %ecx, (%esp)\n"        // store eip on the stack
	"ret\n"
);

#ifndef USE_SERIAL
#define USE_SERIAL
#endif
#define PRINT_NAME printf16
#include "code16_lib.c"

#include "v8086_lib.h"

static u16 failed_cases;
static u16 total_cases;

void inner_report(u16 pass, const char *func_name, const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	printf("%s: ", (pass ? "PASS" : "FAIL"));
	if (func_name) {
		printf("%s()", func_name);
		if (!pass) {
			printf("\n      ");
			vprint(fmt, va);
		}
	} else {
		vprint(fmt, va);
	}
	printf("\n");
	if (!pass)
		failed_cases++;
	total_cases++;
	va_end(va);
}

void report_summary(void)
{
	printf("SUMMARY: %u tests", total_cases);
	if (failed_cases > 0)
		printf(", %u unexpected failures", failed_cases);
	printf("\n");
}

void run_cases(const struct case_func *case_funcs)
{
	for (u16 i = 0; case_funcs[i].rqmid > 0; i++)
		printf("Case ID: %u\n", case_funcs[i].rqmid);
	printf("----\n");

	for (u16 i = 0; case_funcs[i].func != nullptr; i++)
		case_funcs[i].func();
}

void send_cmd(enum func_id fid)
{
	write_func_id(fid);
	asm volatile (
		"movl $1234f, %%gs:"xstr(RET_INS_ADDR)"\n"
		"int $"xstr(V8086_COMM_INT)"\n"
		"1234:\n"
		::: "memory"
	);
}

u16 request_irq(u32 irq, irq_handler_t handler)
{
	struct v8086_irq virq = {
		.irq = irq,
		.handler = (u32)handler
	};

	write_irq(&virq);
	send_cmd(FUNC_REG_INT);
	return (u16)read_output_val();
}

void free_irq(u16 irqidx)
{
	write_input_val(irqidx);
	send_cmd(FUNC_UNREG_INT);
}

u32 read_cr(u32 fid)
{
	send_cmd(fid);
	return read_output_val();
}

void write_cr(u32 fid, u32 val)
{
	write_input_val(val);
	send_cmd(fid);
}

#if 0
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

	if (MAGIC_WORD != result) {
		if (func_name)
			inner_report(0, func_name, "no exception (setjmp result = 0x%x)", result);
		return RET_FAILURE;
	}

	return RET_SUCCESS;
}
#endif
#endif
