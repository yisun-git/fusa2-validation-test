/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __PCI_DEVICE_PASSTHROUGH_H__
#define __PCI_DEVICE_PASSTHROUGH_H__

#include "libcflat.h"
#include "./x86/processor.h"
#include "./x86/desc.h"
#include "./asm/io.h"
#include "string.h"
#include "../lib/x86/asm/io.h"

#define PCI_CFG_ENABLE 0x80000000U

/*PCI  I/O ports */
#define PCI_CONFIG_ADDR 0xCF8U
#define PCI_CONFIG_DATA 0xCFCU

/**
 *	the pci bdf struct
 */
union pci_bdf {
	uint16_t value;
	struct {
		uint8_t f : 3; /* BITs 0-2 */
		uint8_t d : 5; /* BITs 3-7 */
		uint8_t b; /* BITs 8-15 */
	} bits;
};

/**
 * @brief IO Port write 1 byte.
 *
 * @param param_1 uint8_t value:write value.
 * @param param_2 uint16_t port:IO Port
 *
 * @return void
 *
 */
static inline void pio_write8(uint8_t value, uint16_t port)
{
	asm volatile ("outb %0,%1"::"a" (value), "dN"(port));
}

/**
 * @brief IO Port read 1 byte.
 *
 * @param param_2 uint16_t port:IO Port
 *
 * @return read value
 *
 */
static inline uint8_t pio_read8(uint16_t port)
{
	uint8_t value;
	asm volatile ("inb %1,%0":"=a" (value):"dN"(port));
	return value;
}

/**
 * @brief IO Port write 2 byte.
 *
 * @param param_1 uint8_t value:write value.
 * @param param_2 uint16_t port:IO Port
 *
 * @return void
 *
 */
static inline void pio_write16(uint16_t value, uint16_t port)
{
	asm volatile ("outw %0,%1"::"a" (value), "dN"(port));
}

/**
 * @brief IO Port read 2 byte.
 *
 * @param param_2 uint16_t port:IO Port
 *
 * @return read value
 *
 */
static inline uint16_t pio_read16(uint16_t port)
{
	uint16_t value;

	asm volatile ("inw %1,%0":"=a" (value):"dN"(port));
	return value;
}

/**
 * @brief IO Port write 4 byte.
 *
 * @param param_1 uint8_t value:write value.
 * @param param_2 uint16_t port:IO Port
 *
 * @return void
 *
 */
static inline void pio_write32(uint32_t value, uint16_t port)
{
	asm volatile ("outl %0,%1"::"a" (value), "dN"(port));
}

/**
 * @brief IO Port read 4 byte.
 *
 * @param param_2 uint16_t port:IO Port
 *
 * @return read value
 *
 */
static inline uint32_t pio_read32(uint16_t port)
{
	uint32_t value;

	asm volatile ("inl %1,%0":"=a" (value):"dN"(port));
	return value;
}

/**
 * @brief PCI memory write 1 byte.
 *
 * @param param_1 uint8_t value:write value.
 * @param param_2 uint64_t address:PCI memory address
 *
 * @return void
 *
 */
static inline void mem_write8(uint8_t value, uint64_t address)
{
	*(uint8_t *)address = value;
}

/**
 * @brief PCI memory read 1 byte.
 *
 * @param param_2 uint16_t port:PCI memory address
 *
 * @return read value
 *
 */
static inline uint8_t mem_read8(uint64_t address)
{
	uint8_t value;

	value = *(uint8_t *)address;
	return value;
}

/**
 * @brief PCI memory write 2 byte.
 *
 * @param param_1 uint8_t value:write value.
 * @param param_2 uint64_t address:PCI memory address
 *
 * @return void
 *
 */
static inline void mem_write16(uint16_t value, uint64_t address)
{
	*(uint16_t *)address = value;
}

/**
 * @brief PCI memory read 2 byte.
 *
 * @param param_2 uint16_t port:PCI memory address
 *
 * @return read value
 *
 */
static inline uint16_t mem_read16(uint64_t address)
{
	uint16_t value;

	value = *(uint16_t *)address;
	return value;
}

/**
 * @brief PCI memory write 4 byte.
 *
 * @param param_1 uint8_t value:write value.
 * @param param_2 uint64_t address:PCI memory address
 *
 * @return void
 *
 */

static inline void mem_write32(uint32_t value, uint64_t address)
{
	*(uint32_t *)address = value;
}
/**
 * @brief PCI memory read 4 byte.
 *
 * @param param_2 uint16_t port:PCI memory address
 *
 * @return read value
 *
 */
static inline uint32_t mem_read32(uint64_t address)
{
	uint32_t value;

	value = *(uint32_t *)address;
	return value;
}

/**
 * @brief PCI memory write 8 byte.
 *
 * @param param_1 uint8_t value:write value.
 * @param param_2 uint64_t address:PCI memory address
 *
 * @return void
 *
 */
static inline void mem_write64(uint64_t value, uint64_t address)
{
	*(uint64_t *)address = value;
}

/**
 * @brief PCI memory read 8 byte.
 *
 * @param param_2 uint16_t port:PCI memory address
 *
 * @return read value
 *
 */
static inline uint64_t mem_read64(uint64_t address)
{
	uint64_t value;

	value = *(uint64_t *)address;
	return value;
}

#endif	//__PCI_DEVICE_PASSTHROUGH_H__
