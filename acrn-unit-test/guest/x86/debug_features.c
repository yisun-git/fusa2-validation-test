/*
 * Copyright (c) 2019 Intel Corporation. All rights reserved.
 * Test mode: 64-bit
 */

#include "libcflat.h"
#include "apic.h"
#include "vm.h"
#include "smp.h"
#include "desc.h"
#include "isr.h"
#include "msr.h"
#include "atomic.h"
#include "processor.h"
#include "misc.h"
#include "debug_features.h"
#include "regdump.h"


#ifdef IN_NATIVE
static void check_single_step_bp(struct ex_regs *regs)
{
	unsigned ex_val;

	ex_val = regs->vector | (regs->error_code << 16) |
			 (((regs->rflags >> 16) & 1) << 8);

	asm("mov %0, %%gs:"xstr(EXCEPTION_ADDR)"" : : "r"(ex_val));
}
/**
 * @brief case name physical_single_step_debug_exception
 *
 * Summary:Physical platform shall have single-step debug exception support.
 *
 * in protected mode, 64-bit mode, execute setting the TF of #RFLAGS leading?to #DB?exception..
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_27455_physical_single_step_debug_exception(void)
{
	u8 level;
	u16 vec;
	unsigned err_code;
	handler old;

	old = handle_exception(DB_VECTOR, check_single_step_bp);
	asm volatile(
		"pushf\n\t"
		"pushf\n\t"
		"pop %rax\n\t"
		"or $(1<<8), %rax\n\t"
		"push %rax\n\t"
		"popf\n\t"
		"mov %rax, %rax\n\t"
		"popf\n\t"
	);

	handle_exception(DB_VECTOR, old);

	level = read_cs() & 0x3;
	vec = exception_vector();
	err_code = exception_error_code();

	report("%s", (vec == DB_VECTOR) && (err_code == 0) && (level == 0), __FUNCTION__);
}
#else

/**
 * @brief case name Hide Debug extensions 001
 *
 * Summary: ACRN hypervisor shall hide debug extensions to any VM, in compliance with
 *  Chapter 17.2.2, Vol.3 SDM.
 *  This feature is not used. Hide this to keep design simple and safe.
 *
 * When debug extensions are not enabled (when the DE flag is clear),
 * these registers are aliased to debug registers DR6 and DR7.
 * Ignore the behavior when write DR4,At privilege levels(CPL=0) in any mode
 * (64bit mode, protect mode, real-address mode) writing DR4 with MOV instruction shall ignore the operation
 * (all general registers, MSR and DR registers shall unchanged).
 *
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_27447_hide_debug_extensions_001(void)
{
	bool ret1 = false;
	unsigned long cr4;
	u64 dr4;

	cr4 = read_cr4();
	if ((cr4 & X86_CR4_DE) != 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	enable_xsave();
	ret1 = CHECK_INSTRUCTION_REGS(asm volatile("mov %rax, %dr4 \n\t"));
	asm volatile("mov %%dr4,%0 \n\t " : "=r"(dr4));
	report("%s", (ret1 == true) && (dr4 == 0xfffe0ff0), __FUNCTION__);
}
/**
 * @brief case name Hide Debug extensions 002
 *
 * Summary: ACRN hypervisor shall hide debug extensions to any VM, in compliance with
 *  Chapter 17.2.2, Vol.3 SDM.
 *  This feature is not used. Hide this to keep design simple and safe.
 *
 * When debug extensions are not enabled (when the DE flag is clear),
 * these registers are aliased to debug registers DR6 and DR7.
 * Ignore the behavior when write DR5.
 * At privilege levels(CPL=0) in any mode(64bit mode, protect mode, real-address mode)
 * writing DR4 with MOV instruction shall ignore the operation
 * (all general registers, MSR and DR registers shall unchanged).
 *
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38242_hide_debug_extensions_002(void)
{
	bool ret1 = false;
	unsigned long cr4;
	u64 dr5;

	cr4 = read_cr4();
	if ((cr4 & X86_CR4_DE) != 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	enable_xsave();
	ret1 = CHECK_INSTRUCTION_REGS(asm volatile("mov %rax, %dr5 \n\t"));
	asm volatile("mov %%dr5, %0 \n\t " : "=r"(dr5));
	report("%s", (ret1 == true) && (dr5 == 0x400), __FUNCTION__);
}
/**
 * @brief case name Hide Debug extensions 003
 *
 * Summary: ACRN hypervisor shall hide debug extensions to any VM, in compliance with
 *  Chapter 17.2.2, Vol.3 SDM.
 *  This feature is not used. Hide this to keep design simple and safe.
 *
 * At privilege levels(CPL=0) in any mode(64bit mode, protect mode, real-address mode) reading DR4,
 * with MOV instruction, the value shall be fffe0ff0H.
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38243_hide_debug_extensions_003(void)
{
	u64 dr4, cr4;

	cr4 = read_cr4();
	if ((cr4 & X86_CR4_DE) != 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	asm volatile("mov %%dr4, %0 \n\t " : "=r"(dr4));
	report("%s", dr4 == 0xfffe0ff0, __FUNCTION__);
}
/**
 * @brief case name Hide Debug extensions 004
 *
 * Summary: ACRN hypervisor shall hide debug extensions to any VM, in compliance with
 *  Chapter 17.2.2, Vol.3 SDM.
 *  This feature is not used. Hide this to keep design simple and safe.
 *
 * At privilege levels(CPL=0) in any mode(64bit mode, protect mode, real-address mode) reading DR4,
 * with MOV instruction, the value shall be 400H.
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38244_hide_debug_extensions_004(void)
{
	u64 dr5, cr4;

	cr4 = read_cr4();
	if ((cr4 & X86_CR4_DE) != 0) {
		report("%s", 0, __FUNCTION__);
		return;
	}

	asm volatile("mov %%dr5, %0 \n\t " : "=r"(dr5));
	report("%s", dr5 == 0x400, __FUNCTION__);
}

/**
 * @brief case name Inject #GP(0) when running INT 1 on Guest
 *
 * Summary: When run instruction INT 1  on a guest, ACRN hypervisor shall insert #GP(0) to the guest vCPU.
 *
 * in protected mode, 64-bit mode, executing INT 1 on guest,generate #GP(0) exception.
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_27450_inject_gp_when_running_int1_on_guest_001(void)
{
	u8 level;
	u16 vec;
	unsigned err_code;

	level = read_cs() & 0x3;
	asm volatile(
		ASM_TRY("1f")
		".byte 0xf1\n\t"/*int1*/
		"1:" :);

	vec = exception_vector();
	err_code = exception_error_code();

	report("%s", (vec == GP_VECTOR) && (err_code == 0) && (level == 0), __FUNCTION__);
}

static volatile unsigned long bp_addr;
static void handle_bp(struct ex_regs *regs)
{
	bp_addr = regs->rip;
}

/**
 * @brief case name Pass through Breakpoint exception
 *
 * Summary: Breakpoint Exception(#BP) after executing instruction INT 3 on a guest
 *
 * in 64-bit mode, executing INT 3 on guest generate at ring 0 level,
 *	the HV will pass through #BP exception to VM
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_35268_passthrough_BP_exception(void)
{
	u8 level;
	handler old;

	bp_addr = 0;
	level = read_cs() & 0x3;

	old = handle_exception(BP_VECTOR, handle_bp);
	asm volatile("int3; ring0_bp:\n\t");
	extern unsigned char ring0_bp;

	report("%s", (bp_addr == (unsigned long)&ring0_bp) && (level == 0), __FUNCTION__);

	bp_addr = 0;
	handle_exception(BP_VECTOR, old);
}
/**
 * @brief case name Ignore the behavior when write DR0,DR1,DR2,DR3, DR6, DR7_001
 *
 * Summary: When a vCPU attempts to write DR0, DR1, DR2, DR3, DR6 and DR7, ACRN hypervisor shall ignore the write.
 *
 * under 64-bit mode and Ring 0 Level, try to write?DR0,DR1,DR2,DR3,DR6,DR7,
 * Hypervisor shall ignore the write and?all general registers , MSR and DR registers shall be unchanged.
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38138_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_001(void)
{
	u64 dr;
	bool ret;

	enable_xsave();
	asm volatile("mov $0xf,%rax");
	ret = CHECK_INSTRUCTION_REGS(asm volatile("mov %rax,%dr0"));

	asm volatile("mov %%dr0, %0" : "=r"(dr));
	report("%s", (ret == true) && (dr == 0), __FUNCTION__);
}
/**
 * @brief case name Ignore the behavior when write DR0,DR1,DR2,DR3, DR6, DR7_002
 *
 * Summary: When a vCPU attempts to write DR0, DR1, DR2, DR3, DR6 and DR7, ACRN hypervisor shall ignore the write.
 *
 * under 64-bit mode and Ring 0 Level, try to write?DR0,DR1,DR2,DR3,DR6,DR7,
 * Hypervisor shall ignore the write and?all general registers , MSR and DR registers shall be unchanged.
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38139_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_002(void)
{
	u64 dr;
	bool ret;

	enable_xsave();
	asm volatile("mov $0xf,%rax");
	ret = CHECK_INSTRUCTION_REGS(asm volatile("mov %rax, %dr1"));
	asm volatile("mov %%dr1, %0" : "=r"(dr));
	report("%s", (ret == true) && (dr == 0), __FUNCTION__);
}
/**
 * @brief case name Ignore the behavior when write DR0,DR1,DR2,DR3, DR6, DR7_003
 *
 * Summary: When a vCPU attempts to write DR0, DR1, DR2, DR3, DR6 and DR7, ACRN hypervisor shall ignore the write.
 *
 * under 64-bit mode and Ring 0 Level, try to write?DR0,DR1,DR2,DR3,DR6,DR7,
 * Hypervisor shall ignore the write and?all general registers , MSR and DR registers shall be unchanged.
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38142_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_003(void)
{
	u64 dr;
	bool ret;

	enable_xsave();
	asm volatile("mov $0xf,%rax");
	ret = CHECK_INSTRUCTION_REGS(asm volatile("mov %rax, %dr2"));
	asm volatile("mov %%dr2, %0" : "=r"(dr));
	report("%s", (ret == true) && (dr == 0), __FUNCTION__);
}
/**
 * @brief case name Ignore the behavior when write DR0,DR1,DR2,DR3, DR6, DR7_004
 *
 * Summary: When a vCPU attempts to write DR0, DR1, DR2, DR3, DR6 and DR7, ACRN hypervisor shall ignore the write.
 *
 * under 64-bit mode and Ring 0 Level, try to write?DR0,DR1,DR2,DR3,DR6,DR7,
 * Hypervisor shall ignore the write and all general registers , MSR and DR registers shall be unchanged.
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38144_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_004(void)
{
	u64 dr;
	bool ret;

	enable_xsave();
	asm volatile("mov $0xf,%rax");
	ret = CHECK_INSTRUCTION_REGS(asm volatile("mov %rax,%dr3"));
	asm volatile("mov %%dr3, %0" : "=r"(dr));
	report("%s", (ret == true) && (dr == 0), __FUNCTION__);
}
/**
 * @brief case name Ignore the behavior when write DR0,DR1,DR2,DR3, DR6, DR7_005
 *
 * Summary: When a vCPU attempts to write DR0, DR1, DR2, DR3, DR6 and DR7, ACRN hypervisor shall ignore the write.
 *
 * under 64-bit mode and Ring 0 Level, try to write DR0,DR1,DR2,DR3,DR6,DR7,
 * Hypervisor shall ignore the write and?all general registers , MSR and DR registers shall be unchanged.
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38145_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_005(void)
{
	u64 dr;
	bool ret;

	enable_xsave();
	asm volatile("mov $0xf,%rax");
	ret = CHECK_INSTRUCTION_REGS(asm volatile("mov %rax,%dr6"));
	asm volatile("mov %%dr6, %0" : "=r"(dr));
	report("%s", (ret == true) && (dr == 0xfffe0ff0), __FUNCTION__);
}
/**
 * @brief case name Ignore the behavior when write DR0,DR1,DR2,DR3, DR6, DR7_005
 *
 * Summary: When a vCPU attempts to write DR0, DR1, DR2, DR3, DR6 and DR7, ACRN hypervisor shall ignore the write.
 *
 * under 64-bit mode and Ring 0 Level, try to write?DR0,DR1,DR2,DR3,DR6,DR7,
 * Hypervisor shall ignore the write and?all general registers , MSR and DR registers shall be unchanged.
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38146_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_006(void)
{
	u64 dr;
	bool ret;

	enable_xsave();
	asm volatile("mov $0xf,%rax");
	ret = CHECK_INSTRUCTION_REGS(asm volatile("mov %rax,%dr7"));
	asm volatile("mov %%dr7, %0" : "=r"(dr));
	report("%s", (ret == true) && (dr == 0x400), __FUNCTION__);
}
/**
 * @brief case name:Return value when read DR7
 *
 * Summary: When the vCPU attempts to read DR7, ACRN hypervisor shall guarantee that the vCPU gets FFFE0FF0H.
 *
 * At privilege levels(CPL=0) in any mode(64bit mode, protect mode, real-address mode)
 *  reading DR7 with MOV instruction, the value shall be 400H.
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38147_return_value_when_read_dr7(void)
{
	u64 dr = 0;

	asm volatile("mov %%dr7, %0" : "=r"(dr));
	report("%s", (dr == 0x400), __FUNCTION__);
}
/**
 * @brief case name:Return value when read DR6
 *
 * Summary: When a vCPU attempts to read DR0, DR1, DR2 or DR3, ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *
 * At privilege levels(CPL=0) in any mode(64bit mode, protect mode, real-address mode)
 * reading DR6 with MOV instruction, the value shall be FFFE0FF0H.
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38148_return_value_when_read_dr6(void)
{
	u64 dr = 0;

	asm volatile("mov %%dr6, %0" : "=r"(dr));
	report("%s", (dr == 0xFFFE0FF0), __FUNCTION__);
}

/**
 * @brief case name:Return 0 when MOV from DR0,DR1,DR2,DR3_001
 *
 * Summary: When a vCPU attempts to read DR0, DR1, DR2 or DR3, ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *
 * At privilege levels(CPL=0) in any mode(64bit mode, protect mode, real-address mode)
 * reading DR0, DR1, DR2, DR3 with MOV instruction,  the value shall be 0H
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38150_return_0_when_mov_from_dr0_dr1_dr2_dr3_001(void)
{
	u64 dr;

	asm volatile("mov %%dr0, %0" : "=r"(dr));
	report("%s", (dr == 0), __FUNCTION__);
}

/**
 * @brief case name:Return 0 when MOV from DR0,DR1,DR2,DR3_002
 *
 * Summary: When a vCPU attempts to read DR0, DR1, DR2 or DR3, ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *
 * At privilege levels(CPL=0) in any mode(64bit mode, protect mode, real-address mode)
 * reading DR0, DR1, DR2, DR3 with MOV instruction, the value shall be 0H
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38152_return_0_when_mov_from_dr0_dr1_dr2_dr3_002(void)
{
	u64 dr;

	asm volatile("mov %%dr1, %0" : "=r"(dr));
	report("%s", (dr == 0), __FUNCTION__);
}

/**
 * @brief case name:Return 0 when MOV from DR0,DR1,DR2,DR3_003
 *
 * Summary: When a vCPU attempts to read DR0, DR1, DR2 or DR3, ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *
 * At privilege levels(CPL=0) in any mode(64bit mode, protect mode, real-address mode)
 * reading DR0, DR1, DR2, DR3 with MOV instruction,  the value shall be 0H
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38153_return_0_when_mov_from_dr0_dr1_dr2_dr3_003(void)
{
	u64 dr;

	asm volatile("mov %%dr2, %0" : "=r"(dr));
	report("%s", (dr == 0), __FUNCTION__);
}

/**
 * @brief case name:Return 0 when MOV from DR0,DR1,DR2,DR3_004
 *
 * Summary: When a vCPU attempts to read DR0, DR1, DR2 or DR3, ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *
 * At privilege levels(CPL=0) in any mode(64bit mode, protect mode, real-address mode)
 *	reading DR0, DR1, DR2, DR3 with MOV instruction, the value shall be 0H
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38154_return_0_when_mov_from_dr0_dr1_dr2_dr3_004(void)
{
	u64 dr;

	asm volatile("mov %%dr3, %0" : "=r"(dr));
	report("%s", (dr == 0), __FUNCTION__);
}

static void check_single_step_bp(struct ex_regs *regs)
{
	unsigned ex_val;

	ex_val = regs->vector | (regs->error_code << 16) |
			 (((regs->rflags >> 16) & 1) << 8);
	asm("mov %0, %%gs:"xstr(EXCEPTION_ADDR)"" : : "r"(ex_val));
}

/**
 * @brief case name:Initialize CR4.DE following start-up
 *
 * Summary: ACRN hypervisor shall set initial guest CR4.DE to 0H following start-up.
 *
 * Check the the value of?CR4.DE after BP start-up
 *
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38225_initialize_cr4_de_following_startup(void)
{
	report("%s", (*(u64 *)CR4_STARTUP_ADDR & X86_CR4_DE) == 0, __FUNCTION__);
}

/**
 * @brief case name:Initialize CR4.DE following init
 *
 * Summary: ACRN hypervisor shall set initial guest CR4.DE to 0H following start-up.
 *
 * Check the the value of CR4.DE flowing INIT
 *
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38226_initialize_cr4_de_following_INIT(void)
{
	report("%s", (*(u64 *)CR4_INIT_ADDR & X86_CR4_DE) == 0, __FUNCTION__);
}

/**
 * @brief case name Inject #GP(0) when single-step debug exception
 *
 * Summary: When single-step debug exception happens, ACRN hypervisor shall insert #GP(0) to
 *  the guest vCPU, in compliance with Chapter 17.1, Vol.3 SDM.
 *  Can't hide TF flag in EFLAGS register. This feature is useless for FuSa cases so
 *  inject #GP(0) when this exception happens.
 *
 *
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38228_inject_gp0_when_single_step_debug_exception(void)
{
	u8 level;
	u16 vec;
	unsigned err_code;
	handler old;

	old = handle_exception(GP_VECTOR, check_single_step_bp);
	asm volatile(
		"pushf\n\t"
		"pushf\n\t"
		"pop %rax\n\t"
		"or $(1<<8), %rax\n\t"
		"push %rax\n\t"
		"popf\n\t"
		"mov %rax, %rax\n\t"
		"popf\n\t"
	);

	handle_exception(GP_VECTOR, old);

	level = read_cs() & 0x3;
	vec = exception_vector();
	err_code = exception_error_code();
	report("%s", (vec == GP_VECTOR) && (err_code == 0) && (level == 0), __FUNCTION__);
}
/**
 * @brief case name :read cr4.de
 *
 * Summary: When a vCPU attempts to read CR4.DE, ACRN hypervisor shall guarantee that the vCPU gets 0H.
 *  Read the DE Flag bit (CR4.DE[bit 3]) of control register CR4.The value shall be 0.
 *
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38230_read_cr4_de()
{
	ulong cr4;

	cr4 = read_cr4();
	report("%s", (cr4 & X86_CR4_DE) == 0, __FUNCTION__);
}
/**
 * @brief case name :write 1H to cr4.de
 *
 *Summary:
 *	When a vCPU attempts to write 1H to CR4.DE, ACRN hypervisor shall inject #GP(0) to the vCPU.
 *
 * @param None
 *
 * @retval None
 *
 */
static void debug_features_rqmid_38241_write_1h_to_cr4_de()
{
	ulong cr4;

	cr4 = read_cr4();
	cr4 = cr4 | X86_CR4_DE;

	asm volatile(ASM_TRY("1f")
				 "mov %0, %%cr4 \n\r"
				 "1:" : : "r"(cr4));
	report("%s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}

#endif
static void print_case_list(void)
{

	printf("debug_features feature case list:\n\r");
#ifdef IN_NATIVE
	printf("\t Case ID:%d case name:%s\n\r", 27455,
		   "debug_features_rqmid_27455_physical_single_step_debug_exception");
#else
	printf("\t Case ID:%d case name:%s\n\r", 27447,
		   "debug_features_rqmid_27447_hide_debug_extensions_001");
	printf("\t Case ID:%d case name:%s\n\r", 38242,
		   "debug_features_rqmid_38242_hide_debug_extensions_002");
	printf("\t Case ID:%d case name:%s\n\r", 38243,
		   "debug_features_rqmid_38243_hide_debug_extensions_003");
	printf("\t Case ID:%d case name:%s\n\r", 38244,
		   "debug_features_rqmid_38244_hide_debug_extensions_004");
	printf("\t Case ID:%d case name:%s\n\r", 27450,
		   "debug_features_rqmid_27450_inject_gp_when_running_int1_on_guest_001");
	printf("\t Case ID:%d case name:%s\n\r", 35268,
		   "debug_features_rqmid_35268_passthrough_BP_exception");
	printf("\t Case ID:%d case name:%s\n\r", 38138,
		   "debug_features_rqmid_38138_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_001");
	printf("\t Case ID:%d case name:%s\n\r", 38139,
		   "debug_features_rqmid_38139_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_002");
	printf("\t Case ID:%d case name:%s\n\r", 38142,
		   "debug_features_rqmid_38142_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_003");
	printf("\t Case ID:%d case name:%s\n\r", 38144,
		   "debug_features_rqmid_38144_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_004");
	printf("\t Case ID:%d case name:%s\n\r", 38145,
		   "debug_features_rqmid_38145_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_005");
	printf("\t Case ID:%d case name:%s\n\r", 38146,
		   "debug_features_rqmid_38146_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_006");
	printf("\t Case ID:%d case name:%s\n\r", 38147,
		   "debug_features_rqmid_38147_return_value_when_read_dr7");
	printf("\t Case ID:%d case name:%s\n\r", 38148,
		   "debug_features_rqmid_38148_return_value_when_read_dr6");
	printf("\t Case ID:%d case name:%s\n\r", 38150,
		   "debug_features_rqmid_38150_return_0_when_mov_from_dr0_dr1_dr2_dr3_001");
	printf("\t Case ID:%d case name:%s\n\r", 38152,
		   "debug_features_rqmid_38152_return_0_when_mov_from_dr0_dr1_dr2_dr3_002");
	printf("\t Case ID:%d case name:%s\n\r", 38153,
		   "debug_features_rqmid_38153_return_0_when_mov_from_dr0_dr1_dr2_dr3_003");
	printf("\t Case ID:%d case name:%s\n\r", 38154,
		   "debug_features_rqmid_38154_return_0_when_mov_from_dr0_dr1_dr2_dr3_004");
	printf("\t Case ID:%d case name:%s\n\r", 38225,
		   "debug_features_rqmid_38225_initialize_cr4_de_following_startup");
	printf("\t Case ID:%d case name:%s\n\r", 38226,
		   "debug_features_rqmid_38225_initialize_cr4_de_following_INIT");
	printf("\t Case ID:%d case name:%s\n\r", 38228,
		   "debug_features_rqmid_38228_inject_gp0_when_single_step_debug_exception");
	printf("\t Case ID:%d case name:%s\n\r", 38230,
		   "debug_features_rqmid_38230_read_cr4_de");
	printf("\t Case ID:%d case name:%s\n\r", 38241,
		   "debug_features_rqmid_38241_write_1h_to_cr4_de");
#endif
}

int main(int ac, char **av)
{
	setup_idt();
	print_case_list();
#ifdef IN_NATIVE
	debug_features_rqmid_27455_physical_single_step_debug_exception();
#else
	debug_features_rqmid_27447_hide_debug_extensions_001();
	debug_features_rqmid_38242_hide_debug_extensions_002();
	debug_features_rqmid_38243_hide_debug_extensions_003();
	debug_features_rqmid_38244_hide_debug_extensions_004();
	debug_features_rqmid_27450_inject_gp_when_running_int1_on_guest_001();
	debug_features_rqmid_35268_passthrough_BP_exception();
	debug_features_rqmid_38138_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_001();
	debug_features_rqmid_38139_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_002();
	debug_features_rqmid_38142_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_003();
	debug_features_rqmid_38144_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_004();
	debug_features_rqmid_38145_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_005();
	debug_features_rqmid_38146_ignore_when_write_dr0_dr1_dr2_dr3_dr6_dr7_006();
	debug_features_rqmid_38147_return_value_when_read_dr7();
	debug_features_rqmid_38148_return_value_when_read_dr6();
	debug_features_rqmid_38150_return_0_when_mov_from_dr0_dr1_dr2_dr3_001();
	debug_features_rqmid_38152_return_0_when_mov_from_dr0_dr1_dr2_dr3_002();
	debug_features_rqmid_38153_return_0_when_mov_from_dr0_dr1_dr2_dr3_003();
	debug_features_rqmid_38154_return_0_when_mov_from_dr0_dr1_dr2_dr3_004();
	debug_features_rqmid_38225_initialize_cr4_de_following_startup();
	debug_features_rqmid_38226_initialize_cr4_de_following_INIT();
	debug_features_rqmid_38228_inject_gp0_when_single_step_debug_exception();
	debug_features_rqmid_38230_read_cr4_de();
	debug_features_rqmid_38241_write_1h_to_cr4_de();
#endif
	return report_summary();
}
