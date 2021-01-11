#include "common.h"
#include "page.h"

/**
 * print functions
 */
#ifndef USE_SERIAL
#define USE_SERIAL
#endif
#include "code16_lib.c"

/**
 * string functions
 */
void *memset(void *s, int c, u32 n)
{
	u32 i;
	char *a = s;

	for (i = 0; i < n; ++i)
		a[i] = c;
	return s;
}

/**
 * misc functions
 */
extern char bss_start;
extern char edata;
void bss_init(void)
{
	memset(&bss_start, 0, &edata - &bss_start);
}


/**
 * apic functions
 */
#include "apic-defs.h"
static void x2apic_write(u32 reg, u32 val)
{
	asm volatile ("wrmsr"
		:
		: "a"(val), "d"(0), "c"(APIC_BASE_MSR + reg/16));
}

int enable_x2apic(void)
{
	u32 a, b, c, d;

	asm ("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(1));

	if (c & (1 << 21)) {
		asm ("rdmsr" : "=a"(a), "=d"(d) : "c"(MSR_IA32_APICBASE));
		a |= 1 << 10;
		asm ("wrmsr" : : "a"(a), "d"(d), "c"(MSR_IA32_APICBASE));
		return 1;
	} else {
		return 0;
	}
}

void enable_apic(void)
{
	if ((rdmsr(MSR_IA32_APICBASE) & (APIC_EN | APIC_EXTD))
		!= (APIC_EN | APIC_EXTD)) {
		enable_x2apic();
		x2apic_write(0xf0, 0x1ff); /* spurious vector register */
	}
}

void mask_pic_interrupts(void)
{
	outb(0xff, 0x21);
	outb(0xff, 0xa1);
}

/**
 * exception functions
 */
#define EX_COUNT 32

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

struct ex_regs {
	u32 eax, ecx, edx, ebx;
	u32 dummy, ebp, esi, edi;
	u32 vector;
	u32 error_code;
	u32 eip;
	u32 cs;
	u32 eflags;
};

struct ex_record {
	u32 eip;
	u32 handler;
};

struct irq_record {
	u32 irq;
	u32 handler;
};

extern idt_entry_t boot_idt[256];
extern struct ex_record exception_table_start, exception_table_end;

#define REG_EX(name, n, ecode_ins) \
	extern char name##_fault; \
	asm (".pushsection .text \n" \
	     #name"_fault: \n" \
	     ecode_ins \
	     "pushl $"#n" \n" \
	     "jmp __handle_exception \n" \
	     ".popsection")

#define EX(name, n) \
	REG_EX(name, n, "pushl $0\n")

#define EX_E(name, n) \
	REG_EX(name, n, "")

EX(de, 0);
EX(db, 1);
EX(nmi, 2);
EX(bp, 3);
EX(of, 4);
EX(br, 5);
EX(ud, 6);
EX(nm, 7);
EX_E(df, 8);
EX_E(ts, 10);
EX_E(np, 11);
EX_E(ss, 12);
EX_E(gp, 13);
EX_E(pf, 14);
EX(mf, 16);
EX_E(ac, 17);
EX(mc, 18);
EX(xm, 19);

EX(vri, V8086_REG_INT);
EX(vci, V8086_COMM_INT);

static void *idt_handlers[EX_COUNT] = {
	[0] = &de_fault,
	[1] = &db_fault,
	[2] = &nmi_fault,
	[3] = &bp_fault,
	[4] = &of_fault,
	[5] = &br_fault,
	[6] = &ud_fault,
	[7] = &nm_fault,
	[8] = &df_fault,
	[10] = &ts_fault,
	[11] = &np_fault,
	[12] = &ss_fault,
	[13] = &gp_fault,
	[14] = &pf_fault,
	[16] = &mf_fault,
	[17] = &ac_fault,
	[18] = &mc_fault,
	[19] = &xm_fault,
};

static struct irq_record int_table[1];

u32 register_int(u32 irq, u32 handler)
{
	struct irq_record *ir;

	// find a free slot
	for (u32 i = 0; i < ARRAY_SIZE(int_table); i++) {
		ir = int_table + i;
		if (ir->irq == 0 && ir->handler == 0) {
			ir->irq = irq;
			ir->handler = handler;
			set_idt_entry(irq,  (void *)&vri_fault, DPL_3);
			return i;
		}
	}
	return RET_FAILURE;
}

void unregister_int(u32 i)
{
	if (i < ARRAY_SIZE(int_table)) {
		int_table[i].irq = 0;
		int_table[i].handler = 0;
	}
}

void set_idt_entry(int vec, void *addr, int dpl)
{
	idt_entry_t *e = &boot_idt[vec];

	memset(e, 0, sizeof *e);
	e->offset0 = (u32)addr;
	e->selector = read_cs();
	e->ist = 0;
	e->type = 14;
	e->dpl = dpl;
	e->p = 1;
	e->offset1 = (u32)addr >> 16;
}

void setup_idt(void)
{
	for (int i = 0; i < EX_COUNT; i++) {
		if (idt_handlers[i])
			set_idt_entry(i, (void *)idt_handlers[i], DPL_3);
	}

	for (int i = 0; i < ARRAY_SIZE(int_table); i++)
		unregister_int(i);

	set_idt_entry(V8086_COMM_INT, (void *)&vci_fault, DPL_3);
}

void unhandled_exception(struct ex_regs *regs)
{
	printf("Unhandled exception [%u] at ip 0x%x\n",
		regs->vector, regs->eip);

	if (regs->vector == 14) {
		printf("PF at addr 0x%x\n", read_cr2());
	}

	printf("error_code=0x%x, eflags=0x%x, cs=0x%x\n"
		"eax=0x%x, ecx=0x%x, edx=0x%x, ebx=0x%x,\n"
		"ebp=0x%x, esi=0x%x, edi=0x%x,\n"
		"cr0=0x%x, cr2=0x%x, cr3=0x%x, cr4=0x%x,\n",
		regs->error_code, regs->eflags, regs->cs,
		regs->eax, regs->ecx, regs->edx, regs->ebx,
		regs->ebp, regs->esi, regs->edi,
		read_cr0(), read_cr2(), read_cr3(), read_cr4()
	);

	halt();
}

extern v8086_func v8086_funcs[];

__attribute__((regparm(1)))
void handle_exception(struct ex_regs *regs)
{
	struct ex_record *ex;
	struct irq_record *ir;
	u32 ex_val;

	/* handle the invocation between v8086 and protect */
	if (V8086_COMM_INT == regs->vector) {
		enum func_id fid = read_func_id();
		if (FUNC_V8086_EXIT <= fid && fid < FUNC_ID_MAX) {
			if (v8086_funcs[fid] != nullptr)
				v8086_funcs[fid]();
		}
		regs->eip = read_ret_ins_addr();
		return;
	}

	/* handle the registerd interrupts */
	if (V8086_REG_INT == regs->vector) {
		for (int i = 0; i < ARRAY_SIZE(int_table); i++) {
			ir = int_table + i;
			if (ir->irq != 0 && ir->handler != (u32)nullptr) {
				regs->eip = ir->handler;
				return;
			}
		}
	}

	ex_val = regs->vector | (regs->error_code << 16) |
		(((regs->eflags >> 16) & 1) << 8);

	asm("mov %0, %%gs:" xstr(EXCEPTION_ADDR) "" : : "r"(ex_val));

	for (ex = &exception_table_start; ex < &exception_table_end; ++ex) {
		if (ex->eip <= regs->eip && regs->eip < ex->handler) {
			regs->eip = ex->handler;
			return;
		}
	}
	unhandled_exception(regs);
}

asm (".pushsection .text\n\t"
	"__handle_exception:\n\t"
	"mov $0x10, %eax\n\t"
	"mov %eax, %ds\n\t"
	"mov %eax, %es\n\t"
	"mov %eax, %fs\n\t"
	"mov %eax, %gs\n\t"
	"push %edi; push %esi; push %ebp; sub $4,%esp\n\t"
	"push %ebx; push %edx; push %ecx; push %eax\n\t"
	"mov %esp, %eax\n\t"
	"call handle_exception\n\t"
	"pop %eax; pop %ecx; pop %edx; pop %ebx\n\t"
	"add $4, %esp; pop %ebp; pop %esi; pop %edi\n\t"
	"add $4, %esp\n\t"
	"add $4, %esp\n\t"
	"iretl\n\t"
	".popsection"
);


void set_page_control_bit(void *gva, page_level level, page_control_bit bit, u32 value, bool is_invalidate)
{
	if (gva == NULL) {
		printf("this address is NULL!\n");
		return;
	}

	u32 cr3 = read_cr3();
	u32 pde_offset = PGDIR_OFFSET((u32)gva, PAGE_PDE);
	u32 pte_offset = PGDIR_OFFSET((u32)gva, PAGE_PTE);
	pteval_t *pde = (pteval_t *)cr3;
	pteval_t *pte;

	if ((pde[pde_offset] & (1 << PAGE_PS_FLAG)) && (level == PAGE_PTE)) {
		level = PAGE_PDE;
	} else {
		pte = (pteval_t *)(pde[pde_offset] & PAGE_MASK);
	}

	if (level == PAGE_PDE) {
		if (value == 1) {
			pde[pde_offset] |= (1u << bit);
		} else {
			pde[pde_offset] &= ~(1u << bit);
		}
	} else {
		if (value == 1) {
			pte[pte_offset] |= (1u << bit);
		} else {
			pte[pte_offset] &= ~(1u << bit);
		}
	}
	if (is_invalidate) {
		invlpg(gva);
	}

}