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
	write_cr0(cr0 & ~0xc);/*clear TS EM*/

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
#ifdef __x86_64__
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
	write_cr0(cr0 & ~0xc);/*clear TS EM*/

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
	write_cr0(cr0 & ~0xc);/*clear TS EM*/

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
	write_cr0(cr0 & ~0xc);/*clear TS EM*/

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
	write_cr0(cr0 & ~0xc);/*clear TS EM*/

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
	write_cr0(cr0 & ~0xc);/*clear TS EM*/

	op1 = malloc(sizeof(u16));
	op1 = (u16 *)((ulong)(op1) ^ (1ULL << 63)); /*make non-canonical address*/
	asm volatile(ASM_TRY("1f")
				 "fist %0\n\t"
				 "1:"
				 : : "m"(*op1) :);

	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
	write_cr0(cr0);
}
static void fpu_64_bit_mode_FILD_at_ring1(const char *msg)
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

	ring1_ret = (exception_vector() == MF_VECTOR) && (level == 1);
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
static void fpu_rqmid_31907_execution_environment_64_bit_Mode_FILD_MF_002()
{
	u32 cr0;

	cr0 = read_cr0();
	write_cr0(cr0 & ~0xc);/*clear TS EM*/
	write_cr0(read_cr0() | (1 << 5));/*set cr0.ne*/

	do_at_ring1(fpu_64_bit_mode_FILD_at_ring1, "");
	report("%s", ring1_ret,  __FUNCTION__);

	asm volatile("fninit");/*init the FPU exception status*/
	ring1_ret = false;/*reset the ring1 result*/
	write_cr0(cr0);
}
static void fpu_64bit_ss_handler(struct ex_regs *regs)
{
	unsigned ex_val;
	extern unsigned char fsave_ss_ret;
	ex_val = regs->vector | (regs->error_code << 16) |
			 (((regs->rflags >> 16) & 1) << 8);
	asm("mov %0, %%gs:4" : : "r"(ex_val));

	regs->rip = (unsigned long)&fsave_ss_ret;
}
static void fpu_64bit_mode_fsave_ss_at_ring3(const char *msg)
{
	u8 level;
	ulong  op1;
	handler old;
	u8 fsave[94] = {0};

	old = handle_exception(SS_VECTOR, fpu_64bit_ss_handler);
	op1 = (ulong)fsave;
	op1 ^= (1UL << 63);/*make non-cannoical addr*/
	asm volatile(
		"fsave %0 \n\t"
		"fsave_ss_ret:"
		: "=m"(*(ulong *)op1));

	level = read_cs() & 0x3;
	ring3_ret = (exception_vector() == SS_VECTOR) && (level == 3);
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
	write_cr0(cr0 & ~0xc);/*clear EM TS*/

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
	report("%s", ring3_ret,  __FUNCTION__);
	/*restore flags back*/
	asm volatile(
		"mov %0, %%eax \n\t"
		"push %%" R "ax \n\t"
		"popf \n\t"
		: : "m"(eflags) :);

	ring3_ret = false;/*reset the ring3 result*/
	write_cr0(cr0);
}
#endif
#endif
static void print_case_list(void)
{
	printf("FPU feature case list:\n\r");
#ifdef __x86_64__
	printf("\t Case ID:%d case name:%s\n\r", 32381,
		   "cr0 mp state following startup 001");
#endif
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
	printf("\t Case ID:%d case name:%s\n\r", 31189,
		   "execution environment 64 bit Mode FICOMP PF 001");
	printf("\t Case ID:%d case name:%s\n\r", 31436,
		   "execution environment 64 bit Mode FIST GP 001");
	printf("\t Case ID:%d case name:%s\n\r", 31907,
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
#ifdef __x86_64__
	fpu_rqmid_32381_cr0_mp_state_following_startup_001();
#endif
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
	fpu_rqmid_31189_execution_environment_64_bit_Mode_FICOMP_PF_001();
	fpu_rqmid_31436_execution_environment_64_bit_Mode_FIST_GP_001();
	fpu_rqmid_31907_execution_environment_64_bit_Mode_FILD_MF_002();
	fpu_rqmid_31551_execution_environment_64_bit_Mode_FSAVE_SS_010();
#endif
#endif
	return report_summary();
}

