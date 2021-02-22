/*
 * Test for x86 HSI memory management instructions
 *
 * Copyright (c) 2021 Intel
 *
 * Authors:
 *  HeXiang Li <hexiangx.li@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 * Test vmx feature.
 */
#define USE_DEBUG
#include "processor.h"
#include "libcflat.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "apic.h"
#include "isr.h"
#include "atomic.h"
#include "asm/spinlock.h"
#include "asm/io.h"
#include "fwcfg.h"
#include <linux/pci_regs.h>
#include "alloc_page.h"
#include "alloc_phys.h"
#include "asm/page.h"
#include "x86/vm.h"
#include "xsave.h"
#include "segmentation.h"
#include "interrupt.h"
#include "instruction_common.h"
#include "memory_type.h"
#include "debug_print.h"
#include "paging.h"

#include "vm.h"
#include "hsi_virtual_spec.h"
#include "msr.h"
#include "smp.h"
#include "pci.h"
#include "types.h"
#include "vmalloc.h"
#include "delay.h"
#include "pci_log.h"

#if defined(IN_NATIVE)
void *guest_stack, *guest_syscall_stack;

#include "hsi_vmx_32bit.c"
#include "hsi_vmx_exit_entry_contrl.c"
#include "hsi_vmx_exe_contrl0.c"
#include "hsi_vmx_exe_contrl1.c"
#include "hsi_vmx_general2.c"
#include "hsi_vmx_general.c"
struct st_vcpu vcpu_bp;

u64 *vmxon_region;
struct vmcs *vmcs_root;
u32 vpid_cnt;
u32 ctrl_pin, ctrl_enter, ctrl_exit, ctrl_cpu[2];
struct regs regs;

/* accroding SDM general regiset order:
 * For MOV CR, the general-purpose register:
 * 0 = RAX
 * 1 = RCX
 * 2 = RDX
 * 3 = RBX
 * 4 = RSP
 * 5 = RBP
 * 6 = RSI
 * 7 = RDI
 * 8–15 represent R8–R15, respectively
 * (used only on processors that support Intel 64 architecture)
 * For CLTS and LMSW, cleared to 0
 */
static u64 *gp_reg[] = {
	&regs.rax,
	&regs.rcx,
	&regs.rdx,
	&regs.rbx,
	&regs.rsp,
	&regs.rbp,
	&regs.rsi,
	&regs.rdi,
	&regs.r8,
	&regs.r9,
	&regs.r10,
	&regs.r11,
	&regs.r12,
	&regs.r13,
	&regs.r14,
	&regs.r15,
};

struct vmx_test *current;

#define MAX_TEST_TEARDOWN_STEPS 10

struct test_teardown_step {
	test_teardown_func func;
	void *data;
};

static int teardown_count;
static struct test_teardown_step teardown_steps[MAX_TEST_TEARDOWN_STEPS];

static test_guest_func v2_guest_main;

u64 hypercall_field;
bool launched;
static int guest_finished;
static int in_guest;

union vmx_basic basic;
union vmx_ctrl_msr ctrl_pin_rev;
union vmx_ctrl_msr ctrl_cpu_rev[2];
union vmx_ctrl_msr ctrl_exit_rev;
union vmx_ctrl_msr ctrl_enter_rev;
union vmx_ept_vpid  ept_vpid;

extern struct descriptor_table_ptr gdt64_desc;
extern struct descriptor_table_ptr idt_descr;
extern struct descriptor_table_ptr tss_descr;
extern void *vmx_return;
extern void *entry_sysenter;
extern void *guest_entry;

static volatile u32 stage;

st_vm_exit_info vm_exit_info;

static jmp_buf abort_target;

struct vmcs_field {
	u64 mask;
	u64 encoding;
};

u64 vcpu_get_gpreg(u8 idx)
{
	DBG_INFO("idx:%d gp_reg addr:%lx value:%lx", idx, (u64)gp_reg[idx], *gp_reg[idx]);
	return *gp_reg[idx];
}

static u32 check_vmx_ctrl(u32 msr, u32 ctrl_req)
{
	uint64_t vmx_msr;
	u32 vmx_msr_low, vmx_msr_high;
	u32 ctrl = ctrl_req;

	vmx_msr = rdmsr(msr);
	vmx_msr_low = (u32)vmx_msr;
	vmx_msr_high = (u32)(vmx_msr >> 32U);

	/* high 32b: must 0 setting
	 * low 32b:  must 1 setting
	 */
	ctrl &= vmx_msr_high;
	ctrl |= vmx_msr_low;

	if ((ctrl_req & ~ctrl) != 0U) {
		DBG_ERRO("VMX ctrl 0x%x not fully enabled: "
		       "request 0x%x but get 0x%x\n",
			msr, ctrl_req, ctrl);
	}

	return ctrl;
}

/* init all execute control vmcs */
static void vm_exec_ctrl_init(void)
{
	u32 ctrl_pin = (u32)vmcs_read(PIN_CONTROLS);
	ctrl_pin &= ~(PIN_EXTINT | PIN_NMI | PIN_VIRT_NMI);

	/* init pin base control field */
	vmcs_write(PIN_CONTROLS, ctrl_pin);

	/* Set up primary processor based VM execution controls - pg 2900
	 * 24.6.2. Set up for:
	 * Enable TSC offsetting
	 * Enable TSC exiting
	 * guest access to IO bit-mapped ports causes VM exit
	 * guest access to MSR causes VM exit
	 * Activate secondary controls
	 */
	/* These are bits 1,4-6,8,13-16, and 26, the corresponding bits of
	 * the IA32_VMX_PROCBASED_CTRLS MSR are always read as 1 --- A.3.2
	 */
	u32 value32 = check_vmx_ctrl(MSR_IA32_VMX_PROCBASED_CTLS,
		CPU_RDTSCP | CPU_IO_BITMAP |
			CPU_MSR_BITMAP | CPU_SECONDARY);

	/*Disable VM_EXIT for CR3 access*/
	value32 &= ~(CPU_CR3_LOAD | CPU_CR3_STORE);
	value32 &= ~(CPU_CR8_LOAD | CPU_CR8_STORE);

	/*
	 * Disable VM_EXIT for invlpg execution.
	 */
	value32 &= ~CPU_INVLPG;

	/*
	 * Enable VM_EXIT for rdpmc execution.
	 */
	value32 |= CPU_RDPMC;

	/*
	 * Enable VM_EXIT for MWAITs and MONITOR.
	 */
	value32 |= CPU_MWAIT;
	value32 |= CPU_MONITOR;

	vmcs_write(CPU_EXEC_CTRL0, value32);

	value32 = (u32)vmcs_read(CPU_EXEC_CTRL1);

	/** Bitwise AND value32 by ~VMX_PROCBASED_CTLS2_XSVE_XRSTR */
	value32 &= ~CPU_XSAVES_XRSTORS;

	/** Bitwise OR value32 by VMX_PROCBASED_CTLS2_WBINVD */
	value32 |= CPU_WBINVD | CPU_RDTSCP | CPU_EPT | CPU_VPID | CPU_URG;
	vmcs_write(CPU_EXEC_CTRL1, value32);
}

void exec_vmwrite32_bit(u32 field, u32 bitmap, u32 is_bit_set)
{
	vm_exec_ctrl_init();
	u32 value = (u32)vmcs_read(field);
	if (is_bit_set == BIT_TYPE_SET) {
		value |= bitmap;
	} else {
		value &= ~bitmap;
	}
	vmcs_write(field, value);
}

void vmx_set_test_condition(u32 s)
{
	barrier();
	stage = s;
	barrier();
}

u32 vmx_get_test_condition(void)
{
	u32 s;

	barrier();
	s = stage;
	barrier();
	return s;
}

void vmx_inc_test_stage(void)
{
	barrier();
	stage++;
	barrier();
}

void set_exit_reason(u32 exit_reason)
{
	barrier();
	vm_exit_info.exit_reason = exit_reason;
	barrier();
}

u32 get_exit_reason(void)
{
	u32 reason;

	barrier();
	reason = vm_exit_info.exit_reason;
	barrier();
	return reason;
}

/* entry_sysenter */
asm(
	".align	4, 0x90\n\t"
	".globl	entry_sysenter\n\t"
	"entry_sysenter:\n\t"
	SAVE_GPR
	"	and	$0xf, %rax\n\t"
	"	mov	%rax, %rdi\n\t"
	"	call	syscall_handler\n\t"
	LOAD_GPR
	"	vmresume\n\t"
);

static void __attribute__((__used__)) syscall_handler(u64 syscall_no)
{
	if (current->syscall_handler) {
		current->syscall_handler(syscall_no);
	}
}

void print_vmexit_info()
{
	u64 guest_rip, guest_rsp;
	ulong reason = vmcs_read(EXI_REASON) & 0xff;
	ulong exit_qual = vmcs_read(EXI_QUALIFICATION);
	guest_rip = vmcs_read(GUEST_RIP);
	guest_rsp = vmcs_read(GUEST_RSP);
	printf("VMEXIT info:\n");
	printf("\tvmexit reason = %ld\n", reason);
	printf("\texit qualification = %#lx\n", exit_qual);
	printf("\tBit 31 of reason = %lx\n", (vmcs_read(EXI_REASON) >> 31) & 1);
	printf("\tguest_rip = %#lx\n", guest_rip);
	printf("\tRAX=%#lx    RBX=%#lx    RCX=%#lx    RDX=%#lx\n",
		regs.rax, regs.rbx, regs.rcx, regs.rdx);
	printf("\tRSP=%#lx    RBP=%#lx    RSI=%#lx    RDI=%#lx\n",
		guest_rsp, regs.rbp, regs.rsi, regs.rdi);
	printf("\tR8 =%#lx    R9 =%#lx    R10=%#lx    R11=%#lx\n",
		regs.r8, regs.r9, regs.r10, regs.r11);
	printf("\tR12=%#lx    R13=%#lx    R14=%#lx    R15=%#lx\n",
		regs.r12, regs.r13, regs.r14, regs.r15);
}

void
print_vmentry_failure_info(struct vmentry_failure *failure) {
	if (failure->early) {
		printf("Early %s failure: ", failure->instr);
		switch (failure->flags & VMX_ENTRY_FLAGS) {
		case X86_EFLAGS_CF:
			printf("current-VMCS pointer is not valid.\n");
			break;
		case X86_EFLAGS_ZF:
			printf("error number is %ld. See Intel 30.4.\n",
			       vmcs_read(VMX_INST_ERROR));
			break;
		default:
			printf("unexpected flags %lx!\n", failure->flags);
		}
	} else {
		u64 reason = vmcs_read(EXI_REASON);
		u64 qual = vmcs_read(EXI_QUALIFICATION);

		printf("Non-early %s failure (reason=%#lx, qual=%#lx): ",
			failure->instr, reason, qual);

		switch (reason & 0xff) {
		case VMX_FAIL_STATE:
			printf("invalid guest state\n");
			break;
		case VMX_FAIL_MSR:
			printf("MSR loading\n");
			break;
		case VMX_FAIL_MCHECK:
			printf("machine-check event\n");
			break;
		default:
			printf("unexpected basic exit reason %ld\n",
			       reason & 0xff);
		}

		if (!(reason & VMX_ENTRY_FAILURE)) {
			printf("\tVMX_ENTRY_FAILURE BIT NOT SET!\n");
		}
		if (reason & 0x7fff0000) {
			printf("\tRESERVED BITS SET!\n");
		}
	}
}

static void __attribute__((__used__)) guest_main(void)
{
	if (current->v2) {
		v2_guest_main();
	} else {
		current->guest_main();
	}
}

/* guest_entry */
asm(
	".align	4, 0x90\n\t"
	".globl	entry_guest\n\t"
	"guest_entry:\n\t"
	"	call guest_main\n\t"
	"	mov $1, %edi\n\t"
	"	call hypercall\n\t"
);

static void init_vmcs_ctrl(void)
{
	/* 26.2 CHECKS ON VMX CONTROLS AND HOST-STATE AREA */
	/* 26.2.1.1 */
	vmcs_write(PIN_CONTROLS, ctrl_pin);
	/* Disable VMEXIT of IO instruction */
	vmcs_write(CPU_EXEC_CTRL0, ctrl_cpu[0]);
	if (ctrl_cpu_rev[0].set & CPU_SECONDARY) {
		ctrl_cpu[1] = (ctrl_cpu[1] | ctrl_cpu_rev[1].set) &
			ctrl_cpu_rev[1].clr;
		vmcs_write(CPU_EXEC_CTRL1, ctrl_cpu[1]);
	}
	vmcs_write(CR3_TARGET_COUNT, 0);
	vmcs_write(VPID, ++vpid_cnt);
}

static void init_vmcs_host(void)
{
	/* 26.2 CHECKS ON VMX CONTROLS AND HOST-STATE AREA */
	/* 26.2.1.2 */
	vmcs_write(HOST_EFER, rdmsr(MSR_EFER));

	/* 26.2.1.3 */
	vmcs_write(ENT_CONTROLS, ctrl_enter);
	vmcs_write(EXI_CONTROLS, ctrl_exit);

	/* 26.2.2 */
	vmcs_write(HOST_CR0, read_cr0());
	vmcs_write(HOST_CR3, read_cr3());
	vmcs_write(HOST_CR4, read_cr4());
	vmcs_write(HOST_SYSENTER_EIP, (u64)(&entry_sysenter));
	vmcs_write(HOST_SYSENTER_CS,  KERNEL_CS);

	/* 26.2.3 */
	vmcs_write(HOST_SEL_CS, KERNEL_CS);
	vmcs_write(HOST_SEL_SS, KERNEL_DS);
	vmcs_write(HOST_SEL_DS, KERNEL_DS);
	vmcs_write(HOST_SEL_ES, KERNEL_DS);
	vmcs_write(HOST_SEL_FS, KERNEL_DS);
	vmcs_write(HOST_SEL_GS, KERNEL_DS);
	vmcs_write(HOST_SEL_TR, TSS_MAIN);
	vmcs_write(HOST_BASE_TR, tss_descr.base);
	vmcs_write(HOST_BASE_GDTR, gdt64_desc.base);
	vmcs_write(HOST_BASE_IDTR, idt_descr.base);
	vmcs_write(HOST_BASE_FS, 0);
	vmcs_write(HOST_BASE_GS, 0);

	/* Set other vmcs area */
	vmcs_write(PF_ERROR_MASK, 0);
	vmcs_write(PF_ERROR_MATCH, 0);
	vmcs_write(VMCS_LINK_PTR, ~0ul);
	vmcs_write(VMCS_LINK_PTR_HI, ~0ul);
	vmcs_write(HOST_RIP, (u64)(&vmx_return));
}

static void init_vmcs_guest(void)
{
	/* 26.3 CHECKING AND LOADING GUEST STATE */
	ulong guest_cr0, guest_cr4, guest_cr3;
	/* 26.3.1.1 */
	guest_cr0 = read_cr0();
	guest_cr4 = read_cr4();
	guest_cr3 = read_cr3();
	if (ctrl_enter & ENT_GUEST_64) {
		guest_cr0 |= X86_CR0_PG;
		guest_cr4 |= X86_CR4_PAE;
	}
	if ((ctrl_enter & ENT_GUEST_64) == 0) {
		guest_cr4 &= (~X86_CR4_PCIDE);
	}
	if (guest_cr0 & X86_CR0_PG) {
		guest_cr0 |= X86_CR0_PE;
	}
	vmcs_write(GUEST_CR0, guest_cr0);
	vmcs_write(GUEST_CR3, guest_cr3);
	vmcs_write(GUEST_CR4, guest_cr4);
	vmcs_write(GUEST_SYSENTER_CS,  KERNEL_CS);
	vmcs_write(GUEST_SYSENTER_ESP,
		(u64)(guest_syscall_stack + PAGE_SIZE - 1));
	vmcs_write(GUEST_SYSENTER_EIP, (u64)(&entry_sysenter));
	vmcs_write(GUEST_DR7, 0);
	vmcs_write(GUEST_EFER, rdmsr(MSR_EFER));

	/* 26.3.1.2 */
	vmcs_write(GUEST_SEL_CS, KERNEL_CS);
	vmcs_write(GUEST_SEL_SS, KERNEL_DS);
	vmcs_write(GUEST_SEL_DS, KERNEL_DS);
	vmcs_write(GUEST_SEL_ES, KERNEL_DS);
	vmcs_write(GUEST_SEL_FS, KERNEL_DS);
	vmcs_write(GUEST_SEL_GS, KERNEL_DS);
	vmcs_write(GUEST_SEL_TR, TSS_MAIN);
	vmcs_write(GUEST_SEL_LDTR, 0);

	vmcs_write(GUEST_BASE_CS, 0);
	vmcs_write(GUEST_BASE_ES, 0);
	vmcs_write(GUEST_BASE_SS, 0);
	vmcs_write(GUEST_BASE_DS, 0);
	vmcs_write(GUEST_BASE_FS, 0);
	vmcs_write(GUEST_BASE_GS, 0);
	vmcs_write(GUEST_BASE_TR, tss_descr.base);
	vmcs_write(GUEST_BASE_LDTR, 0);

	vmcs_write(GUEST_LIMIT_CS, 0xFFFFFFFF);
	vmcs_write(GUEST_LIMIT_DS, 0xFFFFFFFF);
	vmcs_write(GUEST_LIMIT_ES, 0xFFFFFFFF);
	vmcs_write(GUEST_LIMIT_SS, 0xFFFFFFFF);
	vmcs_write(GUEST_LIMIT_FS, 0xFFFFFFFF);
	vmcs_write(GUEST_LIMIT_GS, 0xFFFFFFFF);
	vmcs_write(GUEST_LIMIT_LDTR, 0xffff);
	vmcs_write(GUEST_LIMIT_TR, tss_descr.limit);

	vmcs_write(GUEST_AR_CS, 0xa09b);
	vmcs_write(GUEST_AR_DS, 0xc093);
	vmcs_write(GUEST_AR_ES, 0xc093);
	vmcs_write(GUEST_AR_FS, 0xc093);
	vmcs_write(GUEST_AR_GS, 0xc093);
	vmcs_write(GUEST_AR_SS, 0xc093);
	vmcs_write(GUEST_AR_LDTR, 0x82);
	vmcs_write(GUEST_AR_TR, 0x8b);

	/* 26.3.1.3 */
	vmcs_write(GUEST_BASE_GDTR, gdt64_desc.base);
	vmcs_write(GUEST_BASE_IDTR, idt_descr.base);
	vmcs_write(GUEST_LIMIT_GDTR, gdt64_desc.limit);
	vmcs_write(GUEST_LIMIT_IDTR, idt_descr.limit);

	/* 26.3.1.4 */
	vmcs_write(GUEST_RIP, (u64)(&guest_entry));
	vmcs_write(GUEST_RSP, (u64)(guest_stack + PAGE_SIZE - 1));
	vmcs_write(GUEST_RFLAGS, 0x2);

	/* 26.3.1.5 */
	vmcs_write(GUEST_ACTV_STATE, ACTV_ACTIVE);
	vmcs_write(GUEST_INTR_STATE, 0);
}

static int init_vmcs(struct vmcs **vmcs)
{
	*vmcs = alloc_page();
	memset(*vmcs, 0, PAGE_SIZE);
	(*vmcs)->hdr.revision_id = basic.revision;
	/* vmclear first to init vmcs */
	if (vmcs_clear(*vmcs)) {
		printf("%s : vmcs_clear error\n", __func__);
		return 1;
	}

	if (make_vmcs_current(*vmcs)) {
		printf("%s : make_vmcs_current error\n", __func__);
		return 1;
	}

	/* All settings to pin/exit/enter/cpu
	 * control fields should be placed here
	 */
	ctrl_pin |= PIN_EXTINT | PIN_NMI | PIN_VIRT_NMI;
	ctrl_exit = EXI_LOAD_EFER | EXI_HOST_64;
	ctrl_enter = (ENT_LOAD_EFER | ENT_GUEST_64);
	/* DIsable IO instruction VMEXIT now */
	ctrl_cpu[0] &= (~(CPU_IO | CPU_IO_BITMAP));
	ctrl_cpu[1] = 0;

	ctrl_pin = (ctrl_pin | ctrl_pin_rev.set) & ctrl_pin_rev.clr;
	ctrl_enter = (ctrl_enter | ctrl_enter_rev.set) & ctrl_enter_rev.clr;
	ctrl_exit = (ctrl_exit | ctrl_exit_rev.set) & ctrl_exit_rev.clr;
	ctrl_cpu[0] = (ctrl_cpu[0] | ctrl_cpu_rev[0].set) & ctrl_cpu_rev[0].clr;

	init_vmcs_ctrl();
	init_vmcs_host();
	init_vmcs_guest();
	return 0;
}

static void init_vmx(void)
{
	ulong fix_cr0_set, fix_cr0_clr;
	ulong fix_cr4_set, fix_cr4_clr;

	vmxon_region = alloc_page();
	memset(vmxon_region, 0, PAGE_SIZE);

	fix_cr0_set =  rdmsr(MSR_IA32_VMX_CR0_FIXED0);
	fix_cr0_clr =  rdmsr(MSR_IA32_VMX_CR0_FIXED1);
	fix_cr4_set =  rdmsr(MSR_IA32_VMX_CR4_FIXED0);
	fix_cr4_clr = rdmsr(MSR_IA32_VMX_CR4_FIXED1);
	basic.val = rdmsr(MSR_IA32_VMX_BASIC);
	ctrl_pin_rev.val = rdmsr(basic.ctrl ? MSR_IA32_VMX_TRUE_PIN
			: MSR_IA32_VMX_PINBASED_CTLS);
	ctrl_exit_rev.val = rdmsr(basic.ctrl ? MSR_IA32_VMX_TRUE_EXIT
			: MSR_IA32_VMX_EXIT_CTLS);
	ctrl_enter_rev.val = rdmsr(basic.ctrl ? MSR_IA32_VMX_TRUE_ENTRY
			: MSR_IA32_VMX_ENTRY_CTLS);
	ctrl_cpu_rev[0].val = rdmsr(basic.ctrl ? MSR_IA32_VMX_TRUE_PROC
			: MSR_IA32_VMX_PROCBASED_CTLS);
	if ((ctrl_cpu_rev[0].clr & CPU_SECONDARY) != 0) {
		ctrl_cpu_rev[1].val = rdmsr(MSR_IA32_VMX_PROCBASED_CTLS2);
	} else {
		ctrl_cpu_rev[1].val = 0;
	}
	if ((ctrl_cpu_rev[1].clr & (CPU_EPT | CPU_VPID)) != 0) {
		ept_vpid.val = rdmsr(MSR_IA32_VMX_EPT_VPID_CAP);
	} else {
		ept_vpid.val = 0;
	}

	write_cr0((read_cr0() & fix_cr0_clr) | fix_cr0_set);
	write_cr4((read_cr4() & fix_cr4_clr) | fix_cr4_set | X86_CR4_VMXE);

	*vmxon_region = basic.revision;

	guest_stack = alloc_page();
	memset(guest_stack, 0, PAGE_SIZE);
	guest_syscall_stack = alloc_page();
	memset(guest_syscall_stack, 0, PAGE_SIZE);
}

/* This function can only be called in guest */
static void __attribute__((__used__)) hypercall(u32 hypercall_no)
{
	u64 val = 0;
	val = (hypercall_no & HYPERCALL_MASK) | HYPERCALL_BIT;
	hypercall_field = val;
	asm volatile("vmcall\n\t");
}

static bool is_hypercall(void)
{
	ulong reason, hyper_bit;

	reason = vmcs_read(EXI_REASON) & 0xff;
	hyper_bit = hypercall_field & HYPERCALL_BIT;
	if ((reason == VMX_VMCALL) && (hyper_bit)) {
		return true;
	}
	return false;
}

static int handle_hypercall(void)
{
	ulong hypercall_no;

	hypercall_no = hypercall_field & HYPERCALL_MASK;
	hypercall_field = 0;
	switch (hypercall_no) {
	case HYPERCALL_VMEXIT:
		return VMX_TEST_VMEXIT;
	case HYPERCALL_VMABORT:
		return VMX_TEST_VMABORT;
	case HYPERCALL_VMSKIP:
		return VMX_TEST_VMSKIP;
	default:
		printf("ERROR : Invalid hypercall number : %ld\n", hypercall_no);
	}
	return VMX_TEST_EXIT;
}

static void continue_abort(void)
{
	assert(!in_guest);
	printf("Host was here when guest aborted:\n");
	dump_stack();
	longjmp(abort_target, 1);
	abort();
}

void __abort_test(void)
{
	if (in_guest) {
		hypercall(HYPERCALL_VMABORT);
	} else {
		longjmp(abort_target, 1);
	}
	abort();
}

static void continue_skip(void)
{
	assert(!in_guest);
	longjmp(abort_target, 1);
	abort();
}

void test_skip(const char *msg)
{
	printf("%s skipping test: %s\n", in_guest ? "Guest" : "Host", msg);
	if (in_guest) {
		hypercall(HYPERCALL_VMABORT);
	} else {
		longjmp(abort_target, 1);
	}
	abort();
}

static int exit_handler(void)
{
	int ret;

	current->exits++;
	regs.rflags = vmcs_read(GUEST_RFLAGS);
	if (is_hypercall()) {
		ret = handle_hypercall();
	} else {
		ret = current->exit_handler();
	}
	vmcs_write(GUEST_RFLAGS, regs.rflags);

	return ret;
}

/*
 * Called if vmlaunch or vmresume fails.
 *	@early    - failure due to "VMX controls and host-state area" (26.2)
 *	@vmlaunch - was this a vmlaunch or vmresume
 *	@rflags   - host rflags
 */
static int
entry_failure_handler(struct vmentry_failure *failure)
{
	if (current->entry_failure_handler) {
		return current->entry_failure_handler(failure);
	} else {
		return VMX_TEST_EXIT;
	}
}

/*
 * Tries to enter the guest. Returns true iff entry succeeded. Otherwise,
 * populates @failure.
 */
static bool vmx_enter_guest(struct vmentry_failure *failure)
{
	failure->early = 0;

	in_guest = 1;
	asm volatile (
		"mov %[HOST_RSP], %%rdi\n\t"
		"vmwrite %%rsp, %%rdi\n\t"
		LOAD_GPR_C
		"cmpb $0, %[launched]\n\t"
		"jne 1f\n\t"
		"vmlaunch\n\t"
		"jmp 2f\n\t"
		"1: "
		"vmresume\n\t"
		"2: "
		SAVE_GPR_C
		"pushf\n\t"
		"pop %%rdi\n\t"
		"mov %%rdi, %[failure_flags]\n\t"
		"movl $1, %[failure_flags]\n\t"
		"jmp 3f\n\t"
		"vmx_return:\n\t"
		SAVE_GPR_C
		"3: \n\t"
		: [failure_early]"+m"(failure->early),
		  [failure_flags]"=m"(failure->flags)
		: [launched]"m"(launched), [HOST_RSP]"i"(HOST_RSP)
		: "rdi", "memory", "cc"
	);
	in_guest = 0;

	failure->vmlaunch = !launched;
	failure->instr = launched ? "vmresume" : "vmlaunch";

	return !failure->early && !(vmcs_read(EXI_REASON) & VMX_ENTRY_FAILURE);
}

static int vmx_run(void)
{
	while (1) {
		u32 ret;
		bool entered;
		struct vmentry_failure failure;

		entered = vmx_enter_guest(&failure);

		if (entered) {
			/*
			 * VMCS isn't in "launched" state if there's been any
			 * entry failure (early or otherwise).
			 */
			launched = 1;
			ret = exit_handler();
		} else {
			ret = entry_failure_handler(&failure);
		}

		switch (ret) {
		case VMX_TEST_RESUME:
			continue;
		case VMX_TEST_VMEXIT:
			guest_finished = 1;
			return 0;
		case VMX_TEST_EXIT:
			break;
		default:
			printf("ERROR : Invalid %s_handler return val %d.\n",
			       entered ? "exit" : "entry_failure",
			       ret);
			break;
		}

		if (entered) {
			print_vmexit_info();
		} else {
			print_vmentry_failure_info(&failure);
		}
		abort();
	}
}

static void run_teardown_step(struct test_teardown_step *step)
{
	step->func(step->data);
}

static int test_run(struct vmx_test *test)
{
	int r;

	if (test->name == NULL) {
		test->name = "(no name)";
	}
	if (vmx_on()) {
		printf("%s : vmxon failed.\n", __func__);
		return 1;
	}
	/* Directly call test->init is ok here,
	 * init_vmcs has done
	 * vmcs init, vmclear and vmptrld
	 */
	init_vmcs(&(test->vmcs));
	/* execute test init call function */
	if ((test->init) && (test->init(test->vmcs) != VMX_TEST_START)) {
		goto out;
	}
	teardown_count = 0;
	v2_guest_main = NULL;
	test->exits = 0;
	current = test;
	regs = test->guest_regs;
	vmcs_write(GUEST_RFLAGS, regs.rflags | 0x2);
	launched = 0;
	guest_finished = 0;
	printf("\nTest suite: %s\n", test->name);

	r = setjmp(abort_target);
	if (r) {
		assert(!in_guest);
		goto out;
	}

	vmx_run();

	while (teardown_count > 0) {
		run_teardown_step(&teardown_steps[--teardown_count]);
	}

	if (launched && !guest_finished) {
		report("Guest didn't run to completion.", 0);
	}

out:
	if (vmx_off()) {
		printf("%s : vmxoff failed.\n", __func__);
		return 1;
	}
	return 0;
}

/*
 * Add a teardown step. Executed after the test's main function returns.
 * Teardown steps executed in reverse order.
 */
void test_add_teardown(test_teardown_func func, void *data)
{
	struct test_teardown_step *step;

	TEST_ASSERT_MSG(teardown_count < MAX_TEST_TEARDOWN_STEPS,
			"There are already %d teardown steps.",
			teardown_count);
	step = &teardown_steps[teardown_count++];
	step->func = func;
	step->data = data;
}

/*
 * Set the target of the first enter_guest call. Can only be called once per
 * test. Must be called before first enter_guest call.
 */
void test_set_guest(test_guest_func func)
{
	assert(current->v2);
	TEST_ASSERT_MSG(!v2_guest_main, "Already set guest func.");
	v2_guest_main = func;
}

/*
 * Enters the guest (or launches it for the first time). Error to call once the
 * guest has returned (i.e., run past the end of its guest() function). Also
 * aborts if guest entry fails.
 */
void enter_guest(void)
{
	struct vmentry_failure failure;

	TEST_ASSERT_MSG(v2_guest_main,
			"Never called test_set_guest_func!");

	TEST_ASSERT_MSG(!guest_finished,
			"Called enter_guest() after guest returned.");

	if (!vmx_enter_guest(&failure)) {
		print_vmentry_failure_info(&failure);
		abort();
	}

	launched = 1;

	if (is_hypercall()) {
		int ret;

		ret = handle_hypercall();
		switch (ret) {
		case VMX_TEST_VMEXIT:
			guest_finished = 1;
			break;
		case VMX_TEST_VMABORT:
			continue_abort();
			break;
		case VMX_TEST_VMSKIP:
			continue_skip();
			break;
		default:
			printf("ERROR : Invalid handle_hypercall return %d.\n",
			       ret);
			abort();
		}
	}
}

/* EPT paging structure related functions */
/* split_large_ept_entry: Split a 2M/1G large page into 512 smaller PTEs.
 * @ptep : large page table entry to split
 * @level : level of ptep (2 or 3)
 */
static void split_large_ept_entry(unsigned long *ptep, int level)
{
	unsigned long *new_pt;
	unsigned long gpa;
	unsigned long pte;
	unsigned long prototype;
	int i;

	pte = *ptep;
	assert(pte & EPT_PRESENT);
	assert(pte & EPT_LARGE_PAGE);
	assert(level == 2 || level == 3);

	new_pt = alloc_page();
	assert(new_pt);
	memset(new_pt, 0, PAGE_SIZE);

	prototype = pte & ~EPT_ADDR_MASK;
	if (level == 2)
		prototype &= ~EPT_LARGE_PAGE;

	gpa = pte & EPT_ADDR_MASK;
	for (i = 0; i < EPT_PGDIR_ENTRIES; i++) {
		new_pt[i] = prototype | gpa;
		gpa += 1ul << EPT_LEVEL_SHIFT(level - 1);
	}

	pte &= ~EPT_LARGE_PAGE;
	pte &= ~EPT_ADDR_MASK;
	pte |= virt_to_phys(new_pt);

	*ptep = pte;
}

/*
 * install_ept_entry : Install a page to a given level in EPT
 * @pml4 : addr of pml4 table
 * @pte_level : level of PTE to set
 * @guest_addr : physical address of guest
 * @pte : pte value to set
 * @pt_page : address of page table, NULL for a new page
 */
void install_ept_entry(unsigned long *pml4,
		int pte_level,
		unsigned long guest_addr,
		unsigned long pte,
		unsigned long *pt_page)
{
	int level;
	unsigned long *pt = pml4;
	unsigned offset;

	/* EPT only uses 48 bits of GPA. */
	assert(guest_addr < (1ul << 48));

	for (level = EPT_PAGE_LEVEL; level > pte_level; --level) {
		offset = (guest_addr >> EPT_LEVEL_SHIFT(level))
				& EPT_PGDIR_MASK;
		if (!(pt[offset] & (EPT_PRESENT))) {
			unsigned long *new_pt = pt_page;
			if (!new_pt) {
				new_pt = alloc_page();
			} else {
				pt_page = 0;
			}
			memset(new_pt, 0, PAGE_SIZE);
			pt[offset] = virt_to_phys(new_pt)
					| EPT_RA | EPT_WA | EPT_EA;
		} else if (pt[offset] & EPT_LARGE_PAGE) {
			split_large_ept_entry(&pt[offset], level);
		}
		pt = phys_to_virt(pt[offset] & EPT_ADDR_MASK);
	}
	offset = (guest_addr >> EPT_LEVEL_SHIFT(level)) & EPT_PGDIR_MASK;
	pt[offset] = pte;
}

/* Map a page, @perm is the permission of the page */
void install_ept(unsigned long *pml4,
		unsigned long phys,
		unsigned long guest_addr,
		u64 perm)
{
	install_ept_entry(pml4, 1, guest_addr, (phys & PAGE_MASK) | perm, 0);
}

void setup_ept_range(unsigned long *pml4, unsigned long start,
		     unsigned long len, u64 perm)
{
	u64 phys = start;
	u64 max = (u64)len + (u64)start;

	while (phys + PAGE_SIZE <= max) {
		install_ept(pml4, phys, phys, perm);
		phys += PAGE_SIZE;
	}
}

/* Enables EPT and sets up the identity map. */
static int setup_ept(struct st_vcpu *vcpu)
{
	unsigned long ept_mapping_size;
	u32 ctrl_cpu[2];
	/* alloc page for eptp */
	u64 *pml4 = alloc_page();
	u64 eptp;
	vcpu->arch.pml4 = pml4;
	memset(pml4, 0, PAGE_SIZE);
	eptp = virt_to_phys(pml4);

	/* according Table 24-8. Format of Extended-Page-Table Pointer */
	eptp |= EPT_MEM_TYPE_WB;

	ctrl_cpu[0] = vmcs_read(CPU_EXEC_CTRL0);
	ctrl_cpu[1] = vmcs_read(CPU_EXEC_CTRL1);
	ctrl_cpu[0] = (ctrl_cpu[0] | CPU_SECONDARY)
		& ctrl_cpu_rev[0].clr;
	ctrl_cpu[1] = (ctrl_cpu[1] | CPU_EPT)
		& ctrl_cpu_rev[1].clr;
	vmcs_write(CPU_EXEC_CTRL0, ctrl_cpu[0]);
	vmcs_write(CPU_EXEC_CTRL1, ctrl_cpu[1]);

	eptp |= (3 << EPTP_PG_WALK_LEN_SHIFT);
	vmcs_write(EPTP, eptp);

	vcpu->arch.eptp = eptp;
	ept_mapping_size = EPT_MAPING_SIZE_4G;

	DBG_INFO("install memory size:0x%lx eptp:0x%lx vcpu->arch.pml4:0x%lx", \
		ept_mapping_size, eptp, (u64)vcpu->arch.pml4);
	/* Cannot use large EPT pages if we need to track EPT
	 * accessed/dirty bits at 4K granularity.
	 */
	setup_ept_range(pml4, 0, ept_mapping_size,
			EPT_WA | EPT_RA | EPT_EA);
	return 0;
}

struct st_vcpu *get_bp_vcpu(void)
{
	return &vcpu_bp;
}

int vm_exec_init(struct vmcs *vmcs)
{
	struct st_vcpu *vcpu = get_bp_vcpu();
	/* one one mapping 4G memory hpa to gpa */
	setup_ept(vcpu);

	/* set up vpid */
	/* Enable VPID at secondary processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL1, CPU_VPID, BIT_TYPE_SET);
	/* Set VPID to VMCS */
	vcpu->arch.vpid = CPU_BP_ID + 1;
	vmcs_write(VPID, vcpu->arch.vpid);

	return VMX_TEST_START;
}

/* use global data enter host create test condition for test */
void set_vmcs(u32 condition)
{
	set_exit_reason(VM_EXIT_REASON_DEFAULT_VALUE);
	vmx_set_test_condition(condition);
	/* if is invalid condition return right now */
	if (condition == CON_BUFF) {
		DBG_INFO("invalid condition do not enter host!");
		return;
	}
	vmcall();
}

int main(__unused int argc, __unused const char *argv[])
{
	int i;

	setup_vm();
	setup_idt();
	asm volatile("fninit");

	print_case_list();

	hypercall_field = 0;
	/* init data */
	memset(&vm_exit_info, 0, sizeof(vm_exit_info));
	memset(&vcpu_bp, 0, sizeof(vcpu_bp));
	set_log_level(DBG_LVL_INFO);

	if (!(cpuid(1).c & (1 << 5))) {
		printf("WARNING: vmx not supported, add '-cpu host'\n");
		goto exit;
	}

	init_vmx();

	hsi_rqmid_40291_virtualization_specific_features_VMX_instruction_001();

	for (i = 0; vmx_tests[i].name != NULL; i++) {
		if (test_run(&vmx_tests[i])) {
			goto exit;
		}
	}

exit:
	return report_summary();
}
#endif

