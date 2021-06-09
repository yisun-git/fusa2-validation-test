#include <stdarg.h>
#include "common.h"

typedef struct {
	u16 prev;
	u16 res1;
	u32 esp0;
	u16 ss0;
	u16 res2;
	u32 esp1;
	u16 ss1;
	u16 res3;
	u32 esp2;
	u16 ss2;
	u16 res4;
	u32 cr3;
	u32 eip;
	u32 eflags;
	u32 eax, ecx, edx, ebx, esp, ebp, esi, edi;
	u16 es;
	u16 res5;
	u16 cs;
	u16 res6;
	u16 ss;
	u16 res7;
	u16 ds;
	u16 res8;
	u16 fs;
	u16 res9;
	u16 gs;
	u16 res10;
	u16 ldt;
	u16 res11;
	u16 t:1;
	u16 res12:15;
	u16 iomap_base;
	u8 iomap[256];
} tss32_t;

#define SW_INT_REMAP         32    /* Software Interrupt Redirection Bit Map */
#define FIRST_SPARE_SEL      0x50
#define VM86_CS_DPL_SEL      (FIRST_SPARE_SEL + 0x10)
#define VM86_CS_CONFORM_SEL  (FIRST_SPARE_SEL + 0x18)

u32 *temp_value = (u32 *)TEMP_VALUE_ADDR;

__noinline void v8_exit(void)
{
	printf("-- v8086 exit --\n");
	halt();
}

/**
 * This interrupt handler is used for communication
 * between the protected mode and the v8086 mode.
 */
__noinline void v8_reg_int(void)
{
	u32 val = RET_FAILURE;
	const struct v8086_irq vi = read_irq();

	val = register_int(vi.irq, vi.handler);
	write_output_val(val);
}

__noinline void v8_unreg_int(void)
{
	u32 irqidx = read_input_val();

	unregister_int(irqidx);
}

__noinline void v8_read_cr0(void)
{
	write_output_val(read_cr0());
}

__noinline void v8_write_cr0(void)
{
	write_cr0(read_input_val());
}

__noinline void v8_read_cr3(void)
{
	write_output_val(read_cr3());
}

__noinline void v8_write_cr3(void)
{
	write_cr3(read_input_val());
}

__noinline void v8_read_cr4(void)
{
	write_output_val(read_cr4());
}

__noinline void v8_write_cr4(void)
{
	write_cr4(read_input_val());
}

__noinline void v8_set_page_present(void)
{
	u32 value = read_input_val();
	u8 present = value >> 16;
	u16 addr = value & 0xffff;
	set_page_control_bit((void *)(u32)addr, PAGE_PTE, PAGE_P_FLAG, present, 1);
}

extern u32 v8086_iopl;
__noinline void v8_set_iopl(void)
{
	v8086_iopl = read_input_val();
	v8086_enter();
	halt();
}

extern tss32_t tss;

/* sdm vol1, 18.5.2 I/O Permission Bit Map */
__noinline void v8_set_iomap(void)
{
	u32 bitpos = read_input_val();
	u32 byte_index = SW_INT_REMAP + (bitpos >> 3);
	tss32_t *ptss = &tss;

	ptss->iomap_base = (u16)(sizeof(tss32_t) - sizeof(ptss->iomap) + SW_INT_REMAP);

	/* Set I/O permission bit map */
	if (byte_index < sizeof(ptss->iomap))
		ptss->iomap[byte_index] |= (1 << (bitpos & 0x7));

	/* Set the last byte of bit map */
	ptss->iomap[sizeof(ptss->iomap) - 1] = 0xff;
}

__noinline void v8_clear_iomap(void)
{
	tss32_t *ptss = &tss;
	for (int i = SW_INT_REMAP; i < sizeof(ptss->iomap) - 1; i++)
		ptss->iomap[i] = 0;
}

extern u32 ptl2[];
extern u32 pt[];
__noinline void v8_set_page(void)
{
	u32 value = read_input_val();
	u32 page_addr = value & 0xffffff;
	u8 page_index = value >> 24;
	ptl2[page_index] = page_addr;
	write_cr3((u32)pt);
}

extern u32 ring0stacktop;
void handle_intr21(void)
{
	u32 ds = 0xff;
	u32 es = 0xff;
	u32 fs = 0xff;
	u32 gs = 0xff;
	u32 esp = 0xff;
	u32 flags = 0xff;
	struct int_regs *regs;

	asm volatile (
		"mov %%ecx, %0\n"
		"mov %%edx, %1\n"
		"mov %%edi, %2\n"
		"mov %%esi, %3\n"
		: "=rm"(ds), "=rm"(es), "=rm"(fs), "=rm"(gs)
		:
		: "memory"
		);

	//add print message to refrash the serial buffer.
	printf("enter: %s()\n", __func__);
	regs = ((struct int_regs *)&ring0stacktop) - 1;
	esp = read_v8086_esp();
	flags = read_input_val();

	*temp_value = MAGIC_DWORD;
	/* current GS == 0, FS == 0, ES == 0, DS == 0 */
	if ((ds == 0) && (es == 0) && (fs == 0) && (gs == 0)) {
		/* check values on the stack */
		if ((regs->gs == 0) && (regs->fs == 0) &&
			(regs->ds == 0) && (regs->es == 0) &&
			(regs->ss == 0) && (regs->cs == 0) &&
			(regs->esp == esp) && ((regs->eflags & 0xffff) == flags)) {
			*temp_value = read_flags();
		}
	}

	if (MAGIC_DWORD == *temp_value) {
		printf("\tgs=%u,fs=%u,ds=%u,es=%u,esp=0x%x,eflags=0x%x\n",
			gs, fs, ds, es, esp, flags);
		printf("\tgs=%u,fs=%u,ds=%u,es=%u,esp=0x%x,eflags=0x%x,ss=%u,cs=%u\n",
			regs->gs, regs->fs, regs->ds, regs->es, regs->esp, regs->eflags, regs->ss, regs->cs);
	}
}

extern void asm_exc_handler(void);
asm (".pushsection .text\n"
"asm_exc_handler:\n"
	"mov %ds, %ecx\n"
	"mov %es, %edx\n"
	"mov %fs, %edi\n"
	"mov %gs, %esi\n"
	"mov $0x10, %eax\n"
	"mov %eax, %ds\n"
	"mov %eax, %es\n"
	"mov %eax, %fs\n"
	"mov %eax, %gs\n"
	"call handle_intr21\n\t"
	"iret\n"
".popsection\n"
);

void user_exc_handler(void)
{
	asm volatile("iret\n");
}

#define STACK_LEN 1024
u8 ts_0_stack[STACK_LEN];
u8 ts_3_stack[STACK_LEN];
void set_v8086_tss(u16 selector)
{
	static tss32_t v8086_tss;
	u32 ts_3_stacktop = (u32)ts_3_stack + STACK_LEN;

	set_gdt_entry(selector, (u32)&v8086_tss, sizeof(tss32_t) - 1,
		SEGMENT_PRESENT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_0 |
		SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TSSG,  0);

	v8086_tss = (tss32_t) {
		.ss0	= 0x10,
		.esp0	= (u32)(&ts_0_stack[STACK_LEN]),
		.cr3    = read_cr3(),
		.eip    = (u32)user_exc_handler,
		.eflags = 0x23202,
		.esp    = ts_3_stacktop,
		.ebp    = ts_3_stacktop,
		.iomap_base = (u16)(sizeof(v8086_tss) - sizeof(v8086_tss.iomap) + SW_INT_REMAP),
	};

	/* Set I/O permission bit map */
	memset(v8086_tss.iomap,  0xff,  SW_INT_REMAP);
	memset(v8086_tss.iomap + SW_INT_REMAP,  0x00,  sizeof(v8086_tss.iomap) - SW_INT_REMAP);

	/* Set the last byte of bit map */
	v8086_tss.iomap[sizeof(v8086_tss.iomap)-1] = 0xff;
}

__noinline void v8_set_igdt(void)
{
	u32 value = read_input_val();
	u8 vector = value & 0xff;
	u8 gate_type = (value >> 8) & 0xff;
	u8 access = (value >> 16) & 0xff;
	u8 selector = (22 == vector ? VM86_CS_DPL_SEL : VM86_CS_CONFORM_SEL);

	switch (vector) {
	case 21:
		set_idt_entry_ex(vector, asm_exc_handler, DPL_3, read_cs(), gate_type);
		break;
	case 22:
	case 23:
		set_idt_entry_ex(vector, user_exc_handler, DPL_0, selector, gate_type);
		set_gdt_entry(selector, 0, 0xfffff, access,
			GRANULARITY_SET | DEFAULT_OPERATION_SIZE_32BIT_SEGMENT);
		break;
	case 24:
		set_idt_entry(vector, user_exc_handler, DPL_3);
		break;
	case 25:
		selector = V8086_TSS_SEL;
		set_v8086_tss(selector);
		set_idt_task_gate(vector, selector, DPL_3);
		break;
	default:
		printf("%s: vector(%u) is not supported!\n", __func__, vector);
		break;
	}
}

static bool in_protected_mode(void)
{
	u32 cr0 = read_cr0();
	u32 eflags = read_flags();
	return (cr0 & X86_CR0_PE) == X86_CR0_PE && (eflags & X86_EFLAGS_VM) == 0;
}

__noinline void v8_read_protected_mode(void)
{
	write_output_val(in_protected_mode());
}

const v8086_func v8086_funcs[FUNC_ID_MAX] = {
	[FUNC_V8086_EXIT] = v8_exit,
	[FUNC_REG_INT]    = v8_reg_int,
	[FUNC_UNREG_INT]  = v8_unreg_int,
	[FUNC_READ_CR0]   = v8_read_cr0,
	[FUNC_WRITE_CR0]  = v8_write_cr0,
	[FUNC_READ_CR3]   = v8_read_cr3,
	[FUNC_WRITE_CR3]  = v8_write_cr3,
	[FUNC_READ_CR4]   = v8_read_cr4,
	[FUNC_WRITE_CR4]  = v8_write_cr4,
	[FUNC_SET_IOPL]   = v8_set_iopl,
	[FUNC_SET_IOMAP]  = v8_set_iomap,
	[FUNC_CLR_IOMAP]  = v8_clear_iomap,
	[FUNC_SET_IGDT]   = v8_set_igdt,
	[FUNC_SET_PAGE]   = v8_set_page,
	[FUNC_SET_PAGE_P] = v8_set_page_present,
	[FUNC_READ_PMODE]   = v8_read_protected_mode,
};

int main(int argc, char **argv)
{
	setup_idt();
	*temp_value = in_protected_mode();
	write_func_id(FUNC_ID_MAX);
	v8086_enter();
	return 0;
}
