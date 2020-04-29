#ifndef __PCI_UTIL_H__
#define __PCI_UTIL_H__
#include "libcflat.h"
#include "pci_type.h"
#include "pci_log.h"
#include "pci_io.h"
#include "pci_regs.h"

#define	PCIR_BAR(x)	(PCI_BASE_ADDRESS_0 + (x) * 4)

/**
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
uint32_t pci_pdev_calc_address(union pci_bdf bdf, uint32_t offset);

/**
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
uint32_t pci_pdev_read_cfg(union pci_bdf bdf, uint32_t offset, uint32_t bytes);

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
int pci_pdev_write_cfg(union pci_bdf bdf, uint32_t offset, uint32_t bytes, uint32_t val);

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
uint64_t pci_pdev_read_mem(union pci_bdf bdf, mem_size address, uint64_t bytes);

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
int pci_pdev_write_mem(union pci_bdf bdf, mem_size address, uint32_t bytes, uint64_t val);

uint32_t pci_pdev_get_bar_size(union pci_bdf bdf, uint32_t bar);

uint64_t shift_umask(uint8_t msb, uint8_t lsb);

uint64_t shift_mask(uint8_t msb, uint8_t lsb);


#define SHIFT_LEFT(val, left)	((val) << (left))
#define SHIFT_RIGHT(val, right)	((val) >> (right))
#define SHIFT_MASK(msb, lsb)	shift_mask(msb, lsb)
#define SHIFT_UMASK(msb, lsb)	shift_umask(msb, lsb)

#endif
