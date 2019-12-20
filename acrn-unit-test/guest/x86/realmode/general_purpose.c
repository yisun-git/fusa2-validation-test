
asm(".code16gcc");
#include "rmode_lib.h"

struct gp_fun_index{
	int rqmid;
	void (*func)(void);
};
typedef void (*gp_trigger_func)(void *data);

/*
* @brief case name: 31693: Data transfer instructions Support_Real Address Mode_PUSHA_#GP_004.
*
* Summary: Under Real Address Mode,
*  If the ESP or SP register contains 7, 9, 11, 13, or 15(ESP SP: hold),
*  executing PUSHA shall generate #GP.
*/
static int pusha_gp_004(void)
{
	asm volatile (ASM_TRY("1f")
		"or $7, %sp \n"
		"pusha\n"
		"1:");
	return exception_vector();
}

static void  gp_rqmid_31693_data_transfer_pusha_gp_004(void)
{
	/* Need modify unit-test framework, PUSH %%cs can't use exception handler. */
	report("gp_rqmid_31693_data_transfer_pusha_gp_004",  pusha_gp_004() == 13);
}

void main()
{
	u32 tmp = 1234567800u;
	u32 tmp1;
	unsigned char size_u32,size_u16;
	u16 sp;
	unsigned char vector;

	tmp1 = tmp;

	size_u32 = sizeof(tmp);
	size_u16 = sizeof(u16);

	print_serial("rmode_start:\n");
	asm volatile("mov %%sp, %%ax \n"
		"mov %%ax, %0 \n"
		:"=r"(sp)
		);
	print_serial("sp:");
	print_serial_u32(sp);
	print_serial("\n\r");
	print_serial("u32 len:");
	print_serial_u32(size_u32);
	print_serial("u16 len:");
	print_serial_u32(size_u16);
	asm volatile("mov $0x3456, %dx");
	print_serial_u32(tmp1);
	print_serial("\n");
	asm volatile("mov $0x2345, %dx");
	asm volatile(ASM_TRY("1f")
		"lock;""pause\n"
		"1:");
	vector = exception_vector();
	print_serial("\n\r vector:");
	print_serial_u32(vector);
	print_serial("\n\r");

	gp_rqmid_31693_data_transfer_pusha_gp_004();

	report_summary();
}

