
asm(".code16gcc");
#include "rmode_lib.h"
void main()
{
	u32 tmp = 1234567800u;
	u32 tmp1;
	unsigned char size_u32, size_u16;
	u16 sp;
	unsigned char vector;
	unsigned short error_code;

	tmp1 = tmp;

	size_u32 = sizeof(tmp);
	size_u16 = sizeof(u16);

	print_serial("rmode_start:\n");
	asm volatile("mov %%sp, %%ax \n"
		"mov %%ax, %0 \n"
		: "=r"(sp)
	);
	print_serial("sp:");
	print_serial_u32(sp);
	print_serial("\r\n");

	print_serial("u32 len:");
	print_serial_u32(size_u32);
	print_serial("\r\n");

	print_serial("u16 len:");
	print_serial_u32(size_u16);
	print_serial("\r\n");

	asm volatile("mov $0x3456, %dx");
	print_serial_u32(tmp1);
	print_serial("\r\n");

	asm volatile("mov $0x2345, %dx");
	asm volatile(ASM_TRY("1f")
		"lock;""pause\n"
		"1:");
	vector = exception_vector();
	print_serial("vector:");
	print_serial_u32(vector);
	print_serial("\r\n");

	error_code = exception_error_code();
	print_serial("error_code:");
	print_serial_u32(error_code);
	print_serial("\r\n");

	report("first", 1);
	report_summary();
}

