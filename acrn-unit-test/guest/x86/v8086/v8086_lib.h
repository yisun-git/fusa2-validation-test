#ifndef V8086_LIB_H
#define V8086_LIB_H

#include "v8086_util.h"

#define V8086_ADDR_MASK 0xfffff
#define printf printf16

/* Just for compatibility */
#define report(fmt, pass, ...) \
	inner_report((pass), nullptr, fmt, ##__VA_ARGS__)

/* Recommend this new format report, print the reason if it fails */
#define report_ex(fmt, pass, ...) \
	inner_report((pass), __func__, fmt, ##__VA_ARGS__)

#define ASM_TRY(catch) \
	"movb $"xstr(NO_EXCEPTION) ", " xstr(EXCEPTION_ADDR)"\n" \
	".pushsection .data.ex\n" \
	".long 1111f, " catch "\n" \
	".popsection\n" \
	"1111:"

typedef void (*irq_handler_t)(void);
typedef void (*irq_handler_ex_t)(void *data);

typedef struct {
	int regs[8];
} jmp_buf_t;
typedef jmp_buf_t jmp_buf[1];
extern int setjmp(jmp_buf_t env[1]);
extern void longjmp(jmp_buf_t env[1], int val)
	__attribute__ ((__noreturn__));

struct case_func {
	u16 rqmid;
	void (*func)(void);
};

int printf16(const char *fmt, ...);
void inner_report(u16 pass, const char *func_name, const char *fmt, ...);
void report_summary(void);
void run_cases(const struct case_func *case_funcs);
void send_cmd(enum func_id fid);
u16 request_irq(u32 irq, irq_handler_t handler);
void free_irq(u16 irqidx);
u32 read_cr(u32 fid);
void write_cr(u32 fid, u32 val);
#if 0
u16 request_irq_ex(u32 irq, irq_handler_ex_t handler,
	void *data, const char *func_name);
#endif
static inline u8 exception_vector(void)
{
	u8 vector;

	asm("movb " xstr(EXCEPTION_ADDR) ", %0" : "=r"(vector));
	return vector;
}

static inline u16 exception_error_code(void)
{
	u16 error_code;

	asm("movw (" xstr(EXCEPTION_ECODE_ADDR)"), %0" : "=rm"(error_code));
	return error_code;
}

static inline u32 read_cr0(void)
{
	return read_cr(FUNC_READ_CR0);
}

static inline void write_cr0(u32 cr0)
{
	write_cr(FUNC_WRITE_CR0, cr0);
}

static inline u32 read_cr3(void)
{
	return read_cr(FUNC_READ_CR3);
}

static inline void write_cr3(u32 cr0)
{
	write_cr(FUNC_WRITE_CR3, cr0);
}

static inline u32 read_cr4(void)
{
	return read_cr(FUNC_READ_CR4);
}

static inline void write_cr4(u32 cr0)
{
	write_cr(FUNC_WRITE_CR4, cr0);
}


static inline u8 v8086_inb(u16 port)
{
	u8 data;

	asm volatile("in %1, %0" : "=a"(data) : "d"(port));
	return data;
}

static inline void v8086_outb(u8 data, u16 port)
{
	asm volatile("out %0, %1" : : "a"(data), "d"(port));
}

static inline void set_iopl(u32 iopl)
{
	write_input_val(X86_EFLAGS_VM | iopl | X86_EFLAGS_IF |
		X86_EFLAGS_ZF | X86_EFLAGS_PF | X86_EFLAGS_FIXED);
	send_cmd(FUNC_SET_IOPL);
}

static inline void set_iomap(u32 iomap)
{
	write_input_val(iomap);
	send_cmd(FUNC_SET_IOMAP);
}

static inline void clear_iomap(void)
{
	send_cmd(FUNC_CLR_IOMAP);
}

static inline void set_igdt(u8 vector, u8 gate_type, u8 access)
{
	u32 value = (access << 16) | (gate_type << 8) | vector;
	write_input_val(value);
	send_cmd(FUNC_SET_IGDT);
}

static inline void set_page(u8 page_index, u32 page_addr)
{
	u32 value = (page_index << 24) | (page_addr & 0xffffff);
	write_input_val(value);
	send_cmd(FUNC_SET_PAGE);
}

static inline void set_page_present(u16 addr, u8 present)
{
	u32 value = present << 16 | addr;
	write_input_val(value);
	send_cmd(FUNC_SET_PAGE_P);
}

static inline u32 read_protected_mode(void)
{
	send_cmd(FUNC_READ_PMODE);
	return read_output_val();
}
#endif
