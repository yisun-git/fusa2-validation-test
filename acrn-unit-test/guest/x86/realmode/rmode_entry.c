asm(".code16gcc");
#include "rmode_lib.h"
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

static u16 failed;
static u16 tests;

void rmode_exit(int code)
{
	outb(code, 0xf4);
	/*notify BP*/
	ap_end_life = 1;
}

void report(const char *name, _Bool ok)
{
	print_serial(ok ? "PASS: " : "FAIL: ");
	print_serial(name);
	print_serial("\n");
	if (!ok) {
		failed++;
	}
	tests++;
}
void report_summary(void)
{
	print_serial("SUMMERY: ");
	print_serial_u32(tests);
	print_serial("tests");
	if (failed) {
		print_serial(", ");
		print_serial_u32(failed);
		print_serial(" unexpected failures");
		print_serial("\n");
	}
}

__attribute__ ((regparm(1)))
void handle_rmode_excp(struct excp_regs *regs)
{
	struct excp_record *ex;
	u16 vector;
	u16 rflags;

	vector = regs->vector;
	rflags = regs->rflags;
	asm volatile("mov %0, %%gs:4" : : "r"(vector));
	asm volatile("mov %0, %%gs:6" : : "r"(rflags));

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
	print_serial_u32(regs->cx);
	print_serial("\n\rbp:");
	print_serial_u32(regs->bp);
	print_serial("\t si:");
	print_serial_u32(regs->si);
	print_serial("\t di:");
	print_serial_u32(regs->di);
	print_serial("\t rflags:");
	print_serial_u32(regs->rflags);
	print_serial("\n\r");
	rmode_exit(0);
}
u16 exception_vector(void)
{
	u16 vector;
	asm("mov %%gs:4, %0" : "=q"(vector));
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
	".byte 0xcf\n\t"
);

void rmode_start()
{
	main();
	rmode_exit(0);
}

