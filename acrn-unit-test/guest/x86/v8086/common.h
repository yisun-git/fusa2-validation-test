#ifndef V8086_H
#define V8086_H

#include "v8086_util.h"
#include "msr.h"
#include "page.h"

typedef void (*v8086_func)(void);
typedef unsigned long long u64;

struct descriptor_table_ptr {
	u16 limit;
	u32 base;
} __attribute__((packed));

typedef struct {
	u16 limit_low;
	u16 base_low;
	u8 base_middle;
	u8 access;
	u8 granularity;
	u8 base_high;
} gdt_entry_t;

typedef struct {
	u16 offset0;
	u16 selector;
	u16 ist : 3;
	u16 resv1 : 5;
	u16 type : 4;
	u16 resv2 : 1;
	u16 dpl : 2;
	u16 p : 1;
	u16 offset1;
} idt_entry_t;

static inline void lgdt(const struct descriptor_table_ptr *ptr)
{
	asm volatile ("lgdt %0" : : "m"(*ptr));
}

static inline void sgdt(struct descriptor_table_ptr *ptr)
{
	asm volatile ("sgdt %0" : "=m"(*ptr));
}

static inline void ltr(u16 val)
{
	asm volatile ("ltr %0" : : "rm"(val));
}

static inline u16 str(void)
{
	u16 val;

	asm volatile ("str %0" : "=rm"(val));
	return val;
}

static inline u64 rdmsr(u32 index)
{
	u32 a, d;

	asm volatile ("rdmsr" : "=a"(a), "=d"(d) : "c"(index) : "memory");
	return a | ((u64)d << 32);
}

static inline u32 read_cr0(void)
{
	u32 val;

	asm volatile ("mov %%cr0, %0" : "=r"(val) : : "memory");
	return val;
}

static inline u32 read_cr2(void)
{
	u32 val;

	asm volatile ("mov %%cr2, %0" : "=r"(val) : : "memory");
	return val;
}

static inline u32 read_cr3(void)
{
	u32 val;

	asm volatile ("mov %%cr3, %0" : "=r"(val) : : "memory");
	return val;
}

static inline u32 read_cr4(void)
{
	u32 val;

	asm volatile ("mov %%cr4, %0" : "=r"(val) : : "memory");
	return val;
}

static inline void write_cr0(u32 val)
{
	asm volatile ("mov %0, %%cr0" : : "r"(val) : "memory");
}

static inline void write_cr2(u32 val)
{
	asm volatile ("mov %0, %%cr2" : : "r"(val) : "memory");
}

static inline void write_cr3(u32 val)
{
	asm volatile ("mov %0, %%cr3" : : "r"(val) : "memory");
}

static inline void write_cr4(u32 val)
{
	asm volatile ("mov %0, %%cr4" : : "r"(val) : "memory");
}

static inline void invlpg(volatile void *va)
{
	asm volatile("invlpg (%0)" ::"r" (va) : "memory");
}

void *memset(void *s, int c, u32 n);
int printf(const char *fmt, ...);
void setup_idt(void);
void set_idt_entry(int vec, void *addr, int dpl);
void set_idt_entry_ex(int vec, void *addr, int dpl, u16 sel, u16 type);
void set_gdt_entry(int sel, u32 base,  u32 limit, u8 access, u8 gran);
void set_idt_task_gate(int vec, u16 sel, int dpl);
void v8086_enter(void);
u32 register_int(u32 irq, u32 handler);
void unregister_int(u32 irqidx);

void set_page_control_bit(void *gva, page_level level, page_control_bit bit, u32 value, bool is_invalidate);

#endif
