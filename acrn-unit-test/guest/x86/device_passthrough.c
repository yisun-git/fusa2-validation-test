/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "device_passthrough.h"
#include "misc.h"
#include "pci_util.h"
#include "pci_check.h"
#include "pci_regs.h"
#include "processor.h"
#include "smp.h"
#include "e1000e.h"
#include "misc.h"
#include "fwcfg.h"
#include "delay.h"

#ifdef PASS_THROUGH_DEBUG
#define PASS_THROUGH_DEBUG_LEVEL		DBG_LVL_INFO
#else
#define PASS_THROUGH_DEBUG_LEVEL		DBG_LVL_ERROR
#endif

union pci_bdf usb_bdf = {
	.bits.b = 0x0,
	.bits.d = 0x1,
	.bits.f = 0x0
};

union pci_bdf ethernet_bdf = {
	.bits.b = 0x0,
	.bits.d = 0x1,
	.bits.f = 0x0
};

union pci_bdf usb_bdf_native = {
	.bits.b = 0x0,
	.bits.d = 0x14,
	.bits.f = 0x0
};

union pci_bdf ethernet_bdf_native = {
	.bits.b = 0x0,
	.bits.d = 0x1f,
	.bits.f = 0x6
};

#ifdef IN_NON_SAFETY_VM
static uint32_t usb_bar0_ori_value = 0U;
static uint32_t usb_bar1_ori_value = 0U;
static uint32_t usb_status_unchanged_value = 0U;
#endif
volatile uint32_t unchanged_bar0_value = 0;
volatile uint32_t unchanged_bar1_value = 0;

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;

__unused static void modify_USB_controller_status_init_value();
__unused static void restore_usb_bar_0_1(void);

__unused void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}

__unused static void notify_ap_modify_and_read_init_value(int case_id)
{
	cur_case_id = case_id;
	need_modify_init_value = 1;
	/* will change INIT value after AP reboot */
	send_sipi();
	wait_ap_ready();
	/* Will check INIT value after AP reboot again */
	send_sipi();
	wait_ap_ready();
}

#ifdef __i386__
void ap_main(void)
{
	asm volatile ("pause");
}

#elif __x86_64__

typedef void (*ap_init_value_modify)(void);
__unused static void ap_init_value_process(ap_init_value_modify modify_init_func)
{
	if (need_modify_init_value) {
		need_modify_init_value = 0;
		modify_init_func();
		wait_ap = 1;
	} else {
		if (cur_case_id == 37941) {
#ifdef IN_NON_SAFETY_VM
			restore_usb_bar_0_1();
#endif
		}
		wait_ap = 1;
	}
}

__unused static void modify_USB_controller_status_init_value()
{
	pci_pdev_write_cfg(usb_bdf, PCI_STATUS, BYTE_2, A_NEW_USB_STATUS_VALUE);
}

static void restore_usb_bar_0_1(void)
{
#ifdef IN_NON_SAFETY_VM
	/*write the original value to BAR[0-1]*/
	pci_pdev_write_cfg(usb_bdf, PCI_BASE_ADDRESS_0, BYTE_4, usb_bar0_ori_value);
	pci_pdev_write_cfg(usb_bdf, PCI_BASE_ADDRESS_1, BYTE_4, usb_bar1_ori_value);
#endif
}

__unused static void modify_device_USB_BAR0_and_1_init_value()
{
#ifdef IN_NON_SAFETY_VM
	/*read the original value of BAR[0-1]*/
	usb_bar0_ori_value = pci_pdev_read_cfg(usb_bdf, PCI_BASE_ADDRESS_0, BYTE_4);
	usb_bar1_ori_value = pci_pdev_read_cfg(usb_bdf, PCI_BASE_ADDRESS_1, BYTE_4);
	/*write a new value to BAR[0-1]*/
	pci_pdev_write_cfg(usb_bdf, PCI_BASE_ADDRESS_0, BYTE_4, A_NEW_USB_BAR0_VALUE);
	pci_pdev_write_cfg(usb_bdf, PCI_BASE_ADDRESS_1, BYTE_4, A_NEW_USB_BAR1_VALUE);
#endif
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_lapic_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 37941:
		fp = modify_device_USB_BAR0_and_1_init_value;
		ap_init_value_process(fp);
		break;
	case 37942:
		fp = modify_USB_controller_status_init_value;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif

/*
 *@brief pci check bar reserve cases
 * This function is to test PCI BAR reserve cases:29274,
 * 29276, 29303, 29301
 *@param union pci_bdf bdf:device BDF information
 *@param uint32_t bar_index:BAR index [0-5]
 *@return the bool result of comparison
 */
static bool pci_check_bar_reserve(union pci_bdf bdf, uint32_t bar_index)
{
	uint32_t value = 0U;
	uint32_t value1 = 0U;
	uint32_t value2 = 0U;
	bool is_pass = false;

	/*1.Read BAR as the Value(A)*/
	value = pci_pdev_read_cfg(bdf, PCIR_BAR(bar_index), PCI_BAR_BYTE);
	/*2.Check BAR value [bit 0]*/
	if ((value & shift_mask(0, 0)) == 0) {
		/*3.For BAR[bit 0] == 0b,Write BAR with a new memory value*/

		/*Add 1M offset to original base address*/
		value1 = value + 0x00100000U;

		/*Do write*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(bar_index), PCI_BAR_BYTE, value1);

		/*4.Read the #BAR Register*/
		value2 = pci_pdev_read_cfg(bdf, PCIR_BAR(bar_index), PCI_BAR_BYTE);

		value &= shift_umask(3, 0);
		value2 &= shift_umask(3, 0);

		/*5.Compare*/
		is_pass = (value == value2) ? true : false;
	} else {
		/*6.For BAR[bit 0] == 1b,Write BAR with a new I/O value*/

		/*Add 1M offset to original base address*/
		value1 = value + 0x00100000U;

		/*Do write*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(bar_index), PCI_BAR_BYTE, value1);

		/*7.Read the #BAR Register*/
		value2 = pci_pdev_read_cfg(bdf, PCIR_BAR(bar_index), PCI_BAR_BYTE);

		value &= shift_umask(1, 0);
		value2 &= shift_umask(1, 0);

		/*8.Compare*/
		is_pass = (value == value2) ? true : false;
	}
	/*Reset BAR register environment*/
	pci_pdev_write_cfg(bdf, PCIR_BAR(bar_index), PCI_BAR_BYTE, value);
	/*9.Report*/
	return is_pass;
}

/*
 *@brief pci check bar read_write cases
 * This function is to test PCI BAR read/write cases:29275,
 * 29273, 29300, 29302
 *@param union pci_bdf bdf:device BDF information
 *@param uint32_t bar_index:BAR index [0-5]
 *@return the bool result of comparison
 */
static bool pci_check_bar_read_write(union pci_bdf bdf, uint32_t bar_index)
{
	uint32_t value = 0U;
	uint32_t value1 = 0U;
	uint32_t value2 = 0U;
	bool is_pass = false;

	/*1.Read BAR as the Value(A)*/
	value = pci_pdev_read_cfg(bdf, PCIR_BAR(bar_index), PCI_BAR_BYTE);
	/*2.Check BAR value [bit 0]*/
	if ((value & shift_mask(0, 0)) == 0) {
		/*3.For BAR[bit 0] == 0b,Write BAR with a new memory value*/

		/*Add 1M offset to original base address*/
		value1 = value + 0x00100000U;
		/*Do write*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(bar_index), PCI_BAR_BYTE, value1);

		/*4.Read the #BAR Register*/
		value2 = pci_pdev_read_cfg(bdf, PCIR_BAR(bar_index), PCI_BAR_BYTE);

		value1 &= shift_umask(3, 0);
		value2 &= shift_umask(3, 0);
		/*5.Compare*/
		is_pass = (value1 == value2) ? true : false;
	} else {
		/*6.For BAR[bit 0] == 1b,Write BAR with a new I/O value*/

		/*Add 1M offset to original base address*/
		value1 = value + 0x00100000U;

		/*Do write*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(bar_index), PCI_BAR_BYTE, value1);

		/*7.Read the #BAR Register*/
		value2 = pci_pdev_read_cfg(bdf, PCIR_BAR(bar_index), PCI_BAR_BYTE);

		value1 &= shift_umask(1, 0);
		value2 &= shift_umask(1, 0);

		/*8.Compare*/
		is_pass = (value1 == value2) ? true : false;
	}
	/*Reset BAR register environment*/
	pci_pdev_write_cfg(bdf, PCIR_BAR(bar_index), PCI_BAR_BYTE, value);
	/*9.Report*/
	return is_pass;
}


#ifdef IN_NATIVE
/*
 *@brief pci probe MSI capability structure
 * This function is to test MSI capability structure
 *@param union pci_bdf bdf:device BDF information
 *@param uint32_t *msi_addr:get msi address found in configuration space
 *@return OK: found MSI capability; ERROR:found no MSI capability
 */
static int pci_probe_msi_capability(union pci_bdf bdf, uint32_t *msi_addr)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	int i = 0;
	int repeat = 0;
	reg_addr = PCI_CAPABILITY_LIST;
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 1);
	DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);

	reg_addr = reg_val;
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
	DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
	/*
	 * pci standard config space [00h,100h).So,MSI register must be in [00h,100h),
	 * if there is MSI register
	 */
	repeat = PCI_CONFIG_SPACE_SIZE / 4;
	for (i = 0; i < repeat; i++) {
		/*Find MSI register group*/
		if (PCI_CAP_ID_MSI == (reg_val & SHIFT_MASK(7, 0))) {
			break;
		}
		reg_addr = (reg_val & SHIFT_MASK(15, 8)) >> 8;
		reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
		//DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
	}
	if (repeat == i) {
		DBG_ERRO("no msi cap found!!!");
		return ERROR;
	}
	*msi_addr = reg_addr;
	return OK;
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe device_Ethernet controller assignment_001
 *
 * Summary:ACRN hypervisor shall assign the physical PCIe device at BDF 00:1F.6 to the safety VM.
 */
static void device_passthrough_rqmid_29267_PCIe_device_Ethernet_controller_assignment_001(void)
{
	uint32_t value = 0U;
	value = pci_pdev_read_cfg(ethernet_bdf, PCI_VENDOR_OFFSET, PCI_VENDOR_BYTE);
	report("%s", value == PCI_VENDOR_VALUE_ETHERNET, __FUNCTION__);
}
#endif

#ifdef IN_NATIVE
/*
 * @brief case name:PCIe device_Ethernet controller BDF_001
 *
 * Summary:The physical platform shall have an Intel Corporation Ethernet Connection I219-LM at BDF 00:1F.6.
 */
static void device_passthrough_rqmid_29272_PCIe_device_Ethernet_controller_BDF_001(void)
{
	uint32_t value = 0U;
	value = pci_pdev_read_cfg(ethernet_bdf_native, PCI_VENDOR_OFFSET, PCI_VENDOR_BYTE);
	report("%s", value == PCI_VENDOR_VALUE_ETHERNET, __FUNCTION__);
}

/*
 * @brief case name:PCIe device_Ethernet BAR read-write_002
 *
 * Summary:ACRN hypervisor shall guarantee that the guest PCI configuration
 * base address register 0 (BAR0) of the Ethernet controller is read-write.
 */
static void device_passthrough_rqmid_29275_PCIe_device_Ethernet_BAR_read_write_002(void)
{
	bool is_pass = false;
	is_pass = pci_check_bar_read_write(ethernet_bdf_native, 0);
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe device_Ethernet BAR0 init_001
 *
 * Summary:ACRN hypervisor shall set initial guest PCI configuration base address
 * register 0 (BAR0) of the Ethernet controller to DF20 0000H following start-up in Safety guest.
 */
static void device_passthrough_rqmid_29278_PCIe_device_Ethernet_BAR0_init_001(void)
{
	/*@cstart64.S will dump ETH Bar0 to STARTUP_ETH_BAR0_ADDRESS */
	report("%s", ETH_START_UP_BAR0_VALUE == *(uint32_t *)STARTUP_ETH_BAR0_ADDRESS, __FUNCTION__);
}

/*
 * @brief case name:PCIe device_Lock LAN Disable write_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration lock LAN disable register
 * of its own Ethernet controller, ACRN hypervisor shall guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_29287_PCIe_device_Lock_LAN_Disable_write_001(void)
{
	uint32_t compare1 = 0U;
	uint32_t compare2 = 0U;
	uint32_t compare3 = 0U;
	compare1 = pci_pdev_read_cfg(ethernet_bdf, LOCKLANDIS, BYTE_4);
	pci_pdev_write_cfg(ethernet_bdf, LOCKLANDIS, BYTE_4, (compare1 ^ 0x0000FFFFU));
	compare2 = pci_pdev_read_cfg(ethernet_bdf, LOCKLANDIS, BYTE_4);
	pci_pdev_write_cfg(ethernet_bdf, LOCKLANDIS, BYTE_4, (compare1 ^ 0xFFFF0000U));
	compare3 = pci_pdev_read_cfg(ethernet_bdf, LOCKLANDIS, BYTE_4);

	report("%s", (compare1 == compare2) && (compare2 ==  compare3), __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe device_USB BAR0 and 1 init_001
 *
 * Summary:ACRN hypervisor shall keep initial guest PCI configuration base address register
 * 0-1 (BAR0-1) of the USB controller unchanged following INIT.
 */
static void device_passthrough_rqmid_29308_PCIe_device_USB_BAR0_and_1_init_001(void)
{
	bool is_pass = false;
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
		is_pass = true;
	} else {
		is_pass = false;
	}
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NATIVE
/*
 * @brief case name:PCIe device_USB controller message control_002
 *
 * Summary:The value in the message control register [bit 8:7] of the PCI MSI capability
 * structure of the physical PCIe device at BDF 00:14.0 shall be 1H.
 */
static void device_passthrough_rqmid_29338_PCIe_device_USB_controller_message_control_002(void)
{
	uint32_t value = 0;
	uint32_t offset_point = 0;
	uint16_t target_data = 0;
	/*1.Found the MSI CAP register.*/
	offset_point = pci_pdev_read_cfg(usb_bdf_native, PCI_CAP_OFFSET, PCI_CAP_BYTE);
	value = pci_pdev_read_cfg(usb_bdf_native, offset_point, BYTE_4);
	while ((value & 0xff) != 5) {
		value = pci_pdev_read_cfg(usb_bdf_native, (value & 0xff00) >> 8U, BYTE_4);
	}
	/*2.Read the message control register.*/
	target_data = value >> 16;
	/*3.Compare the value, the message control register [bit 8:7]  should be 1.*/
	report("%s", ((target_data >> 7) & 0x3u) == 0x1u, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe Ethernet controller subsystem ID_001
 *
 * Summary:The value in the PCI configuration subsystem ID
 * register of the physical PCIe device at BDF 00:1F.6 or BDF 00:14.0 shall be 2070H.
 */
static void device_passthrough_rqmid_29281_PCIe_Ethernet_controller_subsystem_ID_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(ethernet_bdf, PCI_SUB_SYSTEM_ID_OFFSET, PCI_SUB_SYSTEM_ID_BYTE);
	if (value == ETH_SUB_SYSTEM_ID_VALUE) {
		is_pass = true;
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name:PCIe Ethernet controller subsystem vendor ID_001
 *
 * Summary:The value in the PCI configuration subsystem vendor ID register
 * of physical PCIe device at BDF 00:1F.6 or BDF 00:14.0 shall be 8086H.
 */
static void device_passthrough_rqmid_29282_PCIe_Ethernet_controller_subsystem_vendor_ID_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(ethernet_bdf, PCI_SUB_SYS_VENDOR_OFFSET, PCI_SUB_SYS_VENDOR_BYTE);
	if (value == ETH_SUB_SYSTEM_VENDOR_ID_VALUE) {
		is_pass = true;
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name:PCIe System Time Control High Register_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration system time control
 * high register of its own Ethernet controller, ACRN hypervisor shall guarantee
 * that the vCPU gets 10031003H.
 */
static void device_passthrough_rqmid_29514_PCIe_System_Time_Control_High_Register_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(ethernet_bdf, LTRCAP, BYTE_4);
	is_pass = (value == ETH_SYSTEM_TIME_CONTROL_HIGH_REGISTER_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe System Time Control High Register write_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration system time
 * control high register of its own Ethernet controller, ACRN hypervisor shall
 * guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_29286_PCIe_System_Time_Control_High_Register_write_001(void)
{
	uint32_t compare0 = 0;
	uint32_t compare1 = 0;
	uint32_t compare2 = 0;
	bool is_pass = false;
	compare0 = pci_pdev_read_cfg(ethernet_bdf, LTRCAP, BYTE_4);
	pci_pdev_write_cfg(ethernet_bdf, LTRCAP, BYTE_4, (compare0 ^ 0x0000FFFFU));
	compare1 = pci_pdev_read_cfg(ethernet_bdf, LTRCAP, BYTE_4);
	pci_pdev_write_cfg(ethernet_bdf, LTRCAP, BYTE_4, (compare0 ^ 0xFFFF0000U));
	compare2 = pci_pdev_read_cfg(ethernet_bdf, LTRCAP, BYTE_4);
	is_pass = ((compare0 == compare1) && (compare1 == compare2)) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Lock LAN Disable_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration lock
 * LAN disable register of its own Ethernet controller, ACRN hypervisor
 * shall guarantee that the vCPU gets 00000001H.
 */
static void device_passthrough_rqmid_29289_PCIe_Lock_LAN_Disable_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(ethernet_bdf, LOCKLANDIS, BYTE_4);
	is_pass = (ETH_LOCK_LAN_DISABLE_REGISTER_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe LAN Disable Control_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration LAN disable
 * control register of its own Ethernet controller, ACRN hypervisor shall guarantee
 * that the vCPU gets 00000000H.
 */
static void device_passthrough_rqmid_29518_PCIe_LAN_Disable_Control_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(ethernet_bdf, LANDISCTRL, BYTE_4);
	is_pass = (ETH_LAN_DISABLE_CONTROL_REGISTER_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe LAN Disable Control write_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration LAN disable control
 * register of its own Ethernet controller, ACRN hypervisor shall guarantee that
 * the write is ignored.
 */
static void device_passthrough_rqmid_29288_PCIe_LAN_Disable_Control_write_001(void)
{
	uint32_t compare0 = 0;
	uint32_t compare1 = 0;
	uint32_t compare2 = 0;
	bool is_pass = false;
	compare0 = pci_pdev_read_cfg(ethernet_bdf, LANDISCTRL, BYTE_4);
	pci_pdev_write_cfg(ethernet_bdf, LANDISCTRL, BYTE_4, (compare0 ^ 0x0000FFFFU));
	compare1 = pci_pdev_read_cfg(ethernet_bdf, LANDISCTRL, BYTE_4);
	pci_pdev_write_cfg(ethernet_bdf, LANDISCTRL, BYTE_4, (compare0 ^ 0xFFFF0000U));
	compare2 = pci_pdev_read_cfg(ethernet_bdf, LANDISCTRL, BYTE_4);
	is_pass = ((compare0 == compare1) && (compare1 == compare2)) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe XHCI USB3 Overcurrent Pin Mapping 7_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB3
 * overcurrent pin mapping 7 register of its own USB controller,
 * ACRN hypervisor shall guarantee that the vCPU gets 00000000H.
 */
static void device_passthrough_rqmid_29315_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_7_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U3OCM(8), BYTE_4);
	is_pass = (XHCI_USB3_PIN_MAPPING7_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB2 Overcurrent Pin Mapping 7_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI
 * USB2 overcurrent pin mapping 7 register of its own USB controller,
 * ACRN hypervisor shall guarantee that the vCPU gets 00000000H.
 */
static void device_passthrough_rqmid_29322_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_7_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U2OCM(8), BYTE_4);
	is_pass = (XHCI_USB2_PIN_MAPPING7_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB2 Overcurrent Pin Mapping 3_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB2
 * overcurrent pin mapping 3 register of its own USB controller,
 * ACRN hypervisor shall guarantee that the vCPU gets 00000060H.
 */
static void device_passthrough_rqmid_29325_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_3_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U2OCM(4), BYTE_4);
	is_pass = (XHCI_USB2_PIN_MAPPING3_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB2 Overcurrent Pin Mapping 4_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB2
 * overcurrent pin mapping 4 register of its own USB controller,
 * ACRN hypervisor shall guarantee that the vCPU gets 00000000H.
 */
static void device_passthrough_rqmid_29352_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_4_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U2OCM(5), BYTE_4);
	is_pass = (XHCI_USB2_PIN_MAPPING4_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB2 Overcurrent Pin Mapping write_001
 *
 * Summary:When a vCPU attempts to write a guest PCI configuration XHCI USB2
 * overcurrent pin mapping register of its own USB controller,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_29314_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_write_001(void)
{
	uint32_t compare0 = 0;
	uint32_t compare1 = 0;
	uint32_t compare2 = 0;
	bool is_pass = false;
	int i = 0;
	int count = 0;
	for (i = U2OCM(8); i >= U2OCM(1); i -= 4) {
		compare0 = pci_pdev_read_cfg(usb_bdf, i, BYTE_4);
		pci_pdev_write_cfg(usb_bdf, i, BYTE_4, (compare0 ^ 0x0000FFFFU));
		compare1 = pci_pdev_read_cfg(usb_bdf, i, BYTE_4);
		pci_pdev_write_cfg(usb_bdf, i, BYTE_4, (compare0 ^ 0xFFFF0000U));
		compare2 = pci_pdev_read_cfg(usb_bdf, i, BYTE_4);
		if ((compare0 == compare1) && (compare1 == compare2)) {
			count++;
		}
	}
	is_pass = (count == 8) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB2 Overcurrent Pin Mapping 1_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB2
 * overcurrent pin mapping 1 register of its own USB controller,
 * ACRN hypervisor shall guarantee that the vCPU gets 00000018H.
 */
static void device_passthrough_rqmid_29327_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_1_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U2OCM(2), BYTE_4);
	is_pass = (XHCI_USB2_PIN_MAPPING1_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB2 Overcurrent Pin Mapping 6_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB2
 * overcurrent pin mapping 6 register of its own USB controller,
 * ACRN hypervisor shall guarantee that the vCPU gets 00000000H.
 */
static void device_passthrough_rqmid_29323_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_6_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U2OCM(7), BYTE_4);
	is_pass = (XHCI_USB2_PIN_MAPPING6_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB2 Overcurrent Pin Mapping 5_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB2
 * overcurrent pin mapping 5 register of its own USB controller,
 * ACRN hypervisor shall guarantee that the vCPU gets 00000000H.
 */
static void device_passthrough_rqmid_29324_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_5_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U2OCM(6), BYTE_4);
	is_pass = (XHCI_USB2_PIN_MAPPING5_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB2 Overcurrent Pin Mapping 0_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB2
 * overcurrent pin mapping 0 register of its own USB controller,
 * ACRN hypervisor shall guarantee that the vCPU gets 00000006H.
 */
static void device_passthrough_rqmid_29354_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_0_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U2OCM(1), BYTE_4);
	is_pass = (XHCI_USB2_PIN_MAPPING0_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB2 Overcurrent Pin Mapping 2_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB2
 * overcurrent pin mapping 2 register of its own USB controller,
 * ACRN hypervisor shall guarantee that the vCPU gets 00000001H.
 */
static void device_passthrough_rqmid_29326_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_2_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U2OCM(3), BYTE_4);
	is_pass = (XHCI_USB2_PIN_MAPPING2_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe High Speed Configuration 2 write_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration high speed
 * configuration 2 register of its own USB controller, ACRN hypervisor shall
 * guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_29328_PCIe_High_Speed_Configuration_2_write_001(void)
{
	uint32_t compare0 = 0;
	uint32_t compare1 = 0;
	uint32_t compare2 = 0;
	bool is_pass = false;
	compare0 = pci_pdev_read_cfg(usb_bdf, HSCFG2, BYTE_4);
	pci_pdev_write_cfg(usb_bdf, HSCFG2, BYTE_4, (compare0 ^ 0x0000FFFFU));
	compare1 = pci_pdev_read_cfg(usb_bdf, HSCFG2, BYTE_4);
	pci_pdev_write_cfg(usb_bdf, HSCFG2, BYTE_4, (compare0 ^ 0xFFFF0000U));
	compare2 = pci_pdev_read_cfg(usb_bdf, HSCFG2, BYTE_4);
	is_pass = ((compare0 == compare1) && (compare1 == compare2)) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe High Speed Configuration 2_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration
 * high speed configuration 2 register of its own USB controller,
 * ACRN hypervisor shall guarantee that the vCPU gets 00001800H.
 */
static void device_passthrough_rqmid_29329_PCIe_High_Speed_Configuration_2_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, HSCFG2, BYTE_4);
	is_pass = (USB_HIGH_SPEED_CONFIGURATION2_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Power Control Enable_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration power
 * control enable register of its own USB controller, ACRN hypervisor
 * shall guarantee that the vCPU gets 00H.
 */
static void device_passthrough_rqmid_29331_PCIe_Power_Control_Enable_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, PCE_REG, BYTE_2);
	is_pass = (value == USB_POWER_CONTROL_ENABLE_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Power Control Enable write_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration
 * power control enable register of its own USB controller,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_29330_PCIe_Power_Control_Enable_write_001(void)
{
	uint32_t compare0 = 0;
	uint32_t compare1 = 0;
	uint32_t compare2 = 0;
	bool is_pass = false;
	compare0 = pci_pdev_read_cfg(usb_bdf, PCE_REG, BYTE_2);
	pci_pdev_write_cfg(usb_bdf, PCE_REG, BYTE_2, (compare0 ^ 0x00FFU));
	compare1 = pci_pdev_read_cfg(usb_bdf, PCE_REG, BYTE_2);
	pci_pdev_write_cfg(usb_bdf, PCE_REG, BYTE_2, (compare0 ^ 0xFF00U));
	compare2 = pci_pdev_read_cfg(usb_bdf, PCE_REG, BYTE_2);
	is_pass = ((compare0 == compare1) && (compare1 == compare2)) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Vendor Specific Header_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration vendor specific header
 * register of its own USB controller, ACRN hypervisor shall guarantee that
 * the vCPU gets 01400010H.
 */
static void device_passthrough_rqmid_29332_PCIe_Vendor_Specific_Header_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, VSHDR, BYTE_4);
	is_pass = (value == USB_VENDOR_SPECIFIC_HEADER_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Best Effort Service Latency_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration best effort service
 * latency register of its own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 00H.
 */
static void device_passthrough_rqmid_29520_PCIe_Best_Effort_Service_Latency_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, BESL, BYTE_1);
	is_pass = (value == USB_BEST_EFFORT_SERVICE_LATENCY_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Best Effort Service Latency write_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration
 * best effort service latency register of its own USB controller,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_29333_PCIe_Best_Effort_Service_Latency_write_001(void)
{
	uint32_t compare0 = 0;
	uint32_t compare1 = 0;
	uint32_t compare2 = 0;
	bool is_pass = false;
	compare0 = pci_pdev_read_cfg(usb_bdf, BESL, BYTE_1);
	pci_pdev_write_cfg(usb_bdf, BESL, BYTE_1, (compare0 ^ 0x0FU));
	compare1 = pci_pdev_read_cfg(usb_bdf, BESL, BYTE_1);
	pci_pdev_write_cfg(usb_bdf, BESL, BYTE_1, (compare0 ^ 0xF0U));
	compare2 = pci_pdev_read_cfg(usb_bdf, BESL, BYTE_1);
	is_pass = ((compare0 == compare1) && (compare1 == compare2)) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Frame Length Adjustment_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration frame length
 * adjustment register of its own USB controller, ACRN hypervisor shall guarantee
 * that the vCPU gets 60H.
 */
static void device_passthrough_rqmid_29521_PCIe_Frame_Length_Adjustment_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, FLADJ, BYTE_1);
	is_pass = (USB_FRAME_LENGTH_ADJUST_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Serial Bus Release Number_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration serial bus
 * release number register of its own USB controller, ACRN hypervisor shall
 * guarantee that the vCPU gets 30H.
 */
static void device_passthrough_rqmid_29522_PCIe_Serial_Bus_Release_Number_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, SBRN, BYTE_1);
	is_pass = (USB_SERIAL_BUS_NUMBER_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Audio Time Sync_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration audio time
 * synchronization register of its own USB controller, ACRN hypervisor shall
 * guarantee that the vCPU gets 00H.
 */
static void device_passthrough_rqmid_29523_PCIe_Audio_Time_Sync_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, AUDSYNC, BYTE_4);
	is_pass = (USB_AUDIO_TIME_SYNC_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Clock Gating_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration clock gating register
 * of its own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 0FCE6E5BH.
 */
static void device_passthrough_rqmid_29524_PCIe_Clock_Gating_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, XHCLKGETN, BYTE_4);
	is_pass = (USB_CLOCK_GATING_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Clock Gating write_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration clock gating register
 * of its own USB controller, ACRN hypervisor shall guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_29334_PCIe_Clock_Gating_write_001(void)
{
	uint32_t compare0 = 0;
	uint32_t compare1 = 0;
	uint32_t compare2 = 0;
	bool is_pass = false;
	compare0 = pci_pdev_read_cfg(usb_bdf, XHCLKGETN, BYTE_4);
	pci_pdev_write_cfg(usb_bdf, XHCLKGETN, BYTE_4, (compare0 ^ 0x0000FFFF));
	compare1 = pci_pdev_read_cfg(usb_bdf, XHCLKGETN, BYTE_4);
	pci_pdev_write_cfg(usb_bdf, XHCLKGETN, BYTE_4, (compare0 ^ 0xFFFF0000));
	compare2 = pci_pdev_read_cfg(usb_bdf, XHCLKGETN, BYTE_4);
	is_pass = ((compare0 == compare1) && (compare1 == compare2)) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name:PCIe XHCC2_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHC system
 * configuration 2 register of its own USB controller, ACRN hypervisor shall
 * guarantee that the vCPU gets 800FC688H.
 */
static void device_passthrough_rqmid_29525_PCIe_XHCC2_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, XHCC2, BYTE_4);
	is_pass = (value == USB_XHC_SYSTEM_CONFIG_2_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCC2 write_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration XHC system configuration 2 register
 * of its own USB controller, ACRN hypervisor shall guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_29335_PCIe_XHCC2_write_001(void)
{
	uint32_t compare0 = 0;
	uint32_t compare1 = 0;
	uint32_t compare2 = 0;
	bool is_pass = false;

	compare0 = pci_pdev_read_cfg(usb_bdf, XHCC2, BYTE_4);
	pci_pdev_write_cfg(usb_bdf, XHCC2, BYTE_4, (compare0 ^ 0x0000FFFFU));
	compare1 = pci_pdev_read_cfg(usb_bdf, XHCC2, BYTE_4);
	pci_pdev_write_cfg(usb_bdf, XHCC2, BYTE_4, (compare0 ^ 0xFFFF0000U));
	compare2 = pci_pdev_read_cfg(usb_bdf, XHCC2, BYTE_4);
	is_pass = ((compare0 == compare1) && (compare1 == compare2)) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCC1_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHC system configuration 1 register
 * of its own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 803401FDH.
 */
static void device_passthrough_rqmid_29526_PCIe_XHCC1_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, XHCC1, BYTE_4);
	is_pass = (USB_XHC_SYSTEM_CONFIG_1_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCC1 write_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration XHC system configuration 1 register
 * of its own USB controller, ACRN hypervisor shall guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_29336_PCIe_XHCC1_write_001(void)
{
	uint32_t compare0 = 0;
	uint32_t compare1 = 0;
	uint32_t compare2 = 0;
	bool is_pass = false;
	compare0 = pci_pdev_read_cfg(usb_bdf, XHCC1, BYTE_4);
	pci_pdev_write_cfg(usb_bdf, XHCC1, BYTE_4, (compare0 ^ 0x0000FFFFU));
	compare1 = pci_pdev_read_cfg(usb_bdf, XHCC1, BYTE_4);
	pci_pdev_write_cfg(usb_bdf, XHCC1, BYTE_4, (compare0 ^ 0xFFFF0000U));
	compare2 = pci_pdev_read_cfg(usb_bdf, XHCC1, BYTE_4);
	is_pass = ((compare0 == compare1) && (compare1 == compare2)) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NATIVE

/*
 * @brief case name:PCIe Ethernet controller message control_001
 *
 * Summary:The value in the message control register [bit 3:1] of the PCI MSI capability structure
 * of the physical PCIe device at BDF 00:1F.6 shall be 0H.
 */
static void device_passthrough_rqmid_29290_PCIe_Ethernet_controller_message_control_001(void)
{
	uint32_t reg_addr = 0U;
	int ret = OK;
	uint32_t reg_value = 0U;
	bool is_pass = false;
	ret = pci_probe_msi_capability(ethernet_bdf_native, &reg_addr);
	if (ret == OK) {
		reg_addr = reg_addr + PCI_MSI_FLAGS;
		reg_value = pci_pdev_read_cfg(ethernet_bdf_native, reg_addr, BYTE_2);
		reg_value &= SHIFT_MASK(3, 1);
		if (reg_value == 0U) {
			is_pass = true;
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Ethernet controller message control_002
 *
 * Summary:The value in the message control register [bit 8:7] of the PCI MSI capability structure
 * of the physical PCIe device at BDF 00:1F.6 shall be 1H.
 */
static void device_passthrough_rqmid_29291_PCIe_Ethernet_controller_message_control_002(void)
{
	uint32_t reg_addr = 0U;
	int ret = OK;
	uint32_t reg_value = 0U;
	bool is_pass = false;
	ret = pci_probe_msi_capability(ethernet_bdf_native, &reg_addr);
	if (ret == OK) {
		reg_addr = reg_addr + PCI_MSI_FLAGS;
		reg_value = pci_pdev_read_cfg(ethernet_bdf_native, reg_addr, BYTE_2);
		if ((reg_value & SHIFT_LEFT(1, 7))) {
			is_pass = true;
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB controller interrupt pin_001
 *
 * Summary:The value in the physical PCI configuration interrupt pin register
 * of the physical PCIe device at BDF 00:14.0 shall be 01H.
 */
static void device_passthrough_rqmid_29339_PCIe_USB_controller_interrupt_pin_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(usb_bdf_native, PCI_INTERRUPT_PIN, BYTE_1);
	is_pass = (reg_value == USB_INTERRUPTE_PIN_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Ethernet controller interrupt pin_001
 *
 * Summary:The value in the physical PCI configuration interrupt pin register
 * of the physical PCIe device at BDF 00:1F.6 shall be 01H.
 */
static void device_passthrough_rqmid_29292_PCIe_Ethernet_controller_interrupt_pin_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(ethernet_bdf_native, PCI_INTERRUPT_PIN, BYTE_1);
	is_pass = (reg_value == ETH_INTERRUPTE_PIN_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe devices_Ethernet controller Header Type_001
 *
 * Summary:The value in the PCI configuration header type register
 * of the physical PCIe device at BDF 00:1F.6 shall be 0H.
 */
static void device_passthrough_rqmid_29268_PCIe_devices_Ethernet_controller_Header_Type_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(ethernet_bdf_native, PCI_HEADER_TYPE, BYTE_1);
	is_pass = (reg_value == ETH_HEADER_TYPE_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe devices_Ethernet controller Class Code_001
 *
 * Summary:The value in the PCI configuration class code register
 * of the physical PCIe device at BDF 00:1F.6 shall be 020000H.
 */
static void device_passthrough_rqmid_29269_PCIe_devices_Ethernet_controller_Class_Code_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(ethernet_bdf_native, PCI_CLASS_REVISION, BYTE_4);
	reg_value = SHIFT_RIGHT(reg_value, 8);
	reg_value &= 0x00FFFFFFU;
	is_pass = (reg_value == ETH_CLASS_CODE_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Ethernet controller Revision ID_001
 *
 * Summary:The value in the PCI configuration revision ID register
 * of the physical PCIe device at BDF 00:1F.6 shall be 21H.
 */
static void device_passthrough_rqmid_29350_PCIe_Ethernet_controller_Revision_ID_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(ethernet_bdf_native, PCI_CLASS_REVISION, BYTE_1);
	is_pass = (ETH_REVISION_ID_VALUE == reg_value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Ethernet controller Device ID_001
 *
 * Summary:The value in the PCI configuration device ID register
 * of the physical PCIe device at BDF 00:1F.6 shall be 156FH.
 */
static void device_passthrough_rqmid_29270_PCIe_Ethernet_controller_Device_ID_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(ethernet_bdf_native, PCI_DEVICE_ID, BYTE_2);
	is_pass = (ETH_DEVICE_ID_VALUE == reg_value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Ethernet controller Vendor ID_001
 *
 * Summary:The value in the PCI configuration vendor ID register
 * of the physical PCIe device at BDF 00:1F.6 shall be 8086H.
 */
static void device_passthrough_rqmid_29271_PCIe_Ethernet_controller_Vendor_ID_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(ethernet_bdf_native, PCI_VENDOR_ID, BYTE_2);
	is_pass = (ETH_VENDOR_ID_VALUE == reg_value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe USB controller assignment_001
 *
 * Summary:ACRN hypervisor shall assign the physical PCIe device at BDF 00:14.0
 * to the non-safety VM. IO Datasheet, Vol.2
 */
static void device_passthrough_rqmid_29293_PCIe_USB_controller_assignment_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(usb_bdf, PCI_VENDOR_ID, BYTE_2);
	is_pass = (USB_VENDOR_ID_VALUE == reg_value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NATIVE
/*
 * @brief case name:PCIe USB controller Header Type_001
 *
 * Summary:The value in the PCI configuration header type register
 * of the physical PCIe device at BDF 00:14.0 shall be 80H. IO Datasheet, Vol.2
 */
static void device_passthrough_rqmid_29294_PCIe_USB_controller_Header_Type_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(usb_bdf_native, PCI_HEADER_TYPE, BYTE_1);
	is_pass = (reg_value == USB_HEADER_TYPE_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB controller Class Code_001
 *
 * Summary:The value in the PCI configuration class code register
 * of the physical PCIe device at BDF 00:14.0 shall be 0C0330H. IO Datasheet, Vol.2
 */
static void device_passthrough_rqmid_29295_PCIe_USB_controller_Class_Code_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(usb_bdf_native, PCI_CLASS_REVISION, BYTE_4);
	reg_value = SHIFT_RIGHT(reg_value, 8);
	reg_value &= 0x00FFFFFFU;
	is_pass = (reg_value == USB_CLASS_CODE_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB controller Revision ID_001
 *
 * Summary:The value in the PCI configuration revision ID register
 * of the physical PCIe device at BDF 00:14.0 shall be 21H. IO Datasheet, Vol.2
 */
static void device_passthrough_rqmid_29296_PCIe_USB_controller_Revision_ID_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(usb_bdf_native, PCI_REVISION_ID, BYTE_1);
	is_pass = (reg_value == USB_REVISION_ID_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB controller Vendor ID_001
 *
 * Summary:The value in the PCI configuration vendor ID register
 * of the physical PCIe device at BDF 00:14.0 shall be 8086H. IO Datasheet, Vol.2
 */
static void device_passthrough_rqmid_29298_PCIe_USB_controller_Vendor_ID_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(usb_bdf_native, PCI_VENDOR_ID, BYTE_2);
	is_pass = (reg_value == USB_VENDOR_ID_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB_controller_Device_ID_001
 *
 * Summary:The value in the PCI configuration device ID register
 * of the physical PCIe device at BDF 00:14.0 shall be 9D2FH. IO Datasheet, Vol.2
 */
static void device_passthrough_rqmid_27935_PCIe_USB_controller_Device_ID_001(void)
{
	uint32_t reg_value = 0U;
	bool is_pass = false;
	reg_value = pci_pdev_read_cfg(usb_bdf_native, PCI_DEVICE_ID, BYTE_2);
	is_pass = (reg_value == USB_DEVICE_ID_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB controller BDF_001
 *
 * Summary:The physical platform shall have an Intel Corporation Sunrise Point-LP USB 3.0
 * xHCI Controller at BDF 00:14.0.
 */
static void device_passthrough_rqmid_29299_PCIe_USB_controller_BDF_001(void)
{
	uint32_t reg_value1 = 0U;
	uint32_t reg_value2 = 0U;
	bool is_pass = false;
	reg_value1 = pci_pdev_read_cfg(usb_bdf_native, PCI_VENDOR_ID, BYTE_2);
	reg_value2 = pci_pdev_read_cfg(usb_bdf_native, PCI_DEVICE_ID, BYTE_2);
	is_pass = ((reg_value1 == USB_VENDOR_ID_VALUE)\
				&& (reg_value2 == USB_DEVICE_ID_VALUE)) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe USB BAR0 startup_001
 *
 * Summary:ACRN hypervisor shall set initial guest PCI configuration base address register 0 (BAR0)
 * of the USB controller to df230000H following start-up.
 */
static void device_passthrough_rqmid_29309_PCIe_USB_BAR0_startup_001(void)
{
	bool is_pass = false;
	uint32_t usb_bar0_value = *(uint32_t *)STARTUP_USB_BAR0_ADDRESS;
	usb_bar0_value &= 0xFFFFFFF0;
	is_pass = (usb_bar0_value == USB_START_UP_BAR0_VALUE);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB BAR1 startup_001
 *
 * Summary:ACRN hypervisor shall set initial guest PCI configuration base address register 1 (BAR1)
 * of the USB controller to 0000 0000H following start-up.
 */
static void device_passthrough_rqmid_29310_PCIe_USB_BAR1_startup_001(void)
{
	bool is_pass = false;
	uint32_t usb_bar1_value = *(uint32_t *)STARTUP_USB_BAR1_ADDRESS;
	is_pass = (usb_bar1_value == USB_START_UP_BAR1_VALUE);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB controller status register following start-up_001
 *
 * Summary:ACRN hypervisor shall set initial guest PCI configuration status register
 * of the USB controller assigned to the non-safety VM to 0290H following start-up.
 */
static void device_passthrough_rqmid_29080_PCIe_USB_controller_status_register_following_start_up_001(void)
{
	bool is_pass = false;
	uint32_t usb_status_value = *(uint32_t *)STARTUP_USB_STATUS_ADDRESS;
	is_pass = (usb_status_value == USB_START_UP_STATUS_VALUE);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB controller status register following INIT_001
 *
 * Summary:ACRN hypervisor shall keep guest PCI configuration status register
 * of the USB controller assigned to the non-safety VM unchanged following INIT.
 */
static void device_passthrough_rqmid_31388_PCIe_USB_controller_status_register_following_INIT_001(void)
{
	bool is_pass = false;
	uint32_t bp_usb_status_value = *(uint32_t *)STARTUP_USB_STATUS_ADDRESS;
	uint32_t ap_usb_status_value = *(uint32_t *)INIT_USB_STATUS_ADDRESS;
	is_pass = (bp_usb_status_value == ap_usb_status_value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe Ethernet controller status register following start-up
 *
 * Summary:ACRN hypervisor shall set initial guest PCI configuration status register
 * of the Ethernet controller assigned to the safety VM to 0010H following start-up.
 */
static void device_passthrough_rqmid_31427_PCIe_Ethernet_controller_status_register_following_start_up(void)
{
	bool is_pass = false;
	uint32_t net_status_value = *(uint32_t *)STARTUP_ETH_STATUS_ADDRESS;
	is_pass = (net_status_value == ETH_START_UP_STATUS_VALUE);
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe device_USB BAR0 and 1 init_002
 *
 * Summary:ACRN hypervisor shall keep initial guest PCI configuration base address
 * register 0-1 (BAR0-1) of the USB controller unchanged following INIT.
 */
static void device_passthrough_rqmid_37941_PCIe_device_USB_BAR0_and_1_init_002(void)
{
	uint32_t value = 0U;
	uint32_t value1 = 0U;
	bool is_pass = false;
	volatile uint32_t unchanged_bar0_value = 0;
	volatile uint32_t unchanged_bar1_value = 0;

	notify_ap_modify_and_read_init_value(37941);

	unchanged_bar0_value = *(volatile uint32_t *)INIT_USB_BAR0_ADDRESS;
	unchanged_bar1_value = *(volatile uint32_t *)INIT_USB_BAR1_ADDRESS;

	value = unchanged_bar0_value & 0xFFFFFFF0;
	value1 = unchanged_bar1_value & 0xFFFFFFF0;

	is_pass = (value == A_NEW_USB_BAR0_VALUE) && (value1 == A_NEW_USB_BAR1_VALUE);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB controller status register following INIT_002
 *
 * Summary:ACRN hypervisor shall keep guest PCI configuration status register
 * of the USB controller assigned to the non-safety VM unchanged following INIT.
 */
static void device_passthrough_rqmid_37942_PCIe_USB_controller_status_register_following_INIT_002(void)
{
	bool is_pass = false;
	uint32_t ap_usb_status_value;
	uint32_t value0 = 0;
	uint32_t value1 = 0;

	notify_ap_modify_and_read_init_value(37942);
	value0 = (usb_status_unchanged_value & SHIFT_LEFT(1, 14));
	ap_usb_status_value = *(uint32_t *)INIT_USB_STATUS_ADDRESS;
	value1 = (ap_usb_status_value & SHIFT_LEFT(1, 14));
	is_pass = (value0 == value1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe Ethernet BAR read-write_001
 *
 * Summary:ACRN hypervisor shall guarantee that the guest PCI configuration base address register 0 (BAR0)
 * of the Ethernet controller is read-write.
 */
static void device_passthrough_rqmid_29273_PCIe_Ethernet_BAR_read_write_001(void)
{	bool is_pass = false;
	is_pass = pci_check_bar_read_write(ethernet_bdf, 0);
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NATIVE
/*
 * @brief case name:PCIe USB BAR read-write_001
 *
 * Summary:The physical platform shall guarantee that the host PCI configuration base address register 0-1 (BAR0-1)
 * of the physical PCIe device at BDF 00:14.0 are read-write.
 */
static void device_passthrough_rqmid_29300_PCIe_USB_BAR_read_write_001(void)
{
	bool is_pass = false;
	int count = 0;
	is_pass = pci_check_bar_read_write(usb_bdf_native, 0);
	if (is_pass) {
		count++;
	}
	is_pass = pci_check_bar_read_write(usb_bdf_native, 1);
	if (is_pass) {
		count++;
	}
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe USB BAR read-write_002
 *
 * Summary:ACRN hypervisor shall guarantee that the guest PCI configuration base address register 0-1 (BAR0-1)
 * of the USB controller are read-write.
 */
static void device_passthrough_rqmid_29302_PCIe_USB_BAR_read_write_002(void)
{
	bool is_pass = false;
	int count = 0;
	is_pass = pci_check_bar_read_write(usb_bdf, 0);
	if (is_pass) {
		count++;
	}
	is_pass = pci_check_bar_read_write(usb_bdf, 1);
	if (is_pass) {
		count++;
	}
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

#endif

#ifdef IN_NATIVE
/*
 * @brief case name:PCIe Ethenet reserve BAR_001
 *
 * Summary:The physical platform shall guarantee that the host PCI configuration base address register 1-5 (BAR1-5)
 * of the physical PCIe device at BDF 00:1F.6 are reserve.
 */
static void device_passthrough_rqmid_29274_PCIe_Ethenet_reserve_BAR_001(void)
{
	bool is_pass = true;
	int i = 0;
	int count = 0;
	for (i = 1; i < 6; i++) {
		is_pass = pci_check_bar_reserve(ethernet_bdf_native, i);
		if (is_pass) {
			count++;
		}
	}
	is_pass = (count == 5) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe Ethenet reserve BAR_002
 *
 * Summary:ACRN hypervisor shall guarantee that the guest PCI configuration base address register 1-5 (BAR1-5)
 * of the Ethernet controller are reserve.
 */
static void device_passthrough_rqmid_29276_PCIe_Ethenet_reserve_BAR_002(void)
{
	bool is_pass = true;
	int i = 0;
	int count = 0;
	for (i = 1; i < 6; i++) {
		is_pass = pci_check_bar_reserve(ethernet_bdf, i);
		if (is_pass) {
			count++;
		}
	}
	is_pass = (count == 5) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe USB reserve BAR_002
 *
 * Summary:ACRN hypervisor shall guarantee that the guest PCI configuration base address register 2-5 (BAR2-5)
 * of the USB controller are reserve.
 */
static void device_passthrough_rqmid_29303_PCIe_USB_reserve_BAR_002(void)
{
	bool is_pass = true;
	int i = 0;
	int count = 0;
	for (i = 2; i < 6; i++) {
		is_pass = pci_check_bar_reserve(usb_bdf, i);
		if (is_pass) {
			count++;
		}
	}
	is_pass = (count == 4) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NATIVE
/*
 * @brief case name:PCIe USB reserve BAR_001
 *
 * Summary:The physical platform shall guarantee that the host PCI configuration base address register 2-5 (BAR2-5)
 * of the physical PCIe device at BDF 00:14.0 are reserve.
 */
static void device_passthrough_rqmid_29301_PCIe_USB_reserve_BAR_001(void)
{
	bool is_pass = true;
	int i = 0;
	int count = 0;
	for (i = 2; i < 6; i++) {
		is_pass = pci_check_bar_reserve(usb_bdf_native, i);
		if (is_pass) {
			count++;
		}
	}
	is_pass = (count == 4) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe Subsystem vendor ID read-only_001
 *
 * Summary:ACRN hypervisor shall guarantee that the guest PCI configuration subsystem vendor ID register
 * of the Ethernet controller or?the USB controller is read-only.
 */
static void device_passthrough_rqmid_29279_PCIe_Subsystem_vendor_ID_read_only_001(void)
{
	bool is_pass = false;
	uint32_t ori_value = 0U;
	uint32_t new_value = 0U;
	ori_value = pci_pdev_read_cfg(ethernet_bdf, PCI_SUBSYSTEM_VENDOR_ID, BYTE_2);
	pci_pdev_write_cfg(ethernet_bdf, PCI_SUBSYSTEM_VENDOR_ID, BYTE_2, (ori_value ^ 0x00FFU));
	new_value = pci_pdev_read_cfg(ethernet_bdf, PCI_SUBSYSTEM_VENDOR_ID, BYTE_2);
	is_pass = (ori_value == new_value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe Subsystem vendor ID read-only_002
 *
 * Summary:ACRN hypervisor shall guarantee that the guest PCI configuration subsystem vendor ID register
 * of the Ethernet controller or?the USB controller is read-only.
 */
static void device_passthrough_rqmid_38003_PCIe_Subsystem_vendor_ID_read_only_002(void)
{
	bool is_pass = false;
	uint32_t ori_value = 0U;
	uint32_t new_value = 0U;
	ori_value = pci_pdev_read_cfg(usb_bdf, PCI_SUBSYSTEM_VENDOR_ID, BYTE_2);
	pci_pdev_write_cfg(usb_bdf, PCI_SUBSYSTEM_VENDOR_ID, BYTE_2, (ori_value ^ 0x00FFU));
	new_value = pci_pdev_read_cfg(usb_bdf, PCI_SUBSYSTEM_VENDOR_ID, BYTE_2);
	is_pass = (ori_value == new_value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe Subsystem ID read-only_001
 *
 * Summary:ACRN hypervisor shall guarantee that the guest PCI configuration subsystem ID register
 * of the Ethernet controller or the USB controller is read-only.
 */
static void device_passthrough_rqmid_29280_PCIe_Subsystem_ID_read_only_001(void)
{
	bool is_pass = false;
	uint32_t ori_value = 0U;
	uint32_t new_value = 0U;
	ori_value = pci_pdev_read_cfg(ethernet_bdf, PCI_SUBSYSTEM_ID, BYTE_2);
	pci_pdev_write_cfg(ethernet_bdf, PCI_SUBSYSTEM_ID, BYTE_2, (ori_value ^ 0x00FFU));
	new_value = pci_pdev_read_cfg(ethernet_bdf, PCI_SUBSYSTEM_ID, BYTE_2);
	is_pass = (ori_value == new_value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe Subsystem ID read-only_002
 *
 * Summary:ACRN hypervisor shall guarantee that the guest PCI configuration subsystem ID register
 * of the Ethernet controller or the USB controller is read-only.
 */
static void device_passthrough_rqmid_38004_PCIe_Subsystem_ID_read_only_002(void)
{
	bool is_pass = false;
	uint32_t ori_value = 0U;
	uint32_t new_value = 0U;
	ori_value = pci_pdev_read_cfg(usb_bdf, PCI_SUBSYSTEM_ID, BYTE_2);
	pci_pdev_write_cfg(usb_bdf, PCI_SUBSYSTEM_ID, BYTE_2, (ori_value ^ 0x00FFU));
	new_value = pci_pdev_read_cfg(usb_bdf, PCI_SUBSYSTEM_ID, BYTE_2);
	is_pass = (ori_value == new_value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB3 Overcurrent Pin Mapping write_001
 *
 * Summary:When a vCPU attempts to write a guest PCI configuration XHCI USB3 overcurrent pin mapping register
 * of its own USB controller, ACRN hypervisor shall guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_29313_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_write_001(void)
{
	bool is_pass = true;
	uint32_t ori_value = 0U;
	uint32_t new_value = 0U;
	int i = 0;
	for (i = U3OCM(8); i >= U3OCM(1); i -= 4) {
		ori_value = pci_pdev_read_cfg(usb_bdf, i, BYTE_4);
		new_value = ori_value ^ 0x1;
		pci_pdev_write_cfg(usb_bdf, i, BYTE_4, new_value);
		new_value = pci_pdev_read_cfg(usb_bdf, i, BYTE_4);
		if (ori_value != new_value) {
			is_pass = false;
			break;
		}
	}
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe USB and Ethernet BAR0 read_001
 *
 * Summary:When a vCPU attempts to read four bytes from a guest PCI configuration base address register 0 (BAR0)
 * of an assigned PCIe device, ACRN hypervisor shall write the current guest base register 0 value to guest EAX.
 */
static void device_passthrough_rqmid_29304_PCIe_USB_and_Ethernet_BAR0_read_001(void)
{
	bool is_pass = false;
	uint32_t value = 0U;
	value = pci_pdev_read_cfg(usb_bdf, PCIR_BAR(0), BYTE_4);
	is_pass = (USB_ORIGINAL_BAR0_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe USB and Ethernet BAR0 read_002
 *
 * Summary:When a vCPU attempts to read four bytes from a guest PCI configuration base address register 0 (BAR0)
 * of an assigned PCIe device, ACRN hypervisor shall write the current guest base register 0 value to guest EAX.
 */
static void device_passthrough_rqmid_38026_PCIe_USB_and_Ethernet_BAR0_read_002(void)
{
	bool is_pass = false;
	uint32_t value = 0U;
	value = pci_pdev_read_cfg(ethernet_bdf, PCIR_BAR(0), BYTE_4);
	is_pass = (ETH_ORIGINAL_BAR0_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe USB BAR1 read_001
 *
 * Summary:When a vCPU attempts to read four bytes from a guest PCI configuration base address register 1 (BAR1)
 * of its own USB controller, ACRN hypervisor shall write the current guest base register 1 value to guest EAX.
 */
static void device_passthrough_rqmid_29305_PCIe_USB_BAR1_read_001(void)
{
	bool is_pass = false;
	uint32_t value = 0U;
	value = pci_pdev_read_cfg(usb_bdf, PCIR_BAR(1), BYTE_4);
	is_pass = (USB_ORIGINAL_BAR1_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB BAR0 write_001
 *
 * Summary:When a vCPU attempts to write guest EAX to four bytes of a guest PCI configuration
 * base address register 0 (BAR0) of its own USB controller, ACRN hypervisor shall write logical
 * and of guest EAX and FFFF0000H to the guest base address register 0.
 */
static void device_passthrough_rqmid_29306_PCIe_USB_BAR0_write_001(void)
{
	bool is_pass = false;
	uint32_t value = 0U;
	pci_pdev_write_cfg(usb_bdf, PCIR_BAR(0), BYTE_4, 0xFFFFFFFFU);
	value = pci_pdev_read_cfg(usb_bdf, PCIR_BAR(0), BYTE_4);
	is_pass = ((USB_BAR0_MASK_VALUE | 0x4) == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB BAR1 write_001
 *
 * Summary:When a vCPU attempts to write guest EAX to four bytes of a guest PCI configuration
 * base address register 1 (BAR1) of its own USB controller, ACRN hypervisor shall
 * write guest EAX to the guest base register 1.
 */
static void device_passthrough_rqmid_29307_PCIe_USB_BAR1_write_001(void)
{
	bool is_pass = false;
	uint32_t value = 0U;
	uint32_t value1 = 0U;
	uint32_t value2 = 0U;
	value = pci_pdev_read_cfg(usb_bdf, PCIR_BAR(1), BYTE_4);
	value1 = value + 0x00100000U;
	pci_pdev_write_cfg(usb_bdf, PCIR_BAR(1), BYTE_4, value1);
	value2 = pci_pdev_read_cfg(usb_bdf, PCIR_BAR(1), BYTE_4);
	is_pass = (value1 == value2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe Ethernet BAR0 write_001
 *
 * Summary:When a vCPU attempts to write guest EAX to four bytes of a guest PCI configuration
 * base address register 0 (BAR0) of its own Ethernet controller, ACRN hypervisor shall write
 * logical and of guest EAX and FFFE0000H to the guest base address register 0.
 */
static void device_passthrough_rqmid_29277_PCIe_Ethernet_BAR0_write_001(void)
{
	bool is_pass = false;
	uint32_t value = 0U;
	pci_pdev_write_cfg(ethernet_bdf, PCIR_BAR(0), BYTE_4, 0xFFFFFFFFU);
	value = pci_pdev_read_cfg(ethernet_bdf, PCIR_BAR(0), BYTE_4);
	is_pass = (ETH_BAR0_MASK_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe USB controller guest BAR read-only bits field_001
 *
 * Summary:ACRN hypervisor shall guarantee that the guest PCI configuration
 * base address register 0 [bit 15:0] of USB controller is read-only.
 */
static void device_passthrough_rqmid_29311_PCIe_USB_controller_guest_BAR_read_only_bits_field_001(void)
{
	bool is_pass = false;
	uint32_t value = 0U;
	uint32_t value0 = 0U;
	value0 = pci_pdev_read_cfg(usb_bdf, PCIR_BAR(0), BYTE_4);
	pci_pdev_write_cfg(usb_bdf, PCIR_BAR(0), BYTE_4, (value0 ^ 0x0000FFFFU));
	value = pci_pdev_read_cfg(usb_bdf, PCIR_BAR(0), BYTE_4);
	value &= USB_BAR0_BIT_15_to_0_MASK_VALUE;
	value0 &= USB_BAR0_BIT_15_to_0_MASK_VALUE;
	is_pass = (value0 == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe Ethernet controller BAR read_001
 *
 * Summary:When a vCPU attempts to read a guest PCI configuration base address register 0 (BAR0) [bit 16:0]
 * of its own Ethernet controller, ACRN hypervisor shall guarantee that the vCPU gets a value
 * in the physical PCI configuration base address register [bit 16:0].
 */
static void device_passthrough_rqmid_29283_PCIe_Ethernet_controller_BAR_read_001(void)
{
	bool is_pass = false;
	uint32_t value = 0U;
	uint32_t value1 = (PHY_ETH_BAR_VALUE & SHIFT_MASK(16, 0));
	value = pci_pdev_read_cfg(ethernet_bdf, PCI_BAR0_OFFSET, PCI_BAR_BYTE);
	value &= SHIFT_MASK(16, 0);
	is_pass = (value == value1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NATIVE
/*
 * @brief case name:PCIe USB controller physical BAR read-only bits field_001
 *
 * Summary:The physical platform shall guarantee that the host PCI configuration base address register 0 [bit 15:0]
 * of its own USB controller is read-only.
 */
static void device_passthrough_rqmid_29312_PCIe_USB_controller_physical_BAR_read_only_bits_field_001(void)
{
	bool is_pass = false;
	uint32_t value = 0U;
	uint32_t value0 = 0U;
	value0 = pci_pdev_read_cfg(usb_bdf_native, PCIR_BAR(0), BYTE_4);
	pci_pdev_write_cfg(usb_bdf_native, PCIR_BAR(0), BYTE_4, (value0 ^ 0xFFFFU));
	value = pci_pdev_read_cfg(usb_bdf_native, PCIR_BAR(0), BYTE_4);
	value &= USB_BAR0_BIT_15_to_0_MASK_VALUE;
	value0 &= USB_BAR0_BIT_15_to_0_MASK_VALUE;
	is_pass = (value0 == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe Ethernet controller BAR write_001
 *
 * Summary:When a vCPU attempts to write a guest PCI configuration base address register 0 (BAR0) [bit 16:0]
 * of its own Ethernet controller, ACRN hypervisor shall guarantee that the write to the bits is ignored.
 */
static void device_passthrough_rqmid_29284_PCIe_Ethernet_controller_BAR_write_001(void)
{
	bool is_pass = false;
	uint32_t value = 0U;
	uint32_t value1 = 0U;
	value = pci_pdev_read_cfg(ethernet_bdf, PCI_BAR0_OFFSET, PCI_BAR_BYTE);
	pci_pdev_write_cfg(ethernet_bdf, PCIR_BAR(0), BYTE_4, (value ^ 0x1FFFFU));
	value1 = pci_pdev_read_cfg(ethernet_bdf, PCIR_BAR(0), BYTE_4);
	value &= SHIFT_MASK(16, 0);
	value1 &= SHIFT_MASK(16, 0);
	is_pass = (value == value1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NATIVE
/*
 * @brief case name:PCIe USB controller BAR_001
 *
 * Summary:The value in the PCI configuration base address register [bit 15:0]
 * of the the physical PCIe device at BDF 00:14.0 shall 4H.
 */
static void device_passthrough_rqmid_29519_PCIe_USB_controller_BAR_001(void)
{
	bool is_pass = false;
	uint32_t value = 0U;
	value = pci_pdev_read_cfg(usb_bdf_native, PCI_BAR0_OFFSET, PCI_BAR_BYTE);
	value &= SHIFT_MASK(15, 0);
	is_pass = (USB_BAR0_BIT_15_to_0_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe Ethernet controller BAR_001
 *
 * Summary:The value in the PCI configuration base address register [bit 16:0]
 * of the the physical PCIe device at BDF 00:1F.6 shall 0H.
 */
static void device_passthrough_rqmid_29285_PCIe_Ethernet_controller_BAR_001(void)
{
	bool is_pass = false;
	uint32_t value = 0U;
	value = pci_pdev_read_cfg(ethernet_bdf_native, PCI_BAR0_OFFSET, PCI_BAR_BYTE);
	value &= SHIFT_MASK(16, 0);
	is_pass = (0U == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe XHCI USB3 Overcurrent Pin Mapping 6_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB3 overcurrent pin mapping 6 register
 * of its own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 00000000H.
 */
static void device_passthrough_rqmid_29316_XHCI_PCIe_USB3_Overcurrent_Pin_Mapping_6_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U3OCM(7), BYTE_4);
	is_pass = (XHCI_USB3_PIN_MAPPING6_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB3 Overcurrent Pin Mapping 5_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB3 overcurrent pin mapping 5 register
 * of its own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 00000000H.
 */
static void device_passthrough_rqmid_29351_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_5_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U3OCM(6), BYTE_4);
	is_pass = (XHCI_USB3_PIN_MAPPING5_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB3 Overcurrent Pin Mapping 4_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB3 overcurrent pin mapping 4 register
 * of its own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 00000000H.
 */
static void device_passthrough_rqmid_29317_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_4_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U3OCM(5), BYTE_4);
	is_pass = (XHCI_USB3_PIN_MAPPING4_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB3 Overcurrent Pin Mapping 3_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB3 overcurrent pin mapping 3 register
 * of its own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 00000000H.
 */
static void device_passthrough_rqmid_29318_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_3_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U3OCM(4), BYTE_4);
	is_pass = (XHCI_USB3_PIN_MAPPING3_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB3 Overcurrent Pin Mapping 2_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB3 overcurrent pin mapping 2 register
 * of its own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 00000001H.
 */
static void device_passthrough_rqmid_29319_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_2_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U3OCM(3), BYTE_4);
	is_pass = (XHCI_USB3_PIN_MAPPING2_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB3 Overcurrent Pin Mapping 1_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB3 overcurrent pin mapping 1 register
 * of its own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 00000018H.
 */
static void device_passthrough_rqmid_29320_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_1_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U3OCM(2), BYTE_4);
	is_pass = (XHCI_USB3_PIN_MAPPING1_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe XHCI USB3 Overcurrent Pin Mapping 0_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration XHCI USB3 overcurrent pin mapping 0 register
 * of its own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 00000006H.
 */
static void device_passthrough_rqmid_29321_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_0_001(void)
{
	uint32_t value = 0;
	bool is_pass = false;
	value = pci_pdev_read_cfg(usb_bdf, U3OCM(1), BYTE_4);
	is_pass = (XHCI_USB3_PIN_MAPPING0_VALUE == value) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB frame write length_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration frame length adjustment register
 * of its own USB controller, ACRN hypervisor shall guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_38083_PCIe_USB_frame_length_write_001(void)
{
	uint32_t value = 0;
	uint32_t value1 = 0;
	bool is_pass = false;
	/*1.read 0x61U as Value(A)*/
	value = pci_pdev_read_cfg(usb_bdf, FLADJ, BYTE_1);
	/*2.write Value(A) ^ 0x00000001 to 0x61U*/
	pci_pdev_write_cfg(usb_bdf, FLADJ, BYTE_1, (value ^ 0x1U));
	/*3.read 0x61U as Value(B)*/
	value1 = pci_pdev_read_cfg(usb_bdf, FLADJ, BYTE_1);
	/*4.Compare Value(A) and Value(B)*/
	is_pass = (value == value1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB vendor specific header register write_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration vendor specific header register
 * of its own USB controller, ACRN hypervisor shall guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_38085_PCIe_USB_vendor_specific_header_register_write_001(void)
{
	uint32_t value = 0;
	uint32_t value1 = 0;
	bool is_pass = false;
	/*1.read 0x94U as Value(A)*/
	value = pci_pdev_read_cfg(usb_bdf, VSHDR, BYTE_4);
	/*2.write Value(A) ^ 0x00000001 to 0x94U*/
	pci_pdev_write_cfg(usb_bdf, VSHDR, BYTE_4, (value ^ 0x1U));
	/*3.read 0x94U as Value(B)*/
	value1 = pci_pdev_read_cfg(usb_bdf, VSHDR, BYTE_4);
	/*4.Compare Value(A) and Value(B)*/
	is_pass = (value == value1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB serial bus release number register write_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration serial bus release number register
 * of its own USB controller, ACRN hypervisor shall guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_38086_PCIe_USB_serial_bus_release_number_register_write_001(void)
{
	uint32_t value = 0;
	uint32_t value1 = 0;
	bool is_pass = false;
	/*1.read 0x60U as Value(A)*/
	value = pci_pdev_read_cfg(usb_bdf, SBRN, BYTE_1);
	/*2.write Value(A) ^ 0x00000001 to 0x60U*/
	pci_pdev_write_cfg(usb_bdf, SBRN, BYTE_1, (value ^ 0x1U));
	/*3.read 0x60U as Value(B)*/
	value1 = pci_pdev_read_cfg(usb_bdf, SBRN, BYTE_1);
	/*4.Compare Value(A) and Value(B)*/
	is_pass = (value == value1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe USB audio time synchronization register write_001
 *
 * Summary:When a vCPU attempts to write guest PCI configuration audio time synchronization register
 * of its own USB controller, ACRN hypervisor shall guarantee that the write is ignored.
 */
static void device_passthrough_rqmid_38087_PCIe_USB_audio_time_synchronization_register_write_001(void)
{
	uint32_t value = 0;
	uint32_t value1 = 0;
	bool is_pass = false;
	/*1.read 0x58U as Value(A)*/
	value = pci_pdev_read_cfg(usb_bdf, AUDSYNC, BYTE_4);
	/*2.write Value(A) ^ 0x00000001 to 0x58U*/
	pci_pdev_write_cfg(usb_bdf, AUDSYNC, BYTE_4, (value ^ 0x1U));
	/*3.read 0x58U as Value(B)*/
	value1 = pci_pdev_read_cfg(usb_bdf, AUDSYNC, BYTE_4);
	/*4.Compare Value(A) and Value(B)*/
	is_pass = (value == value1) ? true : false;
	report("%s", is_pass, __FUNCTION__);

}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe devices ethernet controller bdf_001
 *
 * Summary:ACRN hypervisor shall guarantee the BDF of guest Ethernet controller is 00:1.0.
 */
static void device_passthrough_rqmid_37197_PCIe_devices_ethernet_controller_bdf_001(void)
{
	uint32_t reg_value1 = 0U;
	uint32_t reg_value2 = 0U;
	bool is_pass = false;
	reg_value1 = pci_pdev_read_cfg(ethernet_bdf, PCI_VENDOR_ID, BYTE_2);
	reg_value2 = pci_pdev_read_cfg(ethernet_bdf, PCI_DEVICE_ID, BYTE_2);
	is_pass = ((reg_value1 == ETH_VENDOR_ID_VALUE)\
				&& (reg_value2 == ETH_DEVICE_ID_VALUE)) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe devices usb controller bdf_001
 *
 * Summary:ACRN hypervisor shall guarantee the BDF of guest USB controller is 00:1.0.
 */
static void device_passthrough_rqmid_37196_PCIe_devices_usb_controller_bdf_001(void)
{
	uint32_t reg_value1 = 0U;
	uint32_t reg_value2 = 0U;
	bool is_pass = false;
	reg_value1 = pci_pdev_read_cfg(usb_bdf, PCI_VENDOR_ID, BYTE_2);
	reg_value2 = pci_pdev_read_cfg(usb_bdf, PCI_DEVICE_ID, BYTE_2);
	is_pass = ((reg_value1 == USB_VENDOR_ID_VALUE)\
				&& (reg_value2 == USB_DEVICE_ID_VALUE)) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe_msi_capability_start_offset_USB_001
 *
 * Summary:ACRN hypervisor shall guarantee that start offset of MSI capability structure
 * in PCI configuration space of the guest USB controller shall be 80H.
 */
static void device_passthrough_rqmid_38107_PCIe_msi_capability_start_offset_USB_001(void)
{
	bool is_pass = false;
	uint32_t reg_value1 = 0U;
	reg_value1 = pci_pdev_read_cfg(usb_bdf, USB_MSI_REG_OFFSET, BYTE_1);
	is_pass = (reg_value1 == MSI_CAPABILITY_ID) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe_msi_capability_start_offset_NET_001
 *
 * Summary:ACRN hypervisor shall guarantee that start offset of MSI capability structure
 * in PCI configuration space of the guest Ethernet controller shall be d0H.
 */
static void device_passthrough_rqmid_38108_PCIe_msi_capability_start_offset_NET_001(void)
{
	bool is_pass = false;
	uint32_t reg_value1 = 0U;
	reg_value1 = pci_pdev_read_cfg(ethernet_bdf, ETH_MSI_REG_OFFSET, BYTE_1);
	is_pass = (reg_value1 == MSI_CAPABILITY_ID) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe_PCI capabilities pointer register_Ethernet_001
 *
 * Summary:When a vCPU attempts to read guest PCI capabilities pointer register
 * of its own Ethernet controller, ACRN hypervisor shall guarantee that the vCPU gets d0H.
 */
static void device_passthrough_rqmid_38111_PCIe_PCI_capabilities_pointer_register_Ethernet_001(void)
{
	bool is_pass = false;
	uint32_t reg_value1 = 0U;
	reg_value1 = pci_pdev_read_cfg(ethernet_bdf, PCI_CAP_OFFSET, BYTE_1);
	is_pass = (reg_value1 == ETH_MSI_REG_OFFSET) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe_PCI capabilities pointer register_USB_001
 *
 * Summary:When a vCPU attempts to read guest PCI capabilities pointer register
 * of its own USB controller, ACRN hypervisor shall guarantee that the vCPU gets 80H.
 */
static void device_passthrough_rqmid_38109_PCIe_PCI_capabilities_pointer_register_USB_001(void)
{
	bool is_pass = false;
	uint32_t reg_value1 = 0U;
	reg_value1 = pci_pdev_read_cfg(usb_bdf, PCI_CAP_OFFSET, BYTE_1);
	is_pass = (reg_value1 == USB_MSI_REG_OFFSET) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

#ifdef IN_SAFETY_VM
/*
 * @brief case name:PCIe Non-trapped access to assigned devices_001
 *
 * Summary:When a vCPU accesses address regions mapped by a guest configuration base address register
 * of a PCIe device assigned to it, ACRN hypervisor shall guarantee that no VM exit is triggered.
 */
static __unused void device_passthrough_rqmid_38140_PCIe_Non_trapped_access_to_assigned_devices_001(void)
{
	bool is_pass = false;
	mem_size base_addr = 0U;
	uint32_t reg_value0 = 0U;
	base_addr = pci_pdev_read_cfg(ethernet_bdf, PCIR_BAR(0), BYTE_4);
	base_addr = SHIFT_MASK(31, 4) & base_addr;
	base_addr = (mem_size)phys_to_virt(base_addr + GBECSR_5B54);
	reg_value0 = pci_pdev_read_mem(ethernet_bdf, base_addr, BYTE_4);
	reg_value0 &= SHIFT_LEFT(1, 15);
	is_pass = (reg_value0 == SHIFT_LEFT(1, 15)) ? true : false;
	report("%s", is_pass, __FUNCTION__);

}
#endif

#ifdef IN_NON_SAFETY_VM
/*
 * @brief case name:PCIe Non-trapped access to assigned devices_002
 *
 * Summary:When a vCPU accesses address regions mapped by a guest configuration base address register
 * of a PCIe device assigned to it, ACRN hypervisor shall guarantee that no VM exit is triggered.
 */
static __unused void device_passthrough_rqmid_38149_PCIe_Non_trapped_access_to_assigned_devices_002(void)
{
	bool is_pass = false;
	mem_size base_addr = 0U;
	uint32_t reg_value0 = 0U;
	base_addr = pci_pdev_read_cfg(usb_bdf, PCIR_BAR(0), BYTE_4);
	base_addr = SHIFT_MASK(31, 4) & base_addr;
	base_addr = (mem_size)phys_to_virt(base_addr + HCIVERSION);
	reg_value0 = pci_pdev_read_mem(usb_bdf, base_addr, BYTE_2);
	is_pass = (reg_value0 == USB_HCIVERSION_VALUE) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}
#endif

static void print_case_list(void)
{
	printf("Device passthrough feature case list:\n\r");
/****************************sample cases start***********************************/
#ifdef IN_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29267,
	"PCIe device_Ethernet controller assignment_001");
	printf("\t Case ID:%d case name:%s\n\r", 29278,
	"PCIe device_Ethernet BAR0 init_001");
	printf("\t Case ID:%d case name:%s\n\r", 29287,
	"PCIe device_Lock LAN Disable write_001");
#endif

#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29308,
	"PCIe device_USB BAR0 and 1 init_001");
#endif

#ifdef IN_NATIVE
	printf("\t Case ID:%d case name:%s\n\r", 29272,
	"PCIe device_Ethernet controller BDF_001");
	printf("\t Case ID:%d case name:%s\n\r", 29275,
	"PCIe device_Ethernet controller BDF_001");
	printf("\t Case ID:%d case name:%s\n\r", 29338,
	"PCIe device_USB controller message control_002");
#endif
/****************************sample cases end*************************************/

/****************************scaling cases start***********************************/
//621:Device Application constraint start
#ifdef IN_NATIVE
	printf("\t Case ID:%d case name:%s\n\r", 29312,
	"PCIe USB controller physical BAR read-only bits field_001");
	printf("\t Case ID:%d case name:%s\n\r", 29519,
	"PCIe USB controller BAR_001");
	printf("\t Case ID:%d case name:%s\n\r", 29285,
	"PCIe Ethernet controller BAR_001");
	printf("\t Case ID:%d case name:%s\n\r", 29290,
	"PCIe Ethernet controller message control_001");
	printf("\t Case ID:%d case name:%s\n\r", 29291,
	"PCIe Ethernet controller message control_002");
	printf("\t Case ID:%d case name:%s\n\r", 29339,
	"PCIe USB controller interrupt pin_001");
	printf("\t Case ID:%d case name:%s\n\r", 29292,
	"PCIe Ethernet controller interrupt pin_001");
	printf("\t Case ID:%d case name:%s\n\r", 29268,
	"PCIe devices_Ethernet controller Header Type_001");
	printf("\t Case ID:%d case name:%s\n\r", 29269,
	"PCIe devices_Ethernet controller Class Code_001");
	printf("\t Case ID:%d case name:%s\n\r", 29350,
	"PCIe Ethernet controller Revision ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 29270,
	"PCIe Ethernet controller Device ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 29271,
	"PCIe Ethernet controller Vendor ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 29294,
	"PCIe USB controller Header Type_001");
	printf("\t Case ID:%d case name:%s\n\r", 29295,
	"PCIe USB controller Class Code_001");
	printf("\t Case ID:%d case name:%s\n\r", 29296,
	"PCIe USB controller Revision ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 29298,
	"PCIe USB controller Vendor ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 27935,
	"PCIe USB_controller_Device_ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 29299,
	"PCIe USB controller BDF_001");
	printf("\t Case ID:%d case name:%s\n\r", 29300,
	"PCIe USB BAR read-write_001");
	printf("\t Case ID:%d case name:%s\n\r", 29274,
	"PCIe Ethenet reserve BAR_001");
	printf("\t Case ID:%d case name:%s\n\r", 29301,
	"PCIe USB reserve BAR_001");
#endif
//621:Device Application constraint end


//519:Device pass-through_BAR test start
#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29304,
	"PCIe USB and Ethernet BAR0 read_001");
	printf("\t Case ID:%d case name:%s\n\r", 29305,
	"PCIe USB BAR1 read_001");
	printf("\t Case ID:%d case name:%s\n\r", 29306,
	"PCIe USB BAR0 write_001");
	printf("\t Case ID:%d case name:%s\n\r", 29307,
	"PCIe USB BAR1 write_001");
	printf("\t Case ID:%d case name:%s\n\r", 29311,
	"PCIe USB controller guest BAR read-only bits field_001");
#endif

#ifdef IN_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 38026,
	"PCIe USB and Ethernet BAR0 read_002");
	printf("\t Case ID:%d case name:%s\n\r", 29277,
	"PCIe Ethernet BAR0 write_001");
	printf("\t Case ID:%d case name:%s\n\r", 29283,
	"PCIe Ethernet controller BAR read_001");
	printf("\t Case ID:%d case name:%s\n\r", 29284,
	"PCIe Ethernet controller BAR write_001");
#endif
//519:Device pass-through_BAR test end

//521:Device pass-through_ACRN shall guarantee that the vCPU gets fixed value start
#ifdef IN_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29281,
	"PCIe Ethernet controller subsystem ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 29282,
	"PCIe Ethernet controller subsystem vendor ID_001");
	printf("\t Case ID:%d case name:%s\n\r", 29514,
	"PCIe System Time Control High Register_001");
	printf("\t Case ID:%d case name:%s\n\r", 29289,
	"PCIe Lock LAN Disable_001");
	printf("\t Case ID:%d case name:%s\n\r", 29518,
	"PCIe LAN Disable Control_001");
	printf("\t Case ID:%d case name:%s\n\r", 37197,
	"PCIe devices ethernet controller bdf_001");
	printf("\t Case ID:%d case name:%s\n\r", 38108,
	"PCIe_msi_capability_start_offset_NET_001");
	printf("\t Case ID:%d case name:%s\n\r", 38111,
	"PCIe_PCI capabilities pointer register_Ethernet_001");
#endif

#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29315,
	"PCIe XHCI USB3 Overcurrent Pin Mapping 7_001");
	printf("\t Case ID:%d case name:%s\n\r", 29316,
	"PCIe XHCI USB3 Overcurrent Pin Mapping 6_001");
	printf("\t Case ID:%d case name:%s\n\r", 29351,
	"PCIe XHCI USB3 Overcurrent Pin Mapping 5_001");
	printf("\t Case ID:%d case name:%s\n\r", 29317,
	"PCIe XHCI USB3 Overcurrent Pin Mapping 4_001");
	printf("\t Case ID:%d case name:%s\n\r", 29318,
	"PCIe XHCI USB3 Overcurrent Pin Mapping 3_001");
	printf("\t Case ID:%d case name:%s\n\r", 29319,
	"PCIe XHCI USB3 Overcurrent Pin Mapping 2_001");
	printf("\t Case ID:%d case name:%s\n\r", 29320,
	"PCIe XHCI USB3 Overcurrent Pin Mapping 1_001");
	printf("\t Case ID:%d case name:%s\n\r", 29321,
	"PCIe XHCI USB3 Overcurrent Pin Mapping 0_001");
	printf("\t Case ID:%d case name:%s\n\r", 29322,
	"PCIe XHCI USB2 Overcurrent Pin Mapping 7_001");
	printf("\t Case ID:%d case name:%s\n\r", 29325,
	"PCIe XHCI USB2 Overcurrent Pin Mapping 3_001");
	printf("\t Case ID:%d case name:%s\n\r", 29352,
	"PCIe XHCI USB2 Overcurrent Pin Mapping 4_001");
	printf("\t Case ID:%d case name:%s\n\r", 29327,
	"PCIe XHCI USB2 Overcurrent Pin Mapping 1_001");
	printf("\t Case ID:%d case name:%s\n\r", 29323,
	"PCIe XHCI USB2 Overcurrent Pin Mapping 6_001");
	printf("\t Case ID:%d case name:%s\n\r", 29324,
	"PCIe XHCI USB2 Overcurrent Pin Mapping 5_001");
	printf("\t Case ID:%d case name:%s\n\r", 29354,
	"PCIe XHCI USB2 Overcurrent Pin Mapping 0_001");
	printf("\t Case ID:%d case name:%s\n\r", 29326,
	"PCIe XHCI USB2 Overcurrent Pin Mapping 2_001");
	printf("\t Case ID:%d case name:%s\n\r", 29329,
	"PCIe High Speed Configuration 2_001");
	printf("\t Case ID:%d case name:%s\n\r", 29331,
	"PCIe Power Control Enable_001");
	printf("\t Case ID:%d case name:%s\n\r", 29332,
	"PCIe Vendor Specific Header_001");
	printf("\t Case ID:%d case name:%s\n\r", 29520,
	"PCIe Best Effort Service Latency_001");
	printf("\t Case ID:%d case name:%s\n\r", 29521,
	"PCIe Frame Length Adjustment_001");
	printf("\t Case ID:%d case name:%s\n\r", 29522,
	"PCIe Serial Bus Release Number_001");
	printf("\t Case ID:%d case name:%s\n\r", 29523,
	"PCIe Audio Time Sync_001");
	printf("\t Case ID:%d case name:%s\n\r", 29524,
	"PCIe Clock Gating_001");
	printf("\t Case ID:%d case name:%s\n\r", 29525,
	"PCIe XHCC2_001");
	printf("\t Case ID:%d case name:%s\n\r", 29526,
	"PCIe XHCC1_001");
	printf("\t Case ID:%d case name:%s\n\r", 29293,
	"PCIe USB controller assignment_001");
	printf("\t Case ID:%d case name:%s\n\r", 37196,
	"PCIe devices usb controller bdf_001");
	printf("\t Case ID:%d case name:%s\n\r", 38107,
	"PCIe_msi_capability_start_offset_USB_001");
	printf("\t Case ID:%d case name:%s\n\r", 38109,
	"PCIe_PCI capabilities pointer register_USB_001");
#endif
//521:Device pass-through_ACRN shall guarantee that the vCPU gets fixed value end

//520:Device pass-through_start-up and init start
#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29309,
	"PCIe USB BAR0 startup_001");
	printf("\t Case ID:%d case name:%s\n\r", 29310,
	"PCIe USB BAR1 startup_001");
	printf("\t Case ID:%d case name:%s\n\r", 29080,
	"PCIe USB controller status register following start-up_001");
	printf("\t Case ID:%d case name:%s\n\r", 31388,
	"PCIe USB controller status register following INIT_001");
	printf("\t Case ID:%d case name:%s\n\r", 37941,
	"PCIe device_USB BAR0 and 1 init_002");
	printf("\t Case ID:%d case name:%s\n\r", 37942,
	"PCIe USB controller status register following INIT_002");
#endif

#ifdef IN_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 31427,
	"PCIe Ethernet controller status register following start-up");
#endif
//520:Device pass-through_start-up and init end


//518:Device Pass-through_register set and dump start
#ifdef IN_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29273,
	"PCIe Ethernet BAR read-write_001");
	printf("\t Case ID:%d case name:%s\n\r", 29286,
	"PCIe System Time Control High Register write_001");
	printf("\t Case ID:%d case name:%s\n\r", 29288,
	"PCIe LAN Disable Control write_001");
#endif

#ifdef IN_NON_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29302,
	"PCIe USB BAR read-write_002");
	printf("\t Case ID:%d case name:%s\n\r", 29303,
	"PCIe USB reserve BAR_002");
	printf("\t Case ID:%d case name:%s\n\r", 38003,
	"PCIe Subsystem vendor ID read-only_002");
	printf("\t Case ID:%d case name:%s\n\r", 38004,
	"PCIe Subsystem ID read-only_002");
	printf("\t Case ID:%d case name:%s\n\r", 29313,
	"PCIe XHCI USB3 Overcurrent Pin Mapping write_001");
	printf("\t Case ID:%d case name:%s\n\r", 29314,
	"PCIe XHCI USB2 Overcurrent Pin Mapping write_001");
	printf("\t Case ID:%d case name:%s\n\r", 38083,
	"PCIe USB frame write length_001");
	printf("\t Case ID:%d case name:%s\n\r", 29328,
	"PCIe High Speed Configuration 2 write_001");
	printf("\t Case ID:%d case name:%s\n\r", 29330,
	"PCIe Power Control Enable write_001");
	printf("\t Case ID:%d case name:%s\n\r", 29333,
	"PCIe Best Effort Service Latency write_001");
	printf("\t Case ID:%d case name:%s\n\r", 29334,
	"PCIe Clock Gating write_001");
	printf("\t Case ID:%d case name:%s\n\r", 29335,
	"PCIe XHCC2 write_001");
	printf("\t Case ID:%d case name:%s\n\r", 29336,
	"PCIe XHCC1 write_001");
	printf("\t Case ID:%d case name:%s\n\r", 38085,
	"PCIe USB vendor specific header register write_001");
	printf("\t Case ID:%d case name:%s\n\r", 38086,
	"PCIe USB serial bus release number register write_001");
	printf("\t Case ID:%d case name:%s\n\r", 38087,
	"PCIe USB audio time synchronization register write_001");
#endif

#ifdef IN_SAFETY_VM
	printf("\t Case ID:%d case name:%s\n\r", 29276,
	"PCIe Ethenet reserve BAR_002");
	printf("\t Case ID:%d case name:%s\n\r", 29279,
	"PCIe Subsystem vendor ID read-only_001");
	printf("\t Case ID:%d case name:%s\n\r", 29280,
	"PCIe Subsystem ID read-only_001");
#endif
//518:Device Pass-through_register set and dump end

/****************************scaling cases end*************************************/
}

int main()
{
	setup_idt();
	set_log_level(PASS_THROUGH_DEBUG_LEVEL);

#ifdef __x86_64__
	print_case_list();
/*NOTE:The ethernet device is passed through to safety VM,
 *and the USB device is passed through to non-safey VM.
 */

#ifdef IN_SAFETY_VM
	e1000e_init(ethernet_bdf);
#endif

/****************************sample cases start***********************************/
#ifdef IN_SAFETY_VM
	device_passthrough_rqmid_29267_PCIe_device_Ethernet_controller_assignment_001();
	device_passthrough_rqmid_29278_PCIe_device_Ethernet_BAR0_init_001();
	device_passthrough_rqmid_29287_PCIe_device_Lock_LAN_Disable_write_001();
#endif
#ifdef IN_NON_SAFETY_VM
	device_passthrough_rqmid_29308_PCIe_device_USB_BAR0_and_1_init_001();
#endif
#ifdef IN_NATIVE
	device_passthrough_rqmid_29272_PCIe_device_Ethernet_controller_BDF_001();
	device_passthrough_rqmid_29275_PCIe_device_Ethernet_BAR_read_write_002();
	device_passthrough_rqmid_29338_PCIe_device_USB_controller_message_control_002();
#endif
/****************************sample cases end*************************************/

/****************************scaling cases start***********************************/
//621:Device Application constraint start
#ifdef IN_NATIVE
	device_passthrough_rqmid_29312_PCIe_USB_controller_physical_BAR_read_only_bits_field_001();
	device_passthrough_rqmid_29519_PCIe_USB_controller_BAR_001();
	device_passthrough_rqmid_29285_PCIe_Ethernet_controller_BAR_001();
	device_passthrough_rqmid_29290_PCIe_Ethernet_controller_message_control_001();
	device_passthrough_rqmid_29291_PCIe_Ethernet_controller_message_control_002();
	device_passthrough_rqmid_29339_PCIe_USB_controller_interrupt_pin_001();
	device_passthrough_rqmid_29292_PCIe_Ethernet_controller_interrupt_pin_001();
	device_passthrough_rqmid_29268_PCIe_devices_Ethernet_controller_Header_Type_001();
	device_passthrough_rqmid_29269_PCIe_devices_Ethernet_controller_Class_Code_001();
	device_passthrough_rqmid_29350_PCIe_Ethernet_controller_Revision_ID_001();
	device_passthrough_rqmid_29270_PCIe_Ethernet_controller_Device_ID_001();
	device_passthrough_rqmid_29271_PCIe_Ethernet_controller_Vendor_ID_001();
	device_passthrough_rqmid_29294_PCIe_USB_controller_Header_Type_001();
	device_passthrough_rqmid_29295_PCIe_USB_controller_Class_Code_001();
	device_passthrough_rqmid_29296_PCIe_USB_controller_Revision_ID_001();
	device_passthrough_rqmid_29298_PCIe_USB_controller_Vendor_ID_001();
	device_passthrough_rqmid_27935_PCIe_USB_controller_Device_ID_001();
	device_passthrough_rqmid_29299_PCIe_USB_controller_BDF_001();
	device_passthrough_rqmid_29300_PCIe_USB_BAR_read_write_001();
	device_passthrough_rqmid_29274_PCIe_Ethenet_reserve_BAR_001();
	device_passthrough_rqmid_29301_PCIe_USB_reserve_BAR_001();
#endif
//621:Device Application constraint end

//519:Device pass-through_BAR test start
#ifdef IN_NON_SAFETY_VM
	device_passthrough_rqmid_29304_PCIe_USB_and_Ethernet_BAR0_read_001();
	device_passthrough_rqmid_29305_PCIe_USB_BAR1_read_001();
	device_passthrough_rqmid_29306_PCIe_USB_BAR0_write_001();
	device_passthrough_rqmid_29307_PCIe_USB_BAR1_write_001();
	device_passthrough_rqmid_29311_PCIe_USB_controller_guest_BAR_read_only_bits_field_001();
#endif

#ifdef IN_SAFETY_VM
	device_passthrough_rqmid_38026_PCIe_USB_and_Ethernet_BAR0_read_002();
	device_passthrough_rqmid_29277_PCIe_Ethernet_BAR0_write_001();
	device_passthrough_rqmid_29283_PCIe_Ethernet_controller_BAR_read_001();
	device_passthrough_rqmid_29284_PCIe_Ethernet_controller_BAR_write_001();
#endif
//519:Device pass-through_BAR test end


//521:Device pass-through_ACRN shall guarantee that the vCPU gets fixed value start
#ifdef IN_SAFETY_VM
	device_passthrough_rqmid_29281_PCIe_Ethernet_controller_subsystem_ID_001();
	device_passthrough_rqmid_29282_PCIe_Ethernet_controller_subsystem_vendor_ID_001();
	device_passthrough_rqmid_29514_PCIe_System_Time_Control_High_Register_001();
	device_passthrough_rqmid_29289_PCIe_Lock_LAN_Disable_001();
	device_passthrough_rqmid_29518_PCIe_LAN_Disable_Control_001();
	device_passthrough_rqmid_37197_PCIe_devices_ethernet_controller_bdf_001();
	device_passthrough_rqmid_38108_PCIe_msi_capability_start_offset_NET_001();
	device_passthrough_rqmid_38111_PCIe_PCI_capabilities_pointer_register_Ethernet_001();
#endif

#ifdef IN_NON_SAFETY_VM
	device_passthrough_rqmid_29315_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_7_001();
	device_passthrough_rqmid_29316_XHCI_PCIe_USB3_Overcurrent_Pin_Mapping_6_001();
	device_passthrough_rqmid_29351_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_5_001();
	device_passthrough_rqmid_29317_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_4_001();
	device_passthrough_rqmid_29318_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_3_001();
	device_passthrough_rqmid_29319_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_2_001();
	device_passthrough_rqmid_29320_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_1_001();
	device_passthrough_rqmid_29321_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_0_001();
	device_passthrough_rqmid_29322_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_7_001();
	device_passthrough_rqmid_29325_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_3_001();
	device_passthrough_rqmid_29352_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_4_001();
	device_passthrough_rqmid_29327_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_1_001();
	device_passthrough_rqmid_29323_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_6_001();
	device_passthrough_rqmid_29324_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_5_001();
	device_passthrough_rqmid_29354_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_0_001();
	device_passthrough_rqmid_29326_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_2_001();
	device_passthrough_rqmid_29329_PCIe_High_Speed_Configuration_2_001();
	device_passthrough_rqmid_29331_PCIe_Power_Control_Enable_001();
	device_passthrough_rqmid_29332_PCIe_Vendor_Specific_Header_001();
	device_passthrough_rqmid_29520_PCIe_Best_Effort_Service_Latency_001();
	device_passthrough_rqmid_29521_PCIe_Frame_Length_Adjustment_001();
	device_passthrough_rqmid_29522_PCIe_Serial_Bus_Release_Number_001();
	device_passthrough_rqmid_29523_PCIe_Audio_Time_Sync_001();
	device_passthrough_rqmid_29524_PCIe_Clock_Gating_001();
	device_passthrough_rqmid_29525_PCIe_XHCC2_001();
	device_passthrough_rqmid_29526_PCIe_XHCC1_001();
	device_passthrough_rqmid_29293_PCIe_USB_controller_assignment_001();
	device_passthrough_rqmid_37196_PCIe_devices_usb_controller_bdf_001();
	device_passthrough_rqmid_38107_PCIe_msi_capability_start_offset_USB_001();
	device_passthrough_rqmid_38109_PCIe_PCI_capabilities_pointer_register_USB_001();
#endif
//521:Device pass-through_ACRN shall guarantee that the vCPU gets fixed value end

//520:Device pass-through_start-up and init start
#ifdef IN_NON_SAFETY_VM
	device_passthrough_rqmid_29309_PCIe_USB_BAR0_startup_001();
	device_passthrough_rqmid_29310_PCIe_USB_BAR1_startup_001();
	device_passthrough_rqmid_29080_PCIe_USB_controller_status_register_following_start_up_001();
	device_passthrough_rqmid_31388_PCIe_USB_controller_status_register_following_INIT_001();
	device_passthrough_rqmid_37941_PCIe_device_USB_BAR0_and_1_init_002();
	device_passthrough_rqmid_37942_PCIe_USB_controller_status_register_following_INIT_002();
#endif

#ifdef IN_SAFETY_VM
	device_passthrough_rqmid_31427_PCIe_Ethernet_controller_status_register_following_start_up();
#endif
//520:Device pass-through_start-up and init end

//518:Device Pass-through_register set and dump start
#ifdef IN_SAFETY_VM
	device_passthrough_rqmid_29273_PCIe_Ethernet_BAR_read_write_001();
	device_passthrough_rqmid_29286_PCIe_System_Time_Control_High_Register_write_001();
	device_passthrough_rqmid_29288_PCIe_LAN_Disable_Control_write_001();
#endif

#ifdef IN_NON_SAFETY_VM
	device_passthrough_rqmid_29302_PCIe_USB_BAR_read_write_002();
	device_passthrough_rqmid_29303_PCIe_USB_reserve_BAR_002();
	device_passthrough_rqmid_38003_PCIe_Subsystem_vendor_ID_read_only_002();
	device_passthrough_rqmid_38004_PCIe_Subsystem_ID_read_only_002();
	device_passthrough_rqmid_29313_PCIe_XHCI_USB3_Overcurrent_Pin_Mapping_write_001();
	device_passthrough_rqmid_29314_PCIe_XHCI_USB2_Overcurrent_Pin_Mapping_write_001();
	device_passthrough_rqmid_38083_PCIe_USB_frame_length_write_001();
	device_passthrough_rqmid_29328_PCIe_High_Speed_Configuration_2_write_001();
	device_passthrough_rqmid_29330_PCIe_Power_Control_Enable_write_001();
	device_passthrough_rqmid_29333_PCIe_Best_Effort_Service_Latency_write_001();
	device_passthrough_rqmid_29334_PCIe_Clock_Gating_write_001();
	device_passthrough_rqmid_29335_PCIe_XHCC2_write_001();
	device_passthrough_rqmid_29336_PCIe_XHCC1_write_001();
	device_passthrough_rqmid_38085_PCIe_USB_vendor_specific_header_register_write_001();
	device_passthrough_rqmid_38086_PCIe_USB_serial_bus_release_number_register_write_001();
	device_passthrough_rqmid_38087_PCIe_USB_audio_time_synchronization_register_write_001();
#endif

#ifdef IN_SAFETY_VM
	device_passthrough_rqmid_29276_PCIe_Ethenet_reserve_BAR_002();
	device_passthrough_rqmid_29279_PCIe_Subsystem_vendor_ID_read_only_001();
	device_passthrough_rqmid_29280_PCIe_Subsystem_ID_read_only_001();
#endif
//518:Device Pass-through_register set and dump end



/****************************scaling cases end*************************************/
#endif
	return report_summary();
}
