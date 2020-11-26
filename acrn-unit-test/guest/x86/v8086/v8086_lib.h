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

void printf16(const char *fmt, ...);
void inner_report(u16 pass, const char *func_name, const char *fmt, ...);
void report_summary(void);
void run_cases(const struct case_func *case_funcs);
void send_cmd(enum func_id fid);
u16 request_irq(u8 irq, irq_handler_t handler);
void free_irq(u16 irqidx);
u32 read_cr(u32 fid);
void write_cr(u32 fid, u32 val);

static inline u8 exception_vector(void)
{
	u8 vector;

	asm("movb " xstr(EXCEPTION_ADDR) ", %0" : "=r"(vector));
	return vector;
}

static inline u32 read_cr0(void)
{
	return read_cr(FUNC_READ_CR0);
}

static inline void write_cr0(u32 cr0)
{
	return write_cr(FUNC_WRITE_CR0, cr0);
}

static inline u32 read_cr3(void)
{
	return read_cr(FUNC_READ_CR3);
}

static inline void write_cr3(u32 cr0)
{
	return write_cr(FUNC_WRITE_CR3, cr0);
}

static inline u32 read_cr4(void)
{
	return read_cr(FUNC_READ_CR4);
}

static inline void write_cr4(u32 cr0)
{
	return write_cr(FUNC_WRITE_CR4, cr0);
}

#endif
