asm(".code16gcc");
#include "rmode_lib.h"

#define MAGIC_DWORD       0xfeedbabe
#define MAGIC_WORD        0xcafe

#define RET_SUCCESS       0x0000
#define RET_FAILURE       0xffff

struct far_pointer32 {
	u32 offset;
	u16 selector;
} __attribute__((packed));

#define TRY_SEGMENT(seg) do {\
	u8 vecter = NO_EXCEPTION;\
	u16 ecode = 0;\
	asm volatile(\
		"movl %%ebx, %%ecx\n"\
		"movl $0x10001, %%ebx\n"\
		ASM_TRY("1f")\
		"movw %%" #seg ":(%%ebx), %%ax\n"\
		"1:"\
		"movl %%ecx, %%ebx\n"\
		: );\
	vecter = exception_vector();\
	ecode = exception_error_code();\
	report_ex(#seg "vector=%u, ecode=0x%x",\
	((GP_VECTOR == vecter) && (0 == ecode)), vecter, ecode);\
} while (0)

#define REQUEST_IRQ_EX(vec, handler, ecode) do {\
	test_for_exception((vec), (handler), 0);\
	u8 vecter = exception_vector();\
	u16 err_code = exception_error_code();\
	report_ex("vector=%u, ecode=0x%x",\
		(((vec) == vecter) && ((ecode) == err_code)), vecter, err_code);\
} while (0)

#define TRY_FLUSH(ins, addr) do {\
	u8 vecter = NO_EXCEPTION;\
	asm volatile(\
		ASM_TRY("1f")\
		ins " (%0)\n"\
		"1:"\
		: : "b" (addr));\
	vecter = exception_vector();\
	report_ex(ins ", vector=%u", (GP_VECTOR == vecter), vecter);\
} while (0)

/**
 * Table32 - row 1
 * In v8086 or real mode, move a memory operand effective address, which is outside the range 0
 * through 0xFFFF, to DS segment registers, will generate an #GP.
 */
__noinline void v8086_rqmid_46053_segmentation_table32_001(void)
{
	TRY_SEGMENT(ds);
}

/**
 * Table32 - row 1
 * In v8086 or real mode, move a memory operand effective address, which is outside the range 0
 * through 0xFFFF, to ES segment registers, will generate an #GP.
 */
__noinline void v8086_rqmid_46054_segmentation_table32_002(void)
{
	TRY_SEGMENT(es);
}

/**
 * Table32 - row 1
 * In v8086 or real mode, move a memory operand effective address, which is outside the range 0
 * through 0xFFFF, to FS segment registers, will generate an #GP.
 */
__noinline void v8086_rqmid_46055_segmentation_table32_003(void)
{
	TRY_SEGMENT(fs);
}

/**
 * Table32 - row 1
 * In v8086 or real mode, move a memory operand effective address, which is outside the range 0
 * through 0xFFFF, to GS segment registers, will generate an #GP.
 */
__noinline void v8086_rqmid_46056_segmentation_table32_004(void)
{
	TRY_SEGMENT(gs);
}

__noinline void outside_64k_handler(void *data)
{
	asm volatile (
		"movl $0xffffe, %%ebx\n"
		"movl $0xdead, (%%ebx)\n"
		: :	: "memory"
	);
}

/**
 * Table34 - row 1
 * In v8086 or real mode, move data to a memory operand effective address,
 * which is outside the range 0 through 0xFFFF, will generate an #GP.
 */
__noinline void v8086_rqmid_46061_segmentation_table34_001(void)
{
	REQUEST_IRQ_EX(GP_VECTOR, outside_64k_handler, 0);
}

__noinline void beyond_cs_limit_handler(void *data)
{
	struct far_pointer32 fptr = {
		.offset = 0x10001,
		.selector = 0
	};

	asm volatile (
		"lcall *%0\n"
		:
		: "m"(fptr)
		: "memory"
	);
}

/*
 * Table34 - row 2
 * In v8086 or real mode, move data to a memory operand effective address,
 * which is outside the range 0 through 0xFFFF, will generate an #GP.
 */
__noinline void v8086_rqmid_46065_segmentation_table34_002(void)
{
	REQUEST_IRQ_EX(GP_VECTOR, beyond_cs_limit_handler, 0);
}

/**
 * @brief case name: Real-address and virtual-8086 mode address translation_001
 *
 * Summary: In real mode, the processor does not truncate such an address and uses it
 * as a physical address. (Note, however, that for IA-32 processors beginning with the
 * Intel486 processor, the A20M# signal can be used in real-address mode to mask address
 * line A20, thereby mimicking the 20-bit wrap-around behavior of the 8086 processor.)
 *
 * A magic 0xfeedbabe was written at 0x10f000 in protected mode.
 * (0xffff << 4) + 0xf010 = 0x10f000
 */
void rmode_rqmid_33916_address_translation_001(void)
{
	u32 magic = 0;
	asm volatile (
		"movw $0xffff, %%ax\n"
		 "movw %%ax, %%fs\n"
		 ASM_TRY("1f")
		 "movl %%fs:0xf010, %%ebx\n"
		"1:\n"
		 "movw $0, %%ax\n"
		 "movw %%ax, %%fs\n"
		: "=b"(magic)
		:
		: "memory"
		);
	u8 vecter = exception_vector();
	report_ex("vector=%u, magic=0x%x",
		(NO_EXCEPTION == vecter) && (0xfeedbabe == magic), vecter, magic);
}

/**
 * @brief case name: Cache control CLFLUSH instruction_exception_008
 *
 * Summary: In real mode, if any part of the operand lies outside the effective
 * address space from 0 to FFFFH, execute clflush will generate #GP.
 */
void cache_rqmid_27082_clflush_exception_009(void)
{
	u32 a1 = 0x10001;
	TRY_FLUSH("clflush", a1);
}

/**
 * @brief case name: Cache control CLFLUSHOPT instruction_exception_011
 *
 * Summary: In real mode, if any part of the operand lies outside the effective
 * address space from 0 to FFFFH, execute clflushopt will generate #GP.
 */
void cache_rqmid_27091_clflushopt_exception_011(void)
{
	u32 a1 = 0x10001;
	TRY_FLUSH("clflushopt", a1);
}

void main(void)
{
	printf("## real mode main() ##\n");
	set_handle_exception();
	v8086_rqmid_46053_segmentation_table32_001();
	v8086_rqmid_46054_segmentation_table32_002();
	v8086_rqmid_46055_segmentation_table32_003();
	v8086_rqmid_46056_segmentation_table32_004();
	v8086_rqmid_46061_segmentation_table34_001();
	v8086_rqmid_46065_segmentation_table34_002();
	rmode_rqmid_33916_address_translation_001();
	cache_rqmid_27082_clflush_exception_009();
	cache_rqmid_27091_clflushopt_exception_011();
	report_summary();
}
