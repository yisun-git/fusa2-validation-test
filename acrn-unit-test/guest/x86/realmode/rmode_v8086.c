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

asm(".code16gcc");
#include "rmode_lib.h"

struct ivt {
	u16 offset;
	u16 base;
};

u16 lastsp = 0;

/**
 * @brief Case name: ACRN hypervisor shall expose real-address mode interrupt handling to any VM_001
 *
 * Summary: Check interrupt handling and eflag flags in real mode
 *
 */
void v8086_rqmid_33906_hv_expose_realaddr_interrupt_handling_VM_001(void)
{
	u32 value = 0;

	/* get current flag */
	asm volatile ("pushf; pop %0\n\t" : "=rm"(value));

	/* get the flag be pushed in stack */
	u32 pp = lastsp;
	u16 *p = (u16 *)pp;
	u16 eflags = *(p - 1);

	/* AC(bit 18) RF(bit 16) IF(bit 9) TF(bit 8) */
	if (((eflags & 0x300) == (value & 0x300)) || ((value & 0x50300) != 0))
	{
		report(__FUNCTION__, 0);
	}
	else
	{
		report(__FUNCTION__, 1);
	}
}

#ifndef IN_NATIVE
/**
 * @brief case name: When a vCPU reads CPUID.01H_002.
 *
 * Summary:In real mode,Test hide VME capability, reads cpuid.01h, EDX [bit 1] should be 0H.
 *
 */
static void v8086_rqmid_33876_read_CPUID_01H_002(void)
{
	struct cpuid r = raw_cpuid(1, 0);

	report(__FUNCTION__, (r.d & 0x2) == 0);
}
#endif

void rmode_intr6(void)
{
	v8086_rqmid_33906_hv_expose_realaddr_interrupt_handling_VM_001();
}

struct ivt v86_intr[32];
void  rmode_init_intr6(void)
{
	v86_intr[6].offset = (u32)rmode_intr6 & 0xffff;
	v86_intr[6].base = 0;
	struct ivt *p = 0;
	*(p + 6) = v86_intr[6];
}

void v8086_rqmid_33906_test(void)
{
	rmode_init_intr6();

	u16 value = 0;
	asm volatile ("pushf; pop %0\n\t" : "=rm"(value));
	u16 *p = (u16 *)0xf800;
	*p = value;

	/* get sp of current stack */
	asm volatile("mov %%sp, %%ax \n"
		"mov %%ax, %0 \n"
		: "=r"(lastsp) : :
		);

	asm volatile("int $6\n\t");
}

u32 v8086_val1;
u32 v8086_val2;
u32 v8086_val3;

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_001_1.
 *
 * Summary: In real mode, GS registers are accessible to programs that explicitly access them.
 *
 */
void v8086_rqmid_37950_envir_001_1(void)
{
	asm volatile("mov $20, %%eax\n"
		"mov %%eax, %%gs\n"
		"mov %%gs, %%ebx\n"
		"mov %%ebx, %0\n"
		: "=m"(v8086_val2) : : "memory");

	report_ex("v8086_val2=%u", v8086_val2 == 20, v8086_val2);

	asm volatile("mov $0, %eax\n"
		"mov %eax, %gs\n");

}

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_002_1.
 *
 * Summary: .In real mode, FS registers are accessible to programs that explicitly access them
 *
 */
void v8086_rqmid_37953_envir_002_1(void)
{
	asm volatile("mov $10, %%eax\n"
		"mov %%eax, %%fs\n"
		"mov %%fs, %%ebx\n"
		"mov %%ebx, %0\n"
		: "=m"(v8086_val1) : : "memory");

	report_ex("v8086_val1=%u", v8086_val1 == 10, v8086_val1);

	asm volatile("mov $0, %eax\n"
		"mov %eax, %fs\n");
}

void test_call(void)
{
	asm volatile("mov %%edi, %%eax\n"
		"mov %%eax, %0\n"
		: "=r"(v8086_val1) : :
		);

	u16 *p;
	p = (u16 *)v8086_val1;
	if ((v8086_val2 == *p) && (0 == *(p + 1)))
	{

		v8086_val3 = 11111;
	}
}

asm("call_test_123:\n"
	"mov %esp, %edi\n"
	"call test_call\n"
	"lret\n");

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_003_1.
 *
 * Summary: In real mode, Execute the CALL instruction to check the current EIP and CS pushed on the stack.
 *
 */
void v8086_rqmid_37958_envir_003_1(void)
{
	v8086_val2 = (u32)&&ret_addr;

	asm volatile("lcall $0, $call_test_123\n");

ret_addr:
	report_ex("v8086_val3=%u", v8086_val3 == 11111, v8086_val3);
}

double v8086_add_fpu(double p, double q)
{
	double a = 0;
	double c = p;
	double d = q;

	asm volatile(
		"fldl %1\n\t"
		"faddl %2\n\t"
		"fstpl %0\n\t"
		: "=m"(a) : "m"(c), "m"(d) : "memory");

	return a;
}

double v8086_sub_fpu(double p, double q)
{
	double a = 0;
	double c = p;
	double d = q;

	asm volatile(
		"fldl %1\n\t"
		"fsubl %2\n\t"
		"fstpl %0\n\t"
		: "=m"(a) : "m"(c), "m"(d) : "memory");

	return a;
}

double v8086_mul_fpu(double p, double q)
{
	double a = 0;
	double c = p;
	double d = q;

	asm volatile(
		"fldl %1\n\t"
		"fmull %2\n\t"
		"fstpl %0\n\t"
		: "=m"(a) : "m"(c), "m"(d) : "memory");

	return a;
}

double v8086_div_fpu(double p, double q)
{
	double a = 0;
	double c = p;
	double d = q;

	asm volatile(
		"fldl %1\n\t"
		"fdivl %2\n\t"
		"fstpl %0\n\t"
		: "=m"(a) : "m"(c), "m"(d) : "memory");

	return a;
}

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_004_1.
 *
 * Summary: In real mode, The x87 FPU is available to execute x87 FPU instructions(addition).
 *
 */
void v8086_rqmid_37951_envir_004_1(void)
{
	int aa;
	aa = (int)v8086_add_fpu(20.50, 10.50);

	report_ex("aa=%u", aa == 31, aa);
}

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_005_1.
 *
 * Summary: In real mode, The x87 FPU is available to execute x87 FPU instructions(subtraction).
 *
 */
void v8086_rqmid_37954_envir_005_1(void)
{
	int aa;
	aa = (int)v8086_sub_fpu(20.50, 10.50);

	report_ex("aa=%u", aa == 10, aa);
}


/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_006_1.
 *
 * Summary: In real mode, The x87 FPU is available to execute x87 FPU instructions(multiplication).
 *
 */
void v8086_rqmid_37955_envir_006_1(void)
{
	int aa;
	aa = (int)v8086_mul_fpu(20.50, 10.00);

	report_ex("aa=%u", aa == 205, aa);
}


/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_007_1.
 *
 * Summary: In real mode, The x87 FPU is available to execute x87 FPU instructions(division).
 *
 */
void v8086_rqmid_37956_envir_007_1(void)
{
	int aa;
	aa = (int)v8086_div_fpu(22.50, 1.50);

	report_ex("aa=%u", aa == 15, aa);
}


/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_008_1.
 *
 * Summary: In real mode, check EFLAGS low 16Bit flag is correct.
 *
 */
void v8086_rqmid_37952_envir_008_1(void)
{
	u32 value = 0;

	asm ("stc");
	asm ("std");
	asm ("sti");

	/* get current flag */
	asm volatile ("pushf; pop %0\n\t" : "=rm"(value));
	if (!((value & 1) && (value & (1 << 9)) && (value & (1 << 10))))
	{
		return;
	}

	asm ("clc");
	asm ("cld");
	asm ("cli");

	/* get current flag */
	asm volatile ("pushf; pop %0\n\t" : "=rm"(value));
	if ((value & 1) || (value & (1 << 9)) || (value & (1 << 10)))
	{
		return;
	}

	if (!(value & (1 << 1)) || (value & (1 << 3)) || (value & (1 << 5)))
	{
		return;
	}

	report(__FUNCTION__, 1);
}

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_009.
 *
 * Summary: In real mode, move the 16-bit data Segment Selector to the left by 4 bits
 * and add the guest logical address to get the guest physical address.
 *
 */
#define MAGIC_WORD 0xbeef
__attribute__((aligned(64)))u16 magic = MAGIC_WORD;
void v8086_rqmid_38245_envir_009(void)
{
	u16 ret = 0;
	u16 *p = (u16 *)&magic;
	u32 val = (u32)(p);
	val = val >> 4;

	asm volatile(
		"mov %1, %%eax\n"
		"mov %%eax, %%gs\n"
		"movw %%gs:0, %%ax\n"
		"mov %%ax, %0\n"
		: "=m"(ret) : "m"(val) : "memory");

	report_ex("ret=0x%x", ret == MAGIC_WORD, ret);
}

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_010.
 *
 * Summary: In real mode, Add a data or address prefix before machine code to
 * access 32bit data or addresses.
 *
 */
void v8086_rqmid_38255_envir_010(void)
{
	u32 val2 = 0;

	asm volatile("mov $0x1111, %%eax\n" : : : );
	asm volatile("mov %%eax, %0\n" : "=m"(val2) :: "memory");

	report_ex("val2=0x%x", val2 == 0x1111, val2);
}

struct lseg_st {
	u32 offset;
	u16 selector;
};

struct descriptor_table_ptr {
	u16 limit;
	u32 base;
} __attribute__((packed));

extern u32 gdt32_descr;

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_011.
 *
 * Summary:Check the instructions that have been added to later IA-32 processors
 * can be executed in real-address mode.
 *
 */
void v8086_rqmid_38256_envir_011(void)
{
	u32 val = 1;
	asm volatile ("mov %%dr0, %0" : "=r"(val));
	asm volatile ("mov %0, %%dr0" : : "r"(val) : "memory");

	struct lseg_st sval;

	sval.offset = 0;
	sval.selector = 0;

	asm volatile("lss  %0, %%eax\t\n" ::"m"(sval));
	asm volatile("lgs  %0, %%eax\t\n" ::"m"(sval));

	asm volatile ("mul %0\n" : : "r"(val) : "memory");
	asm volatile ("imul %0\n" : : "r"(val) : "memory");

	asm volatile ("sal $1, %eax\n");
	asm volatile ("shl $1, %eax\n");
	asm volatile ("sar $1, %eax\n");
	asm volatile ("shr $1, %eax\n");
	asm volatile ("shld $1, %eax, %ebx\n");
	asm volatile ("shrd $1, %eax, %ebx\n");
	asm volatile ("rcl $1, %eax\n");
	asm volatile ("rcr $1, %eax\n");
	asm volatile ("rol $1, %eax\n");
	asm volatile ("ror $1, %eax\n");

	asm volatile ("pusha\n");
	asm volatile ("popa\n");
	//asm volatile ("pushad\n");
	//asm volatile ("popad\n");
	asm volatile ("push %eax\n");
	asm volatile ("pop %eax\n");

	asm volatile ("movsx %ax, %ebx\n");
	asm volatile ("movzx %al, %ebx\n");

	asm volatile ("jz 1f;1:\n");

	asm volatile ("cmpxchg %eax, %ebx\n");
	asm volatile ("xadd %eax, %ebx\n");

	asm volatile ("movsb");
	asm volatile ("cmpsb");
	asm volatile ("scasb");
	asm volatile ("lodsb");
	asm volatile ("stosb");

	asm volatile ("bt $0, %eax\n");
	asm volatile ("btc $0, %eax\n");
	asm volatile ("btr $0, %eax\n");
	asm volatile ("bts $0, %eax\n");
	asm volatile ("bsf %ebx, %eax\n");
	asm volatile ("bsr %ebx, %eax\n");

	asm volatile ("setz %al\n");

	asm volatile ("bswap %eax\n");

	asm volatile("pushf\n\t");
	asm volatile("popf\n\t");

	asm volatile("enter $0x10, $0x1\n\t");
	asm volatile("leave\n\t");

	raw_cpuid(1, 0);

	asm volatile("clts\n\t");
#ifdef IN_NATIVE
	asm volatile("wbinvd\n\t");
	asm volatile("invd\n\t");
#endif
	//asm volatile("winvd\n\t");
	u32 aa = 0x1;
	void *va = &aa;
	asm volatile("invlpg (%0)" ::"r" (va) : "memory");
	struct descriptor_table_ptr dt;
	struct descriptor_table_ptr *ptr = (struct descriptor_table_ptr *)(&dt);
	asm volatile ("sgdt %0" : "=m"(*ptr));
	asm volatile ("lgdt %0" : : "m"(*ptr));
	asm volatile ("sidt %0" : "=m"(*ptr));
	asm volatile ("lidt %0" : : "m"(*ptr));
	asm volatile ("smsw %ax");
	asm volatile ("lmsw %ax");

	asm volatile ("rdtsc\n");
	//asm volatile ("rdpmc\n");

	report(__FUNCTION__, 1);
}

void v8086_rqmid_envir(void)
{
	v8086_rqmid_37950_envir_001_1();
	v8086_rqmid_37953_envir_002_1();
	v8086_rqmid_37958_envir_003_1();
	//init FPU before each case.
	asm volatile("fninit");
	v8086_rqmid_37951_envir_004_1();
	asm volatile("fninit");
	v8086_rqmid_37954_envir_005_1();
	asm volatile("fninit");
	v8086_rqmid_37955_envir_006_1();
	asm volatile("fninit");
	v8086_rqmid_37956_envir_007_1();
	v8086_rqmid_37952_envir_008_1();
	v8086_rqmid_38245_envir_009();
	v8086_rqmid_38255_envir_010();
	v8086_rqmid_38256_envir_011();
}

u32 v8086_get_cr4(void)
{
	u32 cr4 = 0;

	/* get cr4 */
	asm volatile("mov %%cr4, %%eax \n"
		"mov %%eax, %0 \n"
		: "=r"(cr4) : :
		);

	return cr4;
}

/**
 * @brief case name: When a vCPU attempts to write CR4 and the new guest CR4.VME is 1H_002.
 *
 * Summary:When cr4.vme flag bit is 1 and Cr4 register is
 * written in real mode, hyperversion should inject GP (0).
 *
 */
void v8086_rqmid_37946_write_CR4_VME_002(void)
{
	asm volatile (
		 "mov %%cr4, %%eax\n"
		 "bts $0, %%eax\n" /* set VME */
		 ASM_TRY("1f")
		 "mov %%eax, %%cr4\n"
		 "1:\n"
		  :);

	u8 vecter = exception_vector();
	report_ex("vector=%u", GP_VECTOR == vecter, vecter);
}

/**
 * @brief case name: When a vCPU attempts to write CR4 and the new guest CR4.PVI is 1H_002.
 *
 * Summary:When cr4.pvi flag bit is 1 and Cr4 register is
 * written in real mode, hyperversion should inject GP (0).
 *
 */
void v8086_rqmid_37945_write_CR4_PVI_002(void)
{
	asm volatile (
		 "mov %%cr4, %%eax\n"
		 "bts $1, %%eax\n" /* set PVI */
		 ASM_TRY("1f")
		 "mov %%eax, %%cr4\n"
		 "1:\n"
		 :);
	u8 vecter = exception_vector();
	report_ex("vector=%u", GP_VECTOR == vecter, vecter);
}

void v8086_rqmid_hide(void)
{
	v8086_rqmid_37946_write_CR4_VME_002();
	v8086_rqmid_37945_write_CR4_PVI_002();
}

void print_case_list(void)
{
	print_serial("v8086 feature case list:\n\r");
	print_serial("\t\tCase ID:33906 case name:ACRN hypervisor shall ");
	print_serial("expose real-address mode interrupt handling to any VM_001\n\r");
#ifndef IN_NATIVE
	print_serial("\t\tCase ID:33876 case name:When a vCPU reads CPUID.01H_002\n\r");
#endif
	print_serial("\t\tCase ID:37950 case name:Real and virtual-8086 mode execution environment_001_1\n\r");
	print_serial("\t\tCase ID:37953 case name:Real and virtual-8086 mode execution environment_002_1\n\r");
	print_serial("\t\tCase ID:37958 case name:Real and virtual-8086 mode execution environment_003_1\n\r");
	print_serial("\t\tCase ID:37951 case name:Real and virtual-8086 mode execution environment_004_1\n\r");
	print_serial("\t\tCase ID:37954 case name:Real and virtual-8086 mode execution environment_005_1\n\r");
	print_serial("\t\tCase ID:37955 case name:Real and virtual-8086 mode execution environment_006_1\n\r");
	print_serial("\t\tCase ID:37956 case name:Real and virtual-8086 mode execution environment_007_1\n\r");
	print_serial("\t\tCase ID:37952 case name:Real and virtual-8086 mode execution environment_008_1\n\r");
	print_serial("\t\tCase ID:38245 case name:Real and virtual-8086 mode execution environment_009\n\r");
	print_serial("\t\tCase ID:38255 case name:Real and virtual-8086 mode execution environment_010\n\r");
	print_serial("\t\tCase ID:38256 case name:Real and virtual-8086 mode execution environment_011\n\r");
	print_serial("\t\tCase ID:37946 case name:write CR4 and the new guest CR4.VME is 1H_002\n\r");
	print_serial("\t\tCase ID:37945 case name:write CR4 and the new guest CR4.PVI is 1H_002\n\r");
}

void main(void)
{
	set_handle_exception();
	print_case_list();

	v8086_rqmid_33906_test();
#ifndef IN_NATIVE
	v8086_rqmid_33876_read_CPUID_01H_002();
#endif
	v8086_rqmid_envir();
	v8086_rqmid_hide();

	report_summary();

	asm volatile("hlt\n\t");
}

