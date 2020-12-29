asm(".code16gcc");

#include "v8086_lib.h"

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

#define TRY_SEGMENT(seg) do {\
	u8 vecter = NO_EXCEPTION;\
	u16 ecode = 0;\
	asm volatile(\
		"movl %ebx, %ecx\n"\
		"movl $0x10001, %ebx\n"\
		ASM_TRY("1f")\
		"movw %" #seg ":(%ebx), %ax\n"\
		"1:"\
		"movl %ecx, %ebx\n"\
		);\
	vecter = exception_vector();\
	ecode = exception_error_code();\
	report_ex(#seg "vector=%u, ecode=0x%x",\
		(GP_VECTOR == vecter && 0 == ecode), vecter, ecode);\
} while (0)

/**
 * @brief Case name: ACRN hypervisor shall expose virtual-8086 mode paging to any VM_001.
 *
 * Summary:Paging of Virtual-8086 Tasks.
 *
 */
__noinline void v8086_rqmid_33898_expose_paging_001(void)
{
	u16 value = 0;
	u32 test_addr = 0x800;   /* 2K */
	u8 page_index = 3;
	u8 page_size_shift = 12; /* 4K */

	*(u16 *)test_addr = MAGIC_WORD;
	set_page(page_index, 0x177 |  (0 << page_size_shift));
	value = *(u16 *)(page_index * (1 << page_size_shift) + test_addr);
	set_page(page_index, 0x177 |  (3 << page_size_shift));
	report_ex("value = 0x%x", MAGIC_WORD == value, value);
}

/**
 * @brief case name: Real-address and virtual-8086 mode execution environment_010_1.
 *
 * Summary: In virtual-8086 mode, Add a data or address prefix before machine code to
 * access 32bit data or addresses.
 *
 */
__noinline void v8086_rqmid_38307_envir_010_1(void)
{
	u32 val = 0;
	asm volatile("mov $" xstr(MAGIC_DWORD) ", %0\n" : "=rm"(val) : : "memory");
	report_ex("val = 0x%x", val == MAGIC_DWORD, val);
}

/**
 * @brief case name: Real-address and virtual-8086 mode address translation_003.
 *
 * Summary: In V8086 mode and real-address mode,  access a memory with an offset greater than
 * 0 -0xFFFFH, a pseudo-protection faults (interrupt 12 or 13) will be generated.
 *
 */
__noinline void v8086_rqmid_40491_check_offset_out_of_range(void)
{
	TRY_SEGMENT(fs);
}

__noinline u8 v8086_check_addr_translation(u16 segment, u16 offset)
{
	u16 output = 0;
	u32 addr = (((u32)segment << 4) + offset) & 0xfffff;
	*(u16 *)addr = MAGIC_WORD;

	asm volatile (
		"movw %%bx, %%cx\n"
		"movw %1, %%fs\n"
		"movw %2, %%bx\n"
		ASM_TRY("1f")
		"movw %%fs:(%%bx), %0\n"
		"1:\n"
		"movw %%cx, %%bx\n"
		: "=rm"(output)
		: "rm"(segment), "rm"(offset)
		: "bx", "cx"
	);

	write_fs(0);
	if (MAGIC_WORD != output)
		printf("segment=0x%x, offset=0x%x, addr=0x%x, output=0x%x\n", segment, offset, addr, output);

	return MAGIC_WORD == output;
}

/**
 * @brief case name: Real-address and virtual-8086 mode address translation_001.
 *
 * Summary: In real-address/v8086 mode, It shifts the segment selector left by 4 bits to form a 20-bit base address .
 * The offset into a segment is added to the base address to create a linear address that maps directly to the
 * physical address space.
 *
 */
__noinline void v8086_rqmid_45232_check_address_translation(void)
{
	u8 data[32] = {0};
	u16 offset = (u16)(u32)data;
	u8 result = v8086_check_addr_translation(0x1, offset);
	u8 vecter  = exception_vector();
	report_ex("vecter=%u", (NO_EXCEPTION == vecter && result), vecter);
}

/**
 * @brief Case name: Real-address and virtual-8086 mode address translation_002.
 *
 * Summary: In 8086 Emulation mode, with a segment selector value of 0xFFFFH and an offset of 0xFFFFH,
 * the linear (and physical) address would be 10FFEFH. Write a Magic number in the memory unit whose
 * physical address is 0xFFEFH and then access the segment selector is 0xFFFFH and offset is the memory
 * unit of 0xFFFFH. The value read should be Magic.
 *
 */
__noinline void v8086_rqmid_40493_check_address_wrapround(void)
{
	//GCC will generate a wrong address for ((0xffff << 4) + 0xffff) & 0xfffff.
	u8 result = v8086_check_addr_translation(0xffff, 0xfffe);
	u8 vecter  = exception_vector();
	report_ex("vecter=%u", (NO_EXCEPTION == vecter && result), vecter);
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_003
 *
 * Summary: Check v8086i/o protection, Execute CLI instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_37673_expose_io_protection_003(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "cli\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_004
 *
 * Summary: Check v8086i/o protection, Execute STI instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_37674_expose_io_protection_004(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "sti\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_005
 *
 * Summary: Check v8086i/o protection, Execute PUSHF instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_37675_expose_io_protection_005(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "pushf\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_006
 *
 * Summary: Check v8086i/o protection, Execute POPF instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_37676_expose_io_protection_006(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "popf\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_007
 *
 * Summary: Check v8086i/o protection, Execute IRET instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_37677_expose_io_protection_007(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "iret\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_008
 *
 * Summary: Check v8086i/o protection, Execute INT instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_37611_expose_io_protection_008(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "int $24\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_003_1
 *
 * Summary: Check v8086 i/o protection, Execute CLI instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_38123_expose_io_protection_003_1(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "cli\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_004_1
 *
 * Summary: Check v8086 i/o protection, Execute STI instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_38122_expose_io_protection_004_1(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "sti\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_005_1
 *
 * Summary: Check v8086 i/o protection, Execute PUSHF instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_38124_expose_io_protection_005_1(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "pushf\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_006_1
 *
 * Summary: Check v8086 i/o protection, Execute POPF instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_38125_expose_io_protection_006_1(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "popf\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_007_1
 *
 * Summary: Check v8086i/o protection, Execute IRET instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_38126_expose_io_protection_007_1(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "iret\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_008_1
 *
 * Summary: Check v8086 i/o protection, Execute INT instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_38127_expose_io_protection_008_1(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "int $24\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_003_2
 *
 * Summary: Check v8086 i/o protection, Execute CLI instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_38128_expose_io_protection_003_2(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "cli\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_004_2
 *
 * Summary: Check v8086i/o protection, Execute STI instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_38129_expose_io_protection_004_2(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "sti\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_005_2
 *
 * Summary: Check v8086 i/o protection, Execute PUSHF instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_38130_expose_io_protection_005_2(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "pushf\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_006_2
 *
 * Summary: Check v8086 i/o protection, Execute POPF instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_38131_expose_io_protection_006_2(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "popf\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_007_2
 *
 * Summary: Check v8086 i/o protection, Execute IRET instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_38132_expose_io_protection_007_2(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "iret\n");
}

/**
 * @brief case name: ACRN hypervisor shall expose virtual-8086 mode I/O protection to any VM_008_2
 *
 * Summary: Check v8086 i/o protection, Execute INT instructions, shall be GP(0).
 *
 */
__noinline void v8086_rqmid_38133_expose_io_protection_008_2(void)
{
	TRY_INSTRUCTION(GP_VECTOR, "int $24\n");
}

extern u32 v8086_iopl;
void v8086_main(void)
{
	u32 iopl = v8086_iopl & X86_EFLAGS_IOPL3;

	if (X86_EFLAGS_IOPL3 == iopl) {
		//printf("flags=0x%x\n",  read_flags());
		set_igdt(24, 0, 0);
		v8086_rqmid_33898_expose_paging_001();
		v8086_rqmid_38307_envir_010_1();
		v8086_rqmid_40491_check_offset_out_of_range();
		v8086_rqmid_45232_check_address_translation();
		v8086_rqmid_40493_check_address_wrapround();
		set_iopl(X86_EFLAGS_IOPL0);
	} else if (X86_EFLAGS_IOPL0 == iopl) {
		v8086_rqmid_37673_expose_io_protection_003();
		v8086_rqmid_37674_expose_io_protection_004();
		v8086_rqmid_37675_expose_io_protection_005();
		v8086_rqmid_37676_expose_io_protection_006();
		v8086_rqmid_37677_expose_io_protection_007();
		v8086_rqmid_37611_expose_io_protection_008();
		set_iopl(X86_EFLAGS_IOPL1);
	} else if (X86_EFLAGS_IOPL1 == iopl) {
		v8086_rqmid_38123_expose_io_protection_003_1();
		v8086_rqmid_38122_expose_io_protection_004_1();
		v8086_rqmid_38124_expose_io_protection_005_1();
		v8086_rqmid_38125_expose_io_protection_006_1();
		v8086_rqmid_38126_expose_io_protection_007_1();
		v8086_rqmid_38127_expose_io_protection_008_1();
		set_iopl(X86_EFLAGS_IOPL2);
	} else if (X86_EFLAGS_IOPL2 == iopl) {
		v8086_rqmid_38128_expose_io_protection_003_2();
		v8086_rqmid_38129_expose_io_protection_004_2();
		v8086_rqmid_38130_expose_io_protection_005_2();
		v8086_rqmid_38131_expose_io_protection_006_2();
		v8086_rqmid_38132_expose_io_protection_007_2();
		v8086_rqmid_38133_expose_io_protection_008_2();
		report_summary();
		send_cmd(FUNC_V8086_EXIT);
	}
}

