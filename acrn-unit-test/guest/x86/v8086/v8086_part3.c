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

/**
 * @brief Case name: Virtual-8086 mode interrupt handling to any VM_002_2.
 *
 * Summary: If the trap or interrupt gate references a procedure in
 *  a segment at a privilege level other than 0, generate the #GP(0).
 *
 */
__noinline void v8086_rqmid_38117_interrupt_handling_002_2(void)
{
	/* intr gate, dpl = 1 */
	set_igdt(22, SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_INTERGATE,
		SEGMENT_PRESENT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_1 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_CODE_EXE_READ_ACCESSED);
	TRY_INSTRUCTION(GP_VECTOR, "int $22\n");
}

/**
 * @brief Case name: Virtual-8086 mode interrupt handling to any VM_002_3.
 *
 * Summary: If the trap or interrupt gate references a procedure in
 *  a segment at a privilege level other than 0, generate the #GP(0).
 *
 */
__noinline void v8086_rqmid_38118_interrupt_handling_002_3(void)
{
	/*trap gate, dpl = 1*/
	set_igdt(22, SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TRAPGATE,
		SEGMENT_PRESENT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_1 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_CODE_EXE_READ_ACCESSED);
	TRY_INSTRUCTION(GP_VECTOR, "int $22\n");
}

/**
 * @brief Case name: Virtual-8086 mode interrupt handling to any VM_002_4.
 *
 * Summary: If the trap or interrupt gate references a procedure in
 *  a segment at a privilege level other than 0, generate the #GP(0).
 *
 */
__noinline void v8086_rqmid_38119_interrupt_handling_002_4(void)
{
	/*trap gate, dpl = 2 */
	set_igdt(22, SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TRAPGATE,
		SEGMENT_PRESENT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_2 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_CODE_EXE_READ_ACCESSED);
	TRY_INSTRUCTION(GP_VECTOR, "int $22\n");
}

/**
 * @brief Case name: Virtual-8086 mode interrupt handling to any VM_002_5.
 *
 * Summary: If the trap or interrupt gate references a procedure in
 *  a segment at a privilege level other than 0, generate the #GP(0).
 *
 */
__noinline void v8086_rqmid_38120_interrupt_handling_002_5(void)
{
	/* trap gate, dpl = 3 */
	set_igdt(22, SYS_SEGMENT_AND_GATE_DESCRIPTOR_32BIT_TRAPGATE,
		SEGMENT_PRESENT_SET | DESCRIPTOR_PRIVILEGE_LEVEL_3 |
		DESCRIPTOR_TYPE_CODE_OR_DATA | SEGMENT_TYPE_CODE_EXE_READ_ACCESSED);
	TRY_INSTRUCTION(GP_VECTOR, "int $22\n");
}

/**
 * @brief Case name: Task switch in virtual-8086 mode_001
 *
 * Summary: GP (selector) is generated when trying to switch task to v8086 on ARCN
 *
 */
__noinline void v8086_rqmid_33859_task_switch_001(void)
{
	set_igdt(25, 0, 0);
	asm volatile(
		ASM_TRY("1f\n")
		"int $25\n"
		"1:\n"
	);
	u8 vecter = exception_vector();
	u16 ecode = exception_error_code();
	report_ex("vector=%u, error code=0x%x", GP_VECTOR == vecter
		&& V8086_TSS_SEL == ecode, vecter, ecode);
}

void v8086_main(void)
{
	if (X86_EFLAGS_IOPL3 == (v8086_iopl & X86_EFLAGS_IOPL3)) {
		v8086_rqmid_38117_interrupt_handling_002_2();
		v8086_rqmid_38118_interrupt_handling_002_3();
		v8086_rqmid_38119_interrupt_handling_002_4();
		v8086_rqmid_38120_interrupt_handling_002_5();
		v8086_rqmid_33859_task_switch_001();
	}

	report_summary();
	send_cmd(FUNC_V8086_EXIT);
}

