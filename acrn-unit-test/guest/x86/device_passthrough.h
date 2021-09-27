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
#define		HSCFG2		(0xA4U)	/*	High Speed Configuration 2 (HSCFG2)—Offset A4h bitsize 32	*/
#define		PCE_REG		(0xA2U)	/*	Power Control Enable (PCE_REG)—Offset A2h bitsize 16		*/
#define		VSHDR		(0x94U)	/*	Vendor Specific Header (VSHDR)—Offset 94h bitsize 32		*/
#define		BESL		(0x62U)	/*	Best Effort Service Latency (BESL)—Offset 62h	bit size 8	*/
#define		FLADJ		(0x61U)	/*	Frame Length Adjustment (FLADJ)—Offset 61h bitsize 8		*/
#define		SBRN		(0x60U)	/*	Serial Bus Release Number (SBRN)—Offset 60h bitsize 8		*/
#define		AUDSYNC		(0x58U)	/*	Audio Time Synchronization (AUDSYNC)—Offset 58h bit size 32	*/
#define		XHCLKGETN	(0x50U)	/*	Clock Gating (XHCLKGTEN)—Offset 50h bitsize 32		*/
#define		XHCC1		(0x40U)	/*	XHC System Bus Configuration 1 (XHCC1)—Offset 40h bitsize 32	*/
#define		XHCC2		(0x44U)	/*	XHC System Bus Configuration 2 (XHCC2)—Offset 44h bitsize 32	*/

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
#define USB_SUB_SYSTEM_VENDOR_ID_VALUE			(0x8086U)
#define ETH_PHYSICAL_BAR0_BIT_16_to_0_VALUE		(0x0U)
#define ETH_INTERRUPTE_PIN_VALUE			(0x1U)
#define ETH_START_UP_STATUS_VALUE			(0x10U)
#define ETH_START_UP_BAR0_VALUE				(0xDF200000U)
#define ETH_BAR0_VALUE					(0xDF200000U)
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
#define USB_START_UP_BAR0_VALUE				(0xDF230000U)
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

#define USB_ORIGINAL_BAR0_VALUE	(0xdf230004U)
#define USB_ORIGINAL_BAR1_VALUE	(0x0U)
#define ETH_ORIGINAL_BAR0_VALUE	(0xdf200000U)

#define A_NEW_USB_BAR0_VALUE	(0xDF240000U)
#define A_NEW_USB_BAR1_VALUE	(0xB3D00000U)
#define A_NEW_USB_STATUS_VALUE	SHIFT_LEFT(1, 14)

#define USB_MSI_REG_OFFSET	(0x80U)
#define ETH_MSI_REG_OFFSET	(0xd0U)
#define MSI_CAPABILITY_ID	(0x05U)

#endif	//__PCI_DEVICE_PASSTHROUGH_H__
