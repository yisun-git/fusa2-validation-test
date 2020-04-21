#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "fpu.h"
#include "vm.h"
#include "alloc.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "alloc_phys.h"
#include "ioram.h"
#include "apic.h"
#include "misc.h"
#include "fwcfg.h"
#include "smp.h"
#include "instruction_common.h"

typedef struct fxsave_struct {
	u16 fcw;
	u16 fsw;
	u8  ftw;
	u8  revd1;
	u16 fop;
	u32 fip;
	u16 fcs;
	u16 rsvd2;
	u32 fdp;
	u16 fds;
	u16 rsvd3;
	u32 mxcsr;
	u32 mxcsr_mask;
	long double fpregs[8];
	sse_xmm xmm_regs[8];
} __attribute__((packed)) fxsave_t;

typedef struct fxsave_64bit_struct {
	u16 fcw;
	u16 fsw;
	u8  ftw;
	u8  revd1;
	u16 fop;
	u64 fip;
	u64 fdp;
	u32 mxcsr;
	u32 mxcsr_mask;
	long double fpregs[8];
	sse_xmm xmm_regs[8];
} __attribute__((packed)) fxsave_64bit_t;

volatile bool ret_ring1 = false;
int do_at_ring1(void (*fn)(void), const char *arg)
{
	static unsigned char user_stack[4096];
	int ret;

	asm volatile ("mov %[user_ds], %%" R "dx\n\t"
				  "mov %%dx, %%ds\n\t"
				  "mov %%dx, %%es\n\t"
				  "mov %%dx, %%fs\n\t"
				  "mov %%dx, %%gs\n\t"
				  "mov %%" R "sp, %%" R "cx\n\t"
				  "push" W " %%" R "dx \n\t"
				  "lea %[user_stack_top], %%" R "dx \n\t"
				  "push" W " %%" R "dx \n\t"
				  "pushf" W "\n\t"
				  "push" W " %[user_cs] \n\t"
				  "push" W " $1f \n\t"
				  "iret" W "\n"
				  "1: \n\t"
				  "push %%" R "cx\n\t"   /* save kernel SP */

#ifndef __x86_64__
				  "push %[arg]\n\t"
#endif
				  "call *%[fn]\n\t"
#ifndef __x86_64__
				  "pop %%ecx\n\t"
#endif

				  "pop %%" R "cx\n\t"
				  "mov $1f, %%" R "dx\n\t"
				  "int %[kernel_entry_vector]\n\t"
				  ".section .text.entry \n\t"
				  "kernel_entry1: \n\t"
				  "mov %%" R "cx, %%" R "sp \n\t"
				  "mov %[kernel_ds], %%cx\n\t"
				  "mov %%cx, %%ds\n\t"
				  "mov %%cx, %%es\n\t"
				  "mov %%cx, %%fs\n\t"
				  "mov %%cx, %%gs\n\t"
				  "jmp *%%" R "dx \n\t"
				  ".section .text\n\t"
				  "1:\n\t"
				  : [ret] "=&a" (ret)
				  : [user_ds] "i" (OSSERVISE1_DS),
				  [user_cs] "i" (OSSERVISE1_CS32),
				  [user_stack_top]"m"(user_stack[sizeof user_stack]),
				  [fn]"r"(fn),
				  [arg]"D"(arg),
				  [kernel_ds]"i"(KERNEL_DS),
				  [kernel_entry_vector]"i"(0x21)
				  : "rcx", "rdx");
	return ret;
}

int do_at_ring2(void (*fn)(void), const char *arg)
{
	static unsigned char user_stack[4096];
	int ret;

	asm volatile ("mov %[user_ds], %%" R "dx\n\t"
				  "mov %%dx, %%ds\n\t"
				  "mov %%dx, %%es\n\t"
				  "mov %%dx, %%fs\n\t"
				  "mov %%dx, %%gs\n\t"
				  "mov %%" R "sp, %%" R "cx\n\t"
				  "push" W " %%" R "dx \n\t"
				  "lea %[user_stack_top], %%" R "dx \n\t"
				  "push" W " %%" R "dx \n\t"
				  "pushf" W "\n\t"
				  "push" W " %[user_cs] \n\t"
				  "push" W " $1f \n\t"
				  "iret" W "\n"
				  "1: \n\t"
				  "push %%" R "cx\n\t"   /* save kernel SP */

#ifndef __x86_64__
				  "push %[arg]\n\t"
#endif
				  "call *%[fn]\n\t"
#ifndef __x86_64__
				  "pop %%ecx\n\t"
#endif

				  "pop %%" R "cx\n\t"
				  "mov $1f, %%" R "dx\n\t"
				  "int %[kernel_entry_vector]\n\t"
				  ".section .text.entry \n\t"
				  "kernel_entry2: \n\t"
				  "mov %%" R "cx, %%" R "sp \n\t"
				  "mov %[kernel_ds], %%cx\n\t"
				  "mov %%cx, %%ds\n\t"
				  "mov %%cx, %%es\n\t"
				  "mov %%cx, %%fs\n\t"
				  "mov %%cx, %%gs\n\t"
				  "jmp *%%" R "dx \n\t"
				  ".section .text\n\t"
				  "1:\n\t"
				  : [ret] "=&a" (ret)
				  : [user_ds] "i" (OSSERVISE2_DS),
				  [user_cs] "i" (OSSERVISE2_CS32),
				  [user_stack_top]"m"(user_stack[sizeof user_stack]),
				  [fn]"r"(fn),
				  [arg]"D"(arg),
				  [kernel_ds]"i"(KERNEL_DS),
				  [kernel_entry_vector]"i"(0x22)
				  : "rcx", "rdx");
	return ret;
}
volatile bool ret_ring3 = false;
int do_at_ring3(void (*fn)(void), const char *arg)
{
	static unsigned char user_stack[4096];
	int ret;

	asm volatile ("mov %[user_ds], %%" R "dx\n\t"
				  "mov %%dx, %%ds\n\t"
				  "mov %%dx, %%es\n\t"
				  "mov %%dx, %%fs\n\t"
				  "mov %%dx, %%gs\n\t"
				  "mov %%" R "sp, %%" R "cx\n\t"
				  "push" W " %%" R "dx \n\t"
				  "lea %[user_stack_top], %%" R "dx \n\t"
				  "push" W " %%" R "dx \n\t"
				  "pushf" W "\n\t"
				  "push" W " %[user_cs] \n\t"
				  "push" W " $1f \n\t"
				  "iret" W "\n"
				  "1: \n\t"
				  /* save kernel SP */
				  "push %%" R "cx\n\t"

#ifndef __x86_64__
				  "push %[arg]\n\t"
#endif
				  "call *%[fn]\n\t"
#ifndef __x86_64__
				  "pop %%ecx\n\t"
#endif

				  "pop %%" R "cx\n\t"
				  "mov $1f, %%" R "dx\n\t"
				  "int %[kernel_entry_vector]\n\t"
				  ".section .text.entry \n\t"
				  "kernel_entry: \n\t"
				  "mov %%" R "cx, %%" R "sp \n\t"
				  "mov %[kernel_ds], %%cx\n\t"
				  "mov %%cx, %%ds\n\t"
				  "mov %%cx, %%es\n\t"
				  "mov %%cx, %%fs\n\t"
				  "mov %%cx, %%gs\n\t"
				  "jmp *%%" R "dx \n\t"
				  ".section .text\n\t"
				  "1:\n\t"
				  : [ret] "=&a" (ret)
				  : [user_ds] "i" (USER_DS),
				  [user_cs] "i" (USER_CS),
				  [user_stack_top]"m"(user_stack[sizeof user_stack]),
				  [fn]"r"(fn),
				  [arg]"D"(arg),
				  [kernel_ds]"i"(KERNEL_DS),
				  [kernel_entry_vector]"i"(0x23)
				  : "rcx", "rdx");
	return ret;
}

#ifdef __i386__
void save_unchanged_reg()
{
	asm volatile ("mov %cr0,%eax");
}
#elif __x86_64__
void fpu_st_reg_unchanged_on_ap()
{
	float m = 1.1;
	ulong cr0;

	volatile struct fxsave_struct *fpu_save;
	fpu_save = (volatile struct fxsave_struct *)INIT_UNCHANGED_FPU_SAVE_ADDR;

	cr0 = read_cr0();
	write_cr0(cr0 | ~0xcU);/*clear TS EM*/

	asm volatile("fninit");
	/*load 1.1 to st7-st0 to change st7-st0 different*/
	for (int i = 0; i < 8; i++) {
		asm volatile("fld %0\n\t" : : "m"(m) : "memory");
	}
	asm volatile("fxsave %0" : "=m"(*fpu_save));

	write_cr0(cr0);
}

void save_unchanged_reg()
{
	fpu_st_reg_unchanged_on_ap();
}
#endif
#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name: fpu shall expose deprecated fpu cs ds 001
 *
 *Summary:
 *  FSAVE/FNSAVE, FSTENV/FNSTENV
 *	If CR0.PE = 1, each instruction saves FCS and FDS into memory. If
 *	CPUID.(EAX=07H,ECX=0H):EBX[bit 13] = 1, the processor deprecates FCS and FDS;
 *	it saves each as 0000H.
 *
 *	Under protect mode, excute FSAVE dump registers FDS and FCS value shall be 0000H
 */
#ifdef __i386__
static void fpu_rqmid_32375_shall_expose_deprecated_cs_ds_001()
{
	u32 cr0;
	u8 fsave[96] = {0};

	cr0 = read_cr0();
	write_cr0(cr0 | ~0xc);/*clear TS EM*/

	asm volatile("fsave %0 \n\t"
				 : "=m"(fsave) : : "memory");

	/*fcs locate byte[12,13];fds locate byte[20,21]*/
	if ((cpuid(0x7).b & (1 << 13))
		&& (fsave[12] == 0) && (fsave[13] == 0)
		&& (fsave[20] == 0) && (fsave[21] == 0)) {
		report("%s", 1, __FUNCTION__);
	} else {
		report("%s", 0, __FUNCTION__);
	}

}
/*
 * @brief case name: FPU execution environment_Protected Mode_FADD_#PF_001
 *
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 * Under 64 bit Mode ,
 * Rebulid the paging structure, to create the page fault(pgfault: occur),? executing FADD shall generate #PF .
 */
static void fpu_rqmid_31656_execution_environment_protected_mode_FADD_pf_001()
{
	ulong cr0;
	u32 *op1;

	cr0 = read_cr0();
	write_cr0(cr0 | ~0xcUL);/*clear TS EM*/

	op1 = malloc(sizeof(u32));
	set_page_control_bit((void *)op1, PAGE_PTE, PAGE_P_FLAG, 0, true);

	asm volatile(
		ASM_TRY("1f")
		"fadd %0\n\t"
		"1:"
		: : "m"(*op1) :);
	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);

	write_cr0(cr0);
}
#endif
#ifdef __x86_64__
/*
 * @brief case name: x87 FPU Data Operand and Inst. Pointers states following INIT_001
 *
 *Summary:
 *	 register                      power up         reset                   init
 *	x87 FPU Data Operand
 *	and Inst. Pointers5            00000000H     00000000H         FINIT/FNINIT: 00000000H
 *
 *	Get X87 FPU Data Operand and Inst. Pointers register value at AP init,
 *the value shall be 00000000H and same with SDM definition.
 *
 */
static void fpu_rqmid_32361_x87_FPU_data_operand_and_Inst_pointer_states_following_INIT_001()
{
	struct fxsave_struct *fpu_save;

	fpu_save = (struct fxsave_struct *)INIT_FPU_SAVE_ADDR;
	if (((fpu_save->fip & 0xffffU) == 0) && (fpu_save->fcs == 0)
		&& ((fpu_save->fdp & 0xffffU) == 0) && (fpu_save->fds == 0)) {

		report("%s", 1, __FUNCTION__);
	} else {
		report("%s", 0, __FUNCTION__);
	}
}

/*
 * @brief case name: st0 through st7 states following INIT_001
 *
 *Summary:
 *	register			power up		reset					init
 *	ST0 through ST7     +0.0            +0.0                    FINIT/FNINIT: Unchanged
 *
 *  After AP receives first INIT, set the value of ST0 through ST7;
 *	Dump ST0 through ST7 value shall get the same value after second INIT.
 *
 *	Note:this case will change st0-st7 register value to INIT memory,so it should run after
 *			other INIT cases which will check st0-st7 init value;
 */
static void fpu_rqmid_32366_st0_through_st7_states_following_INIT_001()
{
	u8 *ptr;
	ulong cr0;
	long double fpregs[8];
	volatile struct fxsave_struct *fpu_save_init;
	volatile struct fxsave_struct *fpu_save_unchanged;

	cr0 = read_cr0();
	write_cr0(cr0 | ~0xcU);/*clear TS EM*/

	/*check INIT value fistly*/
	fpu_save_init = (struct fxsave_struct *)INIT_FPU_SAVE_ADDR;
	ptr = (u8 *)fpu_save_init->fpregs;
	for (int i = 0; i < sizeof(fpu_save_init->fpregs); i++) {
		if (*(ptr + i) != 0) {
			report("%s", 0, __FUNCTION__);
			return;
		}
	}

	/*copy the changed value to buffer*/
	fpu_save_unchanged = (struct fxsave_struct *)INIT_UNCHANGED_FPU_SAVE_ADDR;
	memcpy(fpregs, (void *)fpu_save_unchanged->fpregs, sizeof(fpregs));

	/*send second sipi */
	send_sipi();

	/*compare changed fpregs value with second INIT fpreg value,the result should be equal*/
	if (memcmp(fpregs, (void *)fpu_save_init->fpregs, sizeof(fpregs))) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	write_cr0(cr0);
	report("%s", 1, __FUNCTION__);
}

/*
 * @brief case name: fpu execution environment FDISI 001
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 *	Execute FDISI instruction shall have N/A,
 * and the CRx and related registers listed below should be unchanged
 *
 *	R0.PE [bit 0],CR0.NE [bit 5],CR0.EM [bit 2],CR0.MP [bit 1]CR0.TS [bit 3],
 *CR4.OSFXSR [bit 9]Data Registers R0~R7,Control Register,Status Register,Tag Register,
 *Last instruction pointer register,Last data pointer register,Opcode register,should be unchanged
 *
 */
static void fpu_rqmid_32377_execution_environment_FDISI_001()
{
	bool ret = true;
	ulong cr0_1, cr4_1, cr0_2, cr4_2;
	__attribute__ ((aligned(16))) struct fxsave_64bit_struct fxsave_64bit_1, fxsave_64bit_2;

	cr0_1 = read_cr0();
	cr4_1 = read_cr4();
	asm volatile("fxsave %0":"=m"(fxsave_64bit_1));

	asm volatile("FDISI\n\t");

	cr0_2 = read_cr0();
	cr4_2 = read_cr4();
	asm volatile("fxsave %0":"=m"(fxsave_64bit_2));

	/*0x2F=10 1111:bit0,bit1,bit2,bit3,bit5*/
	if ((cr0_1 & 0x2FUL) != (cr0_2 & 0x2FUL)) {
		ret = false;
	}

	if ((cr4_1 & (1UL << 9)) != (cr4_2 & (1UL << 9))) {
		ret = false;
	}

	if (memcmp(fxsave_64bit_1.fpregs, fxsave_64bit_2.fpregs, 128U)) {
		ret = false;
	}

	if ((fxsave_64bit_1.fcw != fxsave_64bit_2.fcw)
		|| (fxsave_64bit_1.fsw != fxsave_64bit_2.fsw)
		|| (fxsave_64bit_1.ftw != fxsave_64bit_2.ftw)
		|| (fxsave_64bit_1.fop != fxsave_64bit_2.fop)
		|| (fxsave_64bit_1.fip != fxsave_64bit_2.fip)
		|| (fxsave_64bit_1.fdp != fxsave_64bit_2.fdp)) {
		ret = false;
	}

	report("%s", ret, __FUNCTION__);
}
/*
 * @brief case name: FPU capability enumeration_001
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 *	execute CPUID(1):EDX.FPU and CPUID(1):EDX.FXSR shall be 1
 */
static void fpu_rqmid_32378_FPU_capability_enumeration_001()
{
	/*fpu:edx.bit[0];fxsr:edx.bit[24]*/
	report("%s", (cpuid(0x1).d & (1)) && (cpuid(0x1).d & (1 << 24)), __FUNCTION__);
}
/*
 * @brief case name: CR0.MP state following start-up_001
 *
 *Summary:
 *	regitster   power up     reset			init
 *	CR0    60000010H		60000010H       60000010H
 *
 *	Get CR0.MP:cr0.bit[1] at BP start-up, the bit shall be 0 and same with SDM definition.
 */
static void fpu_rqmid_32381_cr0_mp_state_following_startup_001()
{
	report("%s", (*(u32 *)STARTUP_CR0_REG_ADDR & 0x2) == 0, __FUNCTION__);
}
/*
 * @brief case name: CR0.MP state following start-up_001
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 *	Under 64 bit Mode ,  
 * Rebulid the paging structure, to create the page fault(pgfault: occur),executing FICOMP shall generate #PF ..
 */
static void fpu_rqmid_31189_execution_environment_64_bit_Mode_FICOMP_PF_001()
{
	u32 cr0;
	u16 *op1;

	cr0 = read_cr0();
	write_cr0(cr0 | ~0xc);/*clear TS EM*/

	op1 = malloc(sizeof(u16));
	set_page_control_bit((void *)op1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	asm volatile(ASM_TRY("1f")
				 "ficomp %0\n\t"
				 "1:"
				 : : "m"(*op1) :);

	report("%s", exception_vector() == PF_VECTOR, __FUNCTION__);
	write_cr0(cr0);
}

/*
 * @brief case name: FPU execution environment_64 bit Mode_FIST_#GP_001
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 * Under 64 bit Mode and CPL0 ,
 * If the memory address is in a non-canonical form(Ad. Cann.: non mem),  executing FIST shall generate #GP.
 */
static void fpu_rqmid_31436_execution_environment_64_bit_Mode_FIST_GP_001()
{
	u32 cr0;
	u16 *op1;

	if ((read_cs() & 0x3) != 0) {

		report("%s \t should under ring0", 0, __FUNCTION__);
		return;
	}

	cr0 = read_cr0();
	write_cr0(cr0 | ~0xc);/*clear TS EM*/

	op1 = malloc(sizeof(u16));
	op1 = (u16 *)((ulong)(op1) ^ (1ULL << 63)); /*make non-canonical address*/
	asm volatile(ASM_TRY("1f")
				 "fist %0\n\t"
				 "1:"
				 : : "m"(*op1) :);

	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
	write_cr0(cr0);
}
static void fpu_64_bit_mode_FILD_at_ring1()
{
	u8 level;
	float op1 = 1.2;
	int op2 = 0;
	unsigned short cw;

	level = read_cs() & 0x3;

	cw = 0x37b;/*bit2==0,report div0 exception*/
	asm volatile("fninit;fldcw %0;fld %1\n"
				 "fdiv %2\n"
				 ASM_TRY("1f")
				 "FILD %2\n"
				 "1:"
				 : : "m"(cw), "m"(op1), "m"(op2));

	ret_ring1 = (exception_vector() == MF_VECTOR) && (level == 1);
	asm volatile("fninit");
}
/*
 * @brief case name: FPU execution environment_64 bit Mode_FILD_#MF_002
 *
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 * Under 64 bit Mode and CPL1 ,
 * If there is a pending x87 FPU exception(FPU excp: hold), executing FILD shall generate #MF .
 */
static void fpu_rqmid_31097_execution_environment_64_bit_Mode_FILD_MF_002()
{
	u32 cr0;

	cr0 = read_cr0();
	write_cr0(cr0 | ~0xc);/*clear TS EM*/
	write_cr0(read_cr0() | (1 << 5));/*set cr0.ne*/

	do_at_ring1(fpu_64_bit_mode_FILD_at_ring1, "");
	report("%s", ret_ring1,  __FUNCTION__);

	//asm volatile("fninit");/*init the FPU exception status*/
	ret_ring1 = false;/*reset the ring1 result*/
	write_cr0(cr0);
}
static volatile bool fpu_64bit_ss = false;
static void fpu_64bit_ss_handler(struct ex_regs *regs)
{
	fpu_64bit_ss = true;
}
static void fpu_64bit_mode_fsave_ss_at_ring3()
{
	u8 level;
	ulong  op1;
	handler old;
	u8 fsave[94] = {0};

	level = read_cs() & 0x3;

	old = handle_exception(SS_VECTOR, fpu_64bit_ss_handler);
	op1 = (ulong)fsave;
	op1 ^= (1UL << 63);/*make non-cannoical addr*/
	asm volatile(
		"fsave %0 \n\t"
		"1:"
		: "=m"(*(ulong *)op1)::);

	ret_ring3 = fpu_64bit_ss && (level == 3);
	fpu_64bit_ss = false;
	handle_exception(SS_VECTOR, old);
}
/*
 * @brief case name: FPU execution environment_64 bit Mode_FSAVE_#SS_010
 *
 *
 *Summary:
 *  ACRN hypervisor shall expose FPU execution environment to any guest,
 *	in compliance with Chapter 8.1 and 8.3, Vol. 1, SDM and Chapter 2.5, Vol.3, SDM.
 *
 * Under 64 bit Mode and CPL3 ,
 * If a memory address referencing the SS segment is in a non-canonical form(Ad. Cann.: non stack),
 * executing FSAVE shall generate #SS . .
 */
static void fpu_rqmid_31551_execution_environment_64_bit_Mode_FSAVE_SS_010()
{
	u32 cr0;
	ulong eflags;

	cr0 = read_cr0();
	write_cr0(cr0 | ~0xc);/*clear EM TS*/

	/*disable eflags AC check eflags[bit18]*/
	asm volatile(
		"pushf\n\t"
		"pop %%" R "ax\n\t"
		"mov %%" R "ax,%0 \n\t"
		"and $~(1<<18),%%" R "ax\n\t"
		"push %%" R "ax\n\t"
		"popf\n\t"
		: "=m"(eflags) : :);


	do_at_ring3(fpu_64bit_mode_fsave_ss_at_ring3, "");
	report("%s", ret_ring3,  __FUNCTION__);
	/*restore flags back*/
	asm volatile(
		"mov %0, %%eax \n\t"
		"push %%" R "ax \n\t"
		"popf \n\t"
		: : "m"(eflags) :);

	ret_ring3 = false;/*reset the ring3 result*/
	write_cr0(cr0);
}
#endif
#endif
/*
 *	@brief setup_ring
 *	Summary:setup ring environment:gdt entry and idt
 *		which will be used for CPU  switching to ring1, ring2,ring3
 */
void setup_ring_env()
{
	/*for setup ring1 ring2 ring3 environment*/
	extern unsigned char kernel_entry1;
	set_idt_entry(0x21, &kernel_entry1, 1);
	extern unsigned char kernel_entry2;
	set_idt_entry(0x22, &kernel_entry2, 2);
	extern unsigned char kernel_entry;
	set_idt_entry(0x23, &kernel_entry, 3);

#ifdef __x86_64__
	extern gdt_entry_t gdt64[];
	*(u64 *)&gdt64[11] = RING1_CS64_GDT_DESC;
	*(u64 *)&gdt64[12] = RING1_DS_GDT_DESC;
	*(u64 *)&gdt64[13] = RING2_CS64_GDT_DESC;
	*(u64 *)&gdt64[14] = RING2_DS_GDT_DESC;
#elif __i386__
	extern gdt_entry_t gdt32[];
	*(u64 *)&gdt32[11] = RING1_CS32_GDT_DESC;
	*(u64 *)&gdt32[12] = RING1_DS_GDT_DESC;
	*(u64 *)&gdt32[13] = RING2_CS32_GDT_DESC;
	*(u64 *)&gdt32[14] = RING2_DS_GDT_DESC;
#endif

}
static void print_case_list(void)
{
	printf("machine check feature case list:\n\r");
#ifdef IN_NON_SAFETY_VM
#ifdef __i386__
	printf("\t Case ID:%d case name:%s\n\r", 32375, "shall expose deprecated cs ds 001");
	printf("\t Case ID:%d case name:%s\n\r", 31656, "execution environment protected mode FADD pf 001()");
#endif
#ifdef __x86_64__
	printf("\t Case ID:%d case name:%s\n\r", 32361,
		   "x87 FPU data operand and Inst pointer states following INIT 001");
	printf("\t Case ID:%d case name:%s\n\r", 32366,
		   "st0 through st7 states following INIT 001();");
	printf("\t Case ID:%d case name:%s\n\r", 32377,
		   "execution environment FDISI 001");
	printf("\t Case ID:%d case name:%s\n\r", 32378,
		   "FPU capability enumeration 001");
	printf("\t Case ID:%d case name:%s\n\r", 32381,
		   "cr0 mp state following startup 001");
	printf("\t Case ID:%d case name:%s\n\r", 31189,
		   "execution environment 64 bit Mode FICOMP PF 001");
	printf("\t Case ID:%d case name:%s\n\r", 31436,
		   "execution environment 64 bit Mode FIST GP 001");
	printf("\t Case ID:%d case name:%s\n\r", 31097,
		   "execution environment 64 bit Mode FIST MF 002");
	printf("\t Case ID:%d case name:%s\n\r", 31551,
		   "execution environment 64 bit Mode FSAVE SS 010");
#endif
#endif
}

int main(void)
{
	setup_idt();
	setup_vm();
	setup_ring_env();
	print_case_list();
#ifdef IN_NON_SAFETY_VM
#ifdef __i386__
	fpu_rqmid_32375_shall_expose_deprecated_cs_ds_001();
	fpu_rqmid_31656_execution_environment_protected_mode_FADD_pf_001();
#endif
#ifdef __x86_64__
	fpu_rqmid_32361_x87_FPU_data_operand_and_Inst_pointer_states_following_INIT_001();
	fpu_rqmid_32366_st0_through_st7_states_following_INIT_001();
	fpu_rqmid_32377_execution_environment_FDISI_001();
	fpu_rqmid_32378_FPU_capability_enumeration_001();
	fpu_rqmid_32381_cr0_mp_state_following_startup_001();
	fpu_rqmid_31189_execution_environment_64_bit_Mode_FICOMP_PF_001();
	fpu_rqmid_31436_execution_environment_64_bit_Mode_FIST_GP_001();
	fpu_rqmid_31097_execution_environment_64_bit_Mode_FILD_MF_002();
	fpu_rqmid_31551_execution_environment_64_bit_Mode_FSAVE_SS_010();
#endif
#endif
	return report_summary();
}

