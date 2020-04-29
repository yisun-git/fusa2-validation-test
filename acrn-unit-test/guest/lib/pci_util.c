#include "pci_util.h"

#define PCI_CONFIG_ADDR		0xCF8U
#define PCI_CONFIG_DATA		0xCFCU
#define PCI_CONFIG_ENABLE		0x80000000U

/*
 * @brief calculate the PCI_CFG_PORT value.
 *
 * value define:
 * bit31-30-------------24 23-------16 15--------------11-10-------------8 7-------------------0
 *    EN        res          bus num      device num       function num       config register
 * Application Constraints: only to use PCI IO Port 0xCF8
 *
 * @param param_1 union pci_bdf bdf:device BDF information.
 * @param param_2 uint32_t offset:config register offset.
 *
 *
 * @return the value of IO Port 0xCF9.
 *
 */
uint32_t pci_pdev_calc_address(union pci_bdf bdf, uint32_t offset)
{
	uint32_t addr = (uint32_t)bdf.value;

	addr <<= 8U;
	addr |= (offset | PCI_CONFIG_ENABLE);
	return addr;
}

/*
 * @brief pci config register read.
 *
 * read the special data/nbyte to specail pci config register.
 * Application Constraints: none.
 *
 * @param union pci_bdf bdf:device BDF information
 * @param uint32_t offset:config register address
 * @param uint32_t bytes:number of byte
 *
 * @pre param_1 != NULL
 *
 * @return the value of register.
 *
 * @retval (-1) input parameter is invalid.
 *
 */
uint32_t pci_pdev_read_cfg(union pci_bdf bdf, uint32_t offset, uint32_t bytes)
{
	uint32_t addr = 0U;
	uint32_t val = 0U;

	addr = pci_pdev_calc_address(bdf, offset);

	/* Write address to ADDRESS register */
	pio_write32(addr, (uint16_t)PCI_CONFIG_ADDR);

	/* Read result from DATA register */
	switch (bytes) {
	case 1U:
		val = (uint32_t)pio_read8((uint16_t)PCI_CONFIG_DATA + ((uint16_t)offset & 3U));
		break;
	case 2U:
		val = (uint32_t)pio_read16((uint16_t)PCI_CONFIG_DATA + ((uint16_t)offset & 2U));
		break;
	default:
		val = pio_read32((uint16_t)PCI_CONFIG_DATA);
		break;
	}

	return val;
}

/**
 * @brief pci config register write.
 *
 * write the special data/nbyte to specail pci config register.
 * Application Constraints: none.
 *
 * @param union pci_bdf bdf:device BDF information
 * @param uint32_t offset:config register address
 * @param uint32_t bytes:number of byte
 *
 * @pre param_1 != NULL
 *
 * @return OK.
 *
 * @retval (-1) input parameter is invalid.
 *
 */
int pci_pdev_write_cfg(union pci_bdf bdf, uint32_t offset, uint32_t bytes, uint32_t val)
{
	uint32_t addr = 0U;

	addr = pci_pdev_calc_address(bdf, offset);

	/* Write address to ADDRESS register */
	pio_write32(addr, (uint16_t)PCI_CONFIG_ADDR);

	/* Write value to DATA register */
	switch (bytes) {
	case 1U:
		pio_write8((uint8_t)val, (uint16_t)PCI_CONFIG_DATA + ((uint16_t)offset & 3U));
		break;
	case 2U:
		pio_write16((uint16_t)val, (uint16_t)PCI_CONFIG_DATA + ((uint16_t)offset & 2U));
		break;
	default:
		pio_write32(val, (uint16_t)PCI_CONFIG_DATA);
		break;
	}

	return OK;
}

/**
 * @brief pci memory  register read.
 *
 * read the special data/nbyte to specail pci memory register.
 * Application Constraints: none.
 *
 * @param union pci_bdf bdf:device BDF information
 * @param uint32_t offset:config register address
 * @param uint32_t bytes:number of byte
 *
 * @pre param_1 != NULL
 *
 * @return the value of memry.
 *
 * @retval (-1) input parameter is invalid.
 *
 */
uint64_t pci_pdev_read_mem(union pci_bdf bdf, mem_size address, uint64_t bytes)
{
	uint64_t val = 0UL;

	/* Read result from DATA register */
	switch (bytes) {
	case 1U:
		val = (uint64_t)mem_read8(address);
		break;
	case 2U:
		val = (uint64_t)mem_read16(address);
		break;
	case 4U:
		val = (uint64_t)mem_read32(address);
		break;
	default:
		val = (uint64_t)mem_read64(address);
		break;
	}

	return val;
}

/**
 * @brief pci memory  register write.
 *
 * write the special data/nbyte to specail pci memory register.
 * Application Constraints: none.
 *
 * @param union pci_bdf bdf:device BDF information
 * @param uint32_t offset:config register address
 * @param uint32_t bytes:number of byte
 *
 * @pre param_1 != NULL
 *
 * @return OK.
 *
 * @retval (-1) input parameter is invalid.
 *
 */
int pci_pdev_write_mem(union pci_bdf bdf, mem_size address, uint32_t bytes, uint64_t val)
{
	/* Write value to DATA register */
	switch (bytes) {
	case 1U:
		mem_write8((uint8_t)val, address);
		break;
	case 2U:
		mem_write16((uint16_t)val, address);
		break;
	case 4U:
		mem_write32((uint32_t)val, address);
		break;
	default:
		mem_write64((uint64_t)val, address);
		break;
	}

	return OK;
}

uint32_t pci_pdev_get_bar_size(union pci_bdf bdf, uint32_t bar)
{
	uint16_t command = 0;
	uint16_t old_command = 0;
	uint32_t _bar = 0;
	uint32_t size = 0;
	uint32_t bar_id = PCI_BASE_ADDRESS_0 + bar * 4;
	command = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
	old_command = command;
	command &= ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY);
	pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, command);
	_bar = pci_pdev_read_cfg(bdf, bar_id, 4);
	pci_pdev_write_cfg(bdf, bar_id, 4, ~0U);
	size = pci_pdev_read_cfg(bdf, bar_id, 4);
	size = (~size) + 1;
	/*mask low 16 bits for IO space*/
	if (_bar & PCI_BASE_ADDRESS_SPACE) {
		size &= 0xFFFFU;
	}
	pci_pdev_write_cfg(bdf, bar_id, 4, _bar);
	pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, old_command);
	return size;
}

uint64_t shift_umask(uint8_t msb, uint8_t lsb)
{
	uint64_t mask = 0UL;
	uint8_t i = 0;
	mask = 0xFFFFFFFFFFFFFFFFUL;
	for (i = lsb; i <= msb; i++) {
		mask = mask & (~(0x01 << i));
	}
	return mask;
}

uint64_t shift_mask(uint8_t msb, uint8_t lsb)
{
	uint64_t mask = 0UL;
	uint8_t i = 0;
	mask = 0;
	for (i = lsb ; i <= msb ; i++) {
		mask = mask | (0x01 << i);
	}
	return mask;
}
