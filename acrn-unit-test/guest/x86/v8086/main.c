#include <stdarg.h>
#include "common.h"

extern int prepare_v8086;

void v8086_enter(void)
{
	asm volatile ("call %0\n" : : "m"(prepare_v8086) : );
}

__noinline void v8086_exit(void)
{
	printf("-- v8086 exit --\n");
	halt();
}

/**
 * This interrupt handler is used for communication
 * between the protected mode and the v8086 mode.
 */
__noinline void v8_reg_int(void)
{
	u32 val = RET_FAILURE;
	const struct v8086_irq vi = read_irq();

	if (vi.irq > 0 && vi.irq < 255 && vi.handler != 0)
		val = register_int(vi.irq, vi.handler);

	write_output_val(val);
}

__noinline void v8_unreg_int(void)
{
	u32 irqidx = read_input_val();

	unregister_int(irqidx);
}

__noinline void v8_read_cr0(void)
{
	write_output_val(read_cr0());
}

__noinline void v8_write_cr0(void)
{
	write_cr0(read_input_val());
}

__noinline void v8_read_cr3(void)
{
	write_output_val(read_cr3());
}

__noinline void v8_write_cr3(void)
{
	write_cr3(read_input_val());
}

__noinline void v8_read_cr4(void)
{
	write_output_val(read_cr4());
}

__noinline void v8_write_cr4(void)
{
	write_cr4(read_input_val());
}

const v8086_func v8086_funcs[FUNC_ID_MAX] = {
	[FUNC_V8086_EXIT] = v8086_exit,
	[FUNC_REG_INT]    = v8_reg_int,
	[FUNC_UNREG_INT]  = v8_unreg_int,
	[FUNC_READ_CR0]   = v8_read_cr0,
	[FUNC_WRITE_CR0]  = v8_write_cr0,
	[FUNC_READ_CR3]   = v8_read_cr3,
	[FUNC_WRITE_CR3]  = v8_write_cr3,
	[FUNC_READ_CR4]   = v8_read_cr4,
	[FUNC_WRITE_CR4]  = v8_write_cr4,
};

int main(int argc, char **argv)
{
	setup_idt();
	v8086_enter();
	return 0;
}
