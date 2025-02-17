#ifndef __RMODE_LIB_H__
#define __RMODE_LIB_H__
#include "../../lib/x86/types.h"
#include "libcflat.h"
#include "setjmp.h"

#include "types.h"
#define xstr(s...) xxstr(s)
#define xxstr(s...) #s
#define nullptr     ((void *)0)
#define __unused    __attribute__((__unused__))
#define __noinline  __attribute__((__noinline__))

/* Because the first page is used for AP startup,
 * save the exception vector rflags error code to page 16 (this page is not used)
 */
#define EXCEPTION_ADDR	0xF004
#define EXCEPTION_VECTOR_ADDR	EXCEPTION_ADDR
#define EXCEPTION_RFLAGS_ADDR	0xF005
#define EXCEPTION_ECODE_ADDR	0xF006

#define NO_EXCEPTION	0xFF

#define ASM_TRY(catch)				\
	"movw $"xstr(NO_EXCEPTION)", %%gs:"xstr(EXCEPTION_VECTOR_ADDR)" \n\t"			\
	"movw $0, %%gs:"xstr(EXCEPTION_ECODE_ADDR)" \n\t"			\
	".pushsection .data.ex \n\t"		\
	".word 1111f," catch "\n\t"		\
	".popsection \n\t"			\
	"1111:"

#define print_serial_u32(val) printf("%u", val)
#define print_serial(val) printf(val)

/* Just for compatibility */
#define report(fmt, pass, ...) \
	inner_report((pass), nullptr, fmt, ##__VA_ARGS__)

/* Recommend this new format report, print the reason if it fails */
#define report_ex(fmt, pass, ...) \
	inner_report((pass), __func__, fmt, ##__VA_ARGS__)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned u32;

struct excp_record {
	u16 ip;
	u16 handler;
};

struct excp_regs {
	u16 ax, cx, dx, bx;
	u16 bp, si, di;
	u16 vector;
	u16 error_code;
	u16 ip;
	u16 cs;
	u16 rflags;
};

struct idt_descr {
	u16 limit;
	u32 base_addr;
} __attribute__((packed));

typedef void (*handler)(struct excp_regs *regs);
void *memset(void *s, int c, u32 n);
void *memcpy(void *dest, const void *src, size_t n);
u8 exception_vector(void);
u16 exception_error_code(void);
void unhandled_r_exception(struct excp_regs *regs);
handler handle_exception(u8 v, handler fn);
bool test_for_r_exception(unsigned int ex, void (*trigger_func)(void *data),
	void *data);
void inner_report(u16 pass, const char *func_name, const char *fmt, ...);
void __set_r_exception_jmpbuf(jmp_buf *addr);
#define set_r_exception_jmpbuf(jmpbuf) \
	(setjmp(jmpbuf) ? : (__set_r_exception_jmpbuf(&(jmpbuf)), 0))

extern struct excp_record exception_table_start, exception_table_end;
extern u16 execption_inc_len;
void common_exception_handler(struct excp_regs *regs);
void set_handle_exception(void);
void setup_idt(void);
void set_idt_entry(int vec, void *addr);

#define test_for_exception(ex, tr, da)	test_for_r_exception(ex, tr, da)

static inline u16 read_cs(void)
{
	unsigned val;
	asm volatile ("mov %%cs, %0" : "=mr"(val));
	return val;
}

static inline void write_cr0(u32 val)
{
	asm volatile ("mov %0, %%cr0" : : "r"(val) : "memory");
}

static inline u32 read_cr0(void)
{
	u32 val;
	asm volatile ("mov %%cr0, %0" : "=r"(val) : : "memory");
	return val;
}

static inline void write_cr2(u32 val)
{
	asm volatile ("mov %0, %%cr2" : : "r"(val) : "memory");
}

static inline u32 read_cr2(void)
{
	u32 val;
	asm volatile ("mov %%cr2, %0" : "=r"(val) : : "memory");
	return val;
}

static inline void write_cr3(u32 val)
{
	asm volatile ("mov %0, %%cr3" : : "r"(val) : "memory");
}

static inline u32 read_cr3(void)
{
	u32 val;
	asm volatile ("mov %%cr3, %0" : "=r"(val) : : "memory");
	return val;
}

static inline void write_cr4(u32 val)
{
	#define CR4_VME 0x1
	#define CR4_PVI 0x2
	// Known issue, setting CR4.VME or CR4.PVI will cause VMM crash.
	u32 value = val & (~(CR4_VME | CR4_PVI));
	asm volatile ("mov %0, %%cr4" : : "r"(value) : "memory");
}

static inline u32 read_cr4(void)
{
	u32 val;
	asm volatile ("mov %%cr4, %0" : "=r"(val) : : "memory");
	return val;
}

static inline void write_dr7(u32 val)
{
	asm volatile ("mov %0, %%dr7" : : "r"(val) : "memory");
}

static inline u32 read_dr7(void)
{
	u32 val;
	asm volatile ("mov %%dr7, %0" : "=r"(val));
	return val;
}

static inline unsigned long read_rflags(void)
{
	unsigned long f;
	asm volatile ("pushf; pop %0\n\t" : "=rm"(f));
	return f;
}

static inline void write_rflags(unsigned long f)
{
	asm volatile ("push %0; popf\n\t" : : "rm"(f));
}

static inline void write_fs(unsigned val)
{
	asm volatile ("mov %0, %%fs" : : "rm"(val) : "memory");
}

struct cpuid { u32 a, b, c, d; };

static inline struct cpuid raw_cpuid(u32 function, u32 index)
{
	struct cpuid r;
	asm volatile ("cpuid"
				: "=a"(r.a), "=b"(r.b), "=c"(r.c), "=d"(r.d)
				: "0"(function), "2"(index));
	return r;
}

static inline struct cpuid cpuid_indexed(u32 function, u32 index)
{
	u32 level = raw_cpuid(function & 0xF0000000, 0).a;
	if (level < function) {
		return (struct cpuid) { 0, 0, 0, 0 };
	}
	return raw_cpuid(function, index);
}

static inline struct cpuid cpuid(u32 function)
{
	return cpuid_indexed(function, 0);
}

static inline void set_idt_limit(u16 idt_limit)
{
	u16 limit = ((idt_limit > 256) ? 256 : idt_limit);

	struct idt_descr desc = {
		.limit = (limit << 2),
		.base_addr = 0
	};

	asm volatile("lidt %0\n"
		:
		: "m"(desc)
		: "memory"
		);
}
#endif
