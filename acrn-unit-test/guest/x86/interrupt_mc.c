#include "segmentation.h"
#include "interrupt.h"
#include "debug_print.h"

static volatile unsigned int g_irqcounter[256] = { 0 };

int INC_COUNT = 0;
static inline void irqcounter_initialize(void)
{
	/* init g_irqcounter and INC_COUNT to 0
	 * (INC_COUNT is the value added to the corresponding vector every
	 * time an exception is generated. It has been used to determine
	 * the sequence of the exceptions when multiple exceptions are generated)
	 */
	INC_COUNT = 0;
	memset((void *)g_irqcounter, 0, sizeof(g_irqcounter));
}
static inline void irqcounter_incre(unsigned int vector)
{
	INC_COUNT += 1;
	g_irqcounter[vector]  += INC_COUNT;
}

static inline unsigned int irqcounter_query(unsigned int vector)
{
	return g_irqcounter[vector];
}

struct ex_record {
	unsigned long rip;
	unsigned long handler;
};

extern struct ex_record exception_table_start, exception_table_end;

/* The length of instruction to skip when the exception handler returns
 * (equal to the length of instruction in which the exception occurred)
 */
static volatile int execption_inc_len = 0;
/* Used for exception handling function to return error code
 */
ulong save_error_code = 0;
/* save_rflags1 is used to save the rflag of  the stack when an exception is generated.
 * save_rflags2 is used to save the rflag at the
 * beginning of the exception handling function.
 */
ulong save_rflags1 = 0;
ulong save_rflags2 = 0;

static void *get_idt_addr(int vec)
{
	unsigned long addr;
	idt_entry_t *e = &boot_idt[vec];

#ifdef __x86_64__
	addr = ((unsigned long)e->offset0) | ((unsigned long)e->offset1 << 16)
		| ((unsigned long)e->offset2 << 32);
#else
	addr = ((unsigned long)e->offset0) | ((unsigned long)e->offset1 << 16);
#endif
	return (void *)addr;
}

static int rip_index = 0;
static u64 rip[10] = {0, };

static inline uint16_t get_pcpu_id(void)
{
	/** Declare the following local variables of type uint32_t.
	 *  - tsl representing low 32 bit of the processor's time-stamp counter, not initialized.
	 *  - tsh representing high 32 bit of the processor's time-stamp counter, not initialized.
	 *  - cpu_id representing a physical CPU ID, not initialized.
	 */
	uint32_t tsl, tsh, cpu_id;

	/** Execute rdtscp in order to get the current physical CPU ID.
	 *  - Input operands: None
	 *  - Output operands: EAX is stored to tsl, EDX is stored to tsh, ECX is stored to cpu_id.
	 *  - Clobbers: None
	 */
	asm volatile("rdtscp" : "=a"(tsl), "=d"(tsh), "=c"(cpu_id)::);
	/** Return cpu_id. */
	return (uint16_t)cpu_id;
}


static void handled_exception(struct ex_regs *regs)
{
	struct ex_record *ex;
	unsigned ex_val;

	save_error_code = regs->error_code;
	save_rflags1 = regs->rflags;
	save_rflags2 = read_rflags();
	ex_val = regs->vector | (regs->error_code << 16) |
		(((regs->rflags >> 16) & 1) << 8);
	asm("mov %0, %%gs:"xstr(EXCEPTION_ADDR)"" : : "r"(ex_val));

	irqcounter_incre(regs->vector);

	if (rip_index < 10) {
		rip[rip_index++] = regs->rip;
	}

	for (ex = &exception_table_start; ex != &exception_table_end; ++ex) {
		if (ex->rip == regs->rip) {
			regs->rip = ex->handler;
			return;
		}
	}
	if (execption_inc_len >= 0) {
		regs->rip += execption_inc_len;
	}
	else {
		unhandled_exception(regs, false);
	}
}

static volatile  u64 current_rip;
void get_current_rip_function(void)
{
	u64 rsp_64 = 0;
	u64 *rsp_val;
	asm volatile("mov %%rdi, %0\n\t"
		: "=m"(rsp_64)
		);

	rsp_val = (u64 *)rsp_64;

	current_rip = rsp_val[0];
}

asm("get_current_rip:\n"
	"mov %rsp, %rdi\n"
	"call get_current_rip_function\n"
	"ret\n"
);
void get_current_rip(void);

struct check_regs_64 {
	unsigned long CR0, CR2, CR3, CR4, CR8;
	//unsigned long RIP;
	//unsigned long RFLAGS;
	unsigned long RAX, RBX, RCX, RDX;
	unsigned long RDI, RSI;
	unsigned long RBP, RSP;
	unsigned long R8, R9, R10, R11, R12, R13, R14, R15;
	unsigned long CS, DS, ES, FS, GS, SS;
};

struct check_regs_64 regs_check[2];

#define DUMP_REGS1(p_regs)	\
{					\
	asm volatile ("mov %%cr0, %0" : "=d"(p_regs.CR0) : : "memory");	\
	asm volatile ("mov %%cr2, %0" : "=d"(p_regs.CR2) : : "memory");	\
	asm volatile ("mov %%cr3, %0" : "=d"(p_regs.CR3) : : "memory");	\
	asm volatile ("mov %%cr4, %0" : "=d"(p_regs.CR4) : : "memory");	\
	asm volatile ("mov %%cr8, %0" : "=d"(p_regs.CR8) : : "memory");	\
	asm volatile ("mov %%rax, %0" : "=m"(p_regs.RAX) : : "memory");	\
	asm volatile ("mov %%rbx, %0" : "=m"(p_regs.RBX) : : "memory");	\
}

#define DUMP_REGS2(p_regs)	\
{					\
	asm volatile ("mov %%rcx, %0" : "=m"(p_regs.RCX) : : "memory");	\
	asm volatile ("mov %%rdx, %0" : "=m"(p_regs.RDX) : : "memory");	\
	asm volatile ("mov %%rdi, %0" : "=m"(p_regs.RDI) : : "memory");	\
	asm volatile ("mov %%rsi, %0" : "=m"(p_regs.RSI) : : "memory");	\
	asm volatile ("mov %%rbp, %0" : "=m"(p_regs.RBP) : : "memory");	\
	asm volatile ("mov %%rsp, %0" : "=m"(p_regs.RSP) : : "memory");	\
	asm volatile ("mov %%r8, %0" : "=m"(p_regs.R8) : : "memory");	\
	asm volatile ("mov %%r9, %0" : "=m"(p_regs.R9) : : "memory");	\
	asm volatile ("mov %%r10, %0" : "=m"(p_regs.R10) : : "memory");	\
	asm volatile ("mov %%r11, %0" : "=m"(p_regs.R11) : : "memory");	\
	asm volatile ("mov %%r12, %0" : "=m"(p_regs.R12) : : "memory");	\
	asm volatile ("mov %%r13, %0" : "=m"(p_regs.R13) : : "memory");	\
	asm volatile ("mov %%r14, %0" : "=m"(p_regs.R14) : : "memory");	\
	asm volatile ("mov %%r15, %0" : "=m"(p_regs.R15) : : "memory");	\
	asm volatile ("mov %%cs, %0" : "=m"(p_regs.CS));				\
	asm volatile ("mov %%ds, %0" : "=m"(p_regs.DS));				\
	asm volatile ("mov %%es, %0" : "=m"(p_regs.ES));				\
	asm volatile ("mov %%fs, %0" : "=m"(p_regs.FS));				\
	asm volatile ("mov %%gs, %0" : "=m"(p_regs.GS));				\
	asm volatile ("mov %%ss, %0" : "=m"(p_regs.SS));				\
}

static void print_all_regs(struct check_regs_64 *p_regs)
{
	printf("0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n",
		p_regs->CR0, p_regs->CR2, p_regs->CR3, p_regs->CR4, p_regs->CR8);
	printf("0x%lx 0x%lx 0x%lx 0x%lx\n", p_regs->RAX, p_regs->RBX, p_regs->RCX, p_regs->RDX);
	printf("0x%lx 0x%lx\n", p_regs->RDI, p_regs->RSI);
	printf("0x%lx 0x%lx\n", p_regs->RBP, p_regs->RSP);
	printf("0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n",
		p_regs->R8, p_regs->R9, p_regs->R10, p_regs->R11,
		p_regs->R12, p_regs->R13, p_regs->R14, p_regs->R15);
	printf("0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n",
		p_regs->CS, p_regs->DS, p_regs->ES, p_regs->FS, p_regs->GS, p_regs->SS);
}

static int check_all_regs()
{
	int ret;

	ret = memcmp(&regs_check[0], &regs_check[1], sizeof(struct check_regs_64));
	if (ret == 0) {
		return 0;
	}

	print_all_regs(&regs_check[0]);
	print_all_regs(&regs_check[1]);
	return 1;
}

static void set_idt_addr(int vec, void *addr)
{
	idt_entry_t *e = &boot_idt[vec];
	e->offset0 = (unsigned long)addr;
	e->offset1 = (unsigned long)addr >> 16;
#ifdef __x86_64__
	e->offset2 = (unsigned long)addr >> 32;
#endif
}

static int check_rflags()
{
	int check = 0;
	/* TF[bit 8], NT[bit 14], RF[bit 16], VM[bit 17] */
	if ((save_rflags2 & RFLAG_TF_BIT) || (save_rflags2 & RFLAG_NT_BIT)
		|| (save_rflags2 & RFLAG_RF_BIT) || (save_rflags2 & RFLAG_VM_BIT)) {
		check = 1;
	}
	return check;
}

/**
 * @brief case name: Expose exception and interrupt handling_MC_001
 *
 * Summary:The hypervisor, inject #MC exception into vcpu, at 64bit mode,
 * vcpu will call the #MC handler, program state should be unchanged,
 * the saved contents of EIP registers point to the instruction that generated the exception,
 * and EFLAGS.TF, EFLAGS.VM, EFLAGS.RF, EFLAGS.NT is cleared after EFLAGS is saved on the stack
 */
static void interrupt_rqmid_40025_expose_exception_mc_001(void)
{
	int check = 0;
	u64 dump_func_size = 0;
	u64 tmp_rip;
	void *addr = get_idt_addr(MC_VECTOR);

	/* init g_irqcounter */
	irqcounter_initialize();

    /*calc the size for DUMP_REGS1 and DUMP_REGS2*/
	asm volatile("call get_current_rip");
	/* length of the instruction(tmp_rip = current_rip) is 7
	 *  48 8b 0d 37 2a 05 00    mov    0x52a37(%rip),%rcx
	 */
	tmp_rip = current_rip;
	DUMP_REGS1(regs_check[0]);
	DUMP_REGS2(regs_check[0]);

	/* the len is 5 for 'call get_current_rip'
	 * e8 78 f9 ff ff    callq  10029d <get_current_rip>
	 */
	asm volatile("call get_current_rip");
	/* the size for DUMP_REGS1 and DUMP_REGS2 */
	dump_func_size = current_rip - tmp_rip - 7 - 5;
	printf("dump_func_size = 0x%lx current_rip = 0x%lx  tmp_rip = 0x%lx\r\n",
			dump_func_size, current_rip, tmp_rip);
	memset(regs_check, 0, sizeof(regs_check));

	/* prepare the interrupt handler of #MC. */
	handle_exception(MC_VECTOR, &handled_exception);

	/* init rip array and rip_index*/
	memset(rip, 0, sizeof(rip));
	rip_index = 0;

	/* enable CR4.MCE */
	write_cr4(read_cr4() | X86_CR4_MCE);
	printf("%s:Please inject #MC cr4=0x%lx\n", __FUNCTION__, read_cr4());
	/* dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[0]);
	/*  dump all remaining registers*/
	DUMP_REGS2(regs_check[0]);

	/*execute hlt instruction wait #MC*/
	asm volatile("hlt");

	/* dump CR and rax rbx register*/
	DUMP_REGS1(regs_check[1]);
	/*  dump all remaining registers*/
	DUMP_REGS2(regs_check[1]);

	/* get 'hlt' instruction exception address */
	/* If ASM is used to call a function, GCC will not see the
	 * modification of register inside the called function,
	 * which will cause subsequent code to continue to use
	 * the modified register. In this case,
	 * this code must be put in the final test
	 */
	asm volatile("call get_current_rip");
	/* 5 is call get_current_rip instruction len*/
	current_rip -= (5 + dump_func_size);

	/* check #MC exceptionprogram state and EFLAGS after #MC exception */
	check = check_all_regs();
	check |= check_rflags();

	printf("#MC count =%x check = %x current_rip = 0x%lx rip = 0x%lx \n",
			irqcounter_query(MC_VECTOR), check, current_rip, rip[0]);
	report("%s", ((irqcounter_query(MC_VECTOR) > 0) && (check == 0)
		&& (rip[0] == current_rip)), __FUNCTION__);

	/* resume environment */
	set_idt_addr(MC_VECTOR, addr);
}


static void print_case_list(void)
{
	printf("Case ID:%d case name:%s\n\r", 40025,
		"Expose exception and interrupt handling_MC_001");
}

static void test_interrupt(void)
{
	interrupt_rqmid_40025_expose_exception_mc_001();
}

void ap_main(void)
{
	/* Enable CR4.MCE for AP*/
	write_cr4(read_cr4() | X86_CR4_MCE);
}

int main(void)
{
	setup_vm();
	setup_ring_env();
	setup_idt();

	print_case_list();

	test_interrupt();

	return report_summary();
}
