/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "device_passthrough.h"
#include "misc.h"
union pci_bdf usb_bdf = {
	.bits.b = 0U,
	.bits.d = 0x14U,
	.bits.f = 0U
};

union pci_bdf ethernet_bdf = {
	.bits.b = 0U,
	.bits.d = 0x1FU,
	.bits.f = 6U
};

/* define the offset length in byte */
#define BYTE_1			(1U)
#define BYTE_2			(2U)
#define BYTE_4			(4U)

/* pci config space register offset address and the register length */
#define PCI_VENDOR_OFFSET			(0x0U)
#define PCI_VENDOR_BYTE				BYTE_2
#define PCI_DEVICE_OFFSET			(0x2U)
#define PCI_DEVICE_BYTE				BYTE_2
#define PCI_STATUS_OFFSET			(0x6U)
#define PCI_STATUS_BYTE				BYTE_2
#define PCI_REVISION_OFFSET			(0x8U)
#define PCI_REVISION_BYTE			BYTE_1
#define PCI_CLASS_OFFSET0			(0x9U)
#define PCI_CLASS_OFFSET1			(0xAU)
#define PCI_CLASS_OFFSET2			(0xBU)
#define PCI_CLASS_BYTE				BYTE_1
#define PCI_HEADER_OFFSET			(0xEU)
#define PCI_HEADER_BYTE				BYTE_1
#define PCI_BAR0_OFFSET				(0x10U)
#define PCI_BAR1_OFFSET				(0x14U)
#define PCI_BAR2_OFFSET				(0x18U)
#define PCI_BAR3_OFFSET				(0x1CU)
#define PCI_BAR4_OFFSET				(0x20U)
#define PCI_BAR5_OFFSET				(0x24U)
#define PCI_BAR_BYTE				BYTE_4
#define PCI_SUB_SYS_VENDOR_OFFSET		(0x2CU)
#define PCI_SUB_SYS_VENDOR_BYTE			BYTE_2
#define PCI_SUB_SYSTEM_ID_OFFSET		(0x2EU)
#define PCI_SUB_SYSTEM_ID_BYTE			BYTE_2
#define PCI_CAP_OFFSET				(0x34U)
#define PCI_CAP_BYTE				BYTE_4
#define PCI_INTERRUPTE_PIN_OFFSET		(0x3DU)
#define PCI_INTERRUPTE_PIN_BYTE			BYTE_1

/* ethernet config space register offset address */
#define		LANDISCTRL	(0xA0U)	/*	LAN Disable Control (LANDISCTRL)Offset A0h bitsize 32*/
#define		LOCKLANDIS	(0xA4U)	/*	Lock LAN Disable (LOCKLANDIS)Offset A4h bitsize 32*/
#define		LTRCAP		(0xA8U)	/*	System Time Control High Register (LTRCAP) Offset A8h bitsize 32*/

/* usb config space register offset address */
#define		HSCFG2		(0xA4U)	/*	High Speed Configuration 2 (HSCFG2)â€”Offset A4h bitsize 32	*/
#define		PCE_REG		(0xA2U)	/*	Power Control Enable (PCE_REG)â€”Offset A2h bitsize 16		*/
#define		VSHDR		(0x94U)	/*	Vendor Specific Header (VSHDR)â€”Offset 94h bitsize 32		*/
#define		BESL		(0x62U)	/*	Best Effort Service Latency (BESL)â€”Offset 62h	bit size 8	*/
#define		FLADJ		(0x61U)	/*	Frame Length Adjustment (FLADJ)â€”Offset 61h bitsize 8		*/
#define		SBRN		(0x60U)	/*	Serial Bus Release Number (SBRN)â€”Offset 60h bitsize 8		*/
#define		AUDSYNC		(0x58U)	/*	Audio Time Synchronization (AUDSYNC)â€”Offset 58h bit size 32	*/
#define		XHCLKGETN	(0x50U)	/*	Clock Gating (XHCLKGTEN)â€”Offset 50h bitsize 32		*/
#define		XHCC1		(0x40U)	/*	XHC System Bus Configuration 1 (XHCC1)â€”Offset 40h bitsize 32	*/
#define		XHCC2		(0x44U)	/*	XHC System Bus Configuration 2 (XHCC2)â€”Offset 44h bitsize 32	*/

/* start-up store data at the address */
#define STARTUP_USB_BAR0_ADDRESS			(0x7000U)
#define STARTUP_USB_BAR1_ADDRESS			(0x7004U)
#define STARTUP_ETH_BAR0_ADDRESS			(0x7008U)
#define STARTUP_ETH_BAR1_ADDRESS			(0x700CU)
#define STARTUP_USB_STATUS_ADDRESS			(0x7020U)
#define STARTUP_ETH_STATUS_ADDRESS			(0x7024U)

#define INIT_USB_BAR0_ADDRESS				(0x7010U)
#define INIT_USB_BAR1_ADDRESS				(0x7014U)
#define INIT_ETH_BAR0_ADDRESS				(0x7018U)
#define INIT_ETH_BAR1_ADDRESS				(0x701CU)
#define INIT_USB_STATUS_ADDRESS				(0x7028U)
#define INIT_ETH_STATUS_ADDRESS				(0x702CU)

/* the special value of pcie device register*/
#define PCI_VENDOR_VALUE_ETHERNET			(0x8086U)
#define PCI_VENDOR_VALUE_USB				(0x8086U)
#define ETH_CLASS_CODE_VALUE				(0x020000U)
#define ETH_REVISION_ID_VALUE				(0x21U)
#define ETH_DEVICE_ID_VALUE				(0x156FU)
#define ETH_VENDOR_ID_VALUE				(0x8086U)
#define ETH_HEADER_TYPE_VALUE				(0x0U)
#define ETH_SUB_SYSTEM_ID_VALUE				(0x2070U)
#define ETH_SUB_SYSTEM_VENDOR_ID_VALUE			(0x8086U)
#define ETH_PHYSICAL_BAR0_BIT_16_to_0_VALUE		(0x0U)
#define ETH_INTERRUPTE_PIN_VALUE			(0x1U)
#define ETH_START_UP_STATUS_VALUE			(0x10U)
#define ETH_START_UP_BAR0_VALUE				(0xB3C00000U)
#define ETH_BAR0_VALUE					(0xB3C00000U)
#define ETH_SYSTEM_TIME_CONTROL_HIGH_REGISTER_VALUE	(0x10031003U)
#define ETH_LOCK_LAN_DISABLE_REGISTER_VALUE		(0x1U)
#define ETH_LAN_DISABLE_CONTROL_REGISTER_VALUE		(0U)
#define USB_HEADER_TYPE_VALUE				(0x80U)
#define USB_CLASS_CODE_VALUE				(0x0C0330U)
#define USB_REVISION_ID_VALUE				(0x21U)
#define	USB_DEVICE_ID_VALUE					(0x9d2FU)
#define USB_VENDOR_ID_VALUE				(0x8086U)
#define	USB_SUB_SYSTEM_ID_VALUE				(0x2070U)
#define USB_INTERRUPTE_PIN_VALUE			(0x1U)
#define USB_START_UP_STATUS_VALUE			(0x0290U)
#define USB_BAR0_VALUE					(0xB3F00004U)
#define USB_BAR1_VALUE					(0x0U)
#define USB_BAR0_MASK_VALUE				(0xFFFF0000U)
#define ETH_BAR0_MASK_VALUE				(0xFFFE0000U)
#define USB_BAR0_BIT_15_to_0_MASK_VALUE			(0xFFFFU)
#define USB_BAR0_BIT_15_to_0_VALUE			(0x4U)
#define USB_START_UP_BAR0_VALUE				(0xB3F00004U)
#define USB_START_UP_BAR1_VALUE				(0U)
#define MESSAGE_CONTROL_VALUE_BIT_1_TO_3		(0U)
#define MESSAGE_CONTROL_VALUE_BIT_7_TO_8		(1U)
#define MESSAGE_CONTROL_LENGTH_FOR_BIT_7_TO_8		(2U)
#define MESSAGE_CONTROL_OFFSET_FOR_BIT_7_TO_8		(7U)
#define MESSAGE_CONTROL_OFFSET_FOR_BIT_1_TO_3		(1U)
#define MESSAGE_CONTROL_LENGTH_FOR_BIT_1_TO_3		(3U)
#define XHCI_USB3_PIN_MAPPING7_VALUE			(0x0U)
#define XHCI_USB3_PIN_MAPPING6_VALUE			(0x0U)
#define XHCI_USB3_PIN_MAPPING5_VALUE			(0x0U)
#define XHCI_USB3_PIN_MAPPING4_VALUE			(0x0U)
#define XHCI_USB3_PIN_MAPPING3_VALUE			(0x0U)
#define XHCI_USB3_PIN_MAPPING2_VALUE			(0x1U)
#define XHCI_USB3_PIN_MAPPING1_VALUE			(0x18U)
#define XHCI_USB3_PIN_MAPPING0_VALUE			(0x6U)
#define XHCI_USB2_PIN_MAPPING7_VALUE			(0x0U)
#define XHCI_USB2_PIN_MAPPING6_VALUE			(0x0U)
#define XHCI_USB2_PIN_MAPPING5_VALUE			(0x0U)
#define XHCI_USB2_PIN_MAPPING4_VALUE			(0x0U)
#define XHCI_USB2_PIN_MAPPING3_VALUE			(0x60U)
#define XHCI_USB2_PIN_MAPPING2_VALUE			(0x1U)
#define XHCI_USB2_PIN_MAPPING1_VALUE			(0x18U)
#define XHCI_USB2_PIN_MAPPING0_VALUE			(0x6U)

#define VIR_USB_BAR_VALUE				(0xDF230000U)
#define VIR_ETH_BAR_VALUE				(0xDF220000U)
#define PHY_ETH_BAR_VALUE				(0xDF200000U)
#define USB_XHC_SYSTEM_CONFIG_1_VALUE			(0x803401FDU)
/*When a vCPU attempts to read guest PCI configuration XHC system configuration 1 register of its
 *own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 803401FDH.
 */
#define USB_XHC_SYSTEM_CONFIG_2_VALUE			(0x800FC688U)
/*When a vCPU attempts to read guest PCI configuration XHC system configuration 2 register of its
 *own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 800FC688H.
 */
#define USB_CLOCK_GATING_VALUE				(0x0FCE6E5BU)
/*When a vCPU attempts to read guest PCI configuration clock gating register of its
 *own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 0FCE6E5BH.
 */
#define USB_AUDIO_TIME_SYNC_VALUE			(0U)
/*	When a vCPU attempts to read guest PCI configuration audio time synchronization register of its
 *own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 00H.
 */
#define USB_SERIAL_BUS_NUMBER_VALUE			(0x30U)
/*	When a vCPU attempts to read guest PCI configuration serial bus release number register of its
 *own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 30H.
 */
#define USB_FRAME_LENGTH_ADJUST_VALUE			(0x60U)
/*	When a vCPU attempts to read guest PCI configuration frame length adjustment register of its
 *own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 60H.
 */
#define USB_BEST_EFFORT_SERVICE_LATENCY_VALUE		(0U)
/*	When a vCPU attempts to read guest PCI configuration best effort service latency register of its
 *own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 00H.
 */
#define USB_VENDOR_SPECIFIC_HEADER_VALUE		(0x1400010U)
/*	When a vCPU attempts to read guest PCI configuration vendor specific header register of its
 *own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 01400010H.
 */
#define USB_POWER_CONTROL_ENABLE_VALUE			(0U)
/*	When a vCPU attempts to read guest PCI configuration power control enable register of its
 *own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 00H.
 */
#define USB_HIGH_SPEED_CONFIGURATION2_VALUE		(0x1800UL)
/*	When a vCPU attempts to read guest PCI configuration high speed configuration 2 register of its
 *own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 00001800H.
 */

#define	U2OCM(n)			(0xB0U + (n - 1) * 4)
#define	U3OCM(n)			(0xD0U + (n - 1) * 4)

#define PAGESIZE	(0x88U)
#define HCIVERSION	(0x2U)
#define USB_HCIVERSION_VALUE	(0x100U)
#define GBECSR_5B54      (0x5B54U)
#define GBECSR_5B54_VALUE    (0x0U)

/**
 * @brief pci config register write.
 *
 * write the special data/nbyte to specail pci config register.
 * Application Constraints: none.
 *
 * @param union pci_bdf bdf:device BDF information.
 * @param uint32_t offset:config register address.
 * @param uint32_t bytes:number of byte.
 * @param uint32_t val:the write value.
 *
 * @pre param_1 != NULL and param_4 != 0.
 *
 * @return void.
 *
 */
static __unused void pci_pdev_write_cfg(union pci_bdf bdf, uint32_t offset, uint32_t bytes, uint32_t val)
{
	uint32_t addr;

	addr = bdf.value;
	addr = (addr << 8U) | (offset | PCI_CFG_ENABLE);

	/* Write address to ADDRESS register */
	pio_write32(addr, (uint16_t)PCI_CONFIG_ADDR);

	/* Write value to DATA register */
	switch (bytes) {
	case 1U:
		pio_write8((uint8_t)val, (uint16_t)PCI_CONFIG_DATA);
		break;
	case 2U:
		pio_write16((uint16_t)val, (uint16_t)PCI_CONFIG_DATA);
		break;
	case 4U:
		pio_write32(val, (uint16_t)PCI_CONFIG_DATA);
		break;
	default:
		printf("Error byte len to write\n\r");
		break;
	}
}

/**
 * @brief pci config register read.
 *
 * read the special data/nbyte to specail pci config register.
 * Application Constraints: none.
 *
 * @param union pci_bdf bdf:device BDF information.
 * @param uint32_t offset:config register address.
 * @param uint32_t bytes:number of byte.
 *
 * @pre param_1 != NULL and param_3 != 0.
 *
 * @return the value of register.
 *
 */
static uint32_t pci_pdev_read_cfg(union pci_bdf bdf, uint32_t offset, uint32_t bytes)
{
	uint32_t addr;
	uint32_t val = 0;

	addr = bdf.value;
	addr = (addr << 8U) | (offset | PCI_CFG_ENABLE);

	/* Write address to ADDRESS register */
	pio_write32(addr, (uint16_t)PCI_CONFIG_ADDR);

	/* Read result from DATA register */
	switch (bytes) {
	case 1U:
		val = (uint32_t)pio_read8(PCI_CONFIG_DATA);
		break;
	case 2U:
		val = (uint32_t)pio_read16(PCI_CONFIG_DATA);
		break;
	case 4U:
		val = pio_read32((uint16_t)PCI_CONFIG_DATA);
		break;
	default:
		printf("Error byte len to read\n\r");
		break;
	}

	return val;
}

#ifdef IN_SAFETY_VM
/**
 * @brief pci vendor register 2bytes read
 *
 * pci vendor register 2bytes read,from Ethernet device
 * Application Constraints: none.
 *
 * @param none
 *
 * @return void
 *
 * Case name:PCIe device_Ethernet controller assignment_001
 *
 * Summary: In 64-bit mode,the step is to read the correct #vendor id register of the Ethernet device
 * in the hypervisor environment.
 *
 * ACRN hypervisor shall assign the physical PCIe device at BDF 00:1F.6 to the safety VM.
 */
static void device_passthrough_rqmid_29267_ethernet_controller_assignment_001(void)
{
	uint32_t value = 0;

	value = pci_pdev_read_cfg(ethernet_bdf, PCI_VENDOR_OFFSET, PCI_VENDOR_BYTE);
	report("%s", value == PCI_VENDOR_VALUE_ETHERNET, __FUNCTION__);
}
#endif
#ifdef IN_NATIVE
/**
 * @brief pci vendor register 2bytes read
 *
 * pci vendor register 2bytes read,from Ethernet device
 * Application Constraints: none.
 *
 * @param none
 *
 * @return void
 *
 * Case name:PCIe device_Ethernet controller BDF_001
 *
 * Summary: In 64-bit mode,the step is to read the correct #vendor id register of the Ethernet device
 * in the NATIVE environment.
 * The physical platform shall have an Intel Corporation Ethernet Connection I219-LM at BDF 00:1F.6.
 */
static void device_passthrough_rqmid_29272_ethernet_controller_bdf_001(void)
{
	uint32_t value;

	value = pci_pdev_read_cfg(ethernet_bdf, PCI_VENDOR_OFFSET, PCI_VENDOR_BYTE);
	report("%s", value == PCI_VENDOR_VALUE_ETHERNET, __FUNCTION__);
}

/**
 * @brief ethernet bar register read and write.
 *
 * pci bar register 4bytes read and write from Ethernet device.
 * Application Constraints: none.
 *
 * @param none.
 *
 * @return void.
 *
 * Case name:PCIe device_Ethernet BAR read-write_002.
 *
 * Summary: In 64-bit mode,the first step is to read #BAR0 register of the Ethernet device.
 * in the hypervisor environment,recorded the value.
 * The step2 is write another value to the #BAR0 Register for the Ethernet device.
 * The step3 is read #BAR0 register of the NET device in the hypervisor enviroment,get the value1.
 * at last compare the step2 value with step3.
 */
static void device_passthrough_rqmid_29275_ethernet_bar_read_write_002(void)
{
	u32 value, value1;
	/*
	 * here are some bugs for HV, so write 0xffffffff to BAR0 register firstly, then read
	 */
	value = pci_pdev_read_cfg(ethernet_bdf, PCI_BAR0_OFFSET, PCI_BAR_BYTE);
	pci_pdev_write_cfg(ethernet_bdf, PCI_BAR0_OFFSET, PCI_BAR_BYTE, 0xffffffff);
	value1 = pci_pdev_read_cfg(ethernet_bdf, PCI_BAR0_OFFSET, PCI_BAR_BYTE);

	report("%s", (value != value1), __FUNCTION__);
}
#endif
#ifdef IN_SAFETY_VM
/**
 * @brief compare the ethernet init bar0 data
 *
 * compare the ethernet bar0 data at start-up and UNIT-TEST
 * Application Constraints: none.
 *
 * @param none
 *
 * @return void
 *
 * Case name:PCIe device_Ethernet BAR0 init_001
 *
 * Summary: In 64-bit mode,in the BSP and AP launch process, we select two measurement points.
 * If the value getting from the two measured points is the same, and  the value is B3C00000H,
 * then the test is passed, or the test fails.
 * The four measuring points are as follows:
 *		BP first instruction,BP first instruction of the UNIT-TEST.
 *		Measurement method:
 *		Dump the value of the pci  BAR0 register for the Ethernet device.
 */
static void device_passthrough_rqmid_29278_ethernet_bar0_init_001(void)
{
	/*@cstart64.S will dump ETH Bar0 to STARTUP_ETH_BAR0_ADDRESS */
	report("%s", ETH_START_UP_BAR0_VALUE == *(uint32_t *)STARTUP_ETH_BAR0_ADDRESS, __FUNCTION__);
}
#endif
#ifdef IN_SAFETY_VM
/**
 * @brief ethernet lock lan register read and write
 *
 * ethernet register space read write test
 * Application Constraints: none.
 *
 * @param none
 *
 * @return void
 *
 * Case name:PCIe device_Lock LAN Disable write_001
 *
 * Summary:
 * When a vCPU attempts to write guest PCI configuration lock LAN disable register of its own Ethernet controller,
 * ACRN hypervisor shall guarantee that the write is ignored.
 * In 64-bit mode,the first step is write 0 to the lock Lan disable register,and the read it
 * of the Ethernet device in the hypervisor environment,recorded the value.
 * The second step is write the register (all bit 1)
 * the third step is read the lock LAN disable register again ,and compare the two value
 * if it is the same , the test success,or the test is false
 */
static void device_passthrough_rqmid_29287_lock_lan_disable_write_001(void)
{
	uint32_t compare1, compare2;

	pci_pdev_write_cfg(ethernet_bdf, LOCKLANDIS, BYTE_4, 0);
	compare1 = pci_pdev_read_cfg(ethernet_bdf, LOCKLANDIS, BYTE_4);

	pci_pdev_write_cfg(ethernet_bdf, LOCKLANDIS, BYTE_4, 0xffffffff);
	compare2 = pci_pdev_read_cfg(ethernet_bdf, LOCKLANDIS, BYTE_4);

	report("%s", compare1 == compare2, __FUNCTION__);
}
#endif


#ifdef IN_NON_SAFETY_VM
/**
 * @brief pci read bar0 and 1 of USB device
 *
 * pci read bar0 and 1 data of the  BP start-up bar0 ;BP start-up bar1 ;
 * AP init bar0 ;AP init bar1 ;UNIT-TEST bar0 and bar1 of USB device
 * Application Constraints: none.
 *
 * @param none
 *
 * @return void
 *
 * Case name:PCIe device_USB BAR0 and 1 init_001
 *
 * Summary:In 64-bit mode,in the BSP and AP launch process, we select four measurement points.
 * If the value getting from the four measured points is the same, then the test is passed, or the test fails.
 * The four measuring points are as follows:
 *	BP first instruction,AP first instruction,AP last instruction before entering the UNIT-TEST,
 *	BP first instruction of the UNIT-TEST.
 *	Measurement method:
 *	Dump the value of the BAR0/1 register for the USB device.
 */
static void device_passthrough_rqmid_29308_usb_bar0_and_1_init_001(void)
{
	volatile uint32_t usb_bar0_startup_value = 0;
	volatile uint32_t usb_bar0_init_value = 0;
	uint32_t usb_bar0_current_value = 0;
	volatile uint32_t usb_bar1_startup_value = 0;
	volatile uint32_t usb_bar1_init_value = 0;
	uint32_t usb_bar1_current_value = 0;
	volatile uint32_t unchanged_bar0_value = 0;
	volatile uint32_t unchanged_bar1_value = 0;

	usb_bar0_startup_value = *(volatile uint32_t *)STARTUP_USB_BAR0_ADDRESS;
	usb_bar0_init_value = *(volatile uint32_t *)INIT_USB_BAR0_ADDRESS;
	usb_bar1_startup_value = *(volatile uint32_t *)STARTUP_USB_BAR1_ADDRESS;
	usb_bar1_init_value = *(volatile uint32_t *)INIT_USB_BAR1_ADDRESS;

	usb_bar0_current_value = pci_pdev_read_cfg(usb_bdf, PCI_BAR0_OFFSET, PCI_BAR_BYTE);
	usb_bar1_current_value = pci_pdev_read_cfg(usb_bdf, PCI_BAR1_OFFSET, PCI_BAR_BYTE);

	/*init All Aps again*/
	send_sipi();
	unchanged_bar0_value = *(volatile uint32_t *)INIT_USB_BAR0_ADDRESS;
	unchanged_bar1_value = *(volatile uint32_t *)INIT_USB_BAR1_ADDRESS;

	if ((unchanged_bar0_value == usb_bar0_init_value)
		&& (usb_bar0_init_value == usb_bar0_startup_value)
		&& (usb_bar0_startup_value == usb_bar0_current_value)
		&& (unchanged_bar1_value == usb_bar1_init_value)
		&& (usb_bar1_init_value == usb_bar1_startup_value)
		&& (usb_bar1_startup_value == usb_bar1_current_value)) {
		report("%s", 1, __FUNCTION__);
	} else {
		report("%s", 0, __FUNCTION__);
	}
}
#endif
/**
 * @brief find the usb message control register [bit 8:7] value
 *
 * read the pci usb message control register [bit 8:7].
 * Application Constraints: none.
 *
 * @param none
 *
 * @return void
 *
 * Case name:PCIe device_USB controller message control_002
 *
 * Summary: In 64-bit mode,the first step is to read #cap register of the USB device.
 * in the native environment,record the value which target the offset.
 * The second step is found the MSI struct which id is 5,the location come from the offset value.
 * the third step is read the message control register[bit 8:7],offset the MSI struct is 16bit.
 * it shoud be 1 on success.
 * the locations struct like the:
 *	1,fisrt offset value come from the #cap(offset 0x34),the value point some struct .
 *	2,all the struct first byte is the struct id,the MSI struct id should be 5,
 *	  and the second byte the the next some struct offset value.
 *	3,the the message control register is the third byte of the MSI struct.
 */
#ifdef IN_NATIVE
static void device_passthrough_rqmid_29338_usb_controller_message_control_002(void)
{
	uint32_t value = 0;
	uint32_t offset_point = 0;
	uint16_t target_data = 0;

	offset_point = pci_pdev_read_cfg(usb_bdf, PCI_CAP_OFFSET, PCI_CAP_BYTE);
	value = pci_pdev_read_cfg(usb_bdf, offset_point, BYTE_4);
	while ((value & 0xff) != 5) {
		value = pci_pdev_read_cfg(usb_bdf, (value & 0xff00) >> 8U, BYTE_4);
	}

	printf("value:%x", value);
	target_data = value >> 16;
	report("%s", ((target_data >> 7) & 0x3u) == 0x1u, __FUNCTION__);
}
#endif
static void print_case_list(void)
{
	printf("Device passthrough feature case list:\n\r");
#ifdef IN_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29267, "ethernet controller assignment 001");
#endif
#ifdef IN_NATIVE
	printf("\t Case ID:%d case name:%s\n\r", 29272, "ethernet controller bdf 001");
	printf("\t Case ID:%d case name:%s\n\r", 29275, "ethernet bar read write 002");
#endif
#ifdef IN_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29278, "eternet bar init 001");
	printf("\t Case ID:%d case name:%s\n\r", 29287, "lock lan disable write 001");
#endif
#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29308, "usb bar0 and bar1 init 001");
#endif
#ifdef IN_NATIVE
	printf("\t Case ID:%d case name:%s\n\r", 29338, "usb controller message control 002");
#endif
}
int main()
{
	setup_idt();

	print_case_list();
#ifdef IN_SAFETY_VM
	device_passthrough_rqmid_29267_ethernet_controller_assignment_001();
#endif
#ifdef IN_NATIVE
	device_passthrough_rqmid_29272_ethernet_controller_bdf_001();
	device_passthrough_rqmid_29275_ethernet_bar_read_write_002();
#endif
#ifdef IN_SAFETY_VM
	device_passthrough_rqmid_29278_ethernet_bar0_init_001();
	device_passthrough_rqmid_29287_lock_lan_disable_write_001();
#endif
#ifdef IN_NON_SAFETY_VM
	device_passthrough_rqmid_29308_usb_bar0_and_1_init_001();
#endif
#ifdef IN_NATIVE
	device_passthrough_rqmid_29338_usb_controller_message_control_002();
#endif

	return report_summary();
}
