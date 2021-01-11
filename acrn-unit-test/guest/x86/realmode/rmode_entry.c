asm(".code16gcc");
#include "rmode_lib.h"
#include "setjmp.h"
#ifndef USE_SERIAL
#define USE_SERIAL
#endif

extern u16 ap_end_life;
extern struct excp_record exception_table_start, exception_table_end;
extern void main();
static const char *exception_mnemonic(int vector)
{
	switch (vector) {
	case 0:
		return "#DE";
	case 1:
		return "#DB";
	case 2:
		return "#NMI";
	case 3:
		return "#BP";
	case 4:
		return "#OF";
	case 5:
		return "#BR";
	case 6:
		return "#UD";
	case 7:
		return "#NM";
	case 8:
		return "#DF";
	case 10:
		return "#TS";
	case 11:
		return "#NP";
	case 12:
		return "#SS";
	case 13:
		return "#GP";
	case 14:
		return "#PF";
	case 16:
		return "#MF";
	case 17:
		return "#AC";
	case 18:
		return "#MC";
	case 19:
		return "#XM";
	default:
		return "#??";
	}
}

static int rmode_strlen(const char *str)
{
	const char *t_str = str;
	int n;

	for (n = 0; *t_str; ++t_str) {
		++n;
	}
	return n;
}

static void outb(u8 data, u16 port)
{
	asm volatile("out %0, %1" : : "a"(data), "d"(port));
}

#ifdef USE_SERIAL
static int serial_iobase = 0x3f8;
static int serial_inited = 0;

static u8 inb(u16 port)
{
	u8 data;
	asm volatile("in %1, %0" : "=a"(data) : "d"(port));
	return data;
}

static void serial_outb(char ch)
{
	u8 lsr;

	do {
		lsr = inb(serial_iobase + 0x05);
	} while (!(lsr & 0x20));

	outb(ch, serial_iobase + 0x00);
}

static void serial_init(void)
{
	u8 lcr;

	/* set DLAB */
	lcr = inb(serial_iobase + 0x03);
	lcr |= 0x80;
	outb(lcr, serial_iobase + 0x03);

	/* set baud rate to 115200 */
	outb(0x01, serial_iobase + 0x00);
	outb(0x00, serial_iobase + 0x01);

	/* clear DLAB */
	lcr = inb(serial_iobase + 0x03);
	lcr &= ~0x80;
	outb(lcr, serial_iobase + 0x03);
}
#endif

void print_serial(const char *buf)
{
	unsigned long len = rmode_strlen(buf);
	unsigned long i;
	if (!serial_inited) {
		serial_init();
		serial_inited = 1;
	}

	for (i = 0; i < len; i++) {
		serial_outb(buf[i]);
	}
}

void print_serial_u32(u32 value)
{
	u32 t_value = value;
	char n[12], *p;
	p = &n[11];
	*p = 0;
	do {
		*--p = '0' + (t_value % 10);
		t_value /= 10;
	} while (t_value > 0);
	print_serial(p);
}

void rmode_exit(int code)
{
	while (1);
	/*notify BP*/
	ap_end_life = 1;
}

static u16 t = 0;
static u16 f = 0;
static u16 x = 0;
static u16 s = 0;

#define puts(x) print_serial(x)

void va_report(const char *msg_fmt,
		bool pass, bool xfail, bool skip, va_list va)
{
	const char *prefix = skip ? "SKIP"
				  : xfail ? (pass ? "XPASS" : "XFAIL")
					  : (pass ? "PASS"  : "FAIL");
	t++;
	printf("%s: ", prefix);
	vprintf(msg_fmt, va);
	puts("\r\n");
	if (skip) {
		s++;
	}
	else if (xfail && !pass) {
		x++;
	}
	else if (xfail || !pass) {
		f++;
	}
}

void report(const char *msg_fmt, bool pass, ...)
{
	va_list va;
	va_start(va, pass);
	va_report(msg_fmt, pass, 0, 0, va);
	va_end(va);
}

int report_summary(void)
{
	short ret = 0;
	printf("SUMMARY: %d tests", t);
	if (f) {
		printf(", %d unexpected failures", f);
	}
	if (x) {
		printf(", %d expected failures", x);
	}
	if (s) {
		printf(", %d skipped", s);
	}
	printf("\r\n");

	if (t == s) {
		/* Blame AUTOTOOLS for using 77 for skipped test and QEMU for
		 * mangling error codes in a way that gets 77 if we ...
		 */
		return 77 >> 1;
	}

	ret = (f > 0) ? 1 : 0;
	return ret;
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
	print_serial("\n\rUnhandled exception:");
	print_serial_u32(vector);
	print_serial("!!! ");
	print_serial(exception_mnemonic(regs->vector));
	print_serial(" at ip: ");
	print_serial_u32(regs->ip);
	print_serial("\n\r");
	print_serial("ax:");
	print_serial_u32(regs->ax);
	print_serial("\t bx:");
	print_serial_u32(regs->bx);
	print_serial("\t cx:");
	print_serial_u32(regs->cx);
	print_serial("\t dx:");
	print_serial_u32(regs->dx);
	print_serial("\n\rbp:");
	print_serial_u32(regs->bp);
	print_serial("\t si:");
	print_serial_u32(regs->si);
	print_serial("\t di:");
	print_serial_u32(regs->di);
	print_serial("\t cs:");
	print_serial_u32(regs->cs);
	print_serial("\t rflags:");
	print_serial_u32(regs->rflags);
	print_serial("\n\r");
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

__attribute__ ((regparm(1)))
void handle_rmode_excp(struct excp_regs *regs)
{
	u16 vector;
	u16 error_code;
	vector = regs->vector;
	error_code = regs->error_code;
	asm volatile("movw %0, %%gs:"xstr(EXCEPTION_VECTOR_ADDR)"" : : "r"(vector));
	asm volatile("movw %0, %%gs:"xstr(EXCEPTION_ECODE_ADDR)"" : : "r"(error_code));
//#define EXCEPTION_DEBUG
#ifndef EXCEPTION_DEBUG
	if ((regs->vector < 32) && (exception_r_handlers[regs->vector])) {
		exception_r_handlers[regs->vector](regs);
		return;
	}
	unhandled_r_exception(regs);
#else
	struct excp_record *ex = NULL;
	for (ex = &exception_table_start; ex != &exception_table_end; ++ex) {
		if (ex->ip == regs->ip) {
			regs->ip = ex->handler;
			return;
		}
	}
	print_serial("\n\rUnhandled exception:");
	print_serial_u32(vector);
	print_serial("!!! ");
	print_serial(exception_mnemonic(regs->vector));
	print_serial(" at ip: ");
	print_serial_u32(regs->ip);
	print_serial("\n\r");
	print_serial("ax:");
	print_serial_u32(regs->ax);
	print_serial("\t bx:");
	print_serial_u32(regs->bx);
	print_serial("\t cx:");
	print_serial_u32(regs->cx);
	print_serial("\t dx:");
	print_serial_u32(regs->dx);
	print_serial("\n\rbp:");
	print_serial_u32(regs->bp);
	print_serial("\t si:");
	print_serial_u32(regs->si);
	print_serial("\t di:");
	print_serial_u32(regs->di);
	print_serial("\t rflags:");
	print_serial_u32(regs->rflags);
	print_serial("\t cs:");
	print_serial_u32(regs->cs);
	print_serial("\n\r");
	rmode_exit(0);
#endif
}


u8 exception_vector(void)
{
	u8 vector;
	asm("movb %%gs:"xstr(EXCEPTION_VECTOR_ADDR)", %0" : "=q"(vector));
	return vector;
}

u16 exception_error_code(void)
{
	u16 err_code;
	asm("mov %%gs:"xstr(EXCEPTION_ECODE_ADDR)", %0" : "=q"(err_code));
	return err_code;
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

extern char bss_start;
extern char edata;
void bss_init(void)
{
	memset(&bss_start, 0, &edata - &bss_start);
}

void rmode_start()
{
	bss_init();
	main();
	rmode_exit(0);
	while (1);
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

