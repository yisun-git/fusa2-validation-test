#ifndef V8086_UTIL_H
#define V8086_UTIL_H

#include "types.h"

#define X86_CR0_PE        0x00000001
#define X86_CR0_MP        0x00000002
#define X86_CR0_TS        0x00000008
#define X86_CR0_WP        0x00010000
#define X86_CR0_AM        0x00040000
#define X86_CR0_PG        0x80000000
#define X86_CR3_PCID_MASK 0x00000fff
#define X86_CR4_TSD       0x00000004
#define X86_CR4_DE        0x00000008
#define X86_CR4_PSE       0x00000010
#define X86_CR4_PAE       0x00000020
#define X86_CR4_MCE       0x00000040
#define X86_CR4_PCE       0x00000100
#define X86_CR4_UMIP      0x00000800
#define X86_CR4_VMXE      0x00002000
#define X86_CR4_PCIDE     0x00020000
#define X86_CR4_SMAP      0x00200000
#define X86_CR4_PKE       0x00400000

#define X86_EFLAGS_CF     0x00000001
#define X86_EFLAGS_FIXED  0x00000002
#define X86_EFLAGS_PF     0x00000004
#define X86_EFLAGS_AF     0x00000010
#define X86_EFLAGS_ZF     0x00000040
#define X86_EFLAGS_SF     0x00000080
#define X86_EFLAGS_TF     0x00000100
#define X86_EFLAGS_IF     0x00000200
#define X86_EFLAGS_DF     0x00000400
#define X86_EFLAGS_OF     0x00000800
#define X86_EFLAGS_IOPL0  0x00000000
#define X86_EFLAGS_IOPL1  0x00001000
#define X86_EFLAGS_IOPL2  0x00002000
#define X86_EFLAGS_IOPL3  0x00003000
#define X86_EFLAGS_NT     0x00004000
#define X86_EFLAGS_RF     0x00010000
#define X86_EFLAGS_VM     0x00020000
#define X86_EFLAGS_AC     0x00040000

#define X86_EFLAGS_ALU    (X86_EFLAGS_CF | X86_EFLAGS_PF | X86_EFLAGS_AF | \
				X86_EFLAGS_ZF | X86_EFLAGS_SF | X86_EFLAGS_OF)

#define X86_IA32_EFER     0xc0000080
#define X86_EFER_LMA      (1UL << 8)

#define SEGMENT_PRESENT_SET                                 0x80
#define DESCRIPTOR_PRIVILEGE_LEVEL_0                        0x00
#define DESCRIPTOR_PRIVILEGE_LEVEL_1                        0x20
#define DESCRIPTOR_PRIVILEGE_LEVEL_2                        0x40
#define DESCRIPTOR_PRIVILEGE_LEVEL_3                        0x60
#define DESCRIPTOR_TYPE_CODE_OR_DATA                        0x10
#define SEGMENT_TYPE_CODE_EXE_READ_ACCESSED                 0xB
#define SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED 0xF
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TASKGATE      0x5
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TSSG          0xB
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_INTERGATE     0xE
#define SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TRAPGATE      0xF
#define DEFAULT_OPERATION_SIZE_32BIT_SEGMENT                0x40
#define GRANULARITY_SET                                     0x80

#define V8086_TSS_SEL    0x58
/* Used for handling the registered interrupts from v8086 mode */
#define V8086_REG_INT     253
#define V8086_REG_INT_E   254
/* Used for communication between protected and v8086 mode */
#define V8086_COMM_INT    255
#define NO_EXCEPTION      0xff
#define EXCEPTION_ADDR    (shared_data + 0x00)
#define EXCEPTION_ECODE_ADDR (shared_data + 0x02)
#define FUNC_ID_ADDR      (shared_data + 0x04)
#define RET_INS_ADDR      (shared_data + 0x08)
#define FUNC_INPUT_ADDR   (shared_data + 0x10)
#define FUNC_OUTPUT_ADDR  (shared_data + 0x20)
#define V8086_ESP_ADDR    (shared_data + 0x30)
#define TEMP_VALUE_ADDR   (shared_data + 0x34)

#define PAGE_FAULT_ADDR	  0x3000
#define INVALID_CASE_ID   0xffff
#define MAGIC_DWORD       0xfeedbabe
#define MAGIC_WORD        0xcafe

#define RET_SUCCESS       0x0000
#define RET_FAILURE       0xffff

#define DPL_0             0
#define DPL_3             3

#define nullptr           ((void *)0)
#define xxstr(s...)       #s
#define xstr(s...)        xxstr(s)
#define ARRAY_SIZE(a)     (sizeof(a)/sizeof((a)[0]))

#define __unused          __attribute__((__unused__))
#define __noinline        __attribute__((__noinline__))

typedef unsigned char     u8;
typedef unsigned short    u16;
typedef unsigned int      u32;

enum func_id {
	FUNC_V8086_EXIT = 0,
	FUNC_REG_INT,
	FUNC_UNREG_INT,
	FUNC_READ_CR0,
	FUNC_WRITE_CR0,
	FUNC_READ_CR3,
	FUNC_WRITE_CR3,
	FUNC_READ_CR4,
	FUNC_WRITE_CR4,
	FUNC_SET_IOPL,
	FUNC_SET_IOMAP,
	FUNC_CLR_IOMAP,
	FUNC_SET_IGDT,
	FUNC_SET_PAGE,
	FUNC_SET_PAGE_P,
	FUNC_ID_MAX
};

struct far_pointer32 {
	u32 offset;
	u16 selector;
} __attribute__((packed));

struct v8086_irq {
	u32 irq;
	u32 handler;
};

struct int_regs {
	u32 eip;
	u32 cs;
	u32 eflags;
	u32 esp;
	u32 ss;
	u32 es;
	u32 ds;
	u32 fs;
	u32 gs;
};

struct cpuid { u32 a, b, c, d; };

extern u8 shared_data[];
extern u32 *temp_value;

static inline struct cpuid raw_cpuid(u32 function, u32 index)
{
	struct cpuid r;

	asm volatile (
		"cpuid"
		: "=a"(r.a), "=b"(r.b), "=c"(r.c), "=d"(r.d)
		: "0"(function), "2"(index));
	return r;
}

static inline struct cpuid cpuid_indexed(u32 function, u32 index)
{
	u32 level = raw_cpuid(function & 0xf0000000, 0).a;
	if (level < function)
		return (struct cpuid) { 0, 0, 0, 0 };
	return raw_cpuid(function, index);
}

static inline struct cpuid cpuid(u32 function)
{
	return cpuid_indexed(function, 0);
}

static inline void cli(void)
{
	asm volatile ("cli");
}

static inline void sti(void)
{
	asm volatile ("sti");
}

static inline void halt(void)
{
	asm volatile("hlt");
}

static inline void safe_halt(void)
{
	asm volatile("sti; hlt");
}

static inline void pause(void)
{
	asm volatile ("pause");
}

static inline void barrier(void)
{
	asm volatile ("" : : : "memory");
}

static inline u16 read_cs(void)
{
	u32 val;

	asm volatile ("mov %%cs, %0" : "=mr"(val));
	return val;
}

static inline u16 read_ds(void)
{
	u32 val;

	asm volatile ("mov %%ds, %0" : "=mr"(val));
	return val;
}

static inline u16 read_es(void)
{
	u32 val;

	asm volatile ("mov %%es, %0" : "=mr"(val));
	return val;
}

static inline u16 read_fs(void)
{
	u32 val;

	asm volatile ("mov %%fs, %0" : "=mr"(val));
	return val;
}

static inline u16 read_gs(void)
{
	u32 val;

	asm volatile ("mov %%gs, %0" : "=mr"(val));
	return val;
}

static inline u16 read_ss(void)
{
	u32 val;

	asm volatile ("mov %%ss, %0" : "=mr"(val));
	return val;
}

static inline u32 read_flags(void)
{
	u32 flags;

	asm volatile ("pushf; pop %0\n\t" : "=rm"(flags));
	return flags;
}

static inline void write_ds(u32 val)
{
	asm volatile ("mov %0, %%ds" : : "rm"(val) : "memory");
}

static inline void write_es(u32 val)
{
	asm volatile ("mov %0, %%es" : : "rm"(val) : "memory");
}

static inline void write_fs(u32 val)
{
	asm volatile ("mov %0, %%fs" : : "rm"(val) : "memory");
}

static inline void write_gs(u32 val)
{
	asm volatile ("mov %0, %%gs" : : "rm"(val) : "memory");
}

static inline void write_ss(u32 val)
{
	asm volatile ("mov %0, %%ss" : : "rm"(val) : "memory");
}

static inline u32 read_func_id(void)
{
	u32 val;

	asm volatile ("movl %%gs:"xstr(FUNC_ID_ADDR)", %0" : "=r"(val));
	return val;
}

static inline void write_func_id(u32 val)
{
	asm volatile ("movl %0, %%gs:"xstr(FUNC_ID_ADDR) : : "r"(val) : "memory");
}

static inline u32 read_input_val(void)
{
	u32 val;

	asm volatile ("movl %%gs:"xstr(FUNC_INPUT_ADDR)", %0" : "=r"(val));
	return val;
}

static inline void write_input_val(u32 val)
{
	asm volatile ("movl %0, %%gs:"xstr(FUNC_INPUT_ADDR) : : "r"(val) : "memory");
}

static inline u32 read_output_val(void)
{
	u32 val;

	asm volatile ("movl %%gs:"xstr(FUNC_OUTPUT_ADDR)", %0" : "=r"(val));
	return val;
}

static inline void write_output_val(u32 val)
{
	asm volatile ("movl %0, %%gs:"xstr(FUNC_OUTPUT_ADDR) : : "r"(val) : "memory");
}

static inline u32 read_ret_ins_addr(void)
{
	u32 val;

	asm volatile ("movl %%gs:"xstr(RET_INS_ADDR)", %0" : "=r"(val));
	return val;
}

static inline void write_ret_ins_addr(u32 val)
{
	asm volatile ("movl %0, %%gs:"xstr(RET_INS_ADDR) : : "r"(val) : "memory");
}

static inline u32 read_v8086_esp(void)
{
	u32 val;

	asm volatile ("movl %%gs:"xstr(V8086_ESP_ADDR)", %0" : "=r"(val));
	return val;
}

static inline void write_v8086_esp(void)
{
	asm volatile ("movl %%esp, %%gs:"xstr(V8086_ESP_ADDR) : : : "memory");
}

static inline void clear_v8086_esp(void)
{
	asm volatile ("movl $0, %%gs:"xstr(V8086_ESP_ADDR) : : : "memory");
}

static inline struct v8086_irq read_irq(void)
{
	struct v8086_irq val;

	asm volatile (
		"movl %%gs:"xstr(FUNC_INPUT_ADDR)", %0\n"
		"movl %%gs:"xstr(FUNC_INPUT_ADDR + 4)", %1\n"
		: "=r"(val.irq), "=r"(val.handler));
	return val;
}

static inline void write_irq(struct v8086_irq *val)
{
	asm volatile (
		"movl %0, %%gs:"xstr(FUNC_INPUT_ADDR)"\n"
		"movl %1, %%gs:"xstr(FUNC_INPUT_ADDR + 4)"\n"
		:: "r"(val->irq), "r"(val->handler)
		: "memory"
		);
}

static inline void call_int(u8 val)
{
	asm volatile("int %0" : : "i"(val) : "memory");
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

static inline void write_exception_data(u32 val)
{
	asm("mov %0, %%gs:" xstr(EXCEPTION_ADDR) "" : : "r"(val));
}

#endif
