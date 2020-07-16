#include "delay.h"
#include "processor.h"

void delay(u64 count)
{
	while (count--)
		asm volatile("pause");
}

void test_delay(int time)
{
	__unused int count = 0;
	u64 tsc;
	tsc = rdtsc() + ((u64)time * 1000000000);

	while (rdtsc() < tsc) {
		;
	}
}

