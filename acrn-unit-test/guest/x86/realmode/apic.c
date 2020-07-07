#include "apic-defs.h"
#include "msr.h"
#include "processor.h"

static void x2apic_write(unsigned reg, u32 val)
{
	asm volatile ("wrmsr" : : "a"(val), "d"(0), "c"(APIC_BASE_MSR + reg/16));
}

int enable_x2apic(void)
{
	unsigned a, b, c, d;

	asm ("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(1));

	if (c & (1 << 21)) {
		asm ("rdmsr" : "=a"(a), "=d"(d) : "c"(MSR_IA32_APICBASE));
		a |= 1 << 10;
		asm ("wrmsr" : : "a"(a), "d"(d), "c"(MSR_IA32_APICBASE));
		return 1;
	} else {
		return 0;
	}
}

void enable_apic(void)
{
	if ((rdmsr(MSR_IA32_APICBASE) & (APIC_EN | APIC_EXTD)) != (APIC_EN | APIC_EXTD)) {
		enable_x2apic();
		x2apic_write(0xf0, 0x1ff); /* spurious vector register */
	} else {
		/*nop*/
	}
}

static void outb(unsigned char data, unsigned short port)
{
	asm volatile ("out %0, %1" : : "a"(data), "d"(port));
}

void mask_pic_interrupts(void)
{
	outb(0xff, 0x21);
	outb(0xff, 0xa1);
}

