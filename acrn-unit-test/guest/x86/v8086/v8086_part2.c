asm(".code16gcc");

#include "v8086_lib.h"
extern u32 v8086_iopl;

#define TRY_INSTRUCTION(vec, ins) do {\
	u8 vecter = NO_EXCEPTION;\
	asm volatile(\
		ASM_TRY("1f\n")\
		ins \
		"1:\n"\
		);\
	vecter = exception_vector();\
	report_ex("vector=%u", (vec) == vecter, vecter);\
} while (0)

u8 v8086_set_cr4(u16 bit)
{
	u32 cr4 = 0x10 | (1 << bit);

	asm volatile (
		ASM_TRY("1f")
		"mov %0, %%cr4\n"
		"1:"
		:
		: "rm"(cr4)
		: "memory"
	);
	return exception_vector();
}

/**
 * @brief case name: When a vCPU attempts to write CR4 and the new guest CR4.VME is 1H_001.
 *
 * Summary:When cr4.vme flag bit is 1 and Cr4 register is
 * written in v86 mode, hyperversion should inject GP (0).
 *
 */
__noinline void v8086_rqmid_33866_write_CR4_VME_001(void)
{
	u8 vecter = v8086_set_cr4(0); /*0:  set VME */

	report_ex("vector=%u", (GP_VECTOR == vecter), vecter);
}

/**
 * @brief case name: When a vCPU attempts to write CR4 and the new guest CR4.PVI is 1H_001.
 *
 * Summary:When cr4.pvi flag bit is 1 and Cr4 register is
 * written in v86 mode, hyperversion should inject GP (0).
 *
 */
__noinline void v8086_rqmid_33867_write_CR4_PVI_001(void)
{
	u8 vecter = v8086_set_cr4(1); /*1: set PVI */

	report_ex("vector=%u", (GP_VECTOR == vecter), vecter);
}

#ifndef IN_NATIVE
/**
 * @brief case name: When a vCPU reads CPUID.01H_003.
 *
 * Summary:In v8086 mode,Test hide VME capability, reads cpuid.01h, EDX [bit 1] should be 0H.
 *
 */
__noinline void v8086_rqmid_33886_read_CPUID_01H_003(void)
{
	struct cpuid r = raw_cpuid(1, 0);

	report_ex("edx=0x%x", (r.d & 0x2) == 0, r.d);
}
#endif

#define RTC_INDEX_REG       0x70
#define RTC_TARGET_REG      0x71
#define B_INDEX             0xB
#define RTC_B_DEFAULT_MASK  0x58 /* Default: bits 7,5,2,1,0 is undefined */
#define RTC_B_DEFAULT_VALUE 0x00

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_001
 *
 * Summary: Check v8086i/o protection, clear io bitmap, write and read RTC port in v8086 mode.
 *
 */
__noinline void v8086_rqmid_33890_expose_io_protection_001(void)
{
	clear_iomap();
	v8086_outb(B_INDEX, RTC_INDEX_REG);
	u8 val = v8086_inb(RTC_TARGET_REG);
	report_ex("val = 0x%x", (val & RTC_B_DEFAULT_MASK) == RTC_B_DEFAULT_VALUE, val);
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_002
 *
 * Summary: Check v8086i/o protection, set io bitmap, read RTC port in v8086 mode.
 *
 */
__noinline void v8086_rqmid_37666_expose_io_protection_002(void)
{
	u8 vecter = NO_EXCEPTION;
	u16 port = RTC_TARGET_REG;

	set_iomap(port);
	asm volatile(
		ASM_TRY("1f")
		"in %0, %%al\n"
		"1:\n"
		 :
		 : "d"(port)
		 : "memory"
	);
	vecter = exception_vector();
	report_ex("vector=%u", GP_VECTOR == vecter, vecter);
}

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_001.
 *
 * Summary: In virtual-8086 mode, GS registers are accessible to programs that explicitly access them.
 *
 */
__noinline void v8086_rqmid_37857_envir_001(void)
{
	u16 gs = 0;

	write_gs(MAGIC_WORD);
	gs = read_gs();
	write_gs(0);

	report_ex("gs = 0x%x", MAGIC_WORD == gs, gs);
}

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_002.
 *
 * Summary: .In virtual-8086 mode, FS registers are accessible to programs that explicitly access them
 *
 */
__noinline void v8086_rqmid_37858_envir_002(void)
{
	u16 fs = 0;

	write_fs(MAGIC_WORD);
	fs = read_fs();
	write_fs(0);

	report_ex("fs = 0x%x", MAGIC_WORD == fs, fs);
}

u32 v8086_val1;
u32 v8086_val2;
u32 v8086_val3;

void test_call(void)
{
	asm volatile("mov %%edi, %%eax\n"
		"mov %%eax, %0\n"
		: "=r"(v8086_val1) : :
	);

	u16 *p;
	p = (u16 *)(v8086_val1);

	if ((v8086_val2 == *p) && (0 == *(p + 1)))
		v8086_val3 = 11111;
}

asm (
"call_test_123:\n"
	"mov %esp, %edi\n"
	"call test_call\n"
	"lret\n"
);

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_003.
 *
 * Summary: In virtual-8086 mode, Execute the CALL instruction to check the current EIP and CS pushed on the stack.
 *
 */
__noinline void v8086_rqmid_37859_envir_003(void)
{
	v8086_val2 = (u32)&&ret_addr;

	asm volatile("lcall $0, $call_test_123\n");

ret_addr:
	report_ex("v8086_val3 = 0x%x", v8086_val3 == 11111, v8086_val3);
}

__noinline double v8086_add_fpu(double p, double q)
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

__noinline double v8086_sub_fpu(double p, double q)
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

__noinline double v8086_mul_fpu(double p, double q)
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

__noinline double v8086_div_fpu(double p, double q)
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
 * @brief case name: Real-address and virtual-8086 mode execution environment_004.
 *
 * Summary: In virtual-8086 mode, The x87 FPU is available to execute x87 FPU instructions(addition).
 *
 */
__noinline void v8086_rqmid_37864_envir_004(void)
{
	int sum = (int)v8086_add_fpu(20.50, 10.50);

	report_ex("sum = 0x%x", sum == 31, sum);
}

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_005.
 *
 * Summary: In virtual-8086 mode, The x87 FPU is available to execute x87 FPU instructions(subtraction).
 *
 */
__noinline void v8086_rqmid_37865_envir_005(void)
{
	int difference = (int)v8086_sub_fpu(20.50, 10.50);

	report_ex("difference = 0x%x", difference == 10, difference);
}


/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_006.
 *
 * Summary: In virtual-8086 mode, The x87 FPU is available to execute x87 FPU instructions(multiplication).
 *
 */
__noinline void v8086_rqmid_37866_envir_006(void)
{
	int product = (int)v8086_mul_fpu(20.50, 10.00);

	report_ex("product = 0x%x", product == 205, product);
}


/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_007.
 *
 * Summary: In virtual-8086 mode, The x87 FPU is available to execute x87 FPU instructions(division).
 *
 */
__noinline void v8086_rqmid_37867_envir_007(void)
{
	int quotient = (int)v8086_div_fpu(22.50, 1.50);

	report_ex("quotient = 0x%x", quotient == 15, quotient);
}


/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_008.
 *
 * Summary: In virtual-8086 mode, check EFLAGS low 16Bit flag is correct.
 *
 */
__noinline void v8086_rqmid_37868_envir_008(void)
{
	int result = 0;
	u32 value = 0;

	asm ("stc");
	asm ("std");
	asm ("sti");

	/* get current flag */
	value = read_flags();
	if (!((value & 1) && (value & (1 << 9)) && (value & (1 << 10)))) {
		result = 1;
		goto end;
	}

	asm ("clc");
	asm ("cld");
	asm ("cli");

	/* get current flag */
	value = read_flags();
	if ((value & 1) || (value & (1 << 9)) || (value & (1 << 10))) {
		result = 2;
		goto end;
	}

	if (!(value & (1 << 1)) || (value & (1 << 3)) || (value & (1 << 5))) {
		result = 3;
		goto end;
	}

end:
	report_ex("result = 0x%x", 0 == result, result);
}

/**
 * @brief Case name: Virtual-8086 mode interrupt handling to any VM_001.
 *
 * Summary: Realize interrupt processing in v8086 mode
 *
 */
__noinline void v8086_rqmid_33813_interrupt_handling_001(void)
{
	set_igdt(21, SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_INTERGATE, 0);

	write_v8086_esp();
	write_input_val(read_flags());

	asm volatile("int $21\n\t");
	/* VM, RF, NT, TF, IF of eflags is 0 */
	report_ex("eflags = 0x%x", (MAGIC_DWORD != *temp_value) &&
		(*temp_value & (X86_EFLAGS_VM | X86_EFLAGS_RF | X86_EFLAGS_NT | X86_EFLAGS_IF | X86_EFLAGS_TF)) == 0,
		*temp_value);
	clear_v8086_esp();
}

/**
 * @brief Case name: Virtual-8086 mode interrupt handling to any VM_002.
 *
 * Summary: If the trap or interrupt gate references a procedure in
 *  a segment at a privilege level other than 0, generate the #GP(0).
 *
 */
__noinline void v8086_rqmid_37609_interrupt_handling_002(void)
{
	/*intr gate, dpl = 3 */
	set_igdt(22, SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_INTERGATE,
		SEGMENT_PRESENT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_3 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_CODE_EXE_READ_ACCESSED);
	TRY_INSTRUCTION(GP_VECTOR, "int $22\n");
}

/**
 * @brief Case name: Virtual-8086 mode interrupt handling to any VM_003.
 *
 * Summary: If the trap or interrupt gate references a procedure in
 *  a conforming segment, generate the #GP(0).
 *
 */
__noinline void v8086_rqmid_37610_interrupt_handling_003(void)
{
	set_igdt(23, SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_INTERGATE,
		SEGMENT_PRESENT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_0 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED);
	TRY_INSTRUCTION(GP_VECTOR, "int $23\n");
}

/**
 * @brief Case name: Virtual-8086 mode interrupt handling to any VM_003_1.
 *
 * Summary: If the trap or interrupt gate references a procedure in
 *  a conforming segment, generate the #GP(0).
 *
 */
__noinline void v8086_rqmid_38116_interrupt_handling_003_1(void)
{
	set_igdt(23, SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TRAPGATE,
		SEGMENT_PRESENT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_0 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_CODE_EXE_READ_ONLY_CONFORMING_ACCESSED);
	TRY_INSTRUCTION(GP_VECTOR, "int $23\n");
}

/**
 * @brief Case name: Virtual-8086 mode interrupt handling to any VM_001_1.
 *
 * Summary: Realize interrupt processing in v8086 mode
 *
 */
__noinline void v8086_rqmid_38113_interrupt_handling_001_1(void)
{
	set_igdt(21, SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TRAPGATE, 0);

	u32 flags = read_flags();
	write_v8086_esp();
	write_input_val(flags);

	asm volatile("int $21\n\t");

	/* IF of eflags is not changed */
	report_ex("eflags = 0x%x", (MAGIC_DWORD != *temp_value) &&
		(*temp_value & (X86_EFLAGS_VM | X86_EFLAGS_RF | X86_EFLAGS_NT | X86_EFLAGS_TF)) == 0 &&
		(*temp_value & X86_EFLAGS_IF) == (flags & X86_EFLAGS_IF), *temp_value);
	clear_v8086_esp();
}

/**
 * @brief Case name: Virtual-8086 mode interrupt handling to any VM_002_1.
 *
 * Summary: If the trap or interrupt gate references a procedure in
 *  a segment at a privilege level other than 0, generate the #GP(0).
 *
 */
__noinline void v8086_rqmid_38114_interrupt_handling_002_1(void)
{
	/* intr gate, dpl = 2 */
	set_igdt(22, SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_INTERGATE,
		SEGMENT_PRESENT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_2 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_CODE_EXE_READ_ACCESSED);
	TRY_INSTRUCTION(GP_VECTOR, "int $22\n");
}

void v8086_main(void)
{
	if (X86_EFLAGS_IOPL3 == (v8086_iopl & X86_EFLAGS_IOPL3)) {
		v8086_rqmid_33866_write_CR4_VME_001();
		v8086_rqmid_33867_write_CR4_PVI_001();
# ifndef IN_NATIVE
		v8086_rqmid_33886_read_CPUID_01H_003();
# endif
		v8086_rqmid_37666_expose_io_protection_002();
		v8086_rqmid_33890_expose_io_protection_001();

		v8086_rqmid_37857_envir_001();
		v8086_rqmid_37858_envir_002();
		v8086_rqmid_37859_envir_003();
		//init FPU before each case.
		asm volatile("fninit");
		v8086_rqmid_37864_envir_004();
		asm volatile("fninit");
		v8086_rqmid_37865_envir_005();
		asm volatile("fninit");
		v8086_rqmid_37866_envir_006();
		asm volatile("fninit");
		v8086_rqmid_37867_envir_007();
		v8086_rqmid_37868_envir_008();

		v8086_rqmid_33813_interrupt_handling_001();
		v8086_rqmid_37609_interrupt_handling_002();
		v8086_rqmid_37610_interrupt_handling_003();
		v8086_rqmid_38116_interrupt_handling_003_1();
		v8086_rqmid_38113_interrupt_handling_001_1();
		v8086_rqmid_38114_interrupt_handling_002_1();
	}

	report_summary();
	send_cmd(FUNC_V8086_EXIT);
}

