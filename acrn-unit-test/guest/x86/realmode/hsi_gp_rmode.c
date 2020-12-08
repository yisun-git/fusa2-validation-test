
asm(".code16gcc");
#include "rmode_lib.h"

static u8 cmove_checking(void)
{
        u16 op = 2;
        asm volatile(
		"mov $1, %ax\n"
                "xor %bx, %bx\n"
		ASM_TRY("1f"));
        asm volatile(
                /* ZF = 1,It will move */
                "cmove %%bx, %0\n"
                "1:"
                : "=r" (op)
                :
                : "ax", "bx"
        );

        return ((exception_vector() == 0) && (op == 0));
}

/* 0x10 & 0x11 */
static u8 and_checking(void)
{
        u8 op = 2;
        asm volatile("mov $0x11, %al\n"
		ASM_TRY("1f"));
        asm volatile(
                "and $0x10, %%al\n"
		"mov %%al, %0\n"
                "1:"
                : "=r" (op)
                :
                : "ax"
        );
        return ((exception_vector() == 0) && (op == 0x10));
}

/* CLI instruction make the eflags.IF to 0 */
static u8 cli_checking(void)
{
        u16 op = 1;
        asm volatile(
                "pushf\n"
                "pop %bx\n\t"
		ASM_TRY("1f"));
        asm volatile(
                "cli\n\t"
                "pushf\n"
                "pop %0\n"
                "push %%bx\n"
                "popf\n"
                "and $0x100, %0\n"
                "1:"
                : "+r" (op)
                : : "bx"
        );
        return ((exception_vector() == 0) && (op == 0));
}

/* move $0x10 to segment FS */
static u8 mov_checking(void)
{
        u16 op = 0x8;
        asm volatile(
                "mov %fs, %bx\n"
                ASM_TRY("1f"));
        asm volatile(
                "mov $0x10, %%cx\n\t"
                "mov %%cx, %%fs\n"
                "mov %%fs, %0\n"
                "mov %%bx, %%fs\n"
                "1:"
                : "=r" (op)
                : : "ebx", "ecx"
        );
        return ((exception_vector() == 0) && (op == 0x10));
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Data_Move_003
 *
 * Summary: Under Real Address Mode,
 * CMOVE.
 * execution results are all correct and no exception occurs..
 */
static void hsi_rqmid_41189_generic_processor_features_data_move_003(void)
{
	u16 chk = 0;

	if (cmove_checking()) {
                chk++;
        }

	report("hsi_rqmid_41189_generic_processor_features_data_move_003", (chk == 1));
	print_serial("\r\n");
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Logical_003
 *
 * Summary: Under Real Address Mode,
 * AND.
 * execution results are all correct and no exception occurs..
 */
static void hsi_rqmid_41190_generic_processor_features_logical_003(void)
{
	u16 chk = 0;

	if (and_checking()) {
                chk++;
        }

	report("hsi_rqmid_41190_generic_processor_features_logical_003", (chk == 1));
	print_serial("\r\n");
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Flag_Control_003
 *
 * Summary: Under Real Address Mode,
 * CLI.
 * execution results are all correct and no exception occurs..
 */
static void hsi_rqmid_41191_generic_processor_features_flag_control_003(void)
{
	u16 chk = 0;

	if (cli_checking()) {
                chk++;
        }

	report("hsi_rqmid_41191_generic_processor_features_flag_control_003", (chk == 1));
	print_serial("\r\n");
}

/*
 * @brief case name: HSI_Generic_Processor_Features_Segment_Register_003
 *
 * Summary: Under Real Address Mode,
 * MOV.
 * execution results are all correct and no exception occurs..
 */
static void hsi_rqmid_41942_generic_processor_features_segment_register_003(void)
{
	u16 chk = 0;

	if (mov_checking()) {
                chk++;
        }

	report("hsi_rqmid_41942_generic_processor_features_segment_register_003", (chk == 1));
	print_serial("\r\n");
}

void main()
{
	hsi_rqmid_41189_generic_processor_features_data_move_003();
	hsi_rqmid_41190_generic_processor_features_logical_003();
	hsi_rqmid_41191_generic_processor_features_flag_control_003();
	hsi_rqmid_41942_generic_processor_features_segment_register_003();
	report_summary();
	print_serial("\r\n");
}

