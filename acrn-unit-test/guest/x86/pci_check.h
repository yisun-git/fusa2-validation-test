#ifndef __PCI_CHECK_H__
#define __PCI_CHECK_H__

#define HCIVERSION	(0x2U)
#define GBECSR_00	(0x00U)
#define PCI_CONFIG_SPACE_SIZE		0x100U
#define PCI_CONFIG_RESERVE		0x36
#define BAR_REMAP_BASE		0xDFF00000
#define BAR_REMAP_BASE_1		0xF0000000
#define BAR_REMAP_BASE_2		0xDFFF0000
#define BAR_REMAP_BASE_3		0xFFF00000
#define PCI_NET_STATUS_VAL_NATIVE		(0x0010)
#define PCI_USB_STATUS_VAL_NATIVE		(0x0290)
#define	MSI_NET_IRQ_VECTOR	(0x40)
#define VALID_APIC_ID_1 (0x01)
#define INVALID_APIC_ID_A (0x0A)
#define INVALID_APIC_ID_B (0xFFFF)
#define INVALID_REG_VALUE_U	(0xFFFFFFFFU)
#define PCI_EXIST_DEV_NUM	3
//#define BAR_HOLE_LOW	0xC0000000
//#define BAR_HOLE_HIGH 0xDFFFFFFF
#define BAR_HOLE_LOW_SOS	0x8C000000    //Current ACRN PCI hole window for service VM is 0x8C000000-0XFE800000
#define BAR_HOLE_HIGH_SOS	0XFE800000
#define BAR_HOLE_LOW	0x80000000    //Current ACRN PCI hole window for safety VM is 0x80000000-0xE0000000
#define BAR_HOLE_HIGH	0xE0000000
#define BAR_ALLIGN_USB 0x10000
#define BAR_ALLIGN_NET 0x100000
//#define PCI_NET_REVISION_ID	(0x21)
#define PCI_NET_REVISION_ID	(0x31)    //On my SKL
//#define PCI_USB_REVISION_ID (0x21)
#define PCI_USB_REVISION_ID (0x31)    //On my SKL
#define PCI_USB_VENDOR_ID	(0x8086)
#define PCI_NET_VENDOR_ID	(0x8086)
#define PCI_USB_HEADER_TYPE	(0x80)
#define PCI_NET_HEADER_TYPE	(0x00)
//#define PCI_NET_DEVICE_ID	(0x156f)
#define PCI_NET_DEVICE_ID	(0x15b7)    //On my SKL
//#define PCI_USB_DEVICE_ID	(0x9d2f)
#define PCI_USB_DEVICE_ID	(0xa12f)    //On my SKL
//#define HCIVERSION_VALUE	(0x100)
#define HCIVERSION_VALUE	(0x1a00)    //On my SKL
#define GBECSR_00_VALUE		(0x180240)
#define HOSTBRIDGE_CLASS_CODE	(0x060000)
#define HOSTBRIDGE_REVISION_ID	(0x0b)
//#define HOSTBRIDGE_DEVICE_ID	(0x5AF0)
#define HOSTBRIDGE_DEVICE_ID	(0x191F)    //On my SKL
#define HOSTBRIDGE_VENDOR_ID	(0x8086)
#define BP_USB_MSI_CTRL_ADDR	(0x9104)
#define BP_NET_MSI_CTRL_ADDR	(0x8104)
#define BP_USB_STATUS_COMMAND_ADDR	(0x9108)
#define BP_NET_STATUS_COMMAND_ADDR	(0x8108)
#define BP_NET_BAR0_ADDR	(0x8100)
#define BP_USB_BAR0_ADDR	(0x9100)
#define BP_IO_PORT_ADDR		(0x7000)
#define AP_IO_PORT_ADDR		(0x7004)
#define BP_NET_INTERRUP_LINE_ADDR	(0x8000)
#define AP_NET_INTERRUP_LINE_ADDR0	(0x8004)
#define AP_NET_INTERRUP_LINE_ADDR1	(0x8008)
#define BP_USB_INTERRUP_LINE_ADDR	(0x9000)
#define AP_USB_INTERRUP_LINE_ADDR0	(0x9004)
#define AP_USB_INTERRUP_LINE_ADDR1	(0x9008)
#define BP_NET_DEVICE_STATUS_ADDR	(0x810C)
#define BP_USB_DEVICE_HCIVERSION_ADDR	(0x910C)


#define BP_START_UP_COMMAND_VALUE	(0x0406)


#define MAX_PCI_DEV_NUM	(256)
#define NET_DEV_VENDOR	DEV_VEN(PCI_NET_DEVICE_ID, PCI_NET_VENDOR_ID)
#define USB_DEV_VENDOR	DEV_VEN(PCI_USB_DEVICE_ID, PCI_USB_VENDOR_ID)
#define HOSTBRIDGE_DEV_VENDOR	DEV_VEN(HOSTBRIDGE_DEVICE_ID, HOSTBRIDGE_VENDOR_ID)

#endif
