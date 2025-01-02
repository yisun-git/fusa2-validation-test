#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "pci_util.h"
#include "e1000_regs.h"
#include "e1000e.h"
#include "delay.h"
#include "apic.h"
#include "smp.h"
#include "isr.h"
#include "fwcfg.h"
#include "asm/io.h"
#include "pci_check.h"
#include "pci_native_config.h"
#include "misc.h"

#define GBECSR_5B54      (0x5B54U)
#define USB_MSI_REG_OFFSET	(0x80U)
#define ETH_MSI_REG_OFFSET	(0xd0U)
#define MSI_CAPABILITY_ID	(0x05U)
#define A_NEW_VALUE_UNCHANGE	(0xFFF000U)
#define A_NEW_USB_BAR0_VALUE	(0xDF240000U)
#define A_NEW_NET_BAR0_VALUE	(0xDF260000U)
#define USB_ORIGINAL_BAR0_VALUE	(0xdf230004U)
#define NET_ORIGINAL_BAR0_VALUE	(0xdf200000U)

// #define PCI_DEBUG
#ifdef PCI_DEBUG
#define PCI_DEBUG_LEVEL		DBG_LVL_INFO
#else
#define PCI_DEBUG_LEVEL		DBG_LVL_ERROR
#endif

static uint32_t mmcfg_addr = 0;

typedef struct pci_capability_node_s {
	uint8_t cap_id;
	uint8_t next_ptr;
} pci_capability_node_t;

typedef struct pci_cfg_reg_s {
	const char *reg_name;
	uint16_t reg_addr;
	uint8_t reg_width;
	uint32_t reg_val;
	uint8_t is_shadow; // undefine or reserved register 1_yes, 0_no
} pci_cfg_reg_t;

static struct pci_dev pci_devs[MAX_PCI_DEV_NUM];
static uint32_t nr_pci_devs = MAX_PCI_DEV_NUM;
static uint64_t apic_id_bitmap = 0UL;

#ifdef IN_SAFETY_VM
static pci_cfg_reg_t hostbridge_pci_cfg[] =
{
	{"DeviceVendor ID", 0x00, 4, 0x5af08086, 0},
	{"Command", 0x04, 2, 0x0000, 0},
	{"Status", 0x06, 2, 0x0000, 0},
	{"Revision ID", 0x08, 1, 0x0b, 0},
	{"interface", 0x09, 1, 0x00, 0},
	{"Sub Class Code", 0x0a, 1, 0x00, 0},
	{"Base Class Code", 0x0b, 1, 0x06, 0},
	{"Cache Line Size", 0x0c, 1, 0x00, 1},
	{"Latency Tim", 0x0d, 1, 0x00, 1},
	{"Header Type", 0x0e, 1, 0x00, 0},
	{"BIST", 0x0f, 1, 0x00, 1},
	{"BAR0", 0x10, 4, 0x00000000, 0},
	{"BAR1", 0x14, 4, 0x00000000, 0},
	{"BAR2", 0x18, 4, 0x00000000, 1},
	{"BAR3", 0x1c, 4, 0x00000000, 1},
	{"BAR4", 0x20, 4, 0x00000000, 1},
	{"BAR5", 0x24, 4, 0x00000000, 1},
	{"Cardbus CIS Pointer", 0x28, 4, 0x00000000, 1},
	{"Subsystem Vendor ID", 0x2c, 2, 0x00000, 0},
	{"Subsystem ID", 0x2e, 2, 0x0000, 0},
	{"Expansion ROM Base Address", 0x30, 4, 0x00000000, 1},
	{"Cap Pointer", 0x34, 1, 0x00, 0},
	{"res35", 0x35, 1, 0x00, 1},
	{"res36", 0x36, 1, 0x00, 1},
	{"res37", 0x37, 1, 0x00, 1},
	{"Reserved", 0x38, 4, 0x00000000, 1},
	{"Int Line", 0x3c, 1, 0xFF, 0},
	{"Int Pin", 0x3d, 1, 0x00, 0},
	{"Min_Gnt", 0x3e, 1, 0x00, 1},
	{"Min_Lat", 0x3f, 1, 0x00, 1},
	{"CAP reg", 0x60, 4, 0x00000000, 1},
	{"CAP reg", 0x64, 4, 0x00000000, 1},
};
#endif

// For BAR0 init test case
static PCI_MAKE_BDF(usb, 0x0, 0x1, 0x0);

static uint32_t first_usb_bar0_resume_value = 0U;
static uint32_t first_usb_bar0_value = 0U;
static uint32_t first_usb_new_bar0_value = 0U;
// For 0xCF8 port init test case
static uint32_t unchange_resume_value = 0U;
// For MSI control flag init test case
static uint32_t first_usb_msi_ctrl = 0U;
static uint32_t usb_msi_ctrl_resume_value = 0U;
static uint32_t first_usb_new_msi_ctrl = 0U;
// For Command register init test case
static uint32_t usb_command_resume_value = 0U;
static uint32_t first_usb_command = 0U;
static uint32_t first_usb_new_command = 0U;
// For BAR0 function status init test case
static uint32_t first_usb_hci_version = 0U;
static volatile uint32_t first_usb_dn_ctrl = 0U;
static volatile uint32_t second_usb_dn_ctrl = 0U;

//For interrupt line
static uint8_t new_interrupt_line = 0U;

static volatile int cur_case_id = 0;
static volatile int wait_ap = 0;
static volatile int need_modify_init_value = 0;


static int pci_probe_msi_capability(union pci_bdf bdf, uint32_t *msi_addr);
static int pci_data_check(union pci_bdf bdf, uint32_t offset,
uint32_t bytes, uint32_t val1, uint32_t val2, bool sw_flag);


__unused void wait_ap_ready()
{
	while (wait_ap != 1) {
		test_delay(1);
	}
	wait_ap = 0;
}

__unused static void notify_modify_and_read_init_value(int case_id)
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

static
bool test_host_PCI_configuration_header_register_read_only(uint32_t dev_ven)
{
	static pci_cfg_reg_t read_only_regs [] = {
			{"vendor_ID", 0x00, 2, 0U, 0},
			{"device_ID", 0x02, 2, 0U, 0},
			{"revison_ID_class_Code", 0x08, 4, 0U, 0},
			{"cache_Line_Size_Latency_Header_BIST", 0x0C, 4, 0U, 0},
			{"CardBus_CIS_Pointer", 0x28, 4, 0U, 0},
			{"subsystem_ID_subsystem_vendor_ID", 0x2C, 4, 0U, 0},
			{"expansion_ROM_base_address_register", 0x30, 4, 0U, 0},
			{"capabilities_pointer_register", 0x34, 1, 0U, 0},
			{"intPin", 0x3D, 1, 0U, 0},
			{"min_gnt_max_lat", 0x3E, 2, 0U, 0},
	};
	int size = ELEMENT_NUM(read_only_regs);
	int i = 0;
	union pci_bdf bdf = {0};
	bool is_pass = false;
	uint32_t value = 0U;
	int count = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		for (i = 0; i < size; i++) {
			// 1.read the original value of these read-only registers
			read_only_regs[i].reg_val = pci_pdev_read_cfg(bdf,\
			read_only_regs[i].reg_addr, read_only_regs[i].reg_width);
			// 2.write FFH to these read-only registers
			pci_pdev_write_cfg(bdf, read_only_regs[i].reg_addr,\
			read_only_regs[i].reg_width, 0xFFFFFFFF);
			// 3.read the new value from these read-only registers
			value = pci_pdev_read_cfg(bdf,\
			read_only_regs[i].reg_addr, read_only_regs[i].reg_width);
			// 4.compare the new value with the original value
			if (value == read_only_regs[i].reg_val) {
				count++;
			}
		}
	}
	is_pass = (count == size) ? true : false;
	return is_pass;
}

static
bool test_host_MSI_message_control_write(uint32_t dev_ven)
{
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	uint32_t reg_addr1 = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		// pci probe and check MSI capability register
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		if (ret == OK) {
			reg_addr1 = reg_addr + PCI_MSI_FLAGS;
			// Step 4:read the original value of MSI_CTRL bit[15:1]
			reg_val_ori = pci_pdev_read_cfg(bdf, reg_addr1, 2);
			reg_val = reg_val_ori;
			// NOT operation to bit[15:1], bit[0] remains unchanged
			reg_val ^= SHIFT_MASK(15, 1);

			// Step 5:write the new value to MSI_CTRL bit[15:1]
			pci_pdev_write_cfg(bdf, reg_addr1, 2, reg_val);
			// Step 6:read the MSI_CTRL value
			reg_val_new = pci_pdev_read_cfg(bdf, reg_addr1, 2);

			ret |= pci_data_check(bdf, reg_addr1, 2, reg_val_new & SHIFT_MASK(15, 7), \
								(reg_val_ori & SHIFT_MASK(15, 7)), false);

			ret |= pci_data_check(bdf, reg_addr1, 2, reg_val_new & SHIFT_MASK(3, 1), \
								(reg_val_ori & SHIFT_MASK(3, 1)), false);

			// Restore the environment
			pci_pdev_write_cfg(bdf, reg_addr1, 2, reg_val_ori);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static
bool test_host_MSI_message_control_read(uint32_t dev_ven)
{
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	uint32_t reg_addr1 = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		// pci probe and check MSI capability register
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		if (ret == OK) {
			reg_addr1 = reg_addr + PCI_MSI_FLAGS;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 2);
			reg_val &= 0xFFFE;
			ret |= pci_data_check(bdf, reg_addr1, 2, SHIFT_LEFT(0x40U, 1), reg_val, false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static
bool test_host_MSI_message_address_redirection_hint_write(uint32_t dev_ven)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_addr1 = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		// pci probe and check MSI capability register
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		// Check if probe to MSI CAP register
		if (ret == OK) {
			reg_addr1 = reg_addr + PCI_MSI_ADDRESS_LO;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			reg_val_ori = reg_val;
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			// NOT bit[1:0] of original value
			reg_val ^= SHIFT_MASK(1, 0);
			pci_pdev_write_cfg(bdf, reg_addr1, 4, reg_val);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			// The read value should be equal to the ori value
			ret |= pci_data_check(bdf, reg_addr1, 4, (reg_val_ori & SHIFT_MASK(1, 0)), \
								(reg_val & SHIFT_MASK(1, 0)), false);
			// Restore the environment
			pci_pdev_write_cfg(bdf, reg_addr1, 4, reg_val_ori);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}


static
bool test_host_MSI_message_address_redirection_hint_read(uint32_t dev_ven)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_addr1 = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		// pci probe and check MSI capability register
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		// Check if probe to MSI CAP register
		if (ret == OK) {
			reg_addr1 = reg_addr + PCI_MSI_ADDRESS_LO;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			reg_val = reg_val & 0xFU;
			ret |= pci_data_check(bdf, reg_addr1, 4, 0U, reg_val, false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static
bool test_host_MSI_message_address_int_msg_addr_write(uint32_t dev_ven)
{
	uint32_t reg_val_ori_lo = INVALID_REG_VALUE_U;
	uint32_t reg_val_new_lo = INVALID_REG_VALUE_U;
	uint32_t reg_val_ori_hi = INVALID_REG_VALUE_U;
	uint32_t reg_val_new_hi = INVALID_REG_VALUE_U;
	uint32_t reg_addr = 0U;
	uint32_t reg_addr_lo = 0U;
	uint32_t reg_addr_hi = 0U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		// pci probe and check MSI capability register
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == OK) {
			// message address Low register address = pointer + 0x4
			reg_addr_lo = reg_addr + PCI_MSI_ADDRESS_LO;
			// message address High register address = pointer + 0x8
			reg_addr_hi = reg_addr + PCI_MSI_ADDRESS_HI;

			// read the MSI address register bit[20:64]
			reg_val_ori_lo = pci_pdev_read_cfg(bdf, reg_addr_lo, 4);
			reg_val_ori_hi = pci_pdev_read_cfg(bdf, reg_addr_hi, 4);

			// Not operation to bit[64:20]
			reg_val_new_lo = reg_val_ori_lo ^ SHIFT_MASK(31, 20);
			reg_val_new_hi = reg_val_ori_hi ^ SHIFT_MASK(31, 0);

			// write NOT operation of original bit[20:64] to Message address[20:64]
			pci_pdev_write_cfg(bdf, reg_addr_lo, 4, reg_val_new_lo);
			pci_pdev_write_cfg(bdf, reg_addr_hi, 4, reg_val_new_hi);

			// read the MSI address register bit[20:64]
			reg_val_new_lo = pci_pdev_read_cfg(bdf, reg_addr_lo, 4);
			reg_val_new_hi = pci_pdev_read_cfg(bdf, reg_addr_hi, 4);

			// compare bit[20:64] ori and bit[20:64] new
			ret |= pci_data_check(bdf, reg_addr_lo, 4, (reg_val_new_lo & SHIFT_MASK(31, 20)),\
								(reg_val_ori_lo & SHIFT_MASK(31, 20)), false);
			ret |= pci_data_check(bdf, reg_addr_hi, 4, reg_val_new_hi,\
								reg_val_ori_hi, false);
			// reset environment
			pci_pdev_write_cfg(bdf, reg_addr_lo, 4, reg_val_ori_lo);
			pci_pdev_write_cfg(bdf, reg_addr_hi, 4, reg_val_ori_hi);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static
bool test_host_MSI_message_address_interrupt_message_address_read(uint32_t dev_ven)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_addr1 = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		// pci probe and check MSI capability register
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		// Check if probe to MSI CAP register
		if (ret == OK) {
			reg_addr1 = reg_addr + PCI_MSI_ADDRESS_LO;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			ret |= pci_data_check(bdf, reg_addr1, 4, 0x0FEE00000U,\
								(reg_val & SHIFT_MASK(31, 20)), false);
			reg_addr1 = reg_addr + PCI_MSI_ADDRESS_HI;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			ret |= pci_data_check(bdf, reg_addr1, 4, 0x00000000U,\
								(reg_val), false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static
bool test_host_MSI_message_data_delivery_mode_read(uint32_t dev_ven)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		// pci probe and check MSI capability register
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		// Check if probe to MSI CAP register
		if (ret == OK) {
			reg_addr = reg_addr + PCI_MSI_DATA_64;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			ret |= pci_data_check(bdf, reg_addr, 2, 0, (reg_val & SHIFT_MASK(10, 8)), false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static
bool test_host_MSI_message_data_delivery_mode_write(uint32_t dev_ven)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		// pci probe and check MSI capability register
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		// Check if probe to MSI CAP register
		if (ret == OK) {
			reg_addr = reg_addr + PCI_MSI_DATA_64;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			reg_val_ori = reg_val;
			pci_pdev_write_cfg(bdf, reg_addr, 2, 0);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			ret |= pci_data_check(bdf, reg_addr, 2, 0, (reg_val & SHIFT_MASK(10, 8)), false);
			if (ret != OK)
				printf("%s: write 0 but read 0x%x\n", __func__, reg_val);

			pci_pdev_write_cfg(bdf, reg_addr, 2, 1 << 8);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			ret |= pci_data_check(bdf, reg_addr, 2, (1 << 8), (reg_val & SHIFT_MASK(10, 8)), false);
			if (ret != OK)
				printf("%s: write 1 but read 0x%x\n", __func__, reg_val);

			pci_pdev_write_cfg(bdf, reg_addr, 2, 2 << 8);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			ret |= pci_data_check(bdf, reg_addr, 2, (2 << 8), (reg_val & SHIFT_MASK(10, 8)), false);
			if (ret != OK)
				printf("%s: write 2 but read 0x%x\n", __func__, reg_val);

			pci_pdev_write_cfg(bdf, reg_addr, 2, 4 << 8);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			ret |= pci_data_check(bdf, reg_addr, 2, (4 << 8), (reg_val & SHIFT_MASK(10, 8)), false);
			if (ret != OK)
				printf("%s: write 4 but read 0x%x\n", __func__, reg_val);

			pci_pdev_write_cfg(bdf, reg_addr, 2, 5 << 8);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			ret |= pci_data_check(bdf, reg_addr, 2, (5 << 8), (reg_val & SHIFT_MASK(10, 8)), false);
			if (ret != OK)
				printf("%s: write 5 but read 0x%x\n", __func__, reg_val);

			pci_pdev_write_cfg(bdf, reg_addr, 2, 7 << 8);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			ret |= pci_data_check(bdf, reg_addr, 2, (7 << 8), (reg_val & SHIFT_MASK(10, 8)), false);
			if (ret != OK)
				printf("%s: write 5 but read 0x%x\n", __func__, reg_val);

			// Restore environment
			pci_pdev_write_cfg(bdf, reg_addr, 2, reg_val_ori);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static
bool test_host_MSI_message_data_trigger_mode_read(uint32_t dev_ven)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		// pci probe and check MSI capability register
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		// Check if probe to MSI CAP register
		if (ret == OK) {
			reg_addr = reg_addr + PCI_MSI_DATA_64;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			/* bit[15]=0 means edge, per SDM vol3 11.11.2, we don't care bit[14] if it is edge. */
			ret |= pci_data_check(bdf, reg_addr, 2, 0,\
								(reg_val & SHIFT_MASK(15, 15)), false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

/* Per Chapter 11.11.2, Vol3, SDM and Chapter 7.7.1.5 PCIe BS 5.0, Message Data Register bit[15:14] are writable. */
static
bool test_host_MSI_message_data_trigger_mode_write(uint32_t dev_ven)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		// pci probe and check MSI capability register
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		// Check if probe to MSI CAP register
		if (ret == OK) {
			reg_addr = reg_addr + PCI_MSI_DATA_64;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			reg_val_ori = reg_val;
			pci_pdev_write_cfg(bdf, reg_addr, 2, 0);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			// The read value should be equal to the original value
			ret |= pci_data_check(bdf, reg_addr, 2, 0, (reg_val & SHIFT_MASK(15, 14)), false);
			if (ret != OK)
				printf("%s: write 0 but read 0x%x\n", __func__, reg_val);

			pci_pdev_write_cfg(bdf, reg_addr, 2, 1 << 14);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			// The read value should be equal to the original value
			ret |= pci_data_check(bdf, reg_addr, 2, ((1 << 14) & SHIFT_MASK(15, 14)), (reg_val & SHIFT_MASK(15, 14)), false);
			if (ret != OK)
				printf("%s: write 1 but read 0x%x\n", __func__, reg_val);

			pci_pdev_write_cfg(bdf, reg_addr, 2, 2 << 14);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			// The read value should be equal to the original value
			ret |= pci_data_check(bdf, reg_addr, 2, ((2 << 14) & SHIFT_MASK(15, 14)), (reg_val & SHIFT_MASK(15, 14)), false);
			if (ret != OK)
				printf("%s: write 2 but read 0x%x\n", __func__, reg_val);

			pci_pdev_write_cfg(bdf, reg_addr, 2, 3 << 14);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			// The read value should be equal to the original value
			ret |= pci_data_check(bdf, reg_addr, 2, ((3 << 14) & SHIFT_MASK(15, 14)), (reg_val & SHIFT_MASK(15, 14)), false);
			if (ret != OK)
				printf("%s: write 3 but read 0x%x\n", __func__, reg_val);

			// restore environment
			pci_pdev_write_cfg(bdf, reg_addr, 2, reg_val_ori);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static
bool test_MSI_message_address_destination_field_rw(uint32_t dev_ven)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	uint32_t apic_id1 = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		// pci probe and check MSI capability register
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		// Check if probe to MSI CAP register
		if (ret == OK) {
			reg_addr = reg_addr + PCI_MSI_ADDRESS_LO;
			// read the original MSI address lo value
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			reg_val_ori = reg_val;
			// Write APIC ID to Destination ID field
			apic_id1 = apic_id();
			apic_id1 &= 0xFFU;
			reg_val &= SHIFT_UMASK(19, 12);
			reg_val |= SHIFT_LEFT(apic_id1, 12);
			// write the MSI address value
			pci_pdev_write_cfg(bdf, reg_addr, 4, reg_val);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			// Extract APIC ID from Destination ID field
			reg_val &= SHIFT_MASK(19, 12);
			reg_val = SHIFT_RIGHT(reg_val, 12);
			reg_val &= SHIFT_MASK(12, 0);
			ret |= pci_data_check(bdf, reg_addr, 2, reg_val, apic_id1, false);
			// restore environment
			pci_pdev_write_cfg(bdf, reg_addr, 4, reg_val_ori);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static
uint32_t pci_pdev_read_cfg_test(union pci_bdf bdf, uint32_t offset, uint32_t bytes, uint32_t port, bool is_en)
{
	uint32_t addr = 0U;
	uint32_t val = 0U;

	addr = pci_pdev_calc_address(bdf, offset);
	if (false == is_en) {
		addr = addr & (~PCI_CONFIG_ENABLE);
	}

	// Write address to ADDRESS register
	pio_write32(addr, (uint16_t)PCI_CONFIG_ADDR);

	// Read result from DATA register
	switch (bytes) {
	case 1U:
		val = (uint32_t)pio_read8(port);
		break;
	case 2U:
		val = (uint32_t)pio_read16(port);
		break;
	default:
		val = pio_read32((uint16_t)port);
		break;
	}

	return val;
}


static int pci_pdev_write_cfg_test(union pci_bdf bdf, uint32_t offset, uint32_t bytes,
		uint32_t val, uint32_t port, bool is_en)
{
	uint32_t addr = 0U;
	addr = pci_pdev_calc_address(bdf, offset);
	if (false == is_en) {
		addr = addr & (~PCI_CONFIG_ENABLE);
	}
	// Write address to ADDRESS register
	pio_write32(addr, (uint16_t)PCI_CONFIG_ADDR);
	// Write value to DATA register
	switch (bytes) {
	case 1U:
		pio_write8((uint8_t)val, (uint16_t)port);
		break;
	case 2U:
		pio_write16((uint16_t)val, (uint16_t)port);
		break;
	default:
		pio_write32(val, (uint16_t)port);
		break;
	}
	return OK;
}

static
bool  test_Write_with_Disabled_Config_Address(uint8_t msi_offset, uint32_t bytes, uint32_t port)
{
	int count = 0;
	uint32_t reg_val_ori = 0U;
	uint32_t reg_val_new = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	uint32_t size = 0;
	int msb = bytes * 8 - 1;
	switch (msi_offset) {
	case PCI_MSI_FLAGS:
		size = 2;
		break;
	case PCI_MSI_ADDRESS_LO:
		size = 4;
		break;
	case PCI_MSI_DATA_64:
		size = 2;
		break;
	default:
		break;
	}
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			reg_val_ori = pci_pdev_read_cfg(bdf, msi_addr, size);
			reg_val_new = reg_val_ori ^ SHIFT_MASK(msb, 0);
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, reg_val_new, port, false);
			reg_val_new = pci_pdev_read_cfg(bdf, msi_addr, size);
			reg_val_ori &= SHIFT_MASK(msb, 0);
			reg_val_new &= SHIFT_MASK(msb, 0);
			if (reg_val_ori == reg_val_new) {
				count++;
			}
		}
	}
	is_pass = (count == 1) ? true : false;
	return is_pass;
}


static
bool  test_Write_with_enabled_Config_Address(uint8_t msi_offset, uint32_t bytes, uint32_t port)
{
	int count = 0;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	int msb = bytes * 8 - 1;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			reg_val_ori = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			reg_val_new = reg_val_ori ^ SHIFT_MASK(msb, 0);
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, reg_val_new, port, true);
			reg_val = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			reg_val &= SHIFT_MASK(msb, 0);
			reg_val_new &= SHIFT_MASK(msb, 0);
			if (reg_val_new == reg_val) {
				count++;
			}
			// Reset environment
			pci_pdev_write_cfg(bdf, msi_addr, bytes, reg_val_ori);
		}
	}
	is_pass = (count == 1) ? true : false;
	return is_pass;
}

static
bool test_Read_with_Enabled_Config_Address(uint8_t msi_offset, uint32_t bytes, uint32_t port)
{
	int count = 0;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	int msb = bytes * 8 - 1;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			reg_val_ori = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			reg_val_new = reg_val_ori ^ SHIFT_MASK(msb, 0);
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, reg_val_new, port, true);
			reg_val = pci_pdev_read_cfg_test(bdf, msi_addr, bytes, port, true);
			reg_val &= SHIFT_MASK(msb, 0);
			reg_val_new &= SHIFT_MASK(msb, 0);
			if (reg_val ==	reg_val_new) {
				count++;
			}
			pci_pdev_write_cfg(bdf, msi_addr, bytes, reg_val_ori);
		}
	}
	is_pass = (count == 1) ? true : false;
	return is_pass;
}


static void check_bar_map_function(void)
{
	//first_usb_bar0_value = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 4);
	if (first_usb_bar0_resume_value != 0) {
		void *hci_version_addr;
		pci_pdev_write_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 4, first_usb_bar0_resume_value);
		hci_version_addr = phys_to_virt((first_usb_bar0_resume_value & 0xFFFFFFF0) + 0x02U);
		first_usb_hci_version = pci_pdev_read_mem(PCI_GET_BDF(usb), (mem_size)hci_version_addr, 2);

		void *dn_ctrl_addr = phys_to_virt((first_usb_bar0_resume_value & 0xFFFFFFF0) + 0x94U);
		first_usb_dn_ctrl = pci_pdev_read_mem(PCI_GET_BDF(usb), (mem_size)dn_ctrl_addr, 4);
	}
}

static void modify_usb_dn_ctrl(void)
{
	if (first_usb_bar0_resume_value != 0)
	{
		pci_pdev_write_cfg(PCI_GET_BDF(usb), PCI_COMMAND, 2, usb_command_resume_value);
		void *dn_ctrl_addr = phys_to_virt((first_usb_bar0_resume_value & 0xFFFFFFF0) + 0x94U);
		first_usb_dn_ctrl = pci_pdev_read_mem(PCI_GET_BDF(usb), (mem_size)dn_ctrl_addr, 4);
		pci_pdev_write_mem(PCI_GET_BDF(usb), (mem_size)dn_ctrl_addr, 4, first_usb_dn_ctrl ^ 1ul);
		second_usb_dn_ctrl = pci_pdev_read_mem(PCI_GET_BDF(usb), (mem_size)dn_ctrl_addr, 4);
	}
}

static void read_command_register(void)
{
	if (usb_command_resume_value == 0) {
		usb_command_resume_value = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCI_COMMAND, 2);
	}
	first_usb_command = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCI_COMMAND, 2);
}

static void modify_command_register()
{
	uint32_t new_value = 0U;

	first_usb_command = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCI_COMMAND, 2);
	/*bit1:Controls a device's response to Memory Space accesses*/
	new_value = first_usb_command ^ SHIFT_LEFT(0x1, 1);
	pci_pdev_write_cfg(PCI_GET_BDF(usb), PCI_COMMAND, 2, new_value);
	first_usb_new_command = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCI_COMMAND, 2);
}

static
void read_BAR_Address_register(void)
{
	if (first_usb_bar0_resume_value == 0) {
		first_usb_bar0_resume_value = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 4);
	}
	first_usb_bar0_value = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 4);
}

static
void modify_BAR_Address_register(void)
{
	pci_pdev_write_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 4, A_NEW_USB_BAR0_VALUE);
	first_usb_new_bar0_value = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 4);
}

static
void read_msi_control_flag(void)
{
	if (usb_msi_ctrl_resume_value == 0) {
		usb_msi_ctrl_resume_value = pci_pdev_read_cfg(PCI_GET_BDF(usb), USB_MSI_REG_OFFSET + 2, 2);
	}
	first_usb_msi_ctrl = pci_pdev_read_cfg(PCI_GET_BDF(usb), USB_MSI_REG_OFFSET + 2, 2);
}

static
void modify_msi_control_flag(void)
{
	uint32_t new_value = 0U;
	first_usb_msi_ctrl = pci_pdev_read_cfg(PCI_GET_BDF(usb), USB_MSI_REG_OFFSET + 2, 2);
	/*bit 0: MSI enable bit */
	new_value = first_usb_msi_ctrl ^ SHIFT_LEFT(0x01, 0);
	pci_pdev_write_cfg(PCI_GET_BDF(usb), USB_MSI_REG_OFFSET + 2, 2, new_value);
	first_usb_new_msi_ctrl = pci_pdev_read_cfg(PCI_GET_BDF(usb), USB_MSI_REG_OFFSET + 2, 2);
}

static
void read_USB_interrupt_line(void)
{
	uint8_t usb_interrupt_line;

	usb_interrupt_line = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCI_INTERRUPT_LINE, 1);
	*((uint32_t *)AP_USB_INTERRUP_LINE_ADDR0) = usb_interrupt_line;
}

static
void modify_usb_interrupt_line(void)
{
	uint8_t usb_interrupt_line;
	usb_interrupt_line = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCI_INTERRUPT_LINE, 1);
	pci_pdev_write_cfg(PCI_GET_BDF(usb), PCI_INTERRUPT_LINE, 1, usb_interrupt_line ^ 1);
	new_interrupt_line = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCI_INTERRUPT_LINE, 1);
}


static void read_PCI_address_port(void)
{
	// write a new value to $0xCF8 PCI address port and read it on first AP.
	asm volatile("push" W " %%" R "dx\n\t"\
				"push" W " %%" R "ax\n\t"\
				"mov $0xCF8, %%edx\n\t"\
				"inl (%%dx), %%eax\n\t"\
				"mov %%eax, %0\n\t"\
				"pop" W " %%" R "ax\n\t"\
				"pop" W " %%" R "dx\n\t"\
				: "=m" (unchange_resume_value)
				:
				: "memory");
	*((uint32_t *)AP_IO_PORT_ADDR) = unchange_resume_value;
}

static void modify_PCI_address_port(void)
{
	uint32_t new_value = A_NEW_VALUE_UNCHANGE;
	// write a new value to $0xCF8 PCI address port and read it on first AP.
	asm volatile("push" W " %%" R "dx\n\t"\
				"push" W " %%" R "ax\n\t"\
				"mov $0xCF8, %%edx\n\t"\
				"mov %0, %%eax\n\t"\
				"outl %%eax, (%%dx)\n\t"\
				"pop" W " %%" R "ax\n\t"\
				"pop" W " %%" R "dx\n\t"\
				:
				: "m" (new_value)
				: "memory");
}

void save_unchanged_reg(void)
{
	uint32_t id = 0U;
	id = apic_id();
	apic_id_bitmap |= SHIFT_LEFT(1UL, id);

	if (get_cpu_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	read_PCI_address_port();
	read_USB_interrupt_line();
	read_BAR_Address_register();
	read_msi_control_flag();
	read_command_register();
	check_bar_map_function();
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
		wait_ap = 1;
	}
}

void ap_main(void)
{
	ap_init_value_modify fp;
	/*test only on the ap 2,other ap return directly*/
	if (get_cpu_id() != (fwcfg_get_nb_cpus() - 1)) {
		return;
	}

	switch (cur_case_id) {
	case 37251:
		fp = modify_PCI_address_port;
		ap_init_value_process(fp);
		break;
	case 38217:
		fp = modify_msi_control_flag;
		ap_init_value_process(fp);
		break;
	case 37264:
		fp = modify_usb_interrupt_line;
		ap_init_value_process(fp);
		break;
	case 38246:
		fp = modify_command_register;
		ap_init_value_process(fp);
		break;
	case 38204:
		fp = modify_BAR_Address_register;
		ap_init_value_process(fp);
		break;
	case 38265:
		fp = modify_usb_dn_ctrl;
		ap_init_value_process(fp);
		break;
	default:
		asm volatile ("nop\n\t" :::"memory");
		break;
	}
}
#endif

static __unused
uint32_t find_first_apic_id(void)
{
	int i = 0;
	int size = sizeof(uint64_t) * 8;

	for (i = 0; i < size; i++) {
		if (apic_id_bitmap & SHIFT_LEFT(1UL, i)) {
			return i;
		}
	}
	return INVALID_APIC_ID_B;
}

/* pci_data_check
 * sw_flag: false expects val1 == val2, true expects val1 != val2;
 */
static int pci_data_check(union pci_bdf bdf, uint32_t offset,
uint32_t bytes, uint32_t val1, uint32_t val2, bool sw_flag)
{
	int ret = OK;

	if (val1 == val2) {
		ret = OK;
	} else {
		ret = ERROR;
	}

	if (sw_flag) {
		ret = (ret == OK) ? ERROR : OK;
	}

	if (OK == ret) {
		DBG_INFO("dev[%02x:%02x:%d] reg/mem/io [0x%x] len [%d] data check ok:write/expect[%xH] %s read[%xH]",\
		bdf.bits.b, bdf.bits.d, bdf.bits.f, offset, bytes,\
		val1, ((sw_flag == true) ? "!=" : "="), val2);
		return OK;
	} else {
		DBG_WARN("dev[%02x:%02x:%d] reg/mem/io [0x%x] len [%d] data check fail:write/expect[%xH] %s read[%xH]",\
		bdf.bits.b, bdf.bits.d, bdf.bits.f, offset, bytes,\
		val1, ((sw_flag == true) ? "=" : "!="), val2);
		return ERROR;
	}
	return OK;
}

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
		// Find MSI register group
		if (PCI_CAP_ID_MSI == (reg_val & SHIFT_MASK(7, 0))) {
			break;
		}
		reg_addr = (reg_val & SHIFT_MASK(15, 8)) >> 8;
		reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
		// DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
	}
	if (repeat == i) {
		DBG_ERRO("no msi cap found!!!");
		return ERROR;
	}
	*msi_addr = reg_addr;
	return OK;
}

/*
 * @brief case name: PCIe config space and host_Write Reserved register_001
 *
 * Summary: When a vCPU attempts to write a guest reserved PCI configuration register/a register's bits,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static void pci_rqmid_25295_PCIe_config_space_and_host_Write_Reserved_register_001(void)
{
	bool is_pass = false;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	union pci_bdf bdf = {0};
	uint32_t count = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_CONFIG_RESERVE, 2);
		if (reg_val_ori == 0)
			count++;
		reg_val_new = reg_val_ori ^ SHIFT_MASK(15, 0);
		pci_pdev_write_cfg(bdf, PCI_CONFIG_RESERVE, 2, reg_val_new);
		reg_val_new = pci_pdev_read_cfg(bdf, PCI_CONFIG_RESERVE, 2);
		if (reg_val_ori == reg_val_new)
			count++;
		is_pass = (count == 2) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Write Reserved register_002
 *
 * Summary: When a vCPU attempts to write a guest reserved PCI configuration register/a register's bits,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static void pci_rqmid_25298_PCIe_config_space_and_host_Write_Reserved_register_002(void)
{
	bool is_pass = false;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	union pci_bdf bdf = {0};
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		reg_val_new = reg_val_ori ^ SHIFT_MASK(15, 11);
		DBG_INFO("reg_val = 0x%08x", reg_val_new);
		pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, reg_val_new);
		reg_val_new = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		ret |= pci_data_check(bdf, PCI_COMMAND, 2, (reg_val_ori & SHIFT_MASK(15, 11)),\
			(reg_val_new & SHIFT_MASK(15, 11)), false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Read_Reserved_register_001
 *
 * Summary: When a vCPU attempts to read a guest reserved PCI configuration register,
 * ACRN hypervisor shall guarantee that 00H is written to guest EAX.
 */
static void pci_rqmid_26040_PCIe_config_space_and_host_Read_Reserved_register_001(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_CONFIG_RESERVE, 2);
		is_pass = (reg_val == 0U) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/* No exist device tests start */
static int pci_dev_temp_bar0_Set(union pci_bdf bdf, uint32_t value)
{
	int i = 0;
	for (i = 0; i < nr_pci_devs; i++) {
		if (pci_devs[i].bdf.value == bdf.value) {
			pci_devs[i].mmio_start[0] = value;
			return 0;
		}
	}
	return -1;
}

static int pci_dev_temp_bar0_Get(union pci_bdf bdf, uint32_t *value)
{
	int i = 0;
	for (i = 0; i < nr_pci_devs; i++) {
		if (pci_devs[i].bdf.value == bdf.value) {
			*value = pci_devs[i].mmio_start[0];
			return 0;
		}
	}
	return -1;
}

static
void pci_rqmid_26105_PCIe_config_space_and_host_Write_register_to_no_exist_device_002(void)
{
	/*
	 * Ignoring a write to a nonexistent device has two implications.
	 * This means two things:
	 *           1. Writing to a nonexistent device will not affect the existing device
	 *           2. Write to a nonexistent device, the write command is ignored.
	 * In this CASE, for the second aspect.
	 * To determine whether the write configuration space is ignored,
	 * we use the BAR0 register 0x10 as the test object.
	 * For nonexistent devices, write 0xFFFFFFFF to register BAR0 and read back 0xFFFFFFFF.
	 */
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = 0;
	uint8_t func = 0;
	uint16_t bus = 0;
	uint8_t dev = 0;
	// write no-exist device BAR0 register to 0xFFFFFFFF
	for (bus = 0; bus < BUS_NUM; bus++) {
		bdf.bits.b = bus;
		for (dev = 0; dev < DEV_NUM; dev++) {
			bdf.bits.d = dev;
			for (func = 0; func < FUNC_NUM ; func++) {
				bdf.bits.f = func;
				if (is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf)) {
					continue;
				}
				pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, 0xFFFFFFFF);
				DBG_INFO("[%03d:%02xH:%d]", bdf.bits.b, bdf.bits.d, bdf.bits.f);
			}
		}
	}
	// read non-exist device BAR0 register and compare this value with 0xFFFFFFFF
	for (bus = 0; bus < BUS_NUM; bus++) {
		bdf.bits.b = bus;
		for (dev = 0; dev < DEV_NUM; dev++) {
			bdf.bits.d = dev;
			for (func = 0; func < FUNC_NUM; func++) {
				bdf.bits.f = func;
				if (is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf)) {
					continue;
				}
				reg_val = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
				ret |= pci_data_check(bdf, PCIR_BAR(0), 4, 0xFFFFFFFF, reg_val, false);
			}
		}
	}
	is_pass = (ret == OK) ? true : false;
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name: PCIe config space and host_Write register to no-exist device_001
 *
 * Summary: When a vCPU attempts to write a guest PCI configuration register of a non-exist device,
 * ACRN hypervisor shall gurantee that the write is ignored.
 */
static
void pci_rqmid_26095_PCIe_config_space_and_host_Write_register_to_no_exist_device_001(void)
{
	/*
	 *Ignoring a write to a nonexistent device has two implications.
	 *This means two things:
	 *	1. Writing to a nonexistent device will not affect the existing device
	 *	2. Write to a nonexistent device, the write command is ignored.
	 *In this CASE, for the first aspect.
	 *To determine whether the write configuration space is ignored,
	 *we use the BAR0 register 0x10 as the test object.
	 */
	uint32_t reg_val_old = 0;
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	uint8_t func = 0;
	uint16_t bus = 0;
	uint8_t dev = 0;

	// read exist device BAR0 register,and record this value
	for (bus = 0; bus < BUS_NUM; bus++) {
		bdf.bits.b = bus;
		for (dev = 0; dev < DEV_NUM; dev++) {
			bdf.bits.d = dev;
			for (func = 0; func < FUNC_NUM; func++) {
				bdf.bits.f = func;
				if (!is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf)) {
					continue;
				}
				reg_val = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
				if (ERROR == pci_dev_temp_bar0_Set(bdf, reg_val)) {
					DBG_WARN("Not find exist dev[%d:%xH:%d]", bdf.bits.b, bdf.bits.d, bdf.bits.f);
				}
			}
		}
	}

	// write no-exist device BAR0 register
	for (bus = 0; bus < BUS_NUM; bus++) {
		bdf.bits.b = bus;
		for (dev = 0; dev < DEV_NUM; dev++) {
			bdf.bits.d = dev;
			for (func = 0; func < FUNC_NUM; func++) {
				bdf.bits.f = func;
				if (is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf)) {
					continue;
				}
				pci_dev_temp_bar0_Get(bdf, &reg_val_old);
				pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, (reg_val_old ^ SHIFT_MASK(31, 0)));
				DBG_INFO("[%03d:%02xH:%d]", bdf.bits.b, bdf.bits.d, bdf.bits.f);
			}
		}
	}

	// read exist device BAR0 register and compare this value with record data
	for (bus = 0; bus < BUS_NUM; bus++) {
		bdf.bits.b = bus;
		for (dev = 0; dev < DEV_NUM; dev++) {
			bdf.bits.d = dev;
			for (func = 0; func < FUNC_NUM; func++) {
				bdf.bits.f = func;
				if (!is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf)) {
					continue;
				}
				reg_val = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
				if (ERROR == pci_dev_temp_bar0_Get(bdf, &reg_val_old)) {
					DBG_WARN("Not find exist dev[%d:%xH:%d]", bdf.bits.b, bdf.bits.d, bdf.bits.f);
				}
				ret |= pci_data_check(bdf, PCIR_BAR(0), 4, reg_val_old, reg_val, false);
			}
		}
	}
	is_pass = (ret == OK) ? true : false;
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name: PCIe config space and host_Read register from no-exist device_001
 *
 * Summary: When a vCPU attempts to read a guest PCI configuration register of a non-exist PCIe device,
 * ACRN hypervisor shall guarantee that FFFF FFFFH is written to guest EAX.
 */
static
void pci_rqmid_26109_PCIe_config_space_and_host_Read_register_from_no_exist_device_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	uint8_t func = 0;
	uint16_t bus = 0;
	uint8_t dev = 0;
	int count = 0;
	// read exist device BAR0 register and compare this value with 0xFFFFFFFF
	for (bus = 0; bus < BUS_NUM; bus++) {
		bdf.bits.b = bus;
		for (dev = 0; dev < DEV_NUM; dev++) {
			bdf.bits.d = dev;
			for (func = 0; func < FUNC_NUM; func++) {
				bdf.bits.f = func;
				if (is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf)) {
					continue;
				}
				reg_val = pci_pdev_read_cfg(bdf, PCI_VENDOR_ID, 4);
				ret |= pci_data_check(bdf, PCI_VENDOR_ID, 4, 0xFFFFFFFF, reg_val, false);
				count++;
				if (count == 3)
					goto end;
			}
		}
	}
end:
	is_pass = (ret == OK) ? true : false;
	report("%s", is_pass, __FUNCTION__);
	return;
}
/* No exist device tests end */

/* PCIe Hole start */
#ifdef IN_SAFETY_VM
/*
 * @brief case name: PCIe config space and host_PCI hole range_001
 *
 * Summary: ACRN hypervisor shall limit a fixed PCI hole address
 * range (memory 0x80000000-0xdfffffff)to one VM.
 */
static
void pci_rqmid_28743_PCIe_config_space_and_host_PCI_hole_range_001(void)
{
	union pci_bdf bdf = {0};
	mem_size bar_base = 0U;
	uint32_t bar_val = 0U;
	uint32_t reg_val = 0u;
	bool is_pass = false;
	int ret = OK;
	int ret1 = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, 0x00000000);
		for (bar_val = BAR_HOLE_LOW; bar_val < BAR_HOLE_HIGH; bar_val += BAR_ALLIGN_USB) {
			pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_val & 0xFFFFFFFF);
			bar_base = SHIFT_UMASK(31, 0);
			bar_base = SHIFT_MASK(31, 4) & bar_val;
			bar_base = (mem_size)phys_to_virt(bar_base + HCIVERSION);
			reg_val = pci_pdev_read_mem(bdf, bar_base, 2);
			ret1 = pci_data_check(bdf, bar_base, 2, HCIVERSION_VALUE, reg_val, false);
			if (ret1 != OK)
				DBG_ERRO("%s: bar_val=0x%x, bar_base=0x%lx, reg_val=0x%x\n", __func__, bar_val, bar_base, reg_val);
			ret |= ret1;
		}
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

static
void pci_rqmid_28743_PCIe_config_space_and_host_PCI_hole_range_002(void)
{
	union pci_bdf bdf = {0};
	mem_size bar_base = 0U;
	uint32_t bar0_org = 0U;
	uint32_t bar1_org = 0U;
	uint64_t bar_val = 0U;
	uint64_t bar_map = 0U;
	uint32_t reg_val = 0u;
	bool is_pass = false;
	int ret = OK;
	int ret1 = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		bar0_org = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		bar1_org = pci_pdev_read_cfg(bdf, PCIR_BAR(1), 4);
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, 0x00000000);
		pci_pdev_write_cfg(bdf, PCIR_BAR(1), 4, 0x00000000);
		for (bar_val = BAR_HOLE_LOW_64; bar_val < BAR_HOLE_HIGH_64; bar_val += BAR_ALLIGN_USB_64) {
			pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, (bar_val | 0x4) & 0xFFFFFFFF);
			pci_pdev_write_cfg(bdf, PCIR_BAR(1), 4, bar_val >> 32);
			bar_map = (uint64_t)ioremap(bar_val, BAR_ALLIGN_USB);
			bar_base = SHIFT_UMASK(63, 0);
			bar_base = SHIFT_MASK(63, 4) & bar_map;
			bar_base = (mem_size)phys_to_virt(bar_base + HCIVERSION);
			reg_val = pci_pdev_read_mem(bdf, bar_base, 2);
			ret1 = pci_data_check(bdf, bar_base, 2, HCIVERSION_VALUE, reg_val, false);
			if (ret1 != OK)
				DBG_ERRO("%s: bar_val=0x%lx, bar_base=0x%lx, reg_val=0x%x\n", __func__, bar_val, bar_base, reg_val);
			ret |= ret1;
		}
		is_pass = (ret == OK) ? true : false;
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar0_org);
		pci_pdev_write_cfg(bdf, PCIR_BAR(1), 4, bar1_org);
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

static void test_PCI_read_bar_memory_PF(void *arg)
{
	uint32_t *add = (uint32_t *)arg;
	uint32_t *add2 = (uint32_t *)phys_to_virt(*add);
	uint32_t temp = 0;
	asm volatile(ASM_TRY("1f")
							"mov (%%eax), %%ebx\n\t"
							"1:"
							: "=b" (temp)
							: "a" (add2));
}

/*
 * @brief case name: PCIe config space and host_BAR range limitation_001
 *
 * Summary: When a vCPU writes a guest PCI configuration base address register(BAR)
 * of an assigned PCIe device and the new guest physical
 * base address in the guest BAR is out of guest PCI hole,
 * ACRN hypervisor shall guarantee that the control function corresponding to
 * the guest BAR is not mapped to any guest physical address.
 */
static void pci_rqmid_28699_PCIe_config_space_and_host_BAR_range_limitation_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t bar_base = 0U;
	uint32_t bar_base_new = BAR_REMAP_BASE_3;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		bar_base = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, 0x00000000);
		// Write a new BAR out of PCI BAR memory hole
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base_new & 0xFFFFFFFF);
		DBG_INFO("W reg[%xH] = [%xH]", PCIR_BAR(0), bar_base_new);
		// Read the BAR memory should generate #PF
		is_pass = test_for_exception(PF_VECTOR, test_PCI_read_bar_memory_PF, (void *)&bar_base_new);
		// Resume the original BAR address
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base);
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

static void test_PCI_read_bar_memory_PF_64(void *arg)
{
	uint64_t *add = (uint64_t *)arg;
	uint64_t *add2 = (uint64_t *)phys_to_virt(*add);
	uint64_t temp = 0;
	asm volatile(ASM_TRY("1f")
							"mov (%%rax), %%rbx\n\t"
							"1:"
							: "=b" (temp)
							: "a" (add2));
}

static void pci_rqmid_28699_PCIe_config_space_and_host_BAR_range_limitation_002(void)
{
	union pci_bdf bdf = {0};
	uint64_t bar_base = 0U;
	uint64_t bar_base_new = BAR_REMAP_BASE_4;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		bar_base = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		bar_base |= pci_pdev_read_cfg(bdf, PCIR_BAR(1), 4);
		// Write a new BAR out of PCI BAR memory hole
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, (bar_base_new | 0x4) & 0xFFFFFFFF);
		pci_pdev_write_cfg(bdf, PCIR_BAR(1), 4, bar_base_new >> 32);
		// Read the BAR memory should generate #PF
		is_pass = test_for_exception(PF_VECTOR, test_PCI_read_bar_memory_PF_64, (void *)&bar_base_new);
		// Resume the original BAR address
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base & 0xFFFFFFFF);
		pci_pdev_write_cfg(bdf, PCIR_BAR(1), 4, bar_base >> 32);
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}
#endif
/* PCIe Hole end */

/*
 * @brief case name: PCIe config space and host_Interrupt Line Register_001
 *
 * Summary: ACRN hypervisor shall expose PCI configuration interrupt line register
 * of any PCIe device to the VM which the PCIe device is assigned to,
 * in compliance with Chapter 7.5.1.5, PCIe BS.
 */
static
void pci_rqmid_26817_PCIe_config_space_and_host_Interrupt_Line_Register_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_INTERRUPT_LINE, 1);
		DBG_INFO("USB Interrupt_Line reg_val = 0x%x", reg_val);
		ret |= pci_data_check(bdf, PCI_INTERRUPT_LINE, 1, NATIVE_USB_INTERRUPT_LINE, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Device_Identification_Registers_004
 *
 * Summary: When a vCPU attempts to read a guest PCI configuration
 * type register of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU
 * gets the value 0x0 or 0x80 (multi-function device).
 */
static
void pci_rqmid_26245_PCIe_config_space_and_host_Type_Register_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_HEADER_TYPE, 1);
		ret |= pci_data_check(bdf, PCI_HEADER_TYPE, 1, PCI_USB_HEADER_TYPE, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Status Register_001
 *
 * Summary: ACRN hypervisor shall expose PCI configuration status register of any PCIe device to the VM
 * which the PCIe device is assigned to, in compliance with Chapter 7.5.1.2, PCIe BS.
 */
static void pci_rqmid_26793_PCIe_config_space_and_host_Status_Register_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t count = 0;
	int ret = OK;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		// Dump the #status Register value for the USB devices in the hypervisor environment
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		DBG_INFO("Dump the status Register value [%xH] = [%xH]", PCI_STATUS, reg_val_ori);
		if (PCI_USB_STATUS_VAL_NATIVE == reg_val_ori)
			count++;

		reg_val_new = reg_val_ori ^ SHIFT_MASK(7, 0);
		pci_pdev_write_cfg(bdf, PCI_STATUS, 2, reg_val_new);
		reg_val_new = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		ret = pci_data_check(bdf, PCI_STATUS, 2, (reg_val_ori & SHIFT_MASK(7, 0)),\
							(reg_val_new & SHIFT_MASK(7, 0)), false);
		if (ret == OK)
			count++;

		reg_val_new = reg_val_ori ^ SHIFT_MASK(10, 9);
		pci_pdev_write_cfg(bdf, PCI_STATUS, 2, reg_val_new);
		reg_val_new = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		ret = pci_data_check(bdf, PCI_STATUS, 2, (reg_val_ori & SHIFT_MASK(10, 9)),\
							(reg_val_new & SHIFT_MASK(10, 9)), false);
		if (ret == OK)
			count++;

		is_pass = (count == 3) ? true : false;
		pci_pdev_write_cfg(bdf, PCI_STATUS, 2, reg_val_ori);
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name: PCIe_config_space_and_host_Command_Register_001
 *
 * Summary: ACRN hypervisor shall expose PCI configuration command register [bit 15:11, 9:0]
 * of any PCIe device to the VM which the PCIe device is assigned to, in compliance
 * with Chapter 7.5.1.1, PCIe BS.
 */
static void pci_rqmid_26794_PCIe_config_space_and_host_Command_Register_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t reg_val1 = 0U;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		reg_val = reg_val_ori;
		DBG_INFO("USB PCI Command Val = 0x%x\n", reg_val);
		// set readable-writable bit
		reg_val = SHIFT_LEFT(0x1, 10);
		reg_val |= SHIFT_LEFT(0x1, 8);
		reg_val |= SHIFT_LEFT(0x1, 6);
		reg_val |= SHIFT_LEFT(0x1, 2);
		pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, reg_val);
		reg_val1 = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		ret |= pci_data_check(bdf, PCI_COMMAND, 2, reg_val1, reg_val, false);

		reg_val_new = reg_val_ori ^ SHIFT_MASK(5, 3);
		pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, reg_val_new);
		reg_val_new = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		ret |= pci_data_check(bdf, PCI_COMMAND, 2, (reg_val_ori & SHIFT_MASK(5, 3)),\
							(reg_val_new & SHIFT_MASK(5, 3)), false);
		reg_val_new = reg_val_ori ^ SHIFT_MASK(7, 7);
		pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, reg_val_new);
		reg_val_new = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		ret |= pci_data_check(bdf, PCI_COMMAND, 2, (reg_val_ori & SHIFT_MASK(7, 7)),\
							(reg_val_new & SHIFT_MASK(7, 7)), false);
		reg_val_new = reg_val_ori ^ SHIFT_MASK(9, 9);
		pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, reg_val_new);
		reg_val_new = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		ret |= pci_data_check(bdf, PCI_COMMAND, 2, (reg_val_ori & SHIFT_MASK(9, 9)),\
							(reg_val_new & SHIFT_MASK(9, 9)), false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

static
void pci_PCIe_config_space_and_host_Hostbridge_XBAR(void)
{
	union pci_bdf bdf = {0};
	bool is_pass = false;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		mmcfg_addr = pci_pdev_read_cfg(bdf, 0x60, 4);
		/* 0xe0000001 is for safety VM; 0xc0000001 is service VM */
		is_pass = (mmcfg_addr == 0xe0000001 || mmcfg_addr == 0xc0000001) ? true : false;
		if (!is_pass)
			DBG_ERRO("%s: mmcfg_addr read out is 0x%x\n", __func__, mmcfg_addr);
		mmcfg_addr &= 0xFFFFFFFE;
		DBG_INFO("%s: finally, mmcfg_addr is 0x%x\n", __func__, mmcfg_addr);
	}
	report("%s", is_pass, __FUNCTION__);
}

#ifdef IN_SAFETY_VM
/*
 * @brief case name: PCIe config space and host_Hostbridge readonly_001
 *
 * Summary: When a vCPU attempts to write a guest PCI configuration register of its own hostbridge,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static
void pci_rqmid_27486_PCIe_config_space_and_host_Hostbridge_readonly_001(void)
{
	union pci_bdf bdf = {0};
	int i = 0;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	int ret = OK;
	bool is_pass = false;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		for (i = 0; i < (ELEMENT_NUM(hostbridge_pci_cfg) - 1); i++) {
			// Read the register as Value(A)
			reg_val_ori = pci_pdev_read_cfg(bdf, hostbridge_pci_cfg[i].reg_addr,\
				hostbridge_pci_cfg[i].reg_width);
			// Not operation to original value as Value(B)
			reg_val_new = reg_val_ori ^ SHIFT_MASK(31, 0);
			// Write the Value(B) to register
			pci_pdev_write_cfg(bdf, hostbridge_pci_cfg[i].reg_addr,\
				hostbridge_pci_cfg[i].reg_width, reg_val_new);
			// Read the register as Value(C)
			reg_val_new = pci_pdev_read_cfg(bdf, hostbridge_pci_cfg[i].reg_addr,\
				hostbridge_pci_cfg[i].reg_width);
			// Compare Value(A) and Value(C)
			ret |= pci_data_check(bdf, hostbridge_pci_cfg[i].reg_addr,\
				hostbridge_pci_cfg[i].reg_width,\
				reg_val_ori, reg_val_new, false);
		}
	}
	is_pass = (ret == OK) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge Header Type_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration header type
 * register of its own hostbridge, ACRN hypervisor shall guarantee that the vCPU gets 00H.
 */
static
void pci_rqmid_27474_PCIe_config_space_and_host_Hostbridge_Header_Type_001(void)
{
	union pci_bdf bdf = {0};
	bool is_pass = false;
	uint32_t reg_val = 0x0U;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_HEADER_TYPE, 1);
		is_pass = (reg_val == 0 || reg_val == 0x80) ? true : false;
		if (!is_pass)
			DBG_ERRO("%s: header is 0x%x\n", __func__, reg_val);
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge Class Code_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration class code
 * register of its own hostbridge, ACRN hypervisor shall guarantee that the vCPU gets 060000H.
 */
static
void pci_rqmid_27475_PCIe_config_space_and_host_Hostbridge_Class_Code_001(void)
{
	union pci_bdf bdf = {0};
	bool is_pass = false;
	uint32_t reg_val = 0x0U;
	uint32_t reg_val1 = 0x0U;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_CLASS_PROG, 1);
		reg_val1 = pci_pdev_read_cfg(bdf, PCI_CLASS_DEVICE, 2);
		reg_val = (reg_val1 << 8) | (reg_val);
		is_pass = (reg_val == HOSTBRIDGE_CLASS_CODE) ? true : false;
		if (!is_pass)
			DBG_ERRO("%s: class_code is 0x%x\n", __func__, reg_val);
	}
	report("%s", is_pass, __FUNCTION__);
}
#endif

/*
 * @brief case name: PCIe config space and host_2-Byte CFEH Write with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 2 bytes to guest I/O port address CFEH and
 * current guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall ignore the write.
 */
static
void pci_rqmid_27735_PCIe_config_space_and_host_2_Byte_CFEH_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_Disabled_Config_Address(PCI_MSI_FLAGS, 2, 0xCFE);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_1-Byte Config Data Port Write with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFCH and current guest
 * PCI address register value [bit 31] is 0H, ACRN hypervisor shall ignore the write.
 */
static
void pci_rqmid_27747_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_Disabled_Config_Address(PCI_MSI_DATA_64, 1, 0xCFC);
	report("%s", is_pass, __FUNCTION__);

}

/*
 * @brief case name:PCIe config space and host_1-Byte CFEH Write with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFEH and current
 * guest PCI address register value [bit 31] is 0H,ACRN hypervisor shall ignore the write.
 */
static
void pci_rqmid_27742_PCIe_config_space_and_host_1_Byte_CFEH_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_Disabled_Config_Address(PCI_MSI_FLAGS, 1, 0xCFE);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFDH Write with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFDH and
 * current guest PCI address register value [bit 31] is 0H,ACRN hypervisor shall ignore the write.
 */
static
void pci_rqmid_27744_PCIe_config_space_and_host_1_Byte_CFDH_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_Disabled_Config_Address(PCI_MSI_ADDRESS_LO, 1, 0xCFD);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFFH Write with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFFH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall ignore the write.
 */
static
void pci_rqmid_27741_PCIe_config_space_and_host_1_Byte_CFFH_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_val_ori = 0U;
	uint32_t reg_val_new = 0U;
	int count = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		reg_val_new = SHIFT_RIGHT(reg_val_ori, 8);
		reg_val_new = reg_val_new ^ SHIFT_MASK(7, 0);
		pci_pdev_write_cfg_test(bdf, PCI_STATUS + 1, 1, reg_val_new, 0xCFF, false);
		reg_val_new = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		reg_val_ori &= SHIFT_MASK(15, 8);
		reg_val_new &= SHIFT_MASK(15, 8);
		if (reg_val_ori == reg_val_new) {
			count++;
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte Config Data Port Write with Disabled Config Address_001
 *
 * Summary:When a vCPU attempts to write 2 bytes to guest I/O port address CFCH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall ignore the write.
 */
static
void pci_rqmid_27739_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_Disabled_Config_Address(PCI_MSI_DATA_64, 2, 0xCFC);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Normal Write with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to write PCI data register and current guest PCI address
 * register value [bit 31] is 0H, ACRN hypervisor shall ignore the write.
 */
static
void pci_rqmid_27759_PCIe_config_space_and_host_Normal_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	int count = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_1, 4);
		reg_val_new = reg_val_ori ^ SHIFT_MASK(31, 0);
		pci_pdev_write_cfg_test(bdf, PCI_BASE_ADDRESS_1, 4, reg_val_new, 0xCFC, false);
		reg_val_new = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_1, 4);
		if (reg_val_ori == reg_val_new) {
			count++;
		}
		// Reset environment
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_1, 4, reg_val_ori);
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name:PCIe config space and host_1-Byte CFDH Write with Enabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFDH and current
 * guest PCI address register value [bit 31] is 1H, ACRN hypervisor shall write guest AL to
 * the second least significant byte of the guest configuration register located at current
 * guest PCI address register value.
 */
static
void pci_rqmid_27769_PCIe_config_space_and_host_1_Byte_CFDH_Write_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val_new_new = INVALID_REG_VALUE_U;
	uint32_t msi_addr = 0U;
	int count = 0;
	int ret = 0;
	// Message Address Register Format, SDM vol3.10.11.1
	// 31-20 19----------12 11-----4 3  2  1 0
	// 0FEEH Destination ID Reserved RH DM X X
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + PCI_MSI_ADDRESS_LO;
			reg_val_ori = pci_pdev_read_cfg(bdf, msi_addr, 4);

			// bit[15:8]--bit[11:8] is write-ignored, bit[15:12] is R/W.
			reg_val_new = SHIFT_RIGHT(reg_val_ori, 8) & SHIFT_MASK(7, 0);
			// NOT operation to bit[15:12]
			reg_val_new ^= 0xF0;
			pci_pdev_write_cfg_test(bdf, msi_addr + 1, 1, reg_val_new, 0xCFD, true);
			reg_val_new_new = pci_pdev_read_cfg(bdf, msi_addr, 4);
			reg_val_new_new = SHIFT_RIGHT(reg_val_new_new, 8) & SHIFT_MASK(7, 0);
			if (reg_val_new_new == reg_val_new) {
				count++;
			}
			// Reset environment
			pci_pdev_write_cfg(bdf, msi_addr, 4, reg_val_ori);
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name:PCIe config space and host 1-Byte CFFH Write with Enabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFFH and current
 * guest PCI address register value [bit 31] is 1H, ACRN hypervisor shall write guest AL
 * to the highest significant byte of the guest configuration register located at current
 * guest PCI address register value.
 */
static
void pci_rqmid_27767_PCIe_config_space_and_host_1_Byte_CFFH_Write_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	int count = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		pci_pdev_write_cfg_test(bdf, PCI_STATUS + 1, 1, 0xFF, 0xCFF, true);
		reg_val = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		reg_val >>= 8;
		reg_val &= 0xF0;
		if (reg_val == 0x00) {
			count++;
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte Config Data Port Write with Enabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 2 bytes to guest I/O port address CFCH and current
 * guest PCI address register value [bit 31] is 1H, ACRN hypervisor shall write guest AX to
 * the two least significant bytes of the guest configuration register located at current
 * guest PCI address register value.
 */
static
void pci_rqmid_27766_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Write_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t msi_offset = PCI_MSI_DATA_64;
	uint32_t bytes = 2;
	uint32_t port = 0xCFC;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	int msb = bytes * 8 - 1;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			reg_val_ori = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			reg_val_new = reg_val_ori ^ SHIFT_MASK(msb, 0);
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, reg_val_new, port, true);
			reg_val = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			// Message data[15,8] should be write ignored
			// Message data[7,0] is write/read
			reg_val &= SHIFT_MASK(msb, 0);
			reg_val &= SHIFT_MASK(7, 0);
			reg_val_new &= SHIFT_MASK(msb, 0);
			reg_val_new &= SHIFT_MASK(7, 0);
			if (reg_val_new == reg_val) {
				count++;
			}
			// Reset environment
			pci_pdev_write_cfg(bdf, msi_addr, bytes, reg_val_ori);
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte CFEH Write with Enabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 2 bytes to guest I/O port address CFEH and current guest
 * PCI address register value [bit 31] is 1H, ACRN hypervisor shall write guest AX to the two most
 * significant bytes of the guest configuration register located at current guest PCI address register value.
 */
static
void pci_rqmid_27765_PCIe_config_space_and_host_2_Byte_CFEH_Write_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t msi_offset = PCI_MSI_FLAGS;
	uint32_t bytes = 2;
	uint32_t port = 0xCFE;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	int msb = bytes * 8 - 1;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			reg_val_ori = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			reg_val_new = reg_val_ori ^ SHIFT_MASK(msb, 0);
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, reg_val_new, port, true);
			reg_val = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			reg_val &= SHIFT_MASK(msb, 0);
			// Message ctrl[15,1] should be write ignored
			reg_val &= SHIFT_UMASK(15, 1);

			reg_val_new &= SHIFT_MASK(msb, 0);
			// Message ctrl[15,1] should be write ignored
			reg_val_new &= SHIFT_UMASK(15, 1);
			if (reg_val_new == reg_val) {
				count++;
			}
			// Reset environment
			pci_pdev_write_cfg(bdf, msi_addr, bytes, reg_val_ori);
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte Config Data Port Write with Enabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFCH and current guest
 * PCI address register value [bit 31] is 1H, ACRN hypervisor shall write guest AL to the least
 * significant byte of the guest configuration register located at current guest PCI address register value.
 */
static
void pci_rqmid_27770_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Write_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_enabled_Config_Address(PCI_MSI_DATA_64, 1, 0xCFC);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFEH Write with Enabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFEH and current guest
 * PCI address register value [bit 31] is 1H, ACRN hypervisor shall write guest AL to the second
 * highest significant byte of the guest configuration register located at current guest PCI
 * address register value.
 */
static
void pci_rqmid_27768_PCIe_config_space_and_host_1_Byte_CFEH_Write_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t msi_offset = PCI_MSI_FLAGS;
	uint32_t bytes = 1;
	uint32_t port = 0xCFE;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	int msb = bytes * 8 - 1;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			reg_val_ori = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			reg_val_new = reg_val_ori ^ SHIFT_MASK(msb, 0);
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, reg_val_new, port, true);
			reg_val = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			reg_val &= SHIFT_MASK(msb, 0);
			// Message ctrl[7,1] should be write ignored
			reg_val &= SHIFT_UMASK(7, 1);

			reg_val_new &= SHIFT_MASK(msb, 0);
			// Message ctrl[7,1] should be write ignored
			reg_val_new &= SHIFT_UMASK(7, 1);
			if (reg_val_new == reg_val) {
				count++;
			}
			// Reset environment
			pci_pdev_write_cfg(bdf, msi_addr, bytes, reg_val_ori);
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Normal Write with Enabled Config Address_001
 *
 * Summary: When a vCPU attempts to write PCI data register and current guest PCI address
 * register value [bit 31] is 1H, ACRN hypervisor shall write guest EAX to the guest configuration
 * register located at current guest PCI address register value.
 */
static
void pci_rqmid_27771_PCIe_config_space_and_host_Normal_Write_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		reg_val_new = 0x80000000;
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, reg_val_new);
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		reg_val &= 0xFFFFFFF0;
		if (reg_val == reg_val_new) {
			count++;
		}
		// reset BAR0 address
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, reg_val_ori);
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte Config Data Port Read with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to read 2 bytes from guest I/O port address CFCH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall write FFFFH to guest AX.
 */
static
void pci_rqmid_27774_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + PCI_MSI_DATA_64;
			reg_val = pci_pdev_read_cfg_test(bdf, PCI_MSI_DATA_64, 2, 0xCFC, false);
			if (0xFFFF == reg_val) {
				count++;
			}
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Normal Config Data Port Read with Disabled Config Address_001
 *
 * Summary:When a vCPU attempts to read PCI data register and current guest PCI address register
 * value [bit 31] is 0H, ACRN hypervisor shall write FFFFFFFFH to guest EAX.
 */
static
void pci_rqmid_27780_PCIe_config_space_and_host_Normal_Config_Data_Port_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg_test(bdf, PCI_BASE_ADDRESS_0, 4, 0xCFC, false);
		if (reg_val ==  0xFFFFFFFFU) {
			count++;
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFFH Read with Disabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFFH and current guest
 * PCI address register value [bit 31] is 0H, ACRN hypervisor shall write FFH to guest AL.
 */
static
void pci_rqmid_27775_PCIe_config_space_and_host_1_Byte_CFFH_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg_test(bdf, PCI_STATUS + 1, 1, 0xCFF, false);
		if (reg_val ==  0xFF) {
			count++;
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFDH Read with Disabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFDH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall write FFH to guest AL.
 */
static
void pci_rqmid_27778_PCIe_config_space_and_host_1_Byte_CFDH_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + PCI_MSI_ADDRESS_LO;
			reg_val = pci_pdev_read_cfg_test(bdf, msi_addr + 1, 1, 0xCFD, false);
			if (reg_val ==  0xFF) {
				count++;
			}
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte CFEH Read with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to read 2 bytes from guest I/O port address CFEH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall write FFFFH to guest AX.
 */
static
void pci_rqmid_27773_PCIe_config_space_and_host_2_Byte_CFEH_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + PCI_MSI_FLAGS;
			reg_val = pci_pdev_read_cfg_test(bdf, msi_addr, 2, 0xCFE, false);
			if (reg_val ==	0xFFFF) {
				count++;
			}
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte Config Data Port Read with Disabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFCH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall write FFH to guest AL.
 */
static
void pci_rqmid_27779_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + PCI_MSI_DATA_64;
			reg_val = pci_pdev_read_cfg_test(bdf, msi_addr, 1, 0xCFC, false);
			if (reg_val ==	0xFF) {
				count++;
			}
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFEH Read with Disabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFEH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall write FFH to guest AL.
 */
static
void pci_rqmid_27776_PCIe_config_space_and_host_1_Byte_CFEH_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + PCI_MSI_FLAGS;
			reg_val = pci_pdev_read_cfg_test(bdf, msi_addr, 1, 0xCFE, false);
			if (reg_val ==	0xFF) {
				count++;
			}
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFFH Read with Enabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFFH and current
 * guest PCI address register value [bit 31] is 1H, ACRN hypervisor shall write the highest
 * significant byte of the guest configuration register located at current guest PCI address
 * register value to guest AL.
 */
static
void pci_rqmid_27785_PCIe_config_space_and_host_1_Byte_CFFH_Read_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		if (ret == OK) {
			pci_pdev_write_cfg_test(bdf, PCI_STATUS + 1, 1, 0xFF, 0xCFF, true);
			reg_val = pci_pdev_read_cfg_test(bdf, PCI_STATUS + 1, 1, 0xCFF, true);
			reg_val &= 0xF0;
			if (reg_val ==	0x00U) {
				count++;
			}
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFEH Read with Enabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFEH and current guest
 * PCI address register value [bit 31] is 1H,ACRN hypervisor shall write the highest significant
 * byte of the guest configuration register located at current guest PCI address
 * register value to guest AL.
 */
static
void pci_rqmid_27786_PCIe_config_space_and_host_1_Byte_CFEH_Read_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t msi_offset = PCI_MSI_FLAGS;
	uint32_t bytes = 1;
	uint32_t port = 0xCFE;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	int msb = bytes * 8 - 1;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			reg_val_ori = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			reg_val_new = reg_val_ori ^ SHIFT_MASK(msb, 0);
			pci_pdev_write_cfg(bdf, msi_addr, bytes, reg_val_new);
			reg_val = pci_pdev_read_cfg_test(bdf, msi_addr, bytes, port, true);
			reg_val &= SHIFT_MASK(msb, 0);
			// Message ctrl[7,1] should be write ignored
			reg_val &= SHIFT_UMASK(7, 1);

			reg_val_new &= SHIFT_MASK(msb, 0);
			// Message ctrl[3,1] should be write ignored
			reg_val_new &= SHIFT_UMASK(7, 1);
			if (reg_val_new == reg_val) {
				count++;
			}
			// Reset environment
			pci_pdev_write_cfg(bdf, msi_addr, bytes, reg_val_ori);
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Normal Config Data Port Read with Enabled Config Address_001
 *
 * Summary:When a vCPU attempts to read PCI data register and current guest PCI address register
 * value [bit 31] is 1H, ACRN hypervisor shall write the value of the guest configuration register
 * located at current guest PCI address register value to guest EAX.
 */
static
void pci_rqmid_27789_PCIe_config_space_and_host_Normal_Config_Data_Port_Read_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		reg_val_new = 0x80000000U;
		pci_pdev_write_cfg_test(bdf, PCI_BASE_ADDRESS_0, 4, reg_val_new, 0xCFC, true);
		reg_val = pci_pdev_read_cfg_test(bdf, PCI_BASE_ADDRESS_0, 4, 0xCFC, true);
		reg_val &= 0xFFFFFFF0U;
		if (reg_val ==	reg_val_new) {
			count++;
		}
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, reg_val_ori);
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte CFEH Read with Enabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 2 bytes from guest I/O port address CFEH and current guest
 * PCI address register value [bit 31] is 1H, ACRN hypervisor shall write the two most significant
 * bytes of the guest configuration register located at current guest PCI address register value to guest AX.
 */
static
void pci_rqmid_27783_PCIe_config_space_and_host_2_Byte_CFEH_Read_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t msi_offset = PCI_MSI_FLAGS;
	uint32_t bytes = 2;
	uint32_t port = 0xCFE;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	int msb = bytes * 8 - 1;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			reg_val_ori = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			reg_val_new = reg_val_ori ^ SHIFT_MASK(msb, 0);
			pci_pdev_write_cfg(bdf, msi_addr, bytes, reg_val_new);
			reg_val = pci_pdev_read_cfg_test(bdf, msi_addr, bytes, port, true);
			reg_val &= SHIFT_MASK(msb, 0);
			// Message ctrl[15,1] should be write ignored
			reg_val &= SHIFT_UMASK(15, 1);

			reg_val_new &= SHIFT_MASK(msb, 0);
			// Message ctrl[3,1] should be write ignored
			reg_val_new &= SHIFT_UMASK(15, 1);
			if (reg_val_new == reg_val) {
				count++;
			}
			// Reset environment
			pci_pdev_write_cfg(bdf, msi_addr, bytes, reg_val_ori);
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name:PCIe config space and host_1-Byte Config Data Port Read with Enabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFCH and current guest
 * PCI address register value [bit 31] is 1H,ACRN hypervisor shall write the highest significant
 * byte of the guest configuration register located at current guest PCI address register value to guest AL.
 */
static
void pci_rqmid_27788_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Read_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Read_with_Enabled_Config_Address(PCI_MSI_DATA_64, 1, 0xCFC);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte Config Data Port Read with Enabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 2 bytes from guest I/O port address CFCH and current guest
 * PCI address register value [bit 31] is 1H, ACRN hypervisor shall write the two least significant
 * bytes of the guest configuration register located at current guest PCI address register value to guest AX.
 */
static
void pci_rqmid_27784_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Read_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t msi_offset = PCI_MSI_DATA_64;
	uint32_t bytes = 2;
	uint32_t port = 0xCFC;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	int msb = bytes * 8 - 1;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			reg_val_ori = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			reg_val_new = reg_val_ori ^ SHIFT_MASK(msb, 0);
			pci_pdev_write_cfg(bdf, msi_addr, bytes, reg_val_new);
			reg_val = pci_pdev_read_cfg_test(bdf, msi_addr, bytes, port, true);
			// Message data[15,8] should be write ignored
			// Message data[7,0] is write/read
			reg_val &= SHIFT_MASK(msb, 0);
			reg_val &= SHIFT_MASK(7, 0);
			reg_val_new &= SHIFT_MASK(msb, 0);
			reg_val_new &= SHIFT_MASK(7, 0);
			if (reg_val_new == reg_val) {
				count++;
			}
			// Reset environment
			pci_pdev_write_cfg(bdf, msi_addr, bytes, reg_val_ori);
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFDH Read with Enabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFDH and current
 * guest PCI address register value [bit 31] is 1H,ACRN hypervisor shall write the highest
 * significant byte of the guest configuration register located at current guest PCI address
 * register value to guest AL.
 */
static
void pci_rqmid_27787_PCIe_config_space_and_host_1_Byte_CFDH_Read_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t port = 0xCFD;
	uint32_t msi_offset = PCI_MSI_ADDRESS_LO + 1;
	uint32_t bytes = 1;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	int msb = bytes * 8 - 1;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			reg_val_ori = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			reg_val_new = reg_val_ori ^ SHIFT_MASK(msb, 0);
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, reg_val_new, port, true);
			reg_val = pci_pdev_read_cfg_test(bdf, msi_addr, bytes, port, true);
			reg_val &= SHIFT_MASK(msb, 0);
			reg_val_new &= SHIFT_MASK(msb, 0);
			// message address bit[11:8] is reserved and write-ignored
			// message address bit[15:12] is write/read
			reg_val &= SHIFT_MASK(15, 12);
			reg_val_new &= SHIFT_MASK(15, 12);
			if (reg_val ==	reg_val_new) {
				count++;
			}
			pci_pdev_write_cfg(bdf, msi_addr, bytes, reg_val_ori);
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Normal Config Address Port Write_001
 *
 * Summary:When a vCPU attempts to write guest PCI address register, ACRN hypervisor
 * shall keep the logical and of guest EAX and 80FFFFFCH as the current guest PCI address register value.
 */
static
void pci_rqmid_30960_PCIe_config_space_and_host_Normal_Config_Address_Port_Write_001(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		pci_pdev_write_cfg(bdf, 0x8C, 2, SHIFT_LEFT(0x1, 6));
		reg_val = pci_pdev_read_cfg(bdf, 0x8C, 2);
		if (reg_val == SHIFT_LEFT(0x1, 6)) {
			is_pass = true;
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte Config Address Port Write_001
 *
 * Summary:When a vCPU attempts to write 2 bytes to guest I/O port address CF8H,
 * ACRN hypervisor shall ignore the write.
 */
static
void pci_rqmid_27790_PCIe_config_space_and_host_2_Byte_Config_Address_Port_Write_001(void)
{
	bool is_pass = false;
	uint32_t addr0 = 0U;
	uint32_t addr1 = 0U;
	uint16_t value = 0;
	addr0 = pio_read32((uint16_t)PCI_CONFIG_ADDR);
	// NOT operation to original value
	value = (uint16_t)(addr0 ^ SHIFT_MASK(15, 0));
	pio_write16(value, (uint16_t)PCI_CONFIG_ADDR);
	addr1 = pio_read32((uint16_t)PCI_CONFIG_ADDR);
	is_pass = (addr0 == addr1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte Config Address Port Write_001
 *
 * Summary:When a vCPU attempts to write 1 byte to guest I/O port address CF8H,
 * ACRN hypervisor shall ignore the write.
 */
static
void pci_rqmid_27791_PCIe_config_space_and_host_1_Byte_Config_Address_Port_Write_001(void)
{
	bool is_pass = false;
	uint32_t addr0 = 0U;
	uint32_t addr1 = 0U;
	uint8_t value = 0;
	addr0 = pio_read32((uint16_t)PCI_CONFIG_ADDR);
	// NOT operation to original value
	value = (uint8_t)(addr0 ^ SHIFT_MASK(7, 0));
	pio_write8(value, (uint16_t)PCI_CONFIG_ADDR);
	addr1 = pio_read32((uint16_t)PCI_CONFIG_ADDR);
	is_pass = (addr0 == addr1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Normal Config Address Port Read_001
 *
 * Summary:When a vCPU attempts to read guest PCI address register,
 * ACRN hypervisor shall write the current guest address register value to guest EAX.
 */
static
void pci_rqmid_31161_PCIe_config_space_and_host_Normal_Config_Address_Port_Read_001(void)
{
	bool is_pass = false;
	int count = 0;
	uint32_t addr = 0U;
	uint32_t addr1 = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		addr = pci_pdev_calc_address(bdf, PCI_BASE_ADDRESS_0);
		pio_write32(addr, (uint16_t)PCI_CONFIG_ADDR);
		asm volatile("mov %%eax, %0\n\t" : "=m" (addr1) : : "memory");
		pio_read32((uint16_t)PCI_CONFIG_DATA);
		if (addr == addr1) {
			count++;
		}
	}
	is_pass = (count == 1) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe_config_space_and_host_2-Byte_Config_Address_Port_Read_001
 *
 * Summary:When a vCPU attempts to read 2 bytes from guest I/O port address CF8H,
 * ACRN hypervisor shall write FFH to guest AX.
 */
static
void pci_rqmid_27792_PCIe_config_space_and_host_2_Byte_Config_Address_Port_Read_001(void)
{
	bool is_pass = false;
	uint16_t addr = 0;
	addr = pio_read16((uint16_t)PCI_CONFIG_ADDR);
	is_pass = (addr == 0xFFFF) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe_config_space_and_host_1-Byte_Config_Address_Port_Read_001
 *
 * Summary:When a vCPU attempts to read 1 bytes from guest I/O port address CF8H,
 * ACRN hypervisor shall write FFH to guest AL.
 */
static
void pci_rqmid_27793_PCIe_config_space_and_host_1_Byte_Config_Address_Port_Read_001(void)
{
	bool is_pass = false;
	uint8_t addr = 0;
	addr = pio_read8((uint16_t)PCI_CONFIG_ADDR);
	is_pass = (addr == 0xFF) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Config Data Port_001
 *
 * Summary:ACRN hypervisor shall locate the PCI data register of any VM
 * at I/O port address CFCH-CFFH.
 */
static
void pci_rqmid_25099_PCIe_config_space_and_host_Config_Data_Port_001(void)
{
	bool is_pass = false;
	int count = 0;
	union pci_bdf bdf = {0};
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_DEVICE_ID, 2);
		if (PCI_USB_DEVICE_ID == reg_val) {
			count++;
		}
		DBG_INFO("%s: dev_id 0x%x\n", __func__, reg_val);
		reg_val = pci_pdev_read_cfg(bdf, PCI_REVISION_ID, 1);
		if (PCI_USB_REVISION_ID == reg_val) {
			count++;
		}
		DBG_INFO("%s: rev_id 0x%x, PCI_USB_REVISION_ID 0x%x\n", __func__, reg_val, PCI_USB_REVISION_ID);
		// Read BAR0
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		// BAR0 value 0x80000000
		reg_val_new = 0x80000000;
		// Write a new value to BAR0
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, reg_val_new);
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		reg_val &= 0xFFFFFFF0;
		DBG_INFO("%s: bar0 0x%x, input 0x%x\n", __func__, reg_val, reg_val_new);
		if (reg_val == reg_val_new) {
			count++;
		}
		// Reset environment
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, reg_val_ori);

	}
	is_pass = (count == 3) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name:PCIe config space and host_Config Address Port_001
 *
 * Summary:ACRN hypervisor shall locate the PCI address register of any
 * VM at I/O port address CF8H.
 */
static
void pci_rqmid_25292_PCIe_config_space_and_host_Config_Address_Port_001(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	uint32_t reg_val1 = 0U;
	union pci_bdf bdf = {0};
	int count = 0;
	int i = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		for (i = 0; i < 4; i++) {
			reg_val = pci_pdev_calc_address(bdf, PCI_BASE_ADDRESS_0 + i);
			// Write address to ADDRESS register --0xCF8
			pio_write32(reg_val, (uint16_t)PCI_CONFIG_ADDR);
			reg_val1 = pio_read32((uint16_t)PCI_CONFIG_ADDR);
			// Address port CF8: bit[1:0] is write-ignored
			if ((reg_val & SHIFT_UMASK(1, 0)) == reg_val1) {
				count++;
			}
		}
	}
	is_pass = (count == 4) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-address destination ID read-write_002
 *
 * Summary:Message address register [bit 19:12] of guest PCI configuration MSI capability
 * structure shall be a read-write bits field.
 */
static
void pci_rqmid_28884_PCIe_config_space_and_host_MSI_message_address_destination_ID_read_write_002(void)
{
	bool is_pass = false;
	is_pass = test_MSI_message_address_destination_field_rw(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-data trigger mode read_001
 *
 * Summary:When a vCPU attempts to read a guest message data register [bit 15] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the vCPU gets 0H.
 */
static
void pci_rqmid_28908_PCIe_config_space_and_host_MSI_message_data_trigger_mode_read_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_data_trigger_mode_read(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe_config_space_and_host_MSI message-data trigger mode write_001
 *
 * Summary:When a vCPU attempts to write a guest message data register [bit 15:14]
 * of the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor
 * shall guarantee that the write to the bits is ignored.
 */
static
void pci_rqmid_28888_PCIe_config_space_and_host_MSI_message_data_trigger_mode_write_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_data_trigger_mode_write(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-data delivery mode read_001
 *
 * Summary:When a vCPU attempts to read a guest message data register [bit 10:8] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the vCPU gets 0H.
 */
static
void pci_rqmid_28911_PCIe_config_space_and_host_MSI_message_data_delivery_mode_read_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_data_delivery_mode_read(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-data delivery mode write_001
 *
 * Summary:When a vCPU attempts to write a guest message data register [bit 10:8]
 * of the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor
 * shall guarantee that the write to the bits can take effect.
 */
static
void pci_rqmid_28892_PCIe_config_space_and_host_MSI_message_data_delivery_mode_write_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_data_delivery_mode_write(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-address interrupt message address read_001
 *
 * Summary:When a vCPU attempts to read a guest message address register [bit 64:20]
 * of the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the vCPU gets 0FEEH.
 */
static
void pci_rqmid_28914_PCIe_config_space_and_host_MSI_message_address_interrupt_message_address_read_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_address_interrupt_message_address_read(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-address interrupt message address write_001
 *
 * Summary:When a vCPU attempts to write a guest message address register [bit 64:20] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the write to the bits is ignored.
 */
static
void pci_rqmid_28894_PCIe_config_space_and_host_MSI_message_address_interrupt_message_address_write_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_address_int_msg_addr_write(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name:PCIe config space and host_MSI message-address redirection hint read_001
 *
 * Summary:When a vCPU attempts to read a guest message address register [bit 3:0] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the vCPU gets 0H.
 */
static
void pci_rqmid_28917_PCIe_config_space_and_host_MSI_message_address_redirection_hint_read_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_address_redirection_hint_read(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-address redirection hint write_001
 *
 * Summary:When a vCPU attempts to write a guest message address register [bit 1:0] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the write to the bits is ignored.
 */
static
void pci_rqmid_28896_PCIe_config_space_and_host_MSI_message_address_redirection_hint_write_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_address_redirection_hint_write(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-control read_001
 *
 * Summary:When a vCPU attempts to read a guest message control register [bit 15:1]
 * of the PCI MSI capability structure of an assigned PCIe device,
 * ACRN hypervisor shall guarantee that the vCPU gets 40H.
 */
static
void pci_rqmid_28918_PCIe_config_space_and_host_MSI_message_control_read_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_control_read(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-control write_001
 *
 * Summary:When a vCPU attempts to write a guest message control register [bit 15:1]
 * of the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor
 * shall guarantee that the write to the bits is ignored.
 */
static
void pci_rqmid_28899_PCIe_config_space_and_host_MSI_message_control_write_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_control_write(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI capability structure_001
 *
 * Summary:ACRN hypervisor shall expose PCIe MSI capability structure of any PCIe device
 * to the VM which the PCIe device is assigned to, in compliance with Chapter 7.7, PCIe BS and Chapter 6.8, PCI LBS.
 */
static
void pci_rqmid_26821_PCIe_config_space_and_host_MSI_capability_structure_001(void)
{
	/*
	 *This case is to check MSI whole structure, which is:
	 *SDM vol3.chpater11.11.1 and PCIe 5.0 chapter 7.7.1
	 */
	int ret = OK;
	int ret1 = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	uint32_t msi_ctrl = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		if (ret == OK) {
			// Read MSI_CTRL
			msi_ctrl = pci_pdev_read_cfg(bdf, reg_addr + 2, 2);
			reg_val_ori = msi_ctrl;
			reg_val_new = reg_val_ori ^ SHIFT_MASK(3, 1);
			pci_pdev_write_cfg(bdf, reg_addr + 2, 2, reg_val_new);
			reg_val_new = pci_pdev_read_cfg(bdf, reg_addr + 2, 2);
			ret |= pci_data_check(bdf, reg_addr + 2, 2, \
					reg_val_ori, reg_val_new, false);
			if (ret != OK)
				DBG_ERRO("reg[%xH] val_ori=0x%x val_new=0x%x\n", reg_addr + 2, reg_val_ori, reg_val_new);

			reg_val_new = reg_val_ori ^ SHIFT_MASK(15, 11);
			pci_pdev_write_cfg(bdf, reg_addr + 2, 2, reg_val_new);
			reg_val_new = pci_pdev_read_cfg(bdf, reg_addr + 2, 2);
			ret |= pci_data_check(bdf, reg_addr + 2, 2, \
					reg_val_ori, reg_val_new, false);
			if (ret != OK)
				DBG_ERRO("reg[%xH] val_ori=0x%x val_new=0x%x\n", reg_addr + 2, reg_val_ori, reg_val_new);

			// for MSI base
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			/* Mask out the Next Capability Pointer which should not be checked */
			reg_val &= 0xFFFF00FF;
			ret |= pci_data_check(bdf, reg_addr, 4,\
					(SHIFT_LEFT(SHIFT_LEFT(0x40, 1), 16) | 0x05), reg_val, false);
			if (ret != OK)
				DBG_ERRO("%s: 1-reg[%xH] = [%xH]\n", __func__, reg_addr, reg_val);

			// for message-address
			reg_addr += 4;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
			reg_val = reg_val & (SHIFT_MASK(31, 20));
			ret1 = pci_data_check(bdf, reg_addr, 4, 0xFEE00000, reg_val, false);
			ret |= ret1;
			if (ret1 != OK)
				DBG_ERRO("%s: 2-reg[%xH] = [%xH]\n", __func__, reg_addr, reg_val);
			// for message-addres-upper
			if (msi_ctrl & SHIFT_LEFT(1, 7)) {
				reg_addr += 4;
				reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
				// Message address upper bit[63, 32] should be 0H and write-ignored
				ret1 = pci_data_check(bdf, reg_addr, 4, 0x00000000, reg_val, false);
				ret |= ret1;
				if (ret1 != OK)
					DBG_ERRO("%s: 3-reg[%xH] = [%xH]\n", __func__, reg_addr, reg_val);
				DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
			}

			// for message-data
			reg_addr += 4;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			reg_val = reg_val & (SHIFT_MASK(15, 15) | SHIFT_MASK(10, 8));
			ret1 = pci_data_check(bdf, reg_addr, 2, 0x0000, reg_val, false);
			ret |= ret1;
			DBG_INFO("R reg[%xH] = [%xH]\n", reg_addr, reg_val);
			if (ret1 != OK)
				DBG_ERRO("%s: 4-reg[%xH] = [%xH]\n", __func__, reg_addr, reg_val);
		}
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI capability structure_002
 *
 * Summary:ACRN hypervisor shall expose PCIe MSI capability structure of any PCIe device
 * to the VM which the PCIe device is assigned to, in compliance with Chapter 7.7, PCIe BS and Chapter 6.8, PCI LBS.
 */
static
void pci_rqmid_26823_PCIe_config_space_and_host_MSI_capability_structure_002(void)
{
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	uint32_t msi_ctrl = INVALID_REG_VALUE_U;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		if (ret == OK) {
			// Read MSI_CTRL
			msi_ctrl = pci_pdev_read_cfg(bdf, reg_addr + 2, 2);
			// for message-address
			reg_addr += 4;
			/*Write a desination ID to Message address[19:12],
			 *and other bits of Message address are write-ignored
			 */
			// Read original Value
			reg_val_ori = pci_pdev_read_cfg(bdf, reg_addr, 4);
			// Not operation to destination ID
			reg_val_new = reg_val_ori ^ SHIFT_MASK(19, 12);
			// Do write
			pci_pdev_write_cfg(bdf, reg_addr, 4, reg_val_new);
			// Read new Value
			reg_val_new = pci_pdev_read_cfg(bdf, reg_addr, 4);
			// Compare
			ret |= pci_data_check(bdf, reg_addr,\
				4, (reg_val_ori ^ SHIFT_MASK(19, 12)), reg_val_new, false);
			// for message-data
			if (msi_ctrl & SHIFT_LEFT(1, 7)) {
				/*The offset is 0x8 relative to last addr,
				 *if message address is 64-bit width.
				 */
				reg_addr += 0x8;
			} else {
				/*The offset is 0x4 relative to last addr,
				 *if message address is 32-bit width.
				 */
				reg_addr += 0x4;
			}
			/*Write a interrupt vector to Message data[7:0],
			 *and Message data[15:8] is write-ignored.
			 */
			// Read original Value
			reg_val_ori = pci_pdev_read_cfg(bdf, reg_addr, 2);
			// Not operation to Vector
			reg_val_new = reg_val_ori ^ SHIFT_MASK(7, 0);
			// Do write
			pci_pdev_write_cfg(bdf, reg_addr, 2, reg_val_new);
			// Read New Value
			reg_val_new = pci_pdev_read_cfg(bdf, reg_addr, 2);
			// Compare
			ret |= pci_data_check(bdf, reg_addr, 2, \
					(reg_val_ori ^ SHIFT_MASK(7, 0)), reg_val_new, false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_capability ID register of the PCI MSI capability_USB_001
 *
 * Summary:When a vCPU attempts to read a guest capability ID register of the PCI MSI capability structure
 * of an assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU gets 5H.
 */
static
void pci_rqmid_38091_PCIe_config_space_and_host_capability_ID_register_of_the_PCI_MSI_capability_USB_001(void)
{
	bool is_pass = false;
	uint32_t reg_value1 = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_value1 = pci_pdev_read_cfg(bdf, USB_MSI_REG_OFFSET, 1);
		is_pass = (reg_value1 == MSI_CAPABILITY_ID) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_PCI configuration header register_USB_001
 *
 * Summary:Guarantee that any guest register in the PCI configuration header in the following is read-only
 */
static
void pci_rqmid_38095_PCIe_config_space_and_host_PCI_configuration_header_register_USB_001(void)
{
	bool is_pass = false;
	is_pass = test_host_PCI_configuration_header_register_read_only(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_msi_control_register_following_start_up_USB_001
 *
 * Summary:ACRN hypervisor shall set initial guest PCI MSI control register [bit 0] to 0H following start-up.
 */
static
void pci_rqmid_38101_PCIe_config_space_and_host_msi_control_register_following_start_up_USB_001(void)
{
	bool is_pass = false;
	uint32_t bp_usb_msi_ctrl = 0U;
	bool is_usb_exist = false;
	is_usb_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_usb_exist) {
		bp_usb_msi_ctrl = *(uint32_t *)(BP_USB_MSI_CTRL_ADDR);
		bp_usb_msi_ctrl >>= 16;
		bp_usb_msi_ctrl &= SHIFT_LEFT(0x1, 0);
		DBG_INFO("%s: bp_usb_msi_ctrl 0x%x, BP_USB_MSI_CTRL_ADDR 0x%x\n", __func__, bp_usb_msi_ctrl, BP_USB_MSI_CTRL_ADDR);
		is_pass = (bp_usb_msi_ctrl == 0U) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_PCI command register_following_start_up_USB_001
 *
 * Summary:ACRN hypervisor shall set initial guest PCI command register bit[2] to 0H following start-up.
 */
static
void pci_rqmid_38105_PCIe_config_space_and_host_PCI_command_register_following_start_up_USB_001(void)
{
	bool is_pass = false;
	bool is_usb_exist = false;
	uint32_t bp_command = 0U;
	is_usb_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_usb_exist) {
		bp_command = *(uint32_t *)(BP_USB_STATUS_COMMAND_ADDR);
		bp_command &= SHIFT_MASK(2, 2);
		DBG_INFO("%s: bp_command 0x%x\n", __func__, bp_command);
		is_pass = (bp_command == BP_START_UP_COMMAND_VALUE) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

static int pci_probe_msix_capability(union pci_bdf bdf, uint32_t *msix_addr)
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
	 * pci standard config space [00h,100h).So,MSI-X register must be in [00h,100h),
	 * if there is MSI-X register
	 */
	repeat = PCI_CONFIG_SPACE_SIZE / 4;
	for (i = 0; i < repeat; i++) {
		// Find MSI-X register group
		if (PCI_CAP_ID_MSIX == (reg_val & SHIFT_MASK(7, 0))) {
			break;
		}
		reg_addr = (reg_val & SHIFT_MASK(15, 8)) >> 8;
		reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
		// DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
	}
	if (repeat == i) {
		DBG_ERRO("no msi-x cap found!!!");
		return ERROR;
	}
	*msix_addr = reg_addr;
	return OK;
}

static
void pci_PCIe_config_space_and_host_MSIX_capability_structure_001(void)
{
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	uint32_t msix_ctrl = INVALID_REG_VALUE_U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_val_new = INVALID_REG_VALUE_U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, IVSHMEM_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msix_capability(bdf, &reg_addr);
		if (ret == OK) {
			// Read MSI_CTRL
			msix_ctrl = pci_pdev_read_cfg(bdf, reg_addr + 2, 2);
			reg_val_ori = msix_ctrl;
			reg_val_new = reg_val_ori ^ SHIFT_MASK(13, 0);
			pci_pdev_write_cfg(bdf, reg_addr + 2, 2, reg_val_new);
			reg_val_new = pci_pdev_read_cfg(bdf, reg_addr + 2, 2);
			ret |= pci_data_check(bdf, reg_addr + 2, 2, \
					reg_val_ori, reg_val_new, false);
			if (ret != OK)
				DBG_ERRO("reg[%xH] val_ori=0x%x val_new=0x%x\n", reg_addr + 2, reg_val_ori, reg_val_new);

			// for MSI-X base
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 1);
			ret |= pci_data_check(bdf, reg_addr, 4, 0x11, reg_val, false);
			if (ret != OK)
				DBG_ERRO("%s: 1-reg[%xH] = [%xH]\n", __func__, reg_addr, reg_val);

			// for MSI-X table
			reg_addr += 4;
			reg_val_ori = pci_pdev_read_cfg(bdf, reg_addr, 4);
			DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
			reg_val_new = reg_val_ori ^ SHIFT_MASK(31, 0);
			pci_pdev_write_cfg(bdf, reg_addr, 4, reg_val_new);
			reg_val_new = pci_pdev_read_cfg(bdf, reg_addr, 4);
			ret |= pci_data_check(bdf, reg_addr, 4, \
					reg_val_ori, reg_val_new, false);
			if (ret != OK)
				DBG_ERRO("reg[%xH] val_ori=0x%x val_new=0x%x\n", reg_addr, reg_val_ori, reg_val_new);

			/* As MSI-X table entry access is same as BAR which has been verified, no need to do it. */
		}
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

static
void pci_PCIe_config_space_and_host_msix_control_register_following_start_up_IVSHMEM_001(void)
{
	bool is_pass = false;
	uint32_t bp_msix_ctrl = 0U;
	bool is_ivshmem_exist = false;
	is_ivshmem_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, IVSHMEM_DEV_VENDOR);
	if (is_ivshmem_exist) {
		bp_msix_ctrl = *(uint32_t *)(BP_IVSHMEM_MSIX_CTRL_ADDR);
		bp_msix_ctrl >>= 16;
		bp_msix_ctrl &= SHIFT_LEFT(0x1, 15);
		DBG_INFO("bp_msix_ctrl 0x%x, expect 0, BP_IVSHMEM_MSIX_CTRL_ADDR 0x%x\n", bp_msix_ctrl, *(uint32_t *)BP_IVSHMEM_MSIX_CTRL_ADDR);
		printf("bp_msix_ctrl 0x%x, expect 0, BP_IVSHMEM_MSIX_CTRL_ADDR 0x%x\n", bp_msix_ctrl, *(uint32_t *)BP_IVSHMEM_MSIX_CTRL_ADDR);
		is_pass = (bp_msix_ctrl == 0x8000U || bp_msix_ctrl == 0x0U) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

#ifdef IN_SAFETY_VM
static int pci_probe_ats_capability(union pci_bdf bdf, uint32_t *ats_addr)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	int i = 0;
	int repeat = 0;
	reg_addr = PCI_EXP_CAP_LIST;
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
	DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
	/*
	 * pcie extended config space [100h,FFFh).So,ATS register must be in [100h,FFFh),
	 * if there is ATS register
	 */
	repeat = PCIE_CONFIG_SPACE_SIZE_LOCAL / 4;
	for (i = 0; i < repeat; i++) {
		// Find ATS register group
		if (PCI_EXT_CAP_ID_ATS == (reg_val & SHIFT_MASK(15, 0))) {
			break;
		}
		reg_addr = (reg_val & SHIFT_MASK(31, 20)) >> 20;
		reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
		DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
	}
	if (repeat == i) {
		DBG_INFO("no ats cap found!!!");
		return ERROR;
	}
	*ats_addr = reg_addr;
	return OK;
}

static
void pci_PCIe_config_space_and_host_ATS_capability_structure_001(void)
{
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, VGA_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_ats_capability(bdf, &reg_addr);
		is_pass = (ret == OK) ? false : true;
	}
	report("%s", is_pass, __FUNCTION__);
}

static int pci_probe_pri_capability(union pci_bdf bdf, uint32_t *pri_addr)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	int i = 0;
	int repeat = 0;
	reg_addr = PCI_EXP_CAP_LIST;
	reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
	DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
	/*
	 * pcie extended config space [100h,FFFh).So,PRI register must be in [100h,FFFh),
	 * if there is ATS register
	 */
	repeat = PCIE_CONFIG_SPACE_SIZE_LOCAL / 4;
	for (i = 0; i < repeat; i++) {
		// Find PRI register group
		if (PCI_EXT_CAP_ID_PRI == (reg_val & SHIFT_MASK(15, 0))) {
			break;
		}
		reg_addr = (reg_val & SHIFT_MASK(31, 20)) >> 20;
		reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
		DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
	}
	if (repeat == i) {
		DBG_INFO("no pri cap found!!!");
		return ERROR;
	}
	*pri_addr = reg_addr;
	return OK;
}

static
void pci_PCIe_config_space_and_host_PRI_capability_structure_001(void)
{
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, VGA_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_pri_capability(bdf, &reg_addr);
		is_pass = (ret == OK) ? false : true;
	}
	report("%s", is_pass, __FUNCTION__);
}
#endif

int main(void)
{
	setup_idt();
	setup_vm();
	set_log_level(PCI_DEBUG_LEVEL);

#ifdef __x86_64__
	// Enumerate PCI devices to pci_devs
	pci_pdev_enumerate_dev(pci_devs, &nr_pci_devs);
	if (nr_pci_devs == 0) {
		DBG_ERRO("pci_pdev_enumerate_dev finds no devs!");
		return 0;
	}
	printf("\nFind PCI devs nr_pci_devs = %d \n", nr_pci_devs);

	/* Hostbridge */
	pci_PCIe_config_space_and_host_Hostbridge_XBAR();
#ifdef IN_SAFETY_VM
	pci_rqmid_27486_PCIe_config_space_and_host_Hostbridge_readonly_001();    /* TODO: need change hypervisor */
	pci_rqmid_27474_PCIe_config_space_and_host_Hostbridge_Header_Type_001();
	pci_rqmid_27475_PCIe_config_space_and_host_Hostbridge_Class_Code_001();
#endif

	/* PCIe_IO port access */
	pci_rqmid_25292_PCIe_config_space_and_host_Config_Address_Port_001();
	pci_rqmid_25099_PCIe_config_space_and_host_Config_Data_Port_001();

	pci_rqmid_27747_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27739_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27744_PCIe_config_space_and_host_1_Byte_CFDH_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27742_PCIe_config_space_and_host_1_Byte_CFEH_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27735_PCIe_config_space_and_host_2_Byte_CFEH_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27741_PCIe_config_space_and_host_1_Byte_CFFH_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27759_PCIe_config_space_and_host_Normal_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27778_PCIe_config_space_and_host_1_Byte_CFDH_Read_with_Disabled_Config_Address_001();
	pci_rqmid_27776_PCIe_config_space_and_host_1_Byte_CFEH_Read_with_Disabled_Config_Address_001();
	pci_rqmid_27773_PCIe_config_space_and_host_2_Byte_CFEH_Read_with_Disabled_Config_Address_001();
	pci_rqmid_27775_PCIe_config_space_and_host_1_Byte_CFFH_Read_with_Disabled_Config_Address_001();
	pci_rqmid_27779_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Read_with_Disabled_Config_Address_001();
	pci_rqmid_27774_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Read_with_Disabled_Config_Address_001();
	pci_rqmid_27780_PCIe_config_space_and_host_Normal_Config_Data_Port_Read_with_Disabled_Config_Address_001();

	pci_rqmid_27769_PCIe_config_space_and_host_1_Byte_CFDH_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27768_PCIe_config_space_and_host_1_Byte_CFEH_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27767_PCIe_config_space_and_host_1_Byte_CFFH_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27770_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27765_PCIe_config_space_and_host_2_Byte_CFEH_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27766_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27771_PCIe_config_space_and_host_Normal_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27787_PCIe_config_space_and_host_1_Byte_CFDH_Read_with_Enabled_Config_Address_001();
	pci_rqmid_27786_PCIe_config_space_and_host_1_Byte_CFEH_Read_with_Enabled_Config_Address_001();
	pci_rqmid_27785_PCIe_config_space_and_host_1_Byte_CFFH_Read_with_Enabled_Config_Address_001();
	pci_rqmid_27788_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Read_with_Enabled_Config_Address_001();
	pci_rqmid_27783_PCIe_config_space_and_host_2_Byte_CFEH_Read_with_Enabled_Config_Address_001();
	pci_rqmid_27784_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Read_with_Enabled_Config_Address_001();
	pci_rqmid_27789_PCIe_config_space_and_host_Normal_Config_Data_Port_Read_with_Enabled_Config_Address_001();

	pci_rqmid_27791_PCIe_config_space_and_host_1_Byte_Config_Address_Port_Write_001();
	pci_rqmid_27790_PCIe_config_space_and_host_2_Byte_Config_Address_Port_Write_001();
	pci_rqmid_30960_PCIe_config_space_and_host_Normal_Config_Address_Port_Write_001();
	pci_rqmid_27793_PCIe_config_space_and_host_1_Byte_Config_Address_Port_Read_001();
	pci_rqmid_27792_PCIe_config_space_and_host_2_Byte_Config_Address_Port_Read_001();
	pci_rqmid_31161_PCIe_config_space_and_host_Normal_Config_Address_Port_Read_001();

	/* PCIe_Reserved register */
	pci_rqmid_26040_PCIe_config_space_and_host_Read_Reserved_register_001();
	pci_rqmid_25295_PCIe_config_space_and_host_Write_Reserved_register_001();
	pci_rqmid_25298_PCIe_config_space_and_host_Write_Reserved_register_002();

	/* Not exsit device tests */
	pci_rqmid_26109_PCIe_config_space_and_host_Read_register_from_no_exist_device_001();
	pci_rqmid_26095_PCIe_config_space_and_host_Write_register_to_no_exist_device_001();
	pci_rqmid_26105_PCIe_config_space_and_host_Write_register_to_no_exist_device_002();

	/* PCIe_RO registers */
	pci_rqmid_38095_PCIe_config_space_and_host_PCI_configuration_header_register_USB_001();

	/* PCIe_ACRN shall expose PCI configuration register */
	pci_rqmid_26245_PCIe_config_space_and_host_Type_Register_001();
	pci_rqmid_26793_PCIe_config_space_and_host_Status_Register_001();
	pci_rqmid_26794_PCIe_config_space_and_host_Command_Register_001();
	pci_rqmid_26817_PCIe_config_space_and_host_Interrupt_Line_Register_001();

	/* PCIe_MSI register */
	pci_rqmid_26821_PCIe_config_space_and_host_MSI_capability_structure_001();
	pci_rqmid_26823_PCIe_config_space_and_host_MSI_capability_structure_002();
	/* As above two tests has covered all bits RW/RS/RO check, we don't need execute below cases for MSI */
	pci_rqmid_28884_PCIe_config_space_and_host_MSI_message_address_destination_ID_read_write_002();
	pci_rqmid_28908_PCIe_config_space_and_host_MSI_message_data_trigger_mode_read_001();
	pci_rqmid_28888_PCIe_config_space_and_host_MSI_message_data_trigger_mode_write_001();
	pci_rqmid_28911_PCIe_config_space_and_host_MSI_message_data_delivery_mode_read_001();
	pci_rqmid_28892_PCIe_config_space_and_host_MSI_message_data_delivery_mode_write_001();
	pci_rqmid_28914_PCIe_config_space_and_host_MSI_message_address_interrupt_message_address_read_001();
	pci_rqmid_28894_PCIe_config_space_and_host_MSI_message_address_interrupt_message_address_write_001();
	pci_rqmid_28917_PCIe_config_space_and_host_MSI_message_address_redirection_hint_read_001();
	pci_rqmid_28896_PCIe_config_space_and_host_MSI_message_address_redirection_hint_write_001();
	pci_rqmid_28918_PCIe_config_space_and_host_MSI_message_control_read_001();
	pci_rqmid_28899_PCIe_config_space_and_host_MSI_message_control_write_001();
	pci_rqmid_38091_PCIe_config_space_and_host_capability_ID_register_of_the_PCI_MSI_capability_USB_001();

	/* MMIO */
	pci_set_mmcfg_addr(mmcfg_addr);
	pci_rqmid_26821_PCIe_config_space_and_host_MSI_capability_structure_001();
	pci_rqmid_26823_PCIe_config_space_and_host_MSI_capability_structure_002();
	pci_set_mmcfg_addr(0);

	/* MSI-X */
	pci_PCIe_config_space_and_host_MSIX_capability_structure_001();

	/* PCI Hole */
#ifdef IN_SAFETY_VM
	pci_rqmid_28743_PCIe_config_space_and_host_PCI_hole_range_001();
	pci_rqmid_28743_PCIe_config_space_and_host_PCI_hole_range_002();
	pci_rqmid_28699_PCIe_config_space_and_host_BAR_range_limitation_001();
	pci_rqmid_28699_PCIe_config_space_and_host_BAR_range_limitation_002();
#endif

	/* Startup */
	pci_rqmid_38105_PCIe_config_space_and_host_PCI_command_register_following_start_up_USB_001();
	pci_rqmid_38101_PCIe_config_space_and_host_msi_control_register_following_start_up_USB_001();
	pci_PCIe_config_space_and_host_msix_control_register_following_start_up_IVSHMEM_001();

#ifdef IN_SAFETY_VM
	/* ATS and PRI is hidden */
	pci_PCIe_config_space_and_host_ATS_capability_structure_001();
	pci_PCIe_config_space_and_host_PRI_capability_structure_001();
#endif
#endif

	return report_summary();
}
