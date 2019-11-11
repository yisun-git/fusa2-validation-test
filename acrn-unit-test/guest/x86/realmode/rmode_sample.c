
asm(".code16gcc");
#include "rmode_lib.h"
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
	report("first", 1);
	report_summary();
}

