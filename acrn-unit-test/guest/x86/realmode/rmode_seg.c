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

	report_summary();
}
