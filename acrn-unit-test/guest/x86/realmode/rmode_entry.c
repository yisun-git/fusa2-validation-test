asm(".code16gcc");
#include "rmode_lib.h"
#include "setjmp.h"
#ifndef USE_SERIAL
#define USE_SERIAL
#endif
#include "code16_lib.c"

extern u16 ap_end_life;
extern struct excp_record exception_table_start, exception_table_end;
extern void main(void);

void rmode_exit(int code)
{
	while (1);
	/*notify BP*/
	ap_end_life = 1;
}

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
	if (!pass) {
		failed_cases++;
	}
	total_cases++;
	va_end(va);
}

int report_summary(void)
{
	printf("SUMMARY: %u tests", total_cases);
	if (failed_cases > 0) {
		printf(", %u unexpected failures", failed_cases);
	}
	printf("\n");
	return 0;
}

extern char bss_start;
extern char edata;

void *memset(void *s, int c, u32 n)
{
	u32 i;
	char *a = s;

	for (i = 0; i < n; ++i) {
		a[i] = c;
	}
	return s;
}

void bss_init(void)
{
	memset(&bss_start, 0, &edata - &bss_start);
}

void rmode_start(void)
{
	bss_init();
	main();
	rmode_exit(0);
}

static handler exception_r_handlers[32] = {0};
handler handle_exception(u8 v, handler fn)
{
	handler old;

	old = exception_r_handlers[v];
	if (v < 32) {
		exception_r_handlers[v] = fn;
	}
	return old;
}

void do_r_handle_exception(struct excp_regs *regs)
{
	if ((regs->vector < 32) && (exception_r_handlers[regs->vector])) {
		exception_r_handlers[regs->vector](regs);
		return;
	}
	unhandled_r_exception(regs);
}

void unhandled_r_exception(struct excp_regs *regs)
{
	u16 vector;
	vector = regs->vector;
	printf("\nUnhandled exception:%u at ip 0x%x\n",
		vector, regs->ip);

	printf("ax=0x%x, bx=0x%x, cx=0x%x, dx=0x%x\n"
		"bp=0x%x, si=0x%x, di=0x%x, cs=0x%x, rflags=0x%x\n",
		regs->ax, regs->bx, regs->cx, regs->dx,
		regs->bp, regs->si, regs->di, regs->cs, regs->rflags);

	rmode_exit(0);
}

u16 execption_inc_len = 0;

void common_exception_handler(struct excp_regs *regs)
{
	struct excp_record *ex;
	for (ex = &exception_table_start; ex != &exception_table_end; ++ex) {
		if (ex->ip == regs->ip) {
			regs->ip = ex->handler;
			return;
		}
	}
	if (execption_inc_len == 0) {
		unhandled_r_exception(regs);
	} else {
		regs->ip += execption_inc_len;
	}
}

void set_handle_exception(void)
{
	handle_exception(DE_VECTOR, &common_exception_handler);
	handle_exception(DB_VECTOR, &common_exception_handler);
	handle_exception(NMI_VECTOR, &common_exception_handler);
	handle_exception(BP_VECTOR, &common_exception_handler);
	handle_exception(OF_VECTOR, &common_exception_handler);
	handle_exception(BR_VECTOR, &common_exception_handler);
	handle_exception(UD_VECTOR, &common_exception_handler);
	handle_exception(NM_VECTOR, &common_exception_handler);
	handle_exception(DF_VECTOR, &common_exception_handler);
	handle_exception(TS_VECTOR, &common_exception_handler);
	handle_exception(NP_VECTOR, &common_exception_handler);
	handle_exception(SS_VECTOR, &common_exception_handler);
	handle_exception(GP_VECTOR, &common_exception_handler);
	handle_exception(PF_VECTOR, &common_exception_handler);
	handle_exception(MF_VECTOR, &common_exception_handler);
	handle_exception(AC_VECTOR, &common_exception_handler);
	handle_exception(MC_VECTOR, &common_exception_handler);
	handle_exception(XM_VECTOR, &common_exception_handler);
}

static bool r_exception;
static jmp_buf *r_exception_jmpbuf;

void r_exception_handler_longjmp(void)
{
	longjmp(*r_exception_jmpbuf, 1);
}

static void r_exception_handler(struct excp_regs *regs)
{
	/* longjmp must happen after iret, so do not do it now.  */
	r_exception = true;
	regs->ip = (u16)(u32)&r_exception_handler_longjmp;
	/*read_cs() get Ring0's CS, but the return EIP could be Ring3, so can't modify it.*/
	//regs->cs = read_cs();
}

bool test_for_r_exception(unsigned int ex, void (*trigger_func)(void *data),
	void *data)
{
	handler old;
	jmp_buf jmpbuf;
	int ret;

	old = handle_exception(ex, r_exception_handler);
	ret = set_r_exception_jmpbuf(jmpbuf);
	if (ret == 0) {
		/* set vector to no exception*/
		asm("movl $"xstr(NO_EXCEPTION)", %gs:"xstr(EXCEPTION_ADDR)"");
		trigger_func(data);
	}
	handle_exception(ex, old);
	return ret;
}

void __set_r_exception_jmpbuf(jmp_buf *addr)
{
	r_exception_jmpbuf = addr;
}

void set_idt_entry(int vec, void *addr)
{
	*(u32 *)(vec * 4) = (u32)addr;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	size_t i;
	char *a = dest;
	const char *b = src;

	for (i = 0; i < n; ++i) {
		a[i] = b[i];
	}

	return dest;
}

__attribute__ ((regparm(1)))
void handle_rmode_excp(struct excp_regs *regs)
{
	u16 vector;
	u16 error_code;
	vector = regs->vector;
	error_code = regs->error_code;
	asm volatile("movw %0, %%gs:"xstr(EXCEPTION_VECTOR_ADDR)"" : : "r"(vector));
	asm volatile("movw %0, %%gs:"xstr(EXCEPTION_ECODE_ADDR)"" : : "r"(error_code));

	if ((regs->vector < 32) && (exception_r_handlers[regs->vector])) {
		exception_r_handlers[regs->vector](regs);
		return;
	}
	unhandled_r_exception(regs);
}

u8 exception_vector(void)
{
	u8 vector;
	asm("movb %%gs:"xstr(EXCEPTION_VECTOR_ADDR)", %0" : "=q"(vector));
	return vector;
}

u16 exception_error_code(void)
{
	u16 vector;
	asm("mov %%gs:"xstr(EXCEPTION_ECODE_ADDR)", %0" : "=q"(vector));
	return vector;
}

asm (
	".globl __do_handle_rmode_excp\n\t"
	"__do_handle_rmode_excp:\n\t"
	"push %di; push %si; push %bp;\n\t"
	"push %bx; push %dx; push %cx; push %ax\n\t"
	"mov %sp, %ax\n\t"
	"call handle_rmode_excp \n\t"
	"pop %ax; pop %cx; pop %dx; pop %bx\n\t"
	"pop %bp; pop %si; pop %di\n\t"
	"add $0x2, %sp\n\t"
	"add $0x2, %sp\n\t"
	".byte 0xcf\n\t"
);
