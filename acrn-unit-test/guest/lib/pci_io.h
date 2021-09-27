#ifndef __PCI_IO_H__
#define __PCI_IO_H__
#include "libcflat.h"

static inline void pio_write8(uint8_t value, uint16_t port)
{
	asm volatile ("outb %0,%1" : : "a" (value), "dN" (port));
}

static inline uint8_t pio_read8(uint16_t port)
{
	uint8_t value = 0;
	asm volatile ("inb %1,%0" : "=a" (value) : "dN" (port));
	return value;
}

static inline void pio_write16(uint16_t value, uint16_t port)
{
	asm volatile ("outw %0,%1" : : "a" (value), "dN" (port));
}

static inline uint16_t pio_read16(uint16_t port)
{
	uint16_t value = 0;
	asm volatile ("inw %1,%0" : "=a" (value) : "dN" (port));
	return value;
}

static inline void pio_write32(uint32_t value, uint16_t port)
{
	asm volatile ("outl %0,%1" : : "a" (value), "dN" (port));
}

static inline uint32_t pio_read32(uint16_t port)
{
	uint32_t value = 0;
	asm volatile ("inl %1,%0" : "=a" (value) : "dN" (port));
	return value;
}

static inline void pio_write(uint32_t v, uint16_t addr, size_t sz)
{
	if (sz == 1U) {
		pio_write8((uint8_t)v, addr);
	} else if (sz == 2U) {
		pio_write16((uint16_t)v, addr);
	} else {
		pio_write32(v, addr);
	}
}

static inline uint32_t pio_read(uint16_t addr, size_t sz)
{
	uint32_t ret = 0U;
	if (sz == 1U) {
		ret = pio_read8(addr);
	} else if (sz == 2U) {
		ret = pio_read16(addr);
	} else {
		ret = pio_read32(addr);
	}
	return ret;
}

static inline void mem_write8(uint8_t value, mem_size address)
{
	*(uint8_t *)address = value;
}

static inline uint8_t mem_read8(mem_size address)
{
	uint8_t value = 0;
	value = *(uint8_t *)address;
	return value;
}

static inline void mem_write16(uint16_t value, mem_size address)
{
	*(uint16_t *)address = value;
}

static inline uint16_t mem_read16(mem_size address)
{
	uint16_t value = 0;
	value = *(uint16_t *)address;
	return value;
}

static inline void mem_write32(uint32_t value, mem_size address)
{
	*(uint32_t *)address = value;
}

static inline uint32_t mem_read32(mem_size address)
{
	uint32_t value = 0U;
	value = *(uint32_t *)address;
	return value;
}

static inline void mem_write64(uint64_t value, mem_size address)
{
	*(uint64_t *)address = value;
}

static inline uint64_t mem_read64(mem_size address)
{
	uint64_t value = 0UL;
	value = *(uint64_t *)address;
	return value;
}

#endif