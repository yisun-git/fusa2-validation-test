#include "../pci_check.h"
#include "../../lib/e1000_def.h"
#include "../../lib/e1000_regs.h"
#define PCI_CONFIG_ADDR		0xCF8U
#define PCI_CONFIG_DATA		0xCFCU

/*test PCI port address*/
mov $(PCI_CONFIG_ADDR), %edx
inl (%dx), %eax
mov %eax, (BP_IO_PORT_ADDR)

/*Read Net device 00:1f.06 interrupt line.*/
mov $0x8000FE3C, %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
/*Read PCI_CFG_DATA*/
mov $(PCI_CONFIG_DATA), %edx
inb (%dx), %al
/*Save pci device config register value*/
mov %eax, (BP_NET_INTERRUP_LINE_ADDR)

/*Read USB device 00:14.00 interrupt line.*/
mov $0x8000A03C, %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
/*Read PCI_CFG_DATA*/
mov $(PCI_CONFIG_DATA), %edx
inb (%dx), %al
/*Save pci device config register value*/
mov %eax, (BP_USB_INTERRUP_LINE_ADDR)

/*Read Net device 00:1f.06 BAR0*/
mov $0x8000FE10, %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
/*Read PCI_CFG_DATA*/
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_NET_BAR0_ADDR)

/*Read Device CTRL Register*/
and $0xFFFFFFF0, %eax
add $0x8, %eax
mov (%eax), %ecx
mov %ecx, (BP_NET_DEVICE_STATUS_ADDR)

/*Read USB device 00:14.00 BAR0*/
mov $0x8000A010, %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
/*Read PCI_CFG_DATA*/
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_USB_BAR0_ADDR)
/*Read USB HCIVERSION*/
and $0xFFFFFFF0, %eax
add $(HCIVERSION), %eax
mov (%eax), %cx
mov %ecx, (BP_USB_DEVICE_HCIVERSION_ADDR)

/*Read Net device 00:1f.06 MSI FlAG*/
mov $0x8000FED0, %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
/*Read PCI_CFG_DATA*/
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_NET_MSI_CTRL_ADDR)

/*Read USB device 00:14.00 MSI FLAG*/
mov $0x8000A080, %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
/*Read PCI_CFG_DATA*/
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_USB_MSI_CTRL_ADDR)

/*Read Net device 00:1f.06 Status and Command*/
mov $0x8000FE04, %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
/*Read PCI_CFG_DATA*/
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_NET_STATUS_COMMAND_ADDR)

/*Read USB device 00:14.00 Status and Command*/
mov $0x8000A004, %eax
mov $(PCI_CONFIG_ADDR), %edx
outl %eax, (%dx)
/*Read PCI_CFG_DATA*/
mov $(PCI_CONFIG_DATA), %edx
inl (%dx), %eax
mov %eax, (BP_USB_STATUS_COMMAND_ADDR)

