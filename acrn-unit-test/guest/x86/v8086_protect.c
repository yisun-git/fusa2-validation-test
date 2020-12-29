/*
 * Test for x86 v8086
 *
 * Copyright (c) intel, 2020
 *
 * Authors:
 *  wenwumax <wenwux.ma@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "libcflat.h"
#include "desc.h"
#include "apic-defs.h"
#include "apic.h"
#include "processor.h"
#include "vm.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "v8086_protect.h"

void print_case_list(void)
{
	printf("v8086 feature case list:\n\t");
	printf("\t\tCase ID:%d case name:%s\n\t", 37631u, "Initialize CR4.PVI following start-up_001");
	printf("\t\tCase ID:%d case name:%s\n\t", 37632u, "Initialize CR4.PVI following init_001");
	printf("\t\tCase ID:%d case name:%s\n\t", 37633u, "Initialize CR4.VME following start-up_001");
	printf("\t\tCase ID:%d case name:%s\n\t", 37634u, "Initialize CR4.VME following init_001");
	printf("\t\tCase ID:%d case name:%s\n\t", 37652u, "Read CR4.VME_001");
	printf("\t\tCase ID:%d case name:%s\n\t", 37653u, "Read CR4.PVI_001");
#ifndef IN_NATIVE
	printf("\t\tCase ID:%d case name:%s\n\t", 33868u, "reads CPUID.01H_001");
#endif
	printf("\t\tCase ID:%d case name:%s\n\t", 37947u, "write CR4 and the new guest CR4.VME is 1H_003");
	printf("\t\tCase ID:%d case name:%s\n\t", 37948u, "write CR4 and the new guest CR4.PVI is 1H_003");
}

/**
 * @brief case name: Initialize CR4.VME following start-up_001
 *
 * Summary: Get CR4.VME(bit0) at BP start-up, the value shall be 0.
 */
static void v8086_rqmid_37633_CR4_VME_startup_001()
{
	u32 value = *((volatile u32 *)V8086_CR4_STARTUP_ADDR);
	value &= 0x1;

	report("\t\t %s", value == 0, __FUNCTION__);
}

/**
 * @brief case name: Initialize CR4.VME following init_001
 *
 * Summary: Get CR4.VME(bit0) at AP INIT, the value shall be 0.
 *
 */
static void v8086_rqmid_37634_CR4_VME_init_001()
{
	u32 value = *((volatile u32 *)V8086_CR4_INIT_ADDR);
	value &= 0x1;

	report("\t\t %s", value == 0, __FUNCTION__);
}

/**
 * @brief case name: Initialize CR4.PVI following start-up_001
 *
 * Summary: Get CR4.PVI(bit1) at BP start-up, the value shall be 0.
 */
static void v8086_rqmid_37631_CR4_PVI_startup_001()
{
	u32 value = *((volatile u32 *)V8086_CR4_STARTUP_ADDR);
	value &= 0x2;

	report("\t\t %s", value == 0, __FUNCTION__);
}

/**
 * @brief case name: Initialize CR4.PVI following init_001
 *
 * Summary: Get CR4.PVI(bit1) at AP INIT, the value shall be 0.
 *
 */
static void v8086_rqmid_37632_CR4_PVI_init_001()
{
	u32 value = *((volatile u32 *)V8086_CR4_INIT_ADDR);
	value &= 0x2;

	report("\t\t %s", value == 0, __FUNCTION__);
}

/**
 * @brief case name: Read CR4.VME_001.
 *
 * Summary: Read the value of CR4.VME, shall be 0H.
 *
 */
static void v8086_rqmid_37652_read_CR4_VME_001()
{
	u32 cr4 = read_cr4();
	cr4 &= 0x1;

	report("\t\t %s", cr4 == 0, __FUNCTION__);
}

/**
 * @brief case name: Read CR4.PVI_001.
 *
 * Summary: Read the value of CR4.PVI, shall be 0H.
 *
 */
static void v8086_rqmid_37653_read_CR4_PVI_001()
{
	u32 cr4 = read_cr4();
	cr4 &= 0x2;

	report("\t\t %s", cr4 == 0, __FUNCTION__);
}

#ifndef IN_NATIVE
/**
 * @brief case name: When a vCPU reads CPUID.01H_001.
 *
 * Summary:In protected mode,Test hide VME capability, reads cpuid.01h, EDX [bit 1] should be 0H.
 *
 */
static void v8086_rqmid_33868_read_CPUID_01H_001()
{
	struct cpuid r = cpuid(1);
	u32 val = r.d & 0x2;

	report("\t\t %s", val == 0, __FUNCTION__);
}
#endif

/**
 * @brief case name: When a vCPU attempts to write CR4 and the new guest CR4.VME is 1H_003.
 *
 * Summary:When cr4.vme flag bit is 1 and Cr4 register is
 * written in protect mode, hyperversion should inject GP (0).
 *
 */
static void v8086_rqmid_37947_write_CR4_VME_003()
{
	asm volatile (
		 "mov %cr4, %eax\n"
		 "bts $0, %eax\n" /* set VME */
		 );

	asm volatile (ASM_TRY("1f")
		 "mov %%eax, %%cr4\n"
		"1:\n"
		:::);
	report("\t\t %s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}

/**
 * @brief case name: When a vCPU attempts to write CR4 and the new guest CR4.PVI is 1H_003.
 *
 * Summary:When cr4.pvi flag bit is 1 and Cr4 register is
 * written in protect mode, hyperversion should inject GP (0).
 *
 */
static void v8086_rqmid_37948_write_CR4_PVI_003()
{
	asm volatile (
		 "mov %cr4, %eax\n"
		 "bts $1, %eax\n" /* set PVI */
		 );

	asm volatile (ASM_TRY("1f")
		 "mov %%eax, %%cr4\n"
		"1:\n"
		:::);
	report("\t\t %s", (exception_vector() == GP_VECTOR) && (exception_error_code() == 0), __FUNCTION__);
}

int main()
{
	setup_idt();
	print_case_list();

	v8086_rqmid_37631_CR4_PVI_startup_001();
	v8086_rqmid_37632_CR4_PVI_init_001();
	v8086_rqmid_37633_CR4_VME_startup_001();
	v8086_rqmid_37634_CR4_VME_init_001();

	v8086_rqmid_37652_read_CR4_VME_001();
	v8086_rqmid_37653_read_CR4_PVI_001();
#ifndef IN_NATIVE
	v8086_rqmid_33868_read_CPUID_01H_001();
#endif
	v8086_rqmid_37947_write_CR4_VME_003();
	v8086_rqmid_37948_write_CR4_PVI_003();

	return report_summary();
}
