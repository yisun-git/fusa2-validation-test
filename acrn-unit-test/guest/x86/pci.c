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

#define GBECSR_5B54      (0x5B54U)
#define USB_MSI_REG_OFFSET	(0x80U)
#define ETH_MSI_REG_OFFSET	(0xd0U)
#define MSI_CAPABILITY_ID	(0x05U)
#define A_NEW_VALUE_UNCHANGE	(0xFFF000U)
#define A_NEW_USB_BAR0_VALUE	(0xDF240000U)
#define A_NEW_NET_BAR0_VALUE	(0xDF260000U)
#define USB_ORIGINAL_BAR0_VALUE	(0xdf230004U)
#define NET_ORIGINAL_BAR0_VALUE	(0xdf200000U)
#define USB_MSI_REG_OFFSET	(0x80U)
#define NET_MSI_REG_OFFSET	(0xd0U)

//#define PCI_DEBUG
#ifdef PCI_DEBUG
#define PCI_DEBUG_LEVEL		DBG_LVL_INFO
#else
#define PCI_DEBUG_LEVEL		DBG_LVL_ERROR
#endif

typedef struct pci_capability_node_s {
	uint8_t cap_id;
	uint8_t next_ptr;
} pci_capability_node_t;

typedef struct pci_cfg_reg_s {
	const char *reg_name;
	uint16_t reg_addr;
	uint8_t reg_width;
	uint32_t reg_val;
	uint8_t is_shadow;/*undefine or reserved register 1_yes, 0_no*/
} pci_cfg_reg_t;

static struct pci_dev pci_devs[MAX_PCI_DEV_NUM];
static uint32_t nr_pci_devs = MAX_PCI_DEV_NUM;
static uint64_t apic_id_bitmap = 0UL;
static uint32_t ap_count = 0U;
static pci_capability_node_t net_cap_list[] =
{
	[0] = {
		.cap_id = 0x00,
		.next_ptr = 0xc8,
	},
	[1] = {
		.cap_id = 0x01,
		.next_ptr = 0xd0,
	},
	[2] = {
		.cap_id = 0x05,
		.next_ptr = 0xe0,
	},
	[3] = {
		.cap_id = 0x13,
		.next_ptr = 0x00
	}
};

static pci_capability_node_t usb_cap_list[] =
{
	[0] = {
		.cap_id = 0x00,
		.next_ptr = 0x70,
	},
	[1] = {
		.cap_id = 0x01,
		.next_ptr = 0x80,
	},
	[2] = {
		.cap_id = 0x05,
		.next_ptr = 0x00,
	}
};

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
	{"Int Line", 0x3c, 1, 0x00, 0},
	{"Int Pin", 0x3d, 1, 0x00, 0},
	{"Min_Gnt", 0x3e, 1, 0x00, 1},
	{"Min_Lat", 0x3f, 1, 0x00, 1},
	{"CAP reg", 0x40, 4, 0x00000000, 1},
	{"CAP reg", 0x44, 4, 0x00000000, 1},
	{"CAP reg", 0x48, 4, 0x00000000, 1},
	{"CAP reg", 0x4c, 4, 0x00000000, 1},
	{"CAP reg", 0x50, 4, 0x00000000, 1},
	{"CAP reg", 0x54, 4, 0x00000000, 1},
	{"CAP reg", 0x58, 4, 0x00000000, 1},
	{"CAP reg", 0x5c, 4, 0x00000000, 1},
	{"CAP reg", 0x60, 4, 0x00000000, 1},
	{"CAP reg", 0x64, 4, 0x00000000, 1},
	{"CAP reg", 0x68, 4, 0x00000000, 1},
	{"CAP reg", 0x6c, 4, 0x00000000, 1},
	{"CAP reg", 0x70, 4, 0x00000000, 1},
	{"CAP reg", 0x74, 4, 0x00000000, 1},
	{"CAP reg", 0x78, 4, 0x00000000, 1},
	{"CAP reg", 0x7c, 4, 0x00000000, 1},
	{"CAP reg", 0x80, 4, 0x00000000, 1},
	{"CAP reg", 0x84, 4, 0x00000000, 1},
	{"CAP reg", 0x88, 4, 0x00000000, 1},
	{"CAP reg", 0x8c, 4, 0x00000000, 1},
	{"CAP reg", 0x90, 4, 0x00000000, 1},
	{"CAP reg", 0x94, 4, 0x00000000, 1},
	{"CAP reg", 0x98, 4, 0x00000000, 1},
	{"CAP reg", 0x9c, 4, 0x00000000, 1},
	{"CAP reg", 0xa0, 4, 0x00000000, 1},
	{"CAP reg", 0xa4, 4, 0x00000000, 1},
	{"CAP reg", 0xa8, 4, 0x00000000, 1},
	{"CAP reg", 0xac, 4, 0x00000000, 1},
	{"CAP reg", 0xb0, 4, 0x00000000, 1},
	{"CAP reg", 0xb4, 4, 0x00000000, 1},
	{"CAP reg", 0xb8, 4, 0x00000000, 1},
	{"CAP reg", 0xbc, 4, 0x00000000, 1},
	{"CAP reg", 0xc0, 4, 0x00000000, 1},
	{"CAP reg", 0xc4, 4, 0x00000000, 1},
	{"CAP reg", 0xc8, 4, 0x00000000, 1},
	{"CAP reg", 0xcc, 4, 0x00000000, 1},
	{"CAP reg", 0xd0, 4, 0x00000000, 1},
	{"CAP reg", 0xd4, 4, 0x00000000, 1},
	{"CAP reg", 0xd8, 4, 0x00000000, 1},
	{"CAP reg", 0xdc, 4, 0x00000000, 1},
	{"CAP reg", 0xe0, 4, 0x00000000, 1},
	{"CAP reg", 0xe4, 4, 0x00000000, 1},
	{"CAP reg", 0xe8, 4, 0x00000000, 1},
	{"CAP reg", 0xec, 4, 0x00000000, 1},
	{"CAP reg", 0xf0, 4, 0x00000000, 1},
	{"CAP reg", 0xf4, 4, 0x00000000, 1},
	{"CAP reg", 0xf8, 4, 0x00000000, 1},
	{"CAP reg", 0xfc, 4, 0x00000000, 1},
	{"ext reg", 0x100, 4, 0x00000000, 1},
};

/*For BAR0 init test case*/
static PCI_MAKE_BDF(usb, 0x00, 0x14, 0x00);
static PCI_MAKE_BDF(ethernet, 0x00, 0x1f, 0x06);
static uint32_t first_net_bar0_value = 0U;
static uint32_t first_usb_bar0_value = 0U;
static uint32_t first_net_new_bar0_value = 0U;
static uint32_t first_usb_new_bar0_value = 0U;
static uint32_t second_net_bar0_value = 0U;
static uint32_t second_usb_bar0_value = 0U;
/*For 0xCF8 port init test case*/
static uint32_t unchange_reg_value_0 = 0U;
static uint32_t unchange_reg_value_1 = 0U;
static uint32_t unchange_resume_value = 0U;
/*For MSI control flag init test case*/
static uint32_t first_net_msi_ctrl = 0U;
static uint32_t first_usb_msi_ctrl = 0U;
static uint32_t first_net_new_msi_ctrl = 0U;
static uint32_t first_usb_new_msi_ctrl = 0U;
static uint32_t second_net_msi_ctrl = 0U;
static uint32_t second_usb_msi_ctrl = 0U;
/*For Command register init test case*/
static uint32_t first_net_command = 0U;
static uint32_t first_usb_command = 0U;
static uint32_t first_net_new_command = 0U;
static uint32_t first_usb_new_command = 0U;
static uint32_t second_net_command = 0U;
static uint32_t second_usb_command = 0U;
/*For BAR0 function status init test case*/
static uint32_t first_net_status = 0U;
static uint32_t	first_net_rctl = 0U;
static uint32_t	second_net_rctl = 0U;
static uint32_t first_usb_hci_version = 0U;
static uint32_t first_usb_dn_ctrl = 0U;
static uint32_t second_usb_dn_ctrl = 0U;



static __unused int pci_probe_msi_capability(union pci_bdf bdf, uint32_t *msi_addr);
static __unused int pci_data_check(union pci_bdf bdf, uint32_t offset,
uint32_t bytes, uint32_t val1, uint32_t val2, bool sw_flag);

static __unused bool is_cap_ptr_exist(pci_capability_node_t *caplist, uint32_t size, uint8_t next_ptr)
{
	int i = 0;
	for (i = 0; i < size; i++) {
		if (next_ptr == caplist[i].next_ptr) {
			return true;
		}
	}
	return false;
}

static __unused
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
			{"intPin_intLine_min_gnt_max_lat", 0x3C, 4, 0U, 0},
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
			//1.read the original value of these read-only registers
			read_only_regs[i].reg_val = pci_pdev_read_cfg(bdf,\
			read_only_regs[i].reg_addr, read_only_regs[i].reg_width);
			//2.write FFH to these read-only registers
			pci_pdev_write_cfg(bdf, read_only_regs[i].reg_addr,\
			read_only_regs[i].reg_width, 0xFFFFFFFF);
			//3.read the new value from these read-only registers
			value = pci_pdev_read_cfg(bdf,\
			read_only_regs[i].reg_addr, read_only_regs[i].reg_width);
			//4.compare the new value with the original value
			if (value == read_only_regs[i].reg_val) {
				count++;
			}
		}
	}
	is_pass = (count == size) ? true : false;
	return is_pass;
}

static __unused void test_PCI_read_bar_memory_PF(void *arg)
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

static __unused void test_PCI_write_bar_memory_PF(void *arg)
{
	uint32_t *add = (uint32_t *)arg;
	uint32_t *add2 = (uint32_t *)phys_to_virt(*add);
	uint32_t temp = 0;
	asm volatile(ASM_TRY("1f")
							"mov %%ebx, (%%eax)\n\t"
							"1:"
							: "=b" (temp)
							: "a" (add2));
}

static __unused
bool test_BAR_memory_map_to_none_PF(uint32_t dev_ven)
{
	int count = 0;
	union pci_bdf bdf = {0};
	uint32_t bar_base = 0U;
	uint32_t bar_base_new = BAR_REMAP_BASE_3;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		bar_base = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base_new);
		bar_base_new = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		bar_base_new &= 0xFFFFFFF0;
		DBG_INFO("W reg[%xH] = [%xH]", PCIR_BAR(0), bar_base_new);
		/*Read the BAR memory should generate #PF*/
		is_pass = test_for_exception(PF_VECTOR, test_PCI_read_bar_memory_PF, (void *)&bar_base_new);
		if (is_pass) {
			count++;
		}
		/*Write the BAR memory should generate #PF*/
		is_pass = test_for_exception(PF_VECTOR, test_PCI_write_bar_memory_PF, (void *)&bar_base_new);
		if (is_pass) {
			count++;
		}
		/*Resume the original BAR address*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base);
	}
	/*Write the BAR memory should generate #PF*/
	is_pass = (count == 2) ? true : false;
	return is_pass;
}


static __unused
bool test_host_MSI_Next_PTR(uint32_t dev_ven)
{
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	uint32_t reg_addr1 = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		if (ret == OK) {
			/*Read The Next pointer*/
			reg_addr1 = reg_addr + 1;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 1);
			ret |= pci_data_check(bdf, reg_addr1, 1, 0x00U, reg_val, false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}


static __unused
bool test_host_MSI_message_control_write(uint32_t dev_ven)
{
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	uint32_t reg_addr1 = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t value = 0U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		if (ret == OK) {
			reg_addr1 = reg_addr + PCI_MSI_FLAGS;
			reg_val_ori = pci_pdev_read_cfg(bdf, reg_addr1, 2);
			reg_val = reg_val_ori;
			reg_val &= SHIFT_UMASK(6, 4);
			reg_val |= SHIFT_LEFT(1, 5);
			/*write 0x10<<1 to bit[15:1]*/
			pci_pdev_write_cfg(bdf, reg_addr1, 2, reg_val);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 2);
			value |= SHIFT_LEFT(1, 5);
			/*The reg_val should not be equal to 0x10<<1*/
			ret |= pci_data_check(bdf, reg_addr1, 2, (reg_val & SHIFT_MASK(6, 4)),\
								(value & SHIFT_MASK(6, 4)), true);
			/*The reg_val should be equal to reg_val_ori*/
			ret |= pci_data_check(bdf, reg_addr1, 2, reg_val, reg_val_ori, false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static __unused
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
		/*pci probe and check MSI capability register*/
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

static __unused
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
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		/*Check if probe to MSI CAP register*/
		if (ret == OK) {
			reg_addr1 = reg_addr + PCI_MSI_ADDRESS_LO;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			reg_val_ori = reg_val;
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			reg_val = reg_val & SHIFT_UMASK(3, 0);
			reg_val |= 0x04U;
			pci_pdev_write_cfg(bdf, reg_addr1, 4, reg_val);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			/*The read value should not be equal to the write value*/
			ret |= pci_data_check(bdf, reg_addr1, 4, 0x04U, (reg_val & SHIFT_MASK(3, 0)), true);
			/*The read value should be equal to the original value*/
			ret |= pci_data_check(bdf, reg_addr1, 4, (reg_val_ori & SHIFT_MASK(3, 0)),\
								(reg_val & SHIFT_MASK(3, 0)), false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}


static __unused
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
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		/*Check if probe to MSI CAP register*/
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

static __unused
bool test_host_MSI_message_address_int_msg_addr_write(uint32_t dev_ven)
{
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_addr = 0U;
	uint32_t reg_addr1 = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == OK) {
			/*message address L register address = pointer + 0x4*/
			reg_addr1 = reg_addr + PCI_MSI_ADDRESS_LO;

			reg_val_ori = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			DBG_INFO("R reg[%xH] = [%xH]", reg_addr1, reg_val_ori);

			uint32_t lapic_id = apic_id();
			uint32_t msg_addr = MAKE_NET_MSI_MESSAGE_ADDR(0xA5500000, lapic_id);
			/*write 0xA5500000 to Message address[20:64]*/
			pci_pdev_write_cfg(bdf, reg_addr1, 4, msg_addr);
			DBG_INFO("msg_addr = %x", msg_addr);

			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			reg_val = reg_val & SHIFT_MASK(31, 20);
			DBG_INFO("R reg[%xH] = [%xH]", reg_addr1, reg_val);
			/*The Message address should not be equal to 0xA5500000*/
			ret |= pci_data_check(bdf, reg_addr1, 4, msg_addr, reg_val, true);

			/*The Message address should be equal to original val*/
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			ret |= pci_data_check(bdf, reg_addr1, 4, reg_val_ori, reg_val, false);

			/*Message address H register address = pointer + 0x8*/
			reg_addr1 = reg_addr + PCI_MSI_ADDRESS_HI;

			reg_val_ori = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			DBG_INFO("R reg[%xH] = [%xH]", reg_addr1, reg_val_ori);

			pci_pdev_write_cfg(bdf, reg_addr1, 4, 0x0000000A);

			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			DBG_INFO("R reg[%xH] = [%xH]", reg_addr1, reg_val);
			/*The Message address should not be equal to 0xA5500000*/
			ret |= pci_data_check(bdf, reg_addr1, 4, 0x0000000A, reg_val, true);

			/*The Message address should be equal to original val*/
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			ret |= pci_data_check(bdf, reg_addr1, 4, reg_val_ori, reg_val, false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}


static __unused
bool test_host_MSI_message_data_delivery_mode_write(uint32_t dev_ven)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t value = 0U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		/*Check if probe to MSI CAP register*/
		if (ret == OK) {
			reg_addr = reg_addr + PCI_MSI_DATA_64;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			reg_val_ori = reg_val;
			reg_val &= SHIFT_UMASK(10, 8);
			reg_val |= SHIFT_LEFT(0x1, 8);
			pci_pdev_write_cfg(bdf, reg_addr, 2, reg_val);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			value |= SHIFT_LEFT(0x1, 8);
			/*The read value should not be equal to the write value*/
			ret |= pci_data_check(bdf, reg_addr, 2, (value & SHIFT_MASK(10, 8)),\
								(reg_val & SHIFT_MASK(10, 8)), true);
			/*The read value should be equal to the original value*/
			ret |= pci_data_check(bdf, reg_addr, 2, (reg_val_ori & SHIFT_MASK(10, 8)),\
								(reg_val & SHIFT_MASK(10, 8)), false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static __unused
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
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		/*Check if probe to MSI CAP register*/
		if (ret == OK) {
			reg_addr1 = reg_addr + PCI_MSI_ADDRESS_LO;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr1, 4);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			ret |= pci_data_check(bdf, reg_addr1, 4, 0x0FEE0000U,\
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

static __unused
bool test_host_MSI_message_data_delivery_mode_read(uint32_t dev_ven)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		/*Check if probe to MSI CAP register*/
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


static __unused
bool test_host_MSI_message_data_trigger_mode_read(uint32_t dev_ven)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	uint32_t value = 0U;
	value |= SHIFT_LEFT(1, 14);
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		/*Check if probe to MSI CAP register*/
		if (ret == OK) {
			reg_addr = reg_addr + PCI_MSI_DATA_64;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			ret |= pci_data_check(bdf, reg_addr, 2, (value & SHIFT_MASK(15, 14)),\
								(reg_val & SHIFT_MASK(15, 14)), false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static __unused
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
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		/*Check if probe to MSI CAP register*/
		if (ret == OK) {
			reg_addr = reg_addr + PCI_MSI_DATA_64;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			reg_val_ori = reg_val;
			reg_val |= SHIFT_LEFT(0x3, 14);
			pci_pdev_write_cfg(bdf, reg_addr, 2, reg_val);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			/*The read value should not be equal to the write value*/
			ret |= pci_data_check(bdf, reg_addr, 2, SHIFT_LEFT(0x3, 14),\
									(reg_val & SHIFT_LEFT(0x3, 14)), true);
			/*The read value should be equal to the original value*/
			ret |= pci_data_check(bdf, reg_addr, 2, (reg_val_ori & SHIFT_LEFT(0x3, 14)),\
								(reg_val & SHIFT_LEFT(0x3, 14)), false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static __unused
bool test_MSI_message_address_destination_field_rw(uint32_t dev_ven)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	uint32_t apic_id1 = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, dev_ven, &bdf);
	if (is_pass) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		/*Check if probe to MSI CAP register*/
		if (ret == OK) {
			reg_addr = reg_addr + PCI_MSI_ADDRESS_LO;
			/*read the original MSI address lo value*/
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			/*Write APIC ID to Destination ID field*/
			apic_id1 = apic_id();
			apic_id1 &= 0xFFU;
			reg_val &= SHIFT_UMASK(19, 12);
			reg_val |= SHIFT_LEFT(apic_id1, 12);
			/*write the MSI address value*/
			pci_pdev_write_cfg(bdf, reg_addr, 4, reg_val);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			/*Extract APIC ID from Destination ID field*/
			reg_val &= SHIFT_MASK(19, 12);
			reg_val = SHIFT_RIGHT(reg_val, 12);
			reg_val &= 0xFF;
			ret |= pci_data_check(bdf, reg_addr, 1, reg_val, apic_id1, false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	return is_pass;
}

static __unused
uint32_t pci_pdev_read_cfg_test(union pci_bdf bdf, uint32_t offset, uint32_t bytes, uint32_t port, bool is_en)
{
	uint32_t addr = 0U;
	uint32_t val = 0U;

	addr = pci_pdev_calc_address(bdf, offset);
	if (false == is_en) {
		addr = addr & (~PCI_CONFIG_ENABLE);
	}

	/* Write address to ADDRESS register */
	pio_write32(addr, (uint16_t)PCI_CONFIG_ADDR);

	/* Read result from DATA register */
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


static __unused int pci_pdev_write_cfg_test(union pci_bdf bdf, uint32_t offset, uint32_t bytes,
		uint32_t val, uint32_t port, bool is_en)
{
	uint32_t addr = 0U;
	addr = pci_pdev_calc_address(bdf, offset);
	if (false == is_en) {
		addr = addr & (~PCI_CONFIG_ENABLE);
	}
	/* Write address to ADDRESS register */
	pio_write32(addr, (uint16_t)PCI_CONFIG_ADDR);
	/* Write value to DATA register */
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

static __unused
bool  test_Write_with_Disabled_Config_Address(uint8_t msi_offset, uint32_t bytes, uint32_t val, uint32_t port)
{
	int count = 0;
	uint32_t reg_val = 0U;
	uint32_t reg_val1 = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	uint32_t size = 0;
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
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			reg_val = pci_pdev_read_cfg(bdf, msi_addr, size);
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, val, port, false);
			reg_val1 = pci_pdev_read_cfg(bdf, msi_addr, size);
			if (reg_val == reg_val1) {
				count++;
			}
		}
	}
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			reg_val = pci_pdev_read_cfg(bdf, msi_addr, size);
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, val, port, false);
			reg_val1 = pci_pdev_read_cfg(bdf, msi_addr, size);
			if (reg_val == reg_val1) {
				count++;
			}
		}
	}
	is_pass = (count == 2) ? true : false;
	return is_pass;
}


static __unused
bool  test_Write_with_enabled_Config_Address(uint8_t msi_offset, uint32_t bytes, uint32_t val, uint32_t port)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, val, port, true);
			reg_val = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			if (val == reg_val) {
				count++;
			}
		}
	}
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, val, port, true);
			reg_val = pci_pdev_read_cfg(bdf, msi_addr, bytes);
			if (val == reg_val) {
				count++;
			}
		}
	}
	is_pass = (count == 2) ? true : false;
	return is_pass;
}

static __unused
bool test_Read_with_Enabled_Config_Address(uint8_t msi_offset, uint32_t bytes, uint32_t val, uint32_t port)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, val, port, true);
			reg_val = pci_pdev_read_cfg_test(bdf, msi_addr, bytes, port, true);
			if (reg_val ==	val) {
				count++;
			}
		}
	}
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + msi_offset;
			pci_pdev_write_cfg_test(bdf, msi_addr, bytes, val, port, true);
			reg_val = pci_pdev_read_cfg_test(bdf, msi_addr, bytes, port, true);
			if (reg_val ==	val) {
				count++;
			}
		}
	}
	is_pass = (count == 2) ? true : false;
	return is_pass;
}


static __unused void check_bar_map_function_on_first_AP(void)
{
	first_net_bar0_value = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), PCIR_BAR(0), 4);
	first_usb_bar0_value = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 4);

	void *status_addr = phys_to_virt((first_net_bar0_value & 0xFFFFFFF0) + E1000_STATUS);
	first_net_status = pci_pdev_read_mem(PCI_GET_BDF(ethernet), (mem_size)status_addr, 4);

	void *rctl_addr = phys_to_virt((first_net_bar0_value & 0xFFFFFFF0) + E1000_RCTL);
	pci_pdev_write_mem(PCI_GET_BDF(ethernet), (mem_size)rctl_addr, 4, 0x0U);
	first_net_rctl = pci_pdev_read_mem(PCI_GET_BDF(ethernet), (mem_size)rctl_addr, 4);

	void *hci_version_addr = phys_to_virt((first_usb_bar0_value & 0xFFFFFFF0) + 0x02U);
	first_usb_hci_version = pci_pdev_read_mem(PCI_GET_BDF(usb), (mem_size)hci_version_addr, 2);

	void *dn_ctrl_addr = phys_to_virt((first_usb_bar0_value & 0xFFFFFFF0) + 0x94U);
	pci_pdev_write_mem(PCI_GET_BDF(usb), (mem_size)dn_ctrl_addr, 4, 0x1U);
	first_usb_dn_ctrl = pci_pdev_read_mem(PCI_GET_BDF(usb), (mem_size)dn_ctrl_addr, 4);

}

static __unused void check_bar_map_function_on_second_AP(void)
{
	void *rctl_addr = phys_to_virt((first_net_bar0_value & 0xFFFFFFF0) + E1000_RCTL);
	second_net_rctl = pci_pdev_read_mem(PCI_GET_BDF(ethernet), (mem_size)rctl_addr, 4);

	void *dn_ctrl_addr = phys_to_virt((first_usb_bar0_value & 0xFFFFFFF0) + 0x94U);
	second_usb_dn_ctrl = pci_pdev_read_mem(PCI_GET_BDF(usb), (mem_size)dn_ctrl_addr, 4);
}


static __unused void check_command_register_on_first_AP(void)
{
	uint32_t new_value = 0U;
	first_net_command = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), PCI_COMMAND, 2);
	first_usb_command = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCI_COMMAND, 2);
	new_value = first_net_command | SHIFT_LEFT(0x1, 1);
	pci_pdev_write_cfg(PCI_GET_BDF(ethernet), PCI_COMMAND, 2, new_value);
	new_value = first_usb_command | SHIFT_LEFT(0x1, 1);
	pci_pdev_write_cfg(PCI_GET_BDF(usb), PCI_COMMAND, 2, new_value);
	first_net_new_command = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), PCI_COMMAND, 2);
	first_usb_new_command = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCI_COMMAND, 2);
}

static __unused void check_command_register_on_second_AP(void)
{
	second_net_command = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), PCI_COMMAND, 2);
	second_usb_command = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCI_COMMAND, 2);
	pci_pdev_write_cfg(PCI_GET_BDF(ethernet), PCI_COMMAND, 2, first_net_command);
	pci_pdev_write_cfg(PCI_GET_BDF(usb), PCI_COMMAND, 2, first_usb_command);
}

static __unused
void check_bar0_on_first_AP(void)
{
	first_net_bar0_value = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), PCIR_BAR(0), 4);
	first_usb_bar0_value = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 4);
	pci_pdev_write_cfg(PCI_GET_BDF(ethernet), PCIR_BAR(0), 4, A_NEW_NET_BAR0_VALUE);
	pci_pdev_write_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 4, A_NEW_USB_BAR0_VALUE);
	first_net_new_bar0_value = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), PCIR_BAR(0), 4);
	first_usb_new_bar0_value = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 4);
}

static __unused
void check_bar0_on_second_AP(void)
{
	second_net_bar0_value = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), PCIR_BAR(0), 4);
	second_usb_bar0_value = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 4);
	pci_pdev_write_cfg(PCI_GET_BDF(ethernet), PCIR_BAR(0), 4, first_net_bar0_value);
	pci_pdev_write_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 4, first_usb_bar0_value);
}

static __unused
void check_msi_control_flag_on_first_AP(void)
{
	uint32_t new_value = 0U;
	first_usb_msi_ctrl = pci_pdev_read_cfg(PCI_GET_BDF(usb), USB_MSI_REG_OFFSET + 2, 2);
	first_net_msi_ctrl = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), NET_MSI_REG_OFFSET + 2, 2);
	new_value = first_usb_msi_ctrl | SHIFT_LEFT(0x01, 0);
	pci_pdev_write_cfg(PCI_GET_BDF(usb), USB_MSI_REG_OFFSET + 2, 2, new_value);
	new_value = first_net_msi_ctrl | SHIFT_LEFT(0x01, 0);
	pci_pdev_write_cfg(PCI_GET_BDF(ethernet), NET_MSI_REG_OFFSET + 2, 2, new_value);
	first_net_new_msi_ctrl = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), NET_MSI_REG_OFFSET + 2, 2);
	first_usb_new_msi_ctrl = pci_pdev_read_cfg(PCI_GET_BDF(usb), USB_MSI_REG_OFFSET + 2, 2);
}

static __unused
void check_msi_control_flag_on_second_AP(void)
{
	second_usb_msi_ctrl = pci_pdev_read_cfg(PCI_GET_BDF(usb), USB_MSI_REG_OFFSET + 2, 2);
	second_net_msi_ctrl = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), NET_MSI_REG_OFFSET + 2, 2);
}

static __unused void check_PCI_address_port_on_first_AP(void)
{
	uint32_t new_value = A_NEW_VALUE_UNCHANGE;
	/*write a new value to $0xCF8 PCI address port and read it on first AP.*/
	asm volatile("push" W " %%" R "dx\n\t"\
				"push" W " %%" R "ax\n\t"\
				"mov $0xCF8, %%edx\n\t"\
				"inl (%%dx), %%eax\n\t"\
				"mov %%eax, %1\n\t"\
				"mov %2, %%eax\n\t"\
				"outl %%eax, (%%dx)\n\t"\
				"inl (%%dx), %%eax\n\t"\
				"mov %%eax, %0\n\t"\
				"pop" W " %%" R "ax\n\t"\
				"pop" W " %%" R "dx\n\t"\
				: "=m" (unchange_reg_value_0), "=m" (unchange_resume_value)
				: "m" (new_value)
				: "memory");
	*((uint32_t *)AP_IO_PORT_ADDR) = unchange_resume_value;
}

static __unused void check_PCI_address_port_on_second_AP(void)
{
	/*read $0xCF8 PCI address port on second AP.*/
	asm volatile("push" W " %%" R "dx\n\t"\
				"push" W " %%" R "ax\n\t"\
				"mov $0xCF8, %%edx\n\t"\
				"inl (%%dx), %%eax\n\t"\
				"mov %%eax, %0\n\t"\
				"pop" W " %%" R "ax\n\t"\
				"pop" W " %%" R "dx\n\t"\
				: "=m" (unchange_reg_value_1)
				:
				: "memory");
}

static __unused void check_PCI_address_port_on_third_AP(void)
{
	/*resume the original $0xCF8 PCI address port value.*/
	asm volatile("push" W " %%" R "dx\n\t"\
				"push" W " %%" R "ax\n\t"\
				"mov $0xCF8, %%edx\n\t"\
				"mov %0, %%eax\n\t"\
				"outl %%eax, (%%dx)\n\t"\
				"pop" W " %%" R "ax\n\t"\
				"pop" W " %%" R "dx\n\t"\
				:
				: "m" (unchange_resume_value)
				: "memory");
}

static __unused void check_USB_interrupt_line_on_first_AP(void)
{
	/*read the USB interrupt line register on first AP and write a new value 0xEE to it.*/
	asm volatile("push" W " %%" R "dx\n\t"\
			"push" W " %%" R "ax\n\t"\
			"mov $0x8000A03C, %%eax\n\t"\
			"mov $0xCF8, %%edx\n\t"\
			"outl %%eax, (%%dx)\n\t"\
			"mov $0xCFC, %%edx\n\t"\
			"inb (%%dx), %%al\n\t"\
			"mov %%eax, (0x9004)\n\t"\
			"mov $0xEE, %%eax\n\t"\
			"outb %%al, (%%dx)\n\t"\
			"pop" W " %%" R "ax\n\t"\
			"pop" W " %%" R "dx\n\t"\
			:
			:
			:);
}

static __unused void check_USB_interrupt_line_on_second_AP(void)
{
		/*read USB interrupt line on second AP, and save it to 0x9008*/
		asm volatile("push" W " %%" R "dx\n\t"\
			"push" W " %%" R "ax\n\t"\
			"mov $0x8000A03C, %%eax\n\t"\
			"mov $0xCF8, %%edx\n\t"\
			"outl %%eax, (%%dx)\n\t"\
			"mov $0xCFC, %%edx\n\t"\
			"inb (%%dx), %%al\n\t"\
			"mov %%eax, (0x9008)\n\t"\
			"pop" W " %%" R "ax\n\t"\
			"pop" W " %%" R "dx\n\t"\
			:
			:
			:);
}

static __unused void check_NET_interrupt_line_on_first_AP(void)
{
	/*read the NET interrupt line register on first AP and write a new value 0xEE to it.*/
	asm volatile("push" W " %%" R "dx\n\t"\
			"push" W " %%" R "ax\n\t"\
			"mov $0x8000FE3C, %%eax\n\t"\
			"mov $0xCF8, %%edx\n\t"\
			"outl %%eax, (%%dx)\n\t"\
			"mov $0xCFC, %%edx\n\t"\
			"inb (%%dx), %%al\n\t"\
			"mov %%eax, (0x8004)\n\t"\
			"mov $0xEE, %%eax\n\t"\
			"outb %%al, (%%dx)\n\t"\
			"pop" W " %%" R "ax\n\t"\
			"pop" W " %%" R "dx\n\t"\
			:
			:
			:);
}

static __unused void check_NET_interrupt_line_on_second_AP(void)
{
	/*read USB interrupt line on second AP, and save it to 0x8008*/
	asm volatile("push" W " %%" R "dx\n\t"\
			"push" W " %%" R "ax\n\t"\
			"mov $0x8000FE3C, %%eax\n\t"\
			"mov $0xCF8, %%edx\n\t"\
			"outl %%eax, (%%dx)\n\t"\
			"mov $0xCFC, %%edx\n\t"\
			"inb (%%dx), %%al\n\t"\
			"mov %%eax, (0x8008)\n\t"\
			"pop" W " %%" R "ax\n\t"\
			"pop" W " %%" R "dx\n\t"\
			:
			:
			:);
}

static struct spinlock lock;
void save_unchanged_reg(void)
{
	uint32_t id = 0U;
	id = apic_id();
	apic_id_bitmap |= SHIFT_LEFT(1UL, id);

	spin_lock(&lock);
	if (ap_count == 0) {
		check_bar_map_function_on_first_AP();
		check_USB_interrupt_line_on_first_AP();
		check_NET_interrupt_line_on_first_AP();
		check_bar0_on_first_AP();
		check_msi_control_flag_on_first_AP();
		check_command_register_on_first_AP();
		check_PCI_address_port_on_first_AP();
	} else if (ap_count == 1) {
		check_PCI_address_port_on_second_AP();
		check_USB_interrupt_line_on_second_AP();
		check_NET_interrupt_line_on_second_AP();
		check_bar0_on_second_AP();
		check_msi_control_flag_on_second_AP();
		check_command_register_on_second_AP();
		check_bar_map_function_on_second_AP();
	} else if (ap_count == 2) {
		check_PCI_address_port_on_third_AP();
	}
	ap_count++;
	spin_unlock(&lock);
}

static __unused uint32_t find_first_apic_id(void)
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

static __unused int pci_dev_temp_bar0_Set(union pci_bdf bdf, uint32_t value)
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

static __unused int pci_dev_temp_bar0_Get(union pci_bdf bdf, uint32_t *value)
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

static __unused int pci_data_check(union pci_bdf bdf, uint32_t offset,
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

static __unused int pci_probe_msi_capability(union pci_bdf bdf, uint32_t *msi_addr)
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

/*
 * @brief case name: PCIe config space and host_2-byte BAR read_001
 *
 * Summary: When a vCPU attempts to read two byte from
 * a guest PCI configuration base address register of
 * an assigned PCIe device, ACRN hypervisor shall guarantee
 * that the vCPU gets 0FFFFH.
 */
static __unused void pci_rqmid_28936_PCIe_config_space_and_host_2_byte_BAR_read_001(void)
{
	uint32_t reg_val = INVALID_REG_VALUE_U;
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 2);
		reg_val &= 0xFFFFU;
		/*Dump  the #BAR0 register*/
		DBG_INFO("Dump the #BAR0 register, #BAR0 = 0x%x ", reg_val);
		ret |= pci_data_check(bdf, PCIR_BAR(0), 2, 0xFFFF, reg_val, false);
		/*Verify that the #BAR0 register value is 0xFFFF*/
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_MSI message-data vector read-write_001
 *
 * Summary: Message data register [bit 7:0] of guest PCI configuration MSI capability structure
 * shall be a read-write bits field.
 */
static __unused void pci_rqmid_28887_PCIe_config_space_host_MSI_message_data_vector_read_write_001(void)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		/*Check if probe to MSI CAP register*/
		if (ret == OK) {
			/*Set 1byte  to the #Message Data register*/
			reg_addr = reg_addr + PCI_MSI_DATA_64;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			reg_val &= 0xFF00;
			reg_val |= 0x30;
			pci_pdev_write_cfg(bdf, reg_addr, 2, reg_val);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			DBG_INFO("Dump #Message register = 0x%x ", reg_val);
			ret |= pci_data_check(bdf, reg_addr, 2, 0x30, (reg_val & 0xFFU), false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Unmapping upon BAR writes_002
 *
 * Summary: When a vCPU writes a guest PCI configuration base address register (BAR)
 * of an assigned PCIe device, ACRN hypervisor shall guarantee that the old guest
 * physical base address is mapped to none.
 */
static __unused void pci_rqmid_28861_PCIe_config_space_and_host_Unmapping_upon_BAR_writes_002(void)
{
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t bar_base = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		/*Dump the #BAR0 register*/
		reg_val = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		bar_base = reg_val;
		DBG_INFO("Dump origin bar_base=%x", reg_val);

		/*Set the BAR0 register*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, BAR_REMAP_BASE);
		reg_val = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);

		DBG_INFO("Dump new bar_base=%x", reg_val);

		/*Read the data from the address of the #BAR0 original value*/
		asm volatile(ASM_TRY("1f")
					 "mov (%%eax), %0\n\t"
					 "1:"
					: "=b" (reg_val)
					: "a" (bar_base)
					:);
		/*Resume the original bar address 0*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base);
		/*Verify that the #PF is triggered*/
		is_pass = (exception_vector() == PF_VECTOR) && (exception_error_code() == 0);
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name: PCIe config space and host_write Reserved register_004
 *
 * Summary: When a vCPU attempts to write
 * a guest reserved PCI configuration register/a register's bits,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused void pci_rqmid_28792_PCIe_config_space_and_host_write_Reserved_register_004(void)
{
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		/*Set 0xA53C to the #reserved register*/
		reg_val = 0xA53C;
		pci_pdev_write_cfg(bdf, PCI_CONFIG_RESERVE, 2, reg_val);
		reg_val = pci_pdev_read_cfg(bdf, PCI_CONFIG_RESERVE, 2);
		/*	Dump the #reserved register*/
		DBG_INFO("Dump #PCIR_REV(0x36) = 0x%x", reg_val);
		/*Verify that the data from #reserved register is default value*/
		ret |= pci_data_check(bdf, PCI_CONFIG_RESERVE, 2, 0, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name: PCIe config space and host_Status Register_002
 *
 * Summary: ACRN hypervisor shall expose PCI configuration status register of any PCIe device to the VM
 * which the PCIe device is assigned to, in compliance with Chapter 7.5.1.2, PCIe BS.
 */
static __unused void pci_rqmid_28829_PCIe_config_space_and_host_Status_Register_002(void)
{
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		/*Dump the #status Register value for the NET devices in the hypervisor environment*/
		reg_val = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		DBG_INFO("Dump the status Register value [%xH] = [%xH]", PCI_STATUS, reg_val);
		is_pass = (PCI_NET_STATUS_VAL_NATIVE == reg_val) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name: PCIe config space and host_MSI message-address
 * interrupt message address write_002
 *
 * Summary: When a vCPU attempts to write a guest message address
 * register [bit 64:20] of the PCI MSI capability structure of an
 * assigned  PCIe device, ACRN hypervisor shall guarantee that the
 * write to the bits is ignored.
 */
static __unused void pci_rqmid_28893_PCIe_config_space_and_host_MSI_message_address_int_msg_addr_write_002(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_address_int_msg_addr_write(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
	return;
}

static uint64_t g_get_msi_int = 0;
static __unused int msi_int_handler(void *p_arg)
{
	uint32_t lapic_id = apic_id();
	DBG_INFO("[ int ]Run %s(%p), lapic_id=%d", __func__, p_arg, lapic_id);
	g_get_msi_int |= SHIFT_LEFT(1UL, lapic_id);
	return 0;
}

static __unused void msi_trigger_interrupt(void)
{
	/*init e1000e net pci device, get ready for msi int*/
	uint32_t lapic_id = apic_id();
	g_get_msi_int &= ~SHIFT_LEFT(1UL, lapic_id);
	msi_int_connect(msi_int_handler, (void *)1);
	msi_int_simulate();
	delay_loop_ms(5000);
}

/*
 * @brief case name: PCIe config space and host_MSI message-address
 * interrupt message address write_003
 *
 * Summary: When a vCPU attempts to write a guest message address
 * register [bit 64:20] of the PCI MSI capability structure of an
 * assigned PCIe device, ACRN hypervisor shall guarantee that the
 * write to the bits is ignored.
 */
static __unused void pci_rqmid_33824_PCIe_config_space_and_host_MSI_message_address_int_msg_addr_write_003(void)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	uint32_t lapic_id = 0;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == OK) {
			e1000e_msi_reset();
			/*write 0x0FFE00000 to Message address[20:64]*/
			lapic_id = apic_id();
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFFE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(MSI_NET_IRQ_VECTOR);
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();
			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
		}
		reg_val = pci_pdev_read_cfg(bdf, reg_addr + 4, 4);
		DBG_INFO("write MSI ADDR LOW reg[%xH] = [%xH]", reg_addr + 4, reg_val);
		reg_val = pci_pdev_read_cfg(bdf, reg_addr + 0x0C, 2);
		DBG_INFO("write MSI ADDR DATA reg[%xH] = [%xH]", reg_addr + 0x0C, reg_val);
		is_pass = ((ret == OK) && (g_get_msi_int & SHIFT_LEFT(1UL, lapic_id))) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_MSI message-address destination ID read-write_003
 *
 * Summary: Message address register [bit 19:12] of guest PCI configuration MSI capability
 * structure shall be a read-write bits field.
 */
static __unused void pci_rqmid_33822_PCIe_config_space_and_host_MSI_message_address_destination_ID_read_write_003(void)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t lapic_id = 0U;
	uint32_t ap_lapic_id = 0U;
	bool is_pass = false;
	int ret = OK;
	int i = 0;
	int cnt = 0;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		unsigned nr_cpu = fwcfg_get_nb_cpus();
		nr_cpu = (nr_cpu > 2) ? 2 : nr_cpu;
		ap_lapic_id = find_first_apic_id();
		DBG_INFO("nr_cpu=%d ap_lapic_id=%d", nr_cpu, ap_lapic_id);
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == OK) {
			/*message address L register address = pointer + 0x4*/
			for (i = 0; i < nr_cpu; i++) {
				e1000e_msi_reset();
				lapic_id = (i == 0) ? apic_id() : ap_lapic_id;
				DBG_INFO("Dump APIC id lapic_id = %x, ap_lapic_id=%x", lapic_id, ap_lapic_id);
				uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
				uint32_t msi_msg_addr_hi = 0U;
				uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(MSI_NET_IRQ_VECTOR);
				uint64_t msi_addr = msi_msg_addr_hi;
				msi_addr <<= 32;
				msi_addr &= 0xFFFFFFFF00000000UL;
				msi_addr |= msi_msg_addr_lo;
				e1000e_msi_config(msi_addr, msi_msg_data);
				e1000e_msi_enable();

				reg_val = pci_pdev_read_cfg(bdf, reg_addr + 4, 4);
				DBG_INFO("write MSI ADDR LOW reg[%xH] = [%xH]", reg_addr + 4, reg_val);
				reg_val = (reg_val >> 12) & 0xFF;
				/*Generate MSI intterrupt*/
				msi_trigger_interrupt();
				if ((reg_val == lapic_id) && (g_get_msi_int & SHIFT_LEFT(1UL, lapic_id))) {
					cnt++;
				}
			}
		}

		is_pass = ((ret == OK) && (cnt == nr_cpu)) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_invalid destination ID_001
 *
 * Summary: When a vCPU attempts to write a invalid destination ID to a guest
 * message address register [bit 19:12] of guest PCI configuration MSI capability
 * structure of any assigned PCIe device, ACRN hypervisor shall guarantee the interrupt
 * specified by this MSI capability structure is never delivered.
 */
static __unused void pci_rqmid_33823_PCIe_config_space_and_host_invalid_destination_ID_001(void)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		if (ret == OK) {
			/*message address L register address = pointer + 0x4*/
			e1000e_msi_reset();
			lapic_id = apic_id();
			DBG_INFO("Dump APIC id lapic_id = %x", lapic_id);
			/*write a invalid destination ID 111 to a guest message address register [bit 19:12]*/
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, INVALID_APIC_ID_A);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(MSI_NET_IRQ_VECTOR);
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			reg_val = pci_pdev_read_cfg(bdf, reg_addr + 4, 4);
			DBG_INFO("write MSI ADDR LOW reg[%xH] = [%xH]", reg_addr + 4, reg_val);
			reg_val = (reg_val >> 12) & 0xFF;

			/*Generate MSI intterrupt*/
			msi_trigger_interrupt();
		}
		is_pass = ((ret == OK) && !(g_get_msi_int & SHIFT_LEFT(1UL, INVALID_APIC_ID_A))) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

//<The scaling part>
/*
 * @brief case name: PCIe config space and host_Write Reserved register_001
 *
 * Summary: When a vCPU attempts to write a guest reserved PCI configuration register/a register's bits,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused void pci_rqmid_25295_PCIe_config_space_and_host_Write_Reserved_register_001(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		pci_pdev_write_cfg(bdf, PCI_CONFIG_RESERVE, 2, 0xA53C);
		reg_val = pci_pdev_read_cfg(bdf, PCI_CONFIG_RESERVE, 2);
		is_pass = ((reg_val == 0U) && (reg_val != 0xA53C)) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Write Reserved register_002
 *
 * Summary: When a vCPU attempts to write a guest reserved PCI configuration register/a register's bits,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused void pci_rqmid_25298_PCIe_config_space_and_host_Write_Reserved_register_002(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	uint32_t reg_val_new = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_COMMAND, 4);
		reg_val |= SHIFT_MASK(18, 11);
		DBG_INFO("reg_val = 0x%08x", reg_val);
		pci_pdev_write_cfg(bdf, PCI_COMMAND, 4, reg_val);
		reg_val_new = pci_pdev_read_cfg(bdf, PCI_COMMAND, 4);
		reg_val &= SHIFT_MASK(18, 11);
		reg_val_new &= SHIFT_MASK(18, 11);
		is_pass = ((reg_val_new == 0U) && (reg_val != reg_val_new)) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Write Reserved register_003
 *
 * Summary: When a vCPU attempts to write a guest reserved PCI configuration register/a register's bits,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused void pci_rqmid_28791_PCIe_config_space_and_host_Write_Reserved_register_003(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	uint32_t reg_val_new = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_COMMAND, 4);
		reg_val |= SHIFT_MASK(18, 11);
		DBG_INFO("reg_val = 0x%08x", reg_val);
		pci_pdev_write_cfg(bdf, PCI_COMMAND, 4, reg_val);
		reg_val_new = pci_pdev_read_cfg(bdf, PCI_COMMAND, 4);
		reg_val &= SHIFT_MASK(18, 11);
		reg_val_new &= SHIFT_MASK(18, 11);
		is_pass = ((reg_val_new == 0U) && (reg_val != reg_val_new)) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name: PCIe config space and host_Write Reserved register_004
 *
 * Summary: When a vCPU attempts to write a guest reserved PCI configuration register/a register's bits,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused void pci_rqmid_28792_PCIe_config_space_and_host_Write_Reserved_register_004(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		pci_pdev_write_cfg(bdf, PCI_CONFIG_RESERVE, 2, 0xA53C);
		reg_val = pci_pdev_read_cfg(bdf, PCI_CONFIG_RESERVE, 2);
		is_pass = ((reg_val == 0U) && (reg_val != 0xA53C)) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Read_Reserved_register_002
 *
 * Summary: When a vCPU attempts to read a guest reserved PCI configuration register,
 * ACRN hypervisor shall guarantee that 00H is written to guest EAX.
 */
static __unused void pci_rqmid_28793_PCIe_config_space_and_host_Read_Reserved_register_002(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_CONFIG_RESERVE, 2);
		is_pass = (reg_val == 0U) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Read_Reserved_register_001
 *
 * Summary: When a vCPU attempts to read a guest reserved PCI configuration register,
 * ACRN hypervisor shall guarantee that 00H is written to guest EAX.
 */
static __unused void pci_rqmid_26040_PCIe_config_space_and_host_Read_Reserved_register_001(void)
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

static __unused
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
	/*write no-exist device BAR0 register to 0xFFFFFFFF*/
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
	/*read non-exist device BAR0 register and compare this value with 0xFFFFFFFF*/
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
static __unused
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

	/*read exist device BAR0 register,and record this value*/
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

	/*write no-exist device BAR0 register*/
	for (bus = 0; bus < BUS_NUM; bus++) {
		bdf.bits.b = bus;
		for (dev = 0; dev < DEV_NUM; dev++) {
			bdf.bits.d = dev;
			for (func = 0; func < FUNC_NUM; func++) {
				bdf.bits.f = func;
				if (is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf)) {
					continue;
				}
				pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, 0xFFFFFFFF);
				DBG_INFO("[%03d:%02xH:%d]", bdf.bits.b, bdf.bits.d, bdf.bits.f);
			}
		}
	}

	/*read exist device BAR0 register and compare this value with record data*/
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
				if (ERROR == pci_dev_temp_bar0_Get(bdf, (uint32_t *)(&reg_val_old))) {
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
static __unused
void pci_rqmid_26109_PCIe_config_space_and_host_Read_register_from_no_exist_device_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	uint8_t func = 0;
	uint16_t bus = 0;
	uint8_t dev = 0;
	/*read exist device BAR0 register and compare this value with 0xFFFFFFFF*/
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
			}
		}
	}
	is_pass = (ret == OK) ? true : false;
	report("%s", is_pass, __FUNCTION__);
	return;
}


/*
 * @brief case name: PCIe config space and host_BAR range limitation_002
 *
 * Summary: When a vCPU writes a guest PCI configuration base address register(BAR)
 * of an assigned PCIe device and the new guest physical
 * base address in the guest BAR is out of guest PCI hole,
 * ACRN hypervisor shall guarantee that the control function corresponding to
 * the guest BAR is not mapped to any guest physical address.
 */
static __unused void pci_rqmid_28858_PCIe_config_space_and_host_BAR_range_limitation_002(void)
{
	union pci_bdf bdf = {0};
	uint32_t bar_base = 0U;
	uint32_t bar_base_new = BAR_REMAP_BASE_1;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		bar_base = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		/*Write a new BAR out of PCI BAR memory hole*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base_new);
		DBG_INFO("W reg[%xH] = [%xH]", PCIR_BAR(0), bar_base_new);
		/*Read the BAR memory should generate #PF*/
		is_pass = test_for_exception(PF_VECTOR, test_PCI_read_bar_memory_PF, (void *)&bar_base_new);
		/*Resume the original BAR address*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base);
	}
	report("%s", is_pass, __FUNCTION__);
	return;
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
static __unused void pci_rqmid_28699_PCIe_config_space_and_host_BAR_range_limitation_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t bar_base = 0U;
	uint32_t bar_base_new = BAR_REMAP_BASE_1;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		bar_base = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		/*Write a new BAR out of PCI BAR memory hole*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base_new);
		DBG_INFO("W reg[%xH] = [%xH]", PCIR_BAR(0), bar_base_new);
		/*Read the BAR memory should generate #PF*/
		is_pass = test_for_exception(PF_VECTOR, test_PCI_read_bar_memory_PF, (void *)&bar_base_new);
		/*Resume the original BAR address*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base);
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name: PCIe config space and host_Unmapping upon BAR writes_001
 *
 * Summary: When a vCPU writes a guest PCI configuration base address register (BAR)
 * of an assigned PCIe device, ACRN hypervisor shall guarantee that the old guest
 * physical base address is mapped to none.
 */
static __unused void pci_rqmid_28780_PCIe_config_space_and_host_Unmapping_upon_BAR_writes_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t bar_base = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		bar_base = reg_val;
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, BAR_REMAP_BASE);
		is_pass = test_for_exception(PF_VECTOR, test_PCI_read_bar_memory_PF, &bar_base);
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base);
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name: PCIe config space and host_Mapping upon BAR writes_001
 *
 * Summary: When a vCPU writes a guest PCI configuration base address register (BAR) of
 * an assigned PCIe device and the new guest physical base address in the guest BAR is
 * in guest PCI hole, ACRN hypervisor shall guarantee that the control function corresponding
 * to the guest BAR is mapped to the new guest physical base address.
 */
static __unused
void pci_rqmid_28741_PCIe_config_space_and_host_Mapping_upon_BAR_writes_001(void)
{
	union pci_bdf bdf = {0};
	mem_size bar_base = 0U;
	uint32_t bar_base_old = 0U;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		bar_base_old = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		reg_val = pci_pdev_read_mem(bdf, (mem_size)phys_to_virt(bar_base_old + HCIVERSION), 2);
		if (reg_val != 0xFFFF) {
			pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, BAR_REMAP_BASE_2);
			reg_val = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
			DBG_INFO("R reg[%xH] = [%xH]", PCIR_BAR(0), reg_val);
			bar_base = SHIFT_MASK(31, 4) & reg_val;
			bar_base = (mem_size)phys_to_virt(bar_base + HCIVERSION);
			reg_val = pci_pdev_read_mem(bdf, bar_base, 2);
			pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base_old);
		}
		is_pass = (HCIVERSION_VALUE == reg_val) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Mapping upon BAR writes_002
 *
 * Summary: When a vCPU writes a guest PCI configuration base address register (BAR) of
 * an assigned PCIe device and the new guest physical base address in the guest BAR is
 * in guest PCI hole, ACRN hypervisor shall guarantee that the control function corresponding
 * to the guest BAR is mapped to the new guest physical base address.
 */
static __unused
void pci_rqmid_28862_PCIe_config_space_and_host_Mapping_upon_BAR_writes_002(void)
{
	union pci_bdf bdf = {0};
	mem_size bar_base = 0U;
	uint32_t bar_base_old = 0U;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		bar_base_old = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, BAR_REMAP_BASE);
		reg_val = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		DBG_INFO("R reg[%xH] = [%xH]", PCIR_BAR(0), reg_val);
		bar_base = SHIFT_MASK(31, 4) & reg_val;
		bar_base = (mem_size)phys_to_virt(bar_base + GBECSR_5B54);
		reg_val = pci_pdev_read_mem(bdf, bar_base, 4);
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_base_old);
		reg_val &= SHIFT_LEFT(1, 15);
		is_pass = (reg_val == SHIFT_LEFT(1, 15)) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_PCI hole range_002
 *
 * Summary: ACRN hypervisor shall limit a fixed PCI hole address
 * range (memory 0xC0000000-0xdfffffff)to one VM.
 */
static __unused
void pci_rqmid_28863_PCIe_config_space_and_host_PCI_hole_range_002(void)
{
	union pci_bdf bdf = {0};
	mem_size bar_base = 0U;
	uint32_t bar_val = 0U;
	uint32_t reg_val = 0u;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		/*clear BAR register of all valid device BAR, before test*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, 0x00000000);
		is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
		if (is_pass) {
			pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, 0x00000000);
			for (bar_val = BAR_HOLE_LOW; bar_val < BAR_HOLE_HIGH; bar_val += BAR_ALLIGN_NET) {
				pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_val);
				bar_base = SHIFT_UMASK(31, 0);
				bar_base = SHIFT_MASK(31, 4) & bar_val;
				bar_base = (mem_size)phys_to_virt(bar_base + GBECSR_5B54);
				reg_val = pci_pdev_read_mem(bdf, bar_base, 4);
				reg_val &= SHIFT_LEFT(1, 15);
				ret |= pci_data_check(bdf, bar_base, 4, SHIFT_LEFT(1, 15), reg_val, false);
			}
			is_pass = (ret == OK) ? true : false;
		}
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name: PCIe config space and host_PCI hole range_001
 *
 * Summary: ACRN hypervisor shall limit a fixed PCI hole address
 * range (memory 0xC0000000-0xdfffffff)to one VM.
 */
static __unused
void pci_rqmid_28743_PCIe_config_space_and_host_PCI_hole_range_001(void)
{
	union pci_bdf bdf = {0};
	mem_size bar_base = 0U;
	uint32_t bar_val = 0U;
	uint32_t reg_val = 0u;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		/*clear BAR register of all valid device BAR, before test*/
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, 0x00000000);
		is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
		if (is_pass) {
			pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, 0x00000000);
			for (bar_val = BAR_HOLE_LOW; bar_val < BAR_HOLE_HIGH; bar_val += BAR_ALLIGN_USB) {
				pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, bar_val);
				bar_base = SHIFT_UMASK(31, 0);
				bar_base = SHIFT_MASK(31, 4) & bar_val;
				bar_base = (mem_size)phys_to_virt(bar_base + HCIVERSION);
				reg_val = pci_pdev_read_mem(bdf, bar_base, 2);
				ret |= pci_data_check(bdf, bar_base, 2, HCIVERSION_VALUE, reg_val, false);
			}
			is_pass = (ret == OK) ? true : false;
		}
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name: PCIe_config_space_and_host_Device_Identification_Registers_004
 *
 * Summary: When a vCPU attempts to read a guest PCI configuration
 * device identification register of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU
 * gets the value in the physical PCI configuration device identification register of the PCIe device.
 */
static __unused
void pci_rqmid_26245_PCIe_config_space_and_host_Device_Identification_Registers_004(void)
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
 * @brief case name: PCIe_config_space_and_host_Device_Identification_Registers_007
 *
 * Summary: When a vCPU attempts to read a guest PCI configuration
 * device identification register of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU
 * gets the value in the physical PCI configuration device identification register of the PCIe device.
 */
static __unused
void pci_rqmid_28814_PCIe_config_space_and_host_Device_Identification_Registers_007(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_HEADER_TYPE, 1);
		ret |= pci_data_check(bdf, PCI_HEADER_TYPE, 1, PCI_NET_HEADER_TYPE, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Device_Identification_Registers_001
 *
 * Summary: When a vCPU attempts to read a guest PCI configuration
 * device identification register of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU
 * gets the value in the physical PCI configuration device identification register of the PCIe device.
 */
static __unused
void pci_rqmid_26169_PCIe_config_space_and_host_Device_Identification_Registers_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_VENDOR_ID, 2);
		ret |= pci_data_check(bdf, PCI_VENDOR_ID, 2, PCI_USB_VENDOR_ID, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Device_Identification_Registers_006
 *
 * Summary: When a vCPU attempts to read a guest PCI configuration
 * device identification register of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU
 * gets the value in the physical PCI configuration device identification register of the PCIe device.
 */
static __unused
void pci_rqmid_28813_PCIe_config_space_and_host_Device_Identification_Registers_006(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_REVISION_ID, 1);
		ret |= pci_data_check(bdf, PCI_REVISION_ID, 1, PCI_NET_REVISION_ID, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Device_Identification_Registers_008
 *
 * Summary: When a vCPU attempts to read a guest PCI configuration
 * device identification register of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU
 * gets the value in the physical PCI configuration device identification register of the PCIe device.
 */
static __unused
void pci_rqmid_28815_PCIe_config_space_and_host_Device_Identification_Registers_008(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_VENDOR_ID, 2);
		ret |= pci_data_check(bdf, PCI_VENDOR_ID, 2, PCI_NET_VENDOR_ID, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Device_Identification_Registers_005
 *
 * Summary: When a vCPU attempts to read a guest PCI configuration
 * device identification register of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU
 * gets the value in the physical PCI configuration device identification register of the PCIe device.
 */
static __unused
void pci_rqmid_28812_PCIe_config_space_and_host_Device_Identification_Registers_005(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_DEVICE_ID, 2);
		ret |= pci_data_check(bdf, PCI_DEVICE_ID, 2, PCI_NET_DEVICE_ID, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Device_Identification_Registers_002
 *
 * Summary: When a vCPU attempts to read a guest PCI configuration
 * device identification register of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU
 * gets the value in the physical PCI configuration device identification register of the PCIe device.
 */
static __unused
void pci_rqmid_26243_PCIe_config_space_and_host_Device_Identification_Registers_002(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_DEVICE_ID, 2);
		ret |= pci_data_check(bdf, PCI_DEVICE_ID, 2, PCI_USB_DEVICE_ID, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Device_Identification_Registers_003
 *
 * Summary: When a vCPU attempts to read a guest PCI configuration
 * device identification register of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU
 * gets the value in the physical PCI configuration device identification register of the PCIe device.
 */
static __unused
void pci_rqmid_26244_PCIe_config_space_and_host_Device_Identification_Registers_003(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_REVISION_ID, 1);
		ret |= pci_data_check(bdf, PCI_REVISION_ID, 1, PCI_USB_REVISION_ID, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Capabilities_Pointer_Register_002
 *
 * Summary: ACRN hypervisor shall expose PCI configuration capabilities pointer register of any PCIe device
 * to the VM which the PCIe device is assigned to, in compliance with Chapter 6.7, PCI LBS and Charter
 * 3.2.5.12, PCI-to-PCI BAS.
 */
static __unused
void pci_rqmid_28816_PCIe_config_space_and_host_Capabilities_Pointer_Register_002(void)
{
	union pci_bdf bdf = {0};
	bool is_pass = false;
	uint32_t reg_val = 0U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_CAPABILITY_LIST, 1);
		DBG_INFO("PCI_CAPABILITY_LIST reg_val=%x ", reg_val);
		is_pass = is_cap_ptr_exist(net_cap_list, ELEMENT_NUM(net_cap_list), reg_val);
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Capabilities_Pointer_Register_001
 *
 * Summary: ACRN hypervisor shall expose PCI configuration capabilities pointer register of any PCIe device
 * to the VM which the PCIe device is assigned to, in compliance with Chapter 6.7, PCI LBS and Charter
 * 3.2.5.12, PCI-to-PCI BAS.
 */
static __unused
void pci_rqmid_26803_PCIe_config_space_and_host_Capabilities_Pointer_Register_001(void)
{
	union pci_bdf bdf = {0};
	bool is_pass = false;
	uint32_t reg_val = 0U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_CAPABILITY_LIST, 1);
		DBG_INFO("PCI_CAPABILITY_LIST reg_val=%x ", reg_val);
		is_pass = is_cap_ptr_exist(usb_cap_list, ELEMENT_NUM(usb_cap_list), reg_val);
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Interrupt Line Register_001
 *
 * Summary: ACRN hypervisor shall expose PCI configuration interrupt line register
 * of any PCIe device to the VM which the PCIe device is assigned to,
 * in compliance with Chapter 7.5.1.5, PCIe BS.
 */
static __unused
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
		ret |= pci_data_check(bdf, PCI_INTERRUPT_LINE, 1, 0xFF, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Interrupt Line Register_002
 *
 * Summary: ACRN hypervisor shall expose PCI configuration interrupt line register
 * of any PCIe device to the VM which the PCIe device is assigned to,
 * in compliance with Chapter 7.5.1.5, PCIe BS.
 */
static __unused
void pci_rqmid_28818_PCIe_config_space_and_host_Interrupt_Line_Register_002(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_INTERRUPT_LINE, 1);
		DBG_INFO("Net Interrupt_Line reg_val = 0x%x", reg_val);
		ret |= pci_data_check(bdf, PCI_INTERRUPT_LINE, 1, 0xFF, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name: PCIe_config_space_and_host_Cache_Line_Size_Register_002
 *
 * Summary: ACRN hypervisor shall expose PCI configuration cache line size register
 * of any PCIe device to the VM which the PCIe device is assigned to, in compliance with Chapter 7.5.1.3, PCIe BS.
 */
static __unused
void pci_rqmid_28820_PCIe_config_space_and_host_Cache_Line_Size_Register_002(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_CACHE_LINE_SIZE, 4);
		DBG_INFO("ethernet Cache_Line reg_val = 0x%x", reg_val);
		reg_val &= 0xFFU;
		ret |= pci_data_check(bdf, PCI_CACHE_LINE_SIZE, 1, 0x00, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Cache_Line_Size_Register_001
 *
 * Summary: ACRN hypervisor shall expose PCI configuration cache line size register
 * of any PCIe device to the VM which the PCIe device is assigned to, in compliance with Chapter 7.5.1.3, PCIe BS.
 */
static __unused
void pci_rqmid_26818_PCIe_config_space_and_host_Cache_Line_Size_Register_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_CACHE_LINE_SIZE, 4);
		DBG_INFO("USB Cache_Line reg_val = 0x%x", reg_val);
		reg_val &= 0xFFU;
		ret |= pci_data_check(bdf, PCI_CACHE_LINE_SIZE, 1, 0x00, reg_val, false);
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
static __unused void pci_rqmid_26793_PCIe_config_space_and_host_Status_Register_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		/*Dump the #status Register value for the USB devices in the hypervisor environment*/
		reg_val = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		DBG_INFO("Dump the status Register value [%xH] = [%xH]", PCI_STATUS, reg_val);
		is_pass = (PCI_USB_STATUS_VAL_NATIVE == reg_val) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
	return;
}

/*
 * @brief case name: PCIe_config_space_and_host_Command_Register_002
 *
 * Summary: ACRN hypervisor shall expose PCI configuration command register [bit 15:11, 9:0]
 * of any PCIe device to the VM which the PCIe device is assigned to, in compliance
 * with Chapter 7.5.1.1, PCIe BS.
 */
static __unused void pci_rqmid_28830_PCIe_config_space_and_host_Command_Register_002(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t reg_val1 = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		DBG_INFO("NET PCI Command Val = 0x%x ", reg_val);
		/*set readable-writable bit*/
		reg_val = SHIFT_LEFT(0x1, 10);
		reg_val |= SHIFT_LEFT(0x1, 8);
		reg_val |= SHIFT_LEFT(0x1, 6);
		reg_val |= SHIFT_LEFT(0x1, 2);
		pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, reg_val);
		reg_val1 = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		ret |= pci_data_check(bdf, PCI_COMMAND, 2, reg_val1, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Command_Register_001
 *
 * Summary: ACRN hypervisor shall expose PCI configuration command register [bit 15:11, 9:0]
 * of any PCIe device to the VM which the PCIe device is assigned to, in compliance
 * with Chapter 7.5.1.1, PCIe BS.
 */
static __unused void pci_rqmid_26794_PCIe_config_space_and_host_Command_Register_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t reg_val1 = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		DBG_INFO("USB PCI Command Val = 0x%x", reg_val);
		/*set readable-writable bit*/
		reg_val = SHIFT_LEFT(0x1, 10);
		reg_val |= SHIFT_LEFT(0x1, 8);
		reg_val |= SHIFT_LEFT(0x1, 6);
		reg_val |= SHIFT_LEFT(0x1, 2);
		pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, reg_val);
		reg_val1 = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		ret |= pci_data_check(bdf, PCI_COMMAND, 2, reg_val1, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Address register start-up_001
 *
 * Summary: ACRN hypervisor shall set initial guest PCI address register to 00FFFF00H following start-up.
 */
static __unused void pci_rqmid_29069_PCIe_config_space_and_host_Address_register_start_up_001(void)
{
	bool is_pass = false;
	/*Read the BP start-up PCI address register 0xCF8*/
	uint32_t reg_val = 0U;
	reg_val = *(volatile uint32_t *)(BP_IO_PORT_ADDR);
	DBG_INFO("The BP start-up PCI address value = %x", reg_val);
	/*The read value should be 00FFFF00H.*/
	is_pass = (reg_val == 0x00FFFF00U);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Address register init_001
 *
 * Summary: ACRN hypervisor shall keep initial guest PCI address register unchanged following INIT.
 */
static __unused void pci_rqmid_37250_PCIe_config_space_and_host_Address_register_init_001(void)
{
	bool is_pass = false;
	/*Read the AP init PCI address register 0xCF8*/
	uint32_t reg_val = 0U;
	reg_val = *(volatile uint32_t *)(AP_IO_PORT_ADDR);
	DBG_INFO("The AP init PCI address value = %x", reg_val);
	/*The read value should be 00FFFF00H.*/
	is_pass = (reg_val == 0x00FFFF00U);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Address register init_002
 *
 * Summary: ACRN hypervisor shall keep initial guest PCI address register unchanged following INIT.
 */
static __unused void pci_rqmid_37251_PCIe_config_space_and_host_Address_register_init_002(void)
{
	bool is_pass = false;
	is_pass = ((unchange_reg_value_0 == unchange_reg_value_1) && (A_NEW_VALUE_UNCHANGE == unchange_reg_value_0));
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_device interrupt line register value start-up_001
 *
 * Summary: ACRN hypervisor shall set initial guest PCI configuration interrupt line register
 * of any assigned PCIe device to FFH following start-up.
 */
static __unused
void pci_rqmid_29084_PCIe_config_space_and_host_device_interrupt_line_register_value_start_up_001(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	is_pass = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	if (is_pass) {
		reg_val = *(volatile uint32_t *)(BP_NET_INTERRUP_LINE_ADDR);
		is_pass = ((reg_val & 0xFFU) == 0xFFU) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_device interrupt line register value start-up_002
 *
 * Summary: ACRN hypervisor shall set initial guest PCI configuration interrupt line register
 * of any assigned PCIe device to FFH following start-up.
 */
static __unused
void pci_rqmid_29083_PCIe_config_space_and_host_device_interrupt_line_register_value_start_up_002(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	is_pass = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_pass) {
		reg_val = *(volatile uint32_t *)(BP_USB_INTERRUP_LINE_ADDR);
		is_pass = ((reg_val & 0xFFU) == 0xFFU) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name: PCIe config space and host_device interrupt line register value init_001
 *
 * Summary: ACRN hypervisor shall keep guest PCI configuration interrupt line register
 * of any assigned PCIe device unchanged following INIT.
 */
static __unused
void pci_rqmid_37255_PCIe_config_space_and_host_device_interrupt_line_register_value_init_001(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	is_pass = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_pass) {
		reg_val = *(volatile uint32_t *)(AP_USB_INTERRUP_LINE_ADDR0);
		is_pass = ((reg_val & 0xFFU) == 0xFFU) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name: PCIe config space and host_device interrupt line register value init_002
 *
 * Summary: ACRN hypervisor shall keep guest PCI configuration interrupt line register
 * of any assigned PCIe device unchanged following INIT.
 */
static __unused
void pci_rqmid_37256_PCIe_config_space_and_host_device_interrupt_line_register_value_init_002(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	is_pass = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	if (is_pass) {
		reg_val = *(volatile uint32_t *)(AP_NET_INTERRUP_LINE_ADDR0);
		is_pass = ((reg_val & 0xFFU) == 0xFFU) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_device interrupt line register value init_003
 *
 * Summary: ACRN hypervisor shall keep guest PCI configuration interrupt line register
 * of any assigned PCIe device unchanged following INIT.
 */
static __unused
void pci_rqmid_37263_PCIe_config_space_and_host_device_interrupt_line_register_value_init_003(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	is_pass = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	if (is_pass) {
		reg_val = *(volatile uint32_t *)(AP_NET_INTERRUP_LINE_ADDR1);
		is_pass = ((reg_val & 0xFFU) == 0xFFU) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_device interrupt line register value init_004
 *
 * Summary: ACRN hypervisor shall keep guest PCI configuration interrupt line register
 * of any assigned PCIe device unchanged following INIT.
 */
static __unused
void pci_rqmid_37264_PCIe_config_space_and_host_device_interrupt_line_register_value_init_004(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	is_pass = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_pass) {
		reg_val = *(volatile uint32_t *)(AP_USB_INTERRUP_LINE_ADDR1);
		is_pass = ((reg_val & 0xFFU) == 0xFFU) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Host bridge subsystem vendor ID_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration subsystem vendor ID register of its own hostbridge,
 * ACRN hypervisor shall guarantee that the vCPU gets 0000H.
 */
static __unused
void pci_rqmid_29045_PCIe_config_space_and_host_Host_bridge_subsystem_vendor_ID_001(void)
{
	union pci_bdf bdf = {
		.bits = {
			.b = 0x0,
			.d = 0x0,
			.f = 0x0
		}
	};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_SUBSYSTEM_VENDOR_ID, 2);
		ret = pci_data_check(bdf, PCI_SUBSYSTEM_VENDOR_ID, 2, 0x0000, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Host_bridge_subsystem_ID_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration subsystem ID register of its own hostbridge,
 * ACRN hypervisor shall guarantee that the vCPU gets 0000H.
 */
static __unused
void pci_rqmid_29046_PCIe_config_space_and_host_Host_bridge_subsystem_ID_001(void)
{
	union pci_bdf bdf = {
		.bits = {
			.b = 0x0,
			.d = 0x0,
			.f = 0x0
		}
	};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	int ret = OK;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_SUBSYSTEM_ID, 2);
		ret = pci_data_check(bdf, PCI_SUBSYSTEM_ID, 2, 0x0000, reg_val, false);
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Address register clear_003
 *
 * Summary: When a vCPU attempts to access a guest PCI configuration register
 * and current guest PCI address register value [bit 31] is 1H,
 * ACRN hypervisor shall set the guest address register to 00FFFF00H.
 */
static __unused
void pci_rqmid_29062_PCIe_config_space_and_host_Address_register_clear_003(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		reg_val = pio_read32((uint16_t)PCI_CONFIG_ADDR);
		is_pass = (0x00FFFF00U == reg_val) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Address register clear_001
 *
 * Summary: When a vCPU attempts to access a guest PCI configuration register
 * and current guest PCI address register value [bit 31] is 1H,
 * ACRN hypervisor shall set the guest address register to 00FFFF00H.
 */
static __unused
void pci_rqmid_29058_PCIe_config_space_and_host_Address_register_clear_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, 0xFFFFFFFFU);
		reg_val = pio_read32((uint16_t)PCI_CONFIG_ADDR);
		is_pass = (0x00FFFF00U == reg_val) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Address register clear_002
 *
 * Summary: When a vCPU attempts to access a guest PCI configuration register
 * and current guest PCI address register value [bit 31] is 1H,
 * ACRN hypervisor shall set the guest address register to 00FFFF00H.
 */
static __unused
void pci_rqmid_29060_PCIe_config_space_and_host_Address_register_clear_002(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, 0xFFFFFFFFU);
		reg_val = pio_read32((uint16_t)PCI_CONFIG_ADDR);
		is_pass = (0x00FFFF00U == reg_val) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Address register clear_004
 *
 * Summary: When a vCPU attempts to access a guest PCI configuration register
 * and current guest PCI address register value [bit 31] is 1H,
 * ACRN hypervisor shall set the guest address register to 00FFFF00H.
 */
static __unused
void pci_rqmid_29063_PCIe_config_space_and_host_Address_register_clear_004(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		reg_val = pio_read32((uint16_t)PCI_CONFIG_ADDR);
		is_pass = (0x00FFFF00U == reg_val) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge Interrupt Line_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration interrupt line register of its own hostbridge,
 * ACRN hypervisor shall guarantee that the vCPU gets FFH.
 */
static __unused
void pci_rqmid_28928_PCIe_config_space_and_host_Hostbridge_Interrupt_Line_001(void)
{
	union pci_bdf bdf = {
		.bits = {
			.b = 0x0,
			.d = 0x0,
			.f = 0x0
		}
	};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_INTERRUPT_LINE, 1);
		DBG_INFO("hostbridge Interrupt Line reg_val=%x", reg_val);
		is_pass = (reg_val == 0xFF) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge command_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration command register of its own hostbridge,
 * ACRN hypervisor shall guarantee that the vCPU gets 00H.
 */
static __unused
void pci_rqmid_28930_PCIe_config_space_and_host_Hostbridge_command_001(void)
{
	union pci_bdf bdf = {
		.bits = {
			.b = 0x0,
			.d = 0x0,
			.f = 0x0
		}
	};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		is_pass = (reg_val == 0x00U) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge status_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration status register of its own hostbridge,
 * ACRN hypervisor shall guarantee that the vCPU gets 00H.
 */
static __unused
void pci_rqmid_28929_PCIe_config_space_and_host_Hostbridge_status_001(void)
{
	union pci_bdf bdf = {
		.bits = {
			.b = 0x0,
			.d = 0x0,
			.f = 0x0
		}
	};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		is_pass = (reg_val == 0x00U) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_2-byte BAR write_001
 *
 * Summary: When a vCPU attempts to write guest AX to two bytes of a guest PCI configuration base address register,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused
void pci_rqmid_28932_PCIe_config_space_and_host_2_byte_BAR_write_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t reg_val_ori = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 2, 0x2010);
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		is_pass = (reg_val_ori == reg_val) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_2-byte BAR write_002
 *
 * Summary: When a vCPU attempts to write guest AX to two bytes of a guest PCI configuration base address register,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused
void pci_rqmid_28933_PCIe_config_space_and_host_2_byte_BAR_write_002(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t reg_val_ori = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 2, 0x2010);
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		is_pass = (reg_val_ori == reg_val) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_1-byte BAR write_002
 *
 * Summary: When a vCPU attempts to write guest AL to one byte of a guest PCI configuration base address
 * register of an assigned PCIe device, ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused
void pci_rqmid_28935_PCIe_config_space_and_host_1_byte_BAR_write_002(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t reg_val_ori = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 1, 0x10);
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		is_pass = (reg_val_ori == reg_val) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_1-byte BAR write_001
 *
 * Summary: When a vCPU attempts to write guest AL to one byte of a guest PCI configuration base address
 * register of an assigned PCIe device, ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused
void pci_rqmid_28934_PCIe_config_space_and_host_1_byte_BAR_write_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t reg_val_ori = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val_ori = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 1, 0x10);
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		is_pass = (reg_val_ori == reg_val) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_2-byte BAR read_002
 *
 * Summary: When a vCPU attempts to read two byte from a guest PCI configuration
 * base address register of an assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU gets 0FFFFH.
 */
static __unused
void pci_rqmid_28937_PCIe_config_space_and_host_2_byte_BAR_read_002(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 2);
		is_pass = (reg_val == 0xFFFFU) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_1-byte BAR read_002
 *
 * Summary: When a vCPU attempts to read one byte from a guest PCI configuration
 * base address register of an assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU gets 0FFH.
 */
static __unused
void pci_rqmid_28939_PCIe_config_space_and_host_1_byte_BAR_read_002(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 1);
		is_pass = (reg_val == 0xFFU) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_1-byte BAR read_001
 *
 * Summary: When a vCPU attempts to read one byte from a guest PCI configuration
 * base address register of an assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU gets 0FFH.
 */
static __unused
void pci_rqmid_28938_PCIe_config_space_and_host_1_byte_BAR_read_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 1);
		is_pass = (reg_val == 0xFFU) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge read reserved or undefined location_001
 *
 * Summary: When a vCPU attempts to read a guest register in the PCI configuration header
 * and the guest register is different from the following
 * vendor ID register,
 * device ID register,
 * command register,
 * status register,
 * revision ID register,
 * class code register,
 * header type register,
 * subsystem ID,
 * subsystem vendor ID,
 * capabilities pointer register,
 * interrupt pin register,
 * read-write base address register,
 * interrupt line register,
 * ACRN hypervisor shall guarantee that the vCPU gets 0H.
 */
static __unused
void pci_rqmid_29913_PCIe_config_space_and_host_Hostbridge_read_reserved_or_undefined_location_001(void)
{
	union pci_bdf bdf = {0};
	int i = 0;
	uint32_t reg_val = 0x0U;
	int ret = OK;
	bool is_pass = false;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		for (i = 0; i < (ELEMENT_NUM(hostbridge_pci_cfg) - 1); i++) {
			reg_val = pci_pdev_read_cfg(bdf, hostbridge_pci_cfg[i].reg_addr,
				hostbridge_pci_cfg[i].reg_width);
			ret |= pci_data_check(bdf, hostbridge_pci_cfg[i].reg_addr,
				hostbridge_pci_cfg[i].reg_width,
				hostbridge_pci_cfg[i].reg_val, reg_val, false);
		}
	}
	is_pass = (ret == OK) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge readonly_001
 *
 * Summary: When a vCPU attempts to write a guest PCI configuration register of its own hostbridge,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused
void pci_rqmid_27486_PCIe_config_space_and_host_Hostbridge_readonly_001(void)
{
	union pci_bdf bdf = {0};
	int i = 0;
	uint32_t reg_val = 0x0U;
	int ret = OK;
	bool is_pass = false;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		for (i = 0; i < (ELEMENT_NUM(hostbridge_pci_cfg) - 1); i++) {
			pci_pdev_write_cfg(bdf, hostbridge_pci_cfg[i].reg_addr,
				hostbridge_pci_cfg[i].reg_width, 0xA5A5A5A5);
			reg_val = pci_pdev_read_cfg(bdf, hostbridge_pci_cfg[i].reg_addr,
				hostbridge_pci_cfg[i].reg_width);
			ret |= pci_data_check(bdf, hostbridge_pci_cfg[i].reg_addr,
				hostbridge_pci_cfg[i].reg_width,
				hostbridge_pci_cfg[i].reg_val, reg_val, false);
		}
	}
	is_pass = (ret == OK) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge BAR Size_001
 *
 * Summary: When a vCPU attempts to read a guest PCI configuration
 * base address register of its own hostbridge, ACRN hypervisor shall guarantee that the vCPU gets 0H.
 */
static __unused
void pci_rqmid_27469_PCIe_config_space_and_host_Hostbridge_BAR_Size_001(void)
{
	union pci_bdf bdf = {0};
	bool is_pass = false;
	uint32_t reg_val = 0x0U;
	uint32_t reg_val1 = 0x0U;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		pci_pdev_write_cfg(bdf, PCIR_BAR(0), 4, 0xFFFFFFFFU);
		reg_val = pci_pdev_read_cfg(bdf, PCIR_BAR(0), 4);
		pci_pdev_write_cfg(bdf, PCIR_BAR(1), 4, 0xFFFFFFFFU);
		reg_val1 = pci_pdev_read_cfg(bdf, PCIR_BAR(1), 4);
		is_pass = ((reg_val == 0U) && (reg_val1 == 0U));
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge Interrupt Pin_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration interrupt
 * pin register of its own hostbridge, ACRN hypervisor shall guarantee that the vCPU gets 00H.
 */
static __unused
void pci_rqmid_27470_PCIe_config_space_and_host_Hostbridge_Interrupt_Pin_001(void)
{
	union pci_bdf bdf = {0};
	bool is_pass = false;
	uint32_t reg_val = 0x0U;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_INTERRUPT_PIN, 1);
		is_pass = (reg_val == 0) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge Capabilities Pointer_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration capabilities pointer
 * register of its own hostbridge, ACRN hypervisor shall guarantee that the vCPU gets 00H.
 */
static __unused
void pci_rqmid_27471_PCIe_config_space_and_host_Hostbridge_Capabilities_Pointer_001(void)
{
	union pci_bdf bdf = {0};
	bool is_pass = false;
	uint32_t reg_val = 0x0U;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_CAPABILITY_LIST, 1);
		is_pass = (reg_val == 0) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge Header Type_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration header type
 * register of its own hostbridge, ACRN hypervisor shall guarantee that the vCPU gets 00H.
 */
static __unused
void pci_rqmid_27474_PCIe_config_space_and_host_Hostbridge_Header_Type_001(void)
{
	union pci_bdf bdf = {0};
	bool is_pass = false;
	uint32_t reg_val = 0x0U;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_HEADER_TYPE, 1);
		is_pass = (reg_val == 0) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge Class Code_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration class code
 * register of its own hostbridge, ACRN hypervisor shall guarantee that the vCPU gets 060000H.
 */
static __unused
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
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge Revision ID_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration revision ID
 * register of its own hostbridge, ACRN hypervisor shall guarantee that the vCPU gets 0BH.
 */
static __unused
void pci_rqmid_27477_PCIe_config_space_and_host_Hostbridge_Revision_ID_001(void)
{
	union pci_bdf bdf = {0};
	bool is_pass = false;
	uint32_t reg_val = 0x0U;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_CLASS_REVISION, 1);
		is_pass = (reg_val == HOSTBRIDGE_REVISION_ID) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name: PCIe config space and host_Hostbridge Device ID_001
 *
 * Summary:When a vCPU attempts to read guest PCI configuration device ID
 * register of its own hostbridge, ACRN hypervisor shall guarantee that the vCPU gets 5AF0H.
 */
static __unused
void pci_rqmid_27478_PCIe_config_space_and_host_Hostbridge_Device_ID_001(void)
{
	union pci_bdf bdf = {0};
	bool is_pass = false;
	uint32_t reg_val = 0x0U;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_DEVICE_ID, 2);
		is_pass = (reg_val == HOSTBRIDGE_DEVICE_ID) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Hostbridge Vendor ID_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration vendor ID
 * register of its own hostbridge, ACRN hypervisor shall guarantee that the vCPU gets 8086H.
 */
static __unused
void pci_rqmid_27480_PCIe_config_space_and_host_Hostbridge_Vendor_ID_001(void)
{
	union pci_bdf bdf = {0};
	bool is_pass = false;
	uint32_t reg_val = 0x0U;
	is_pass = is_dev_exist_by_bdf(pci_devs, nr_pci_devs, bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_VENDOR_ID, 2);
		is_pass = (reg_val == HOSTBRIDGE_VENDOR_ID) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Write to RO bit in a RW register_001
 *
 * Summary: When a vCPU attempts to write a guest read-only bit in PCI configuration
 * read-write register, ACRN hypervisor shall guarantee that the write to the bit is ignored.
 * it is commonly done in real-life PCIe bridges/devices.
 */
static __unused
bool check_PCIe_config_space_and_host_Write_to_RO_bit_in_a_RW_register(uint32_t device_vendor)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t reg_val1 = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, device_vendor, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		reg_val1 = reg_val | SHIFT_LEFT(15, 11);
		pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, reg_val1);
		reg_val1 = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		is_pass = ((reg_val == reg_val1) \
			&& ((SHIFT_MASK(15, 11) & reg_val1) == 0U)) ? true : false;
	}
	return is_pass;
}

static __unused
void pci_rqmid_26141_PCIe_config_space_and_host_Write_to_RO_bit_in_a_RW_register_001(void)
{
	bool is_pass = false;
	is_pass = check_PCIe_config_space_and_host_Write_to_RO_bit_in_a_RW_register(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name: PCIe config space and host_Write to RO bit in a RW register_002
 *
 * Summary: When a vCPU attempts to write a guest read-only bit in PCI configuration
 * read-write register, ACRN hypervisor shall guarantee that the write to the bit is ignored.
 * it is commonly done in real-life PCIe bridges/devices.
 */
static __unused
void pci_rqmid_28794_PCIe_config_space_and_host_Write_to_RO_bit_in_a_RW_register_002(void)
{
	bool is_pass = false;
	is_pass = check_PCIe_config_space_and_host_Write_to_RO_bit_in_a_RW_register(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Write to RO register_002
 *
 * Summary: When a vCPU attempts to write a guest read-only PCI configuration register,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused
bool check_config_space_and_host_Write_to_RO_register(uint32_t device_vendor)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t reg_val1 = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, device_vendor, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_VENDOR_ID, 4);
		reg_val1 = 0xAA55CC33;
		pci_pdev_write_cfg(bdf, PCI_VENDOR_ID, 4, reg_val1);
		reg_val1 = pci_pdev_read_cfg(bdf, PCI_VENDOR_ID, 4);
		is_pass = (reg_val == reg_val1) ? true : false;
	}
	return is_pass;
}

static __unused
void pci_rqmid_28795_PCIe_config_space_and_host_Write_to_RO_register_002(void)
{
	bool is_pass = false;
	is_pass = check_config_space_and_host_Write_to_RO_register(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Write to RO register_001
 *
 * Summary: When a vCPU attempts to write a guest read-only PCI configuration register,
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused
void pci_rqmid_26142_PCIe_config_space_and_host_Write_to_RO_register_001(void)
{
	bool is_pass = false;
	is_pass = check_config_space_and_host_Write_to_RO_register(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Interrupt_Pin_Register_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration interrupt pin register
 * of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU gets 0H.
 */
static __unused
void pci_rqmid_26806_PCIe_config_space_and_host_Interrupt_Pin_Register_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_INTERRUPT_PIN, 1);
		DBG_INFO("USB Interrupt Pin reg_val=%x", reg_val);
		is_pass = (reg_val == 0x00) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Interrupt_Pin_Register_002
 *
 * Summary: When a vCPU attempts to read guest PCI configuration interrupt pin register
 * of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU gets 0H.
 */
static __unused
void pci_rqmid_28796_PCIe_config_space_and_host_Interrupt_Pin_Register_002(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_INTERRUPT_PIN, 1);
		DBG_INFO("NET Interrupt Pin reg_val=%x", reg_val);
		is_pass = (reg_val == 0x00) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Control_of_INTx_interrupt_001
 *
 * Summary: When a vCPU attempts to read guest PCI configuration command register[bit 10]
 * of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU gets 1H.
 */
static __unused
void pci_rqmid_25232_PCIe_config_space_and_host_Control_of_INTx_interrupt_001(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		is_pass = (reg_val & PCI_COMMAND_INTX_DISABLE) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe_config_space_and_host_Control_of_INTx_interrupt_002
 *
 * Summary: When a vCPU attempts to read guest PCI configuration command register[bit 10]
 * of any assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU gets 1H.
 */
static __unused
void pci_rqmid_28797_PCIe_config_space_and_host_Control_of_INTx_interrupt_002(void)
{
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	bool is_pass = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		is_pass = (reg_val & PCI_COMMAND_INTX_DISABLE) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_2-Byte CFEH Write with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 2 bytes to guest I/O port address CFEH and
 * current guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall ignore the write.
 */
static __unused
void pci_rqmid_27735_PCIe_config_space_and_host_2_Byte_CFEH_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_Disabled_Config_Address(PCI_MSI_FLAGS, 2, 0x0001, 0xCFE);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_1-Byte Config Data Port Write with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFCH and current guest
 * PCI address register value [bit 31] is 0H, ACRN hypervisor shall ignore the write.
 */
static __unused
void pci_rqmid_27747_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_Disabled_Config_Address(PCI_MSI_DATA_64, 1, 0x01, 0xCFC);
	report("%s", is_pass, __FUNCTION__);

}

/*
 * @brief case name:PCIe config space and host_1-Byte CFEH Write with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFEH and current
 * guest PCI address register value [bit 31] is 0H,ACRN hypervisor shall ignore the write.
 */
static __unused
void pci_rqmid_27742_PCIe_config_space_and_host_1_Byte_CFEH_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_Disabled_Config_Address(PCI_MSI_FLAGS, 1, 0x01, 0xCFE);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFDH Write with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFDH and
 * current guest PCI address register value [bit 31] is 0H,ACRN hypervisor shall ignore the write.
 */
static __unused
void pci_rqmid_27744_PCIe_config_space_and_host_1_Byte_CFDH_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_Disabled_Config_Address(PCI_MSI_ADDRESS_LO, 1, 0x01, 0xCFD);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFFH Write with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFFH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall ignore the write.
 */
static __unused
void pci_rqmid_27741_PCIe_config_space_and_host_1_Byte_CFFH_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t reg_val1 = 0U;
	int count = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		pci_pdev_write_cfg_test(bdf, PCI_STATUS + 1, 1, 0xFF, 0xCFF, false);
		reg_val1 = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		if (reg_val == reg_val1) {
			count++;
		}
	}
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		pci_pdev_write_cfg_test(bdf, PCI_STATUS + 1, 1, 0xFF, 0xCFF, false);
		reg_val1 = pci_pdev_read_cfg(bdf, PCI_STATUS, 2);
		if (reg_val == reg_val1) {
			count++;
		}
	}
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte Config Data Port Write with Disabled Config Address_001
 *
 * Summary:When a vCPU attempts to write 2 bytes to guest I/O port address CFCH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall ignore the write.
 */
static __unused
void pci_rqmid_27739_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_Disabled_Config_Address(PCI_MSI_DATA_64, 2, 0x1001, 0xCFC);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Normal Write with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to write PCI data register and current guest PCI address
 * register value [bit 31] is 0H, ACRN hypervisor shall ignore the write.
 */
static __unused
void pci_rqmid_27759_PCIe_config_space_and_host_Normal_Write_with_Disabled_Config_Address_001(void)
{
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t reg_val1 = 0U;
	int count = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_1, 4);
		pci_pdev_write_cfg_test(bdf, PCI_BASE_ADDRESS_1, 4, 0x80000000, 0xCFC, false);
		reg_val1 = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_1, 4);
		if (reg_val == reg_val1) {
			count++;
		}
	}
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_1, 4);
		pci_pdev_write_cfg_test(bdf, PCI_BASE_ADDRESS_1, 4, 0x80000000, 0xCFC, false);
		reg_val1 = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_1, 4);
		if (reg_val == reg_val1) {
			count++;
		}
	}
	is_pass = (count == 2) ? true : false;
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
static __unused
void pci_rqmid_27769_PCIe_config_space_and_host_1_Byte_CFDH_Write_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	uint32_t msi_addr = 0U;
	int count = 0;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + PCI_MSI_ADDRESS_LO;
			pci_pdev_write_cfg_test(bdf, msi_addr + 1, 1, 0x10, 0xCFD, true);
			reg_val = pci_pdev_read_cfg(bdf, msi_addr, 4);
			reg_val >>= 8;
			reg_val &= 0xFF;
			if (reg_val == 0x10) {
				count++;
			}
		}
	}
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &msi_addr);
		if (ret == OK) {
			msi_addr = msi_addr + PCI_MSI_ADDRESS_LO;
			pci_pdev_write_cfg_test(bdf, msi_addr + 1, 1, 0x10, 0xCFD, true);
			reg_val = pci_pdev_read_cfg(bdf, msi_addr, 4);
			reg_val >>= 8;
			reg_val &= 0xFF;
			if (reg_val == 0x10) {
				count++;
			}
		}
	}
	is_pass = (count == 2) ? true : false;
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
static __unused
void pci_rqmid_27767_PCIe_config_space_and_host_1_Byte_CFFH_Write_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	int count = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
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
	is_pass = (count == 2) ? true : false;
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
static __unused
void pci_rqmid_27766_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Write_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_enabled_Config_Address(PCI_MSI_DATA_64, 2, 0x1234, 0xCFC);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte CFEH Write with Enabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 2 bytes to guest I/O port address CFEH and current guest
 * PCI address register value [bit 31] is 1H, ACRN hypervisor shall write guest AX to the two most
 * significant bytes of the guest configuration register located at current guest PCI address register value.
 */
static __unused
void pci_rqmid_27765_PCIe_config_space_and_host_2_Byte_CFEH_Write_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_enabled_Config_Address(PCI_MSI_FLAGS, 2, 0x0001, 0xCFE);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte Config Data Port Write with Enabled Config Address_001
 *
 * Summary: When a vCPU attempts to write 1 byte to guest I/O port address CFCH and current guest
 * PCI address register value [bit 31] is 1H, ACRN hypervisor shall write guest AL to the least
 * significant byte of the guest configuration register located at current guest PCI address register value.
 */
static __unused
void pci_rqmid_27770_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Write_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_enabled_Config_Address(PCI_MSI_DATA_64, 1, 0x01, 0xCFC);
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
static __unused
void pci_rqmid_27768_PCIe_config_space_and_host_1_Byte_CFEH_Write_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Write_with_enabled_Config_Address(PCI_MSI_FLAGS, 1, 0x11, 0xCFE);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Normal Write with Enabled Config Address_001
 *
 * Summary: When a vCPU attempts to write PCI data register and current guest PCI address
 * register value [bit 31] is 1H, ACRN hypervisor shall write guest EAX to the guest configuration
 * register located at current guest PCI address register value.
 */
static __unused
void pci_rqmid_27771_PCIe_config_space_and_host_Normal_Write_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, 0x80000000);
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		reg_val &= 0xFFFFFFF0;
		if (reg_val == 0x80000000) {
			count++;
		}
		/*reset BAR0 address*/
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, 0x00000000);
	}
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, 0x80000000);
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		reg_val &= 0xFFFFFFF0;
		if (reg_val == 0x80000000) {
			count++;
		}
		/*reset BAR0 address*/
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, 0x70000000);
	}
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte Config Data Port Read with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to read 2 bytes from guest I/O port address CFCH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall write FFFFH to guest AX.
 */
static __unused
void pci_rqmid_27774_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
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
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Normal Config Data Port Read with Disabled Config Address_001
 *
 * Summary:When a vCPU attempts to read PCI data register and current guest PCI address register
 * value [bit 31] is 0H, ACRN hypervisor shall write FFFFFFFFH to guest EAX.
 */
static __unused
void pci_rqmid_27780_PCIe_config_space_and_host_Normal_Config_Data_Port_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg_test(bdf, PCI_BASE_ADDRESS_0, 4, 0xCFC, false);
		if (reg_val ==  0xFFFFFFFFU) {
			count++;
		}
	}
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg_test(bdf, PCI_BASE_ADDRESS_0, 4, 0xCFC, false);
		if (reg_val ==  0xFFFFFFFFU) {
			count++;
		}
	}
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFFH Read with Disabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFFH and current guest
 * PCI address register value [bit 31] is 0H, ACRN hypervisor shall write FFH to guest AL.
 */
static __unused
void pci_rqmid_27775_PCIe_config_space_and_host_1_Byte_CFFH_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg_test(bdf, PCI_STATUS + 1, 1, 0xCFF, false);
		if (reg_val ==  0xFF) {
			count++;
		}
	}
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg_test(bdf, PCI_STATUS + 1, 1, 0xCFF, false);
		if (reg_val ==  0xFF) {
			count++;
		}
	}
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFDH Read with Disabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFDH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall write FFH to guest AL.
 */
static __unused
void pci_rqmid_27778_PCIe_config_space_and_host_1_Byte_CFDH_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
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
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte CFEH Read with Disabled Config Address_001
 *
 * Summary: When a vCPU attempts to read 2 bytes from guest I/O port address CFEH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall write FFFFH to guest AX.
 */
static __unused
void pci_rqmid_27773_PCIe_config_space_and_host_2_Byte_CFEH_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
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
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte Config Data Port Read with Disabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFCH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall write FFH to guest AL.
 */
static __unused
void pci_rqmid_27779_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
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
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_1-Byte CFEH Read with Disabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFEH and current
 * guest PCI address register value [bit 31] is 0H, ACRN hypervisor shall write FFH to guest AL.
 */
static __unused
void pci_rqmid_27776_PCIe_config_space_and_host_1_Byte_CFEH_Read_with_Disabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t msi_addr = 0U;
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
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
	is_pass = (count == 2) ? true : false;
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
static __unused
void pci_rqmid_27785_PCIe_config_space_and_host_1_Byte_CFFH_Read_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	int ret = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
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
	is_pass = (count == 2) ? true : false;
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
static __unused
void pci_rqmid_27786_PCIe_config_space_and_host_1_Byte_CFEH_Read_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Read_with_Enabled_Config_Address(PCI_MSI_FLAGS, 1, 0x04, 0xCFE);
	report("%s", is_pass, __FUNCTION__);

}

/*
 * @brief case name:PCIe config space and host_Normal Config Data Port Read with Enabled Config Address_001
 *
 * Summary:When a vCPU attempts to read PCI data register and current guest PCI address register
 * value [bit 31] is 1H, ACRN hypervisor shall write the value of the guest configuration register
 * located at current guest PCI address register value to guest EAX.
 */
static __unused
void pci_rqmid_27789_PCIe_config_space_and_host_Normal_Config_Data_Port_Read_with_Enabled_Config_Address_001(void)
{
	int count = 0;
	uint32_t reg_val = 0U;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		pci_pdev_write_cfg_test(bdf, PCI_BASE_ADDRESS_0, 4, 0x80000000U, 0xCFC, true);
		reg_val = pci_pdev_read_cfg_test(bdf, PCI_BASE_ADDRESS_0, 4, 0xCFC, true);
		reg_val &= 0xFFFFFFF0U;
		if (reg_val ==	0x80000000U) {
			count++;
		}
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, 0U);
	}
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		pci_pdev_write_cfg_test(bdf, PCI_BASE_ADDRESS_0, 4, 0x80000000U, 0xCFC, true);
		reg_val = pci_pdev_read_cfg_test(bdf, PCI_BASE_ADDRESS_0, 4, 0xCFC, true);
		reg_val &= 0xFFFFFFF0U;
		if (reg_val ==	0x80000000U) {
			count++;
		}
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, 0x70000000U);
	}
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte CFEH Read with Enabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 2 bytes from guest I/O port address CFEH and current guest
 * PCI address register value [bit 31] is 1H, ACRN hypervisor shall write the two most significant
 * bytes of the guest configuration register located at current guest PCI address register value to guest AX.
 */
static __unused
void pci_rqmid_27783_PCIe_config_space_and_host_2_Byte_CFEH_Read_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Read_with_Enabled_Config_Address(PCI_MSI_FLAGS, 2, 0x0404, 0xCFE);
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name:PCIe config space and host_1-Byte Config Data Port Read with Enabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 1 byte from guest I/O port address CFCH and current guest
 * PCI address register value [bit 31] is 1H,ACRN hypervisor shall write the highest significant
 * byte of the guest configuration register located at current guest PCI address register value to guest AL.
 */
static __unused
void pci_rqmid_27788_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Read_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Read_with_Enabled_Config_Address(PCI_MSI_DATA_64, 1, 0x11, 0xCFC);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_2-Byte Config Data Port Read with Enabled Config Address_001
 *
 * Summary:When a vCPU attempts to read 2 bytes from guest I/O port address CFCH and current guest
 * PCI address register value [bit 31] is 1H, ACRN hypervisor shall write the two least significant
 * bytes of the guest configuration register located at current guest PCI address register value to guest AX.
 */
static __unused
void pci_rqmid_27784_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Read_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Read_with_Enabled_Config_Address(PCI_MSI_DATA_64, 2, 0x0110, 0xCFC);
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
static __unused
void pci_rqmid_27787_PCIe_config_space_and_host_1_Byte_CFDH_Read_with_Enabled_Config_Address_001(void)
{
	bool is_pass = false;
	is_pass = test_Read_with_Enabled_Config_Address(PCI_MSI_ADDRESS_LO + 1, 1, 0x01, 0xCFD);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Normal Config Address Port Write_002
 *
 * Summary:When a vCPU attempts to write guest PCI address register, ACRN hypervisor
 * shall keep the logical and of guest EAX and 80FFFFFCH as the current guest PCI address register value.
 */
static __unused
void pci_rqmid_30962_PCIe_config_space_and_host_Normal_Config_Address_Port_Write_002(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		pci_pdev_write_cfg(bdf, 0xDC, 2, SHIFT_LEFT(0x1, 6));
		reg_val = pci_pdev_read_cfg(bdf, 0xDC, 2);
		if (reg_val == SHIFT_LEFT(0x1, 6)) {
			is_pass = true;
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Normal Config Address Port Write_001
 *
 * Summary:When a vCPU attempts to write guest PCI address register, ACRN hypervisor
 * shall keep the logical and of guest EAX and 80FFFFFCH as the current guest PCI address register value.
 */
static __unused
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
static __unused
void pci_rqmid_27790_PCIe_config_space_and_host_2_Byte_Config_Address_Port_Write_001(void)
{
	bool is_pass = false;
	uint32_t addr0 = 0U;
	uint32_t addr1 = 0U;
	uint16_t value = 0;
	/*generate a 16-bit random value*/
	value = (uint16_t)rdtsc();
	addr0 = pio_read32((uint16_t)PCI_CONFIG_ADDR);
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
static __unused
void pci_rqmid_27791_PCIe_config_space_and_host_1_Byte_Config_Address_Port_Write_001(void)
{
	bool is_pass = false;
	uint32_t addr0 = 0U;
	uint32_t addr1 = 0U;
	uint8_t value = 0;
	/*generate a 8-bit random value*/
	value = (uint16_t)rdtsc();
	addr0 = pio_read32((uint16_t)PCI_CONFIG_ADDR);
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
static __unused
void pci_rqmid_31161_PCIe_config_space_and_host_Normal_Config_Address_Port_Read_001(void)
{
	bool is_pass = false;
	int count = 0;
	uint32_t addr = 0U;
	uint32_t addr1 = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		addr = pci_pdev_calc_address(bdf, PCI_BASE_ADDRESS_0);
		pio_write32(addr, (uint16_t)PCI_CONFIG_ADDR);
		asm volatile("mov %%eax, %0\n\t" : "=m" (addr1) : : "memory");
		pio_read32((uint16_t)PCI_CONFIG_DATA);
		if (addr == addr1) {
			count++;
		}
	}

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
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe_config_space_and_host_2-Byte_Config_Address_Port_Read_001
 *
 * Summary:When a vCPU attempts to read 2 bytes from guest I/O port address CF8H,
 * ACRN hypervisor shall write FFH to guest AX.
 */
static __unused
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
static __unused
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
static __unused
void pci_rqmid_25099_PCIe_config_space_and_host_Config_Data_Port_001(void)
{
	bool is_pass = false;
	int count = 0;
	union pci_bdf bdf = {0};
	uint32_t reg_val = 0U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_DEVICE_ID, 2);
		if (PCI_NET_DEVICE_ID == reg_val) {
			count++;
		}
		reg_val = pci_pdev_read_cfg(bdf, PCI_REVISION_ID, 1);
		if (PCI_NET_REVISION_ID == reg_val) {
			count++;
		}
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, 0x80000000U);
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		reg_val &= 0xFFFFFFF0;
		if (reg_val == 0x80000000U) {
			count++;
		}
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, 0U);
	}

	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_read_cfg(bdf, PCI_DEVICE_ID, 2);
		if (PCI_USB_DEVICE_ID == reg_val) {
			count++;
		}
		reg_val = pci_pdev_read_cfg(bdf, PCI_REVISION_ID, 1);
		if (PCI_USB_REVISION_ID == reg_val) {
			count++;
		}
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, 0x80000000U);
		reg_val = pci_pdev_read_cfg(bdf, PCI_BASE_ADDRESS_0, 4);
		reg_val &= 0xFFFFFFF0;
		if (reg_val == 0x80000000U) {
			count++;
		}
		pci_pdev_write_cfg(bdf, PCI_BASE_ADDRESS_0, 4, 0x70000000U);
	}
	is_pass = (count == 6) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name:PCIe config space and host_Config Address Port_001
 *
 * Summary:ACRN hypervisor shall locate the PCI address register of any
 * VM at I/O port address CF8H.
 */
static __unused
void pci_rqmid_25292_PCIe_config_space_and_host_Config_Address_Port_001(void)
{
	bool is_pass = false;
	uint32_t reg_val = 0U;
	uint32_t reg_val1 = 0U;
	union pci_bdf bdf = {0};
	int count = 0;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_calc_address(bdf, PCI_BASE_ADDRESS_0);
		/* Write address to ADDRESS register --0xCF8*/
		pio_write32(reg_val, (uint16_t)PCI_CONFIG_ADDR);
		reg_val1 = pio_read32((uint16_t)PCI_CONFIG_ADDR);
		if (reg_val == reg_val1) {
			count++;
		}
	}
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_val = pci_pdev_calc_address(bdf, PCI_BASE_ADDRESS_0);
		/* Write address to ADDRESS register --0xCF8*/
		pio_write32(reg_val, (uint16_t)PCI_CONFIG_ADDR);
		reg_val1 = pio_read32((uint16_t)PCI_CONFIG_ADDR);
		if (reg_val == reg_val1) {
			count++;
		}
	}
	is_pass = (count == 2) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-address destination ID read-write_002
 *
 * Summary:Message address register [bit 19:12] of guest PCI configuration MSI capability
 * structure shall be a read-write bits field.
 */
static __unused
void pci_rqmid_28884_PCIe_config_space_and_host_MSI_message_address_destination_ID_read_write_002(void)
{
	bool is_pass = false;
	is_pass = test_MSI_message_address_destination_field_rw(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-address destination ID read-write_001
 *
 * Summary:Message address register [bit 19:12] of guest PCI configuration MSI capability
 * structure shall be a read-write bits field.
 */
static __unused
void pci_rqmid_28882_PCIe_config_space_and_host_MSI_message_address_destination_ID_read_write_001(void)
{
	bool is_pass = false;
	is_pass = test_MSI_message_address_destination_field_rw(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-data vector read-write_002
 *
 * Summary:Message data register [bit 7:0] of guest PCI configuration MSI capability
 * structure shall be a read-write bits field.
 */
static __unused
void pci_rqmid_28886_PCIe_config_space_and_host_MSI_message_data_vector_read_write_002(void)
{
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		/*pci probe and check MSI capability register*/
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
		/*Check if probe to MSI CAP register*/
		if (ret == OK) {
			/*Set 1byte  to the #Message Data register*/
			reg_addr = reg_addr + PCI_MSI_DATA_64;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			reg_val &= 0xFF00;
			reg_val |= 0x30;
			pci_pdev_write_cfg(bdf, reg_addr, 2, reg_val);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			DBG_INFO("Dump #Message register = 0x%x", reg_val);
			ret |= pci_data_check(bdf, reg_addr, 2, 0x30, (reg_val & 0xFFU), false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name:PCIe config space and host_MSI message-data trigger mode read_002
 *
 * Summary:When a vCPU attempts to read a guest message data register [bit 15:14] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the vCPU gets 1H.
 */
static __unused
void pci_rqmid_28909_PCIe_config_space_and_host_MSI_message_data_trigger_mode_read_002(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_data_trigger_mode_read(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-data trigger mode read_001
 *
 * Summary:When a vCPU attempts to read a guest message data register [bit 15:14] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the vCPU gets 1H.
 */
static __unused
void pci_rqmid_28908_PCIe_config_space_and_host_MSI_message_data_trigger_mode_read_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_data_trigger_mode_read(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe_config_space_and_host_MSI message-data trigger mode write_002
 *
 * Summary:When a vCPU attempts to write a guest message data register [bit 15:14]
 * of the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor
 * shall guarantee that the write to the bits is ignored.
 */
static __unused
void pci_rqmid_28889_PCIe_config_space_and_host_MSI_message_data_trigger_mode_write_002(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_data_trigger_mode_write(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name:PCIe_config_space_and_host_MSI message-data trigger mode write_001
 *
 * Summary:When a vCPU attempts to write a guest message data register [bit 15:14]
 * of the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor
 * shall guarantee that the write to the bits is ignored.
 */
static __unused
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
static __unused
void pci_rqmid_28911_PCIe_config_space_and_host_MSI_message_data_delivery_mode_read_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_data_delivery_mode_read(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-data delivery mode read_002
 *
 * Summary:When a vCPU attempts to read a guest message data register [bit 10:8] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the vCPU gets 0H.
 */
static __unused
void pci_rqmid_28910_PCIe_config_space_and_host_MSI_message_data_delivery_mode_read_002(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_data_delivery_mode_read(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-data delivery mode write_001
 *
 * Summary:When a vCPU attempts to write a guest message data register [bit 10:8]
 * of the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor
 * shall guarantee that the write to the bits is ignored.
 */
static __unused
void pci_rqmid_28892_PCIe_config_space_and_host_MSI_message_data_delivery_mode_write_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_data_delivery_mode_write(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-data delivery mode write_002
 *
 * Summary:When a vCPU attempts to write a guest message data register [bit 10:8]
 * of the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor
 * shall guarantee that the write to the bits is ignored.
 */
static __unused
void pci_rqmid_28891_PCIe_config_space_and_host_MSI_message_data_delivery_mode_write_002(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_data_delivery_mode_write(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-address interrupt message address read_001
 *
 * Summary:When a vCPU attempts to read a guest message address register [bit 64:20]
 * of the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the vCPU gets 0FEEH.
 */
static __unused
void pci_rqmid_28914_PCIe_config_space_and_host_MSI_message_address_interrupt_message_address_read_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_address_interrupt_message_address_read(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-address interrupt message address read_002
 *
 * Summary:When a vCPU attempts to read a guest message address register [bit 64:20]
 * of the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the vCPU gets 0FEEH.
 */
static __unused
void pci_rqmid_28915_PCIe_config_space_and_host_MSI_message_address_interrupt_message_address_read_002(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_address_interrupt_message_address_read(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-address interrupt message address write_001
 *
 * Summary:When a vCPU attempts to write a guest message address register [bit 64:20] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the write to the bits is ignored.
 */
static __unused
void pci_rqmid_28894_PCIe_config_space_and_host_MSI_message_address_interrupt_message_address_write_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_address_int_msg_addr_write(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
	return;
}


/*
 * @brief case name:PCIe config space and host_MSI message-address redirection hint read_002
 *
 * Summary:When a vCPU attempts to read a guest message address register [bit 3:0] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the vCPU gets 0H.
 */
static __unused
void pci_rqmid_28916_PCIe_config_space_and_host_MSI_message_address_redirection_hint_read_002(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_address_redirection_hint_read(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-address redirection hint read_001
 *
 * Summary:When a vCPU attempts to read a guest message address register [bit 3:0] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the vCPU gets 0H.
 */
static __unused
void pci_rqmid_28917_PCIe_config_space_and_host_MSI_message_address_redirection_hint_read_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_address_redirection_hint_read(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-address redirection hint write_001
 *
 * Summary:When a vCPU attempts to write a guest message address register [bit 3:0] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the write to the bits is ignored.
 */
static __unused
void pci_rqmid_28896_PCIe_config_space_and_host_MSI_message_address_redirection_hint_write_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_address_redirection_hint_write(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-address redirection hint write_002
 *
 * Summary:When a vCPU attempts to write a guest message address register [bit 3:0] of
 * the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor shall
 * guarantee that the write to the bits is ignored.
 */
static __unused
void pci_rqmid_28895_PCIe_config_space_and_host_MSI_message_address_redirection_hint_write_002(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_address_redirection_hint_write(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-control read_001
 *
 * Summary:When a vCPU attempts to read a guest message control register [bit 15:1]
 * of the PCI MSI capability structure of an assigned PCIe device,
 * ACRN hypervisor shall guarantee that the vCPU gets 40H.
 */
static __unused
void pci_rqmid_28918_PCIe_config_space_and_host_MSI_message_control_read_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_control_read(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-control read_002
 *
 * Summary:When a vCPU attempts to read a guest message control register [bit 15:1]
 * of the PCI MSI capability structure of an assigned PCIe device,
 * ACRN hypervisor shall guarantee that the vCPU gets 40H.
 */
static __unused
void pci_rqmid_28920_PCIe_config_space_and_host_MSI_message_control_read_002(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_control_read(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-control write_001
 *
 * Summary:When a vCPU attempts to write a guest message control register [bit 15:1]
 * of the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor
 * shall guarantee that the write to the bits is ignored.
 */
static __unused
void pci_rqmid_28899_PCIe_config_space_and_host_MSI_message_control_write_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_control_write(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI message-control write_002
 *
 * Summary:When a vCPU attempts to write a guest message control register [bit 15:1]
 * of the PCI MSI capability structure of an assigned PCIe device, ACRN hypervisor
 * shall guarantee that the write to the bits is ignored.
 */
static __unused
void pci_rqmid_28897_PCIe_config_space_and_host_MSI_message_control_write_002(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_message_control_write(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI Next-PTR_001
 *
 * Summary:When a vCPU attempts to read a guest next pointer of the PCI MSI capability
 * structure of an assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU gets 00H.
 */
static __unused
void pci_rqmid_28923_PCIe_config_space_and_host_MSI_Next_PTR_001(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_Next_PTR(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI Next-PTR_002
 *
 * Summary:When a vCPU attempts to read a guest next pointer of the PCI MSI capability
 * structure of an assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU gets 00H.
 */
static __unused
void pci_rqmid_28921_PCIe_config_space_and_host_MSI_Next_PTR_002(void)
{
	bool is_pass = false;
	is_pass = test_host_MSI_Next_PTR(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI capability structure_001
 *
 * Summary:ACRN hypervisor shall expose PCIe MSI capability structure of any PCIe device
 * to the VM which the PCIe device is assigned to, in compliance with Chapter 7.7, PCIe BS and Chapter 6.8, PCI LBS.
 */
static __unused
void pci_rqmid_26821_PCIe_config_space_and_host_MSI_capability_structure_001(void)
{
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		if (ret == OK) {
			/*for MSI base*/
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			/*msi-controller=0x40<<1,next-pointer=0x00,msi id=0x05*/
			ret |= pci_data_check(bdf, reg_addr, 4,\
			(SHIFT_LEFT(SHIFT_LEFT(0x40, 1), 16) | 0x05), reg_val, false);
			/*for message-address*/
			reg_addr += 4;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
			reg_val = reg_val & (SHIFT_MASK(31, 20) | SHIFT_MASK(3, 0));
			ret |= pci_data_check(bdf, reg_addr, 4, 0xFEE00000, reg_val, false);
			/*for message-addres-upper*/
			reg_addr += 4;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			ret |= pci_data_check(bdf, reg_addr, 4, 0x00000000, reg_val, false);
			DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
			/*for message-data*/
			reg_addr += 4;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			reg_val = reg_val & (SHIFT_MASK(15, 14) | SHIFT_MASK(10, 8));
			ret |= pci_data_check(bdf, reg_addr, 2, 0x0000, reg_val, false);
			DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
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
static __unused
void pci_rqmid_26823_PCIe_config_space_and_host_MSI_capability_structure_002(void)
{
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		if (ret == OK) {
			/*for message-address*/
			reg_addr += 4;
			reg_val = SHIFT_LEFT(0x01, 12);
			pci_pdev_write_cfg(bdf, reg_addr, 4, reg_val);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			reg_val = reg_val & SHIFT_MASK(19, 12);
			ret |= pci_data_check(bdf, reg_addr,\
				4, SHIFT_LEFT(0x01, 12), reg_val, false);

			/*for message-data*/
			reg_addr += 8;
			pci_pdev_write_cfg(bdf, reg_addr, 2, 0x30);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			reg_val = reg_val & SHIFT_MASK(7, 0);
			ret |= pci_data_check(bdf, reg_addr,\
								2, 0x30, reg_val, false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI capability structure_003
 *
 * Summary:ACRN hypervisor shall expose PCIe MSI capability structure of any PCIe device
 * to the VM which the PCIe device is assigned to, in compliance with Chapter 7.7, PCIe BS and Chapter 6.8, PCI LBS.
 */
static __unused
void pci_rqmid_28836_PCIe_config_space_and_host_MSI_capability_structure_003(void)
{
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		if (ret == OK) {
			/*for MSI base*/
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			/*msi-controller=0x40<<1,next-pointer=0x00,msi id=0x05*/
			ret |= pci_data_check(bdf, reg_addr, 4,\
			(SHIFT_LEFT(SHIFT_LEFT(0x40, 1), 16) | 0x05), reg_val, false);
			/*for message-address*/
			reg_addr += 4;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
			reg_val = reg_val & (SHIFT_MASK(31, 20) | SHIFT_MASK(3, 0));
			ret |= pci_data_check(bdf, reg_addr, 4, 0xFEE00000, reg_val, false);
			/*for message-addres-upper*/
			reg_addr += 4;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			ret |= pci_data_check(bdf, reg_addr, 4, 0x00000000, reg_val, false);
			DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
			/*for message-data*/
			reg_addr += 4;
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			reg_val = reg_val & (SHIFT_MASK(15, 14) | SHIFT_MASK(10, 8));
			ret |= pci_data_check(bdf, reg_addr, 2, 0x0000, reg_val, false);
			DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
		}
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_MSI capability structure_004
 *
 * Summary:ACRN hypervisor shall expose PCIe MSI capability structure of any PCIe device
 * to the VM which the PCIe device is assigned to, in compliance with Chapter 7.7, PCIe BS and Chapter 6.8, PCI LBS.
 */
static __unused
void pci_rqmid_28835_PCIe_config_space_and_host_MSI_capability_structure_004(void)
{
	int ret = OK;
	bool is_pass = false;
	union pci_bdf bdf = {0};
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		ret = pci_probe_msi_capability(bdf, &reg_addr);
		if (ret == OK) {
			/*for message-address*/
			reg_addr += 4;
			reg_val = SHIFT_LEFT(0x01, 12);
			pci_pdev_write_cfg(bdf, reg_addr, 4, reg_val);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
			reg_val = reg_val & SHIFT_MASK(19, 12);
			ret |= pci_data_check(bdf, reg_addr,\
				4, SHIFT_LEFT(0x01, 12), reg_val, false);

			/*for message-data*/
			reg_addr += 8;
			pci_pdev_write_cfg(bdf, reg_addr, 2, 0x30);
			reg_val = pci_pdev_read_cfg(bdf, reg_addr, 2);
			reg_val = reg_val & SHIFT_MASK(7, 0);
			ret |= pci_data_check(bdf, reg_addr,\
								2, 0x30, reg_val, false);
		}
		is_pass = (ret == OK) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_hostbridge_bdf_001
 *
 * Summary:ACRN hypervisor shall guarantee that BDF of the guest hostbridge is 00:0.0.
 */
static __unused
void pci_rqmid_38068_PCIe_config_space_and_host_hostbridge_bdf_001(void)
{
	int i = 0;
	bool is_pass = false;
	struct pci_dev _pci_devs[MAX_PCI_DEV_NUM];
	uint32_t _nr_pci_devs = MAX_PCI_DEV_NUM;
	pci_pdev_enumerate_dev(_pci_devs, &_nr_pci_devs);
	if (_nr_pci_devs > 0) {
		for (i = 0; i < _nr_pci_devs; i++) {
			if (_pci_devs[i].bdf.value == 0) {
				is_pass = true;
				break;
			}
		}
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_capability ID register of the PCI MSI capability_USB_001
 *
 * Summary:When a vCPU attempts to read a guest capability ID register of the PCI MSI capability structure
 * of an assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU gets 5H.
 */
static __unused
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
 * @brief case name:PCIe config space and host_capability ID register of the PCI MSI capability_Ethernet_001
 *
 * Summary:When a vCPU attempts to read a guest capability ID register of the PCI MSI capability structure
 * of an assigned PCIe device, ACRN hypervisor shall guarantee that the vCPU gets 5H.
 */
static __unused
void pci_rqmid_38094_PCIe_config_space_and_host_capability_ID_register_of_the_PCI_MSI_capability_Ethernet_001(void)
{
	bool is_pass = false;
	uint32_t reg_value1 = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_value1 = pci_pdev_read_cfg(bdf, ETH_MSI_REG_OFFSET, 1);
		is_pass = (reg_value1 == MSI_CAPABILITY_ID) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_Write_PCI_cammand_register_USB_001
 *
 * Summary:When a vCPU attempts to write guest PCI command register [bit 10],
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused
void pci_rqmid_38097_PCIe_config_space_and_host_Write_PCI_cammand_register_USB_001(void)
{
	bool is_pass = false;
	uint32_t reg_value = 0U;
	uint32_t reg_value1 = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_value = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		reg_value1 = reg_value;
		reg_value1 |= SHIFT_LEFT(1, 10);
		pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, reg_value1);
		reg_value1 = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		is_pass = (reg_value1 == reg_value) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name:PCIe config space and host_Write_PCI_cammand_register_Ethernet_001
 *
 * Summary:When a vCPU attempts to write guest PCI command register [bit 10],
 * ACRN hypervisor shall guarantee that the write is ignored.
 */
static __unused
void pci_rqmid_38098_PCIe_config_space_and_host_Write_PCI_cammand_register_Ethernet_001(void)
{
	bool is_pass = false;
	uint32_t reg_value = 0U;
	uint32_t reg_value1 = 0U;
	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		reg_value = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		reg_value1 = reg_value;
		reg_value1 |= SHIFT_LEFT(1, 10);
		pci_pdev_write_cfg(bdf, PCI_COMMAND, 2, reg_value1);
		reg_value1 = pci_pdev_read_cfg(bdf, PCI_COMMAND, 2);
		is_pass = (reg_value1 == reg_value) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_BAR memory map to none_USB_001
 *
 * Summary:When a vCPU attempts to access a guest physical address that maps to none,
 * ACRN hypervisor shall guarantee that the vCPU receives #PF(0)
 */
static __unused
void pci_rqmid_38089_PCIe_config_space_and_host_BAR_memory_map_to_none_USB_001(void)
{
	bool is_pass = false;
	is_pass = test_BAR_memory_map_to_none_PF(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_BAR memory map to none_ethernet_001
 *
 * Summary:When a vCPU attempts to access a guest physical address that maps to none,
 * ACRN hypervisor shall guarantee that the vCPU receives #PF(0)
 */
static __unused
void pci_rqmid_38090_PCIe_config_space_and_host_BAR_memory_map_to_none_ethernet_001(void)
{
	bool is_pass = false;
	is_pass = test_BAR_memory_map_to_none_PF(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name:PCIe config space and host_PCI configuration header register_USB_001
 *
 * Summary:Guarantee that any guest register in the PCI configuration header in the following is read-only
 */
static __unused
void pci_rqmid_38095_PCIe_config_space_and_host_PCI_configuration_header_register_USB_001(void)
{
	bool is_pass = false;
	is_pass = test_host_PCI_configuration_header_register_read_only(USB_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_PCI configuration header register_Ethernet_001
 *
 * Summary:Guarantee that any guest register in the PCI configuration header in the following is read-only
 */
static __unused
void pci_rqmid_38096_PCIe_config_space_and_host_PCI_configuration_header_register_Ethernet_001(void)
{
	bool is_pass = false;
	is_pass = test_host_PCI_configuration_header_register_read_only(NET_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and BAR_Address register init_001
 *
 * Summary:ACRN hypervisor shall keep the initial guest physical base address
 * of guest BAR mapping unchanged following INIT
 */
static __unused
void pci_rqmid_37661_PCIe_config_space_and_BAR_Address_register_init_001(void)
{
	bool is_pass = false;
	uint32_t bp_net_bar0_addr = 0U;
	uint32_t bp_usb_bar0_addr = 0U;
	bool is_net_exist = false;
	bool is_usb_exist = false;
	is_net_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	is_usb_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_net_exist && is_usb_exist) {
		bp_net_bar0_addr = *(uint32_t *)(BP_NET_BAR0_ADDR);
		bp_usb_bar0_addr = *(uint32_t *)(BP_USB_BAR0_ADDR);
		is_pass = (bp_net_bar0_addr == first_net_bar0_value)\
				&& (bp_usb_bar0_addr == first_usb_bar0_value);
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and BAR_Address register init_002
 *
 * Summary:ACRN hypervisor shall keep the initial guest physical base address
 * of guest BAR mapping unchanged following INIT
 */
static __unused
void pci_rqmid_38204_PCIe_config_space_and_BAR_Address_register_init_002(void)
{
	bool is_pass = false;
	uint32_t value0 = 0U;
	uint32_t value1 = 0U;
	bool is_net_exist = false;
	bool is_usb_exist = false;
	is_net_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	is_usb_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_net_exist && is_usb_exist) {
		value0 = first_net_new_bar0_value & 0xFFFFFFF0;
		value1 = first_usb_new_bar0_value & 0xFFFFFFF0;
		is_pass = (A_NEW_NET_BAR0_VALUE == value0)\
			&& (A_NEW_USB_BAR0_VALUE == value1)\
			&& (first_net_new_bar0_value == second_net_bar0_value)\
			&& (first_usb_new_bar0_value == second_usb_bar0_value);
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_msi_control_register_following_init_USB_001
 *
 * Summary:ACRN hypervisor shall keep guest PCI MSI control register [bit 0] unchanged following INIT.
 */
static __unused
void pci_rqmid_38099_PCIe_config_space_and_host_msi_control_register_following_init_USB_001(void)
{
	bool is_pass = false;
	bool is_usb_exist = false;
	uint32_t bp_usb_msi_ctrl = 0U;
	is_usb_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_usb_exist) {
		bp_usb_msi_ctrl = *(uint32_t *)(BP_USB_MSI_CTRL_ADDR);
		bp_usb_msi_ctrl >>= 16;
		bp_usb_msi_ctrl &= 0xFFFFU;
		is_pass = (bp_usb_msi_ctrl == first_usb_msi_ctrl) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_msi_control_register_following_init_USB_002
 *
 * Summary:ACRN hypervisor shall keep guest PCI MSI control register [bit 0] unchanged following INIT.
 */
static __unused
void pci_rqmid_38217_PCIe_config_space_and_host_msi_control_register_following_init_USB_002(void)
{
	bool is_pass = false;
	uint32_t value1 = 0U;
	uint32_t value3 = 0U;
	value1 = first_usb_new_msi_ctrl & SHIFT_LEFT(0x1, 0);
	value3 = second_usb_msi_ctrl & SHIFT_LEFT(0x1, 0);
	is_pass = (value1 == value3)\
				&& (value1 == 0x01);
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_msi_control_register_following_init_Ethernet_001
 *
 * Summary:ACRN hypervisor shall keep guest PCI MSI control register [bit 0] unchanged following INIT.
 */
static __unused
void pci_rqmid_38100_PCIe_config_space_and_host_msi_control_register_following_init_Ethernet_001(void)
{
	bool is_pass = false;
	bool is_net_exist = false;
	uint32_t bp_net_msi_ctrl = 0U;
	is_net_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	if (is_net_exist) {
		bp_net_msi_ctrl = *(uint32_t *)(BP_NET_MSI_CTRL_ADDR);
		bp_net_msi_ctrl >>= 16;
		bp_net_msi_ctrl &= 0xFFFFU;
		is_pass = (bp_net_msi_ctrl == first_net_msi_ctrl) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_msi_control_register_following_init_Ethernet_002
 *
 * Summary:ACRN hypervisor shall keep guest PCI MSI control register [bit 0] unchanged following INIT.
 */
static __unused
void pci_rqmid_38222_PCIe_config_space_and_host_msi_control_register_following_init_Ethernet_002(void)
{
	bool is_pass = false;
	uint32_t value0 = 0U;
	uint32_t value2 = 0U;
	bool is_net_exist = false;
	is_net_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	if (is_net_exist) {
		value0 = first_net_new_msi_ctrl & SHIFT_LEFT(0x1, 0);
		value2 = second_net_msi_ctrl & SHIFT_LEFT(0x1, 0);
		is_pass = (value0 == value2)\
					&& (value0 == 0x01);
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_msi_control_register_following_start_up_USB_001
 *
 * Summary:ACRN hypervisor shall set initial guest PCI MSI control register [bit 0] to 0H following start-up.
 */
static __unused
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
		is_pass = (bp_usb_msi_ctrl == 0U) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_msi_control_register_following_start_up_Ethernet_001
 *
 * Summary:ACRN hypervisor shall set initial guest PCI MSI control register [bit 0] to 0H following start-up.
 */
static __unused
void pci_rqmid_38102_PCIe_config_space_and_host_msi_control_register_following_start_up_Ethernet_001(void)
{
	bool is_pass = false;
	uint32_t bp_net_msi_ctrl = 0U;
	bool is_net_exist = false;
	is_net_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	if (is_net_exist) {
		bp_net_msi_ctrl = *(uint32_t *)(BP_NET_MSI_CTRL_ADDR);
		bp_net_msi_ctrl >>= 16;
		bp_net_msi_ctrl &= SHIFT_LEFT(0x1, 0);
		is_pass = (bp_net_msi_ctrl == 0U) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_PCI command register_following_init_USB_001
 *
 * Summary:ACRN hypervisor shall keep guest PCI command register unchanged following INIT.
 */
static __unused
void pci_rqmid_38103_PCIe_config_space_and_host_PCI_command_register_following_init_USB_001(void)
{
	bool is_pass = false;
	bool is_usb_exist = false;
	uint32_t bp_command = 0U;
	is_usb_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_usb_exist) {
		bp_command =  *(uint32_t *)(BP_USB_STATUS_COMMAND_ADDR);
		bp_command &= SHIFT_MASK(15, 0);
		is_pass = (bp_command == first_usb_command) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_PCI command register_following_init_USB_002
 *
 * Summary:ACRN hypervisor shall keep guest PCI command register unchanged following INIT.
 */
static __unused
void pci_rqmid_38246_PCIe_config_space_and_host_PCI_command_register_following_init_USB_002(void)
{
	bool is_pass = false;
	bool is_usb_exist = false;
	uint32_t command1 = 0U;
	uint32_t command2 = 0U;
	is_usb_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_usb_exist) {
		command1 = first_usb_new_command & SHIFT_LEFT(0x01, 1);
		command2 = second_usb_command & SHIFT_LEFT(0x01, 1);
		is_pass = ((command1 == command2) && (command1 == 0x2)) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_PCI command register_following_init_Ethernet_001
 *
 * Summary:ACRN hypervisor shall keep guest PCI command register unchanged following INIT.
 */
static __unused
void pci_rqmid_38104_PCIe_config_space_and_host_PCI_command_register_following_init_Ethernet_001(void)
{
	bool is_pass = false;
	bool is_net_exist = false;
	uint32_t bp_command = 0U;
	is_net_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	if (is_net_exist) {
		bp_command =  *(uint32_t *)(BP_NET_STATUS_COMMAND_ADDR);
		bp_command &= SHIFT_MASK(15, 0);
		is_pass = (bp_command == first_net_command) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_PCI command register_following_init_Ethernet_002
 *
 * Summary:ACRN hypervisor shall keep guest PCI command register unchanged following INIT.
 */
static __unused
void pci_rqmid_38249_PCIe_config_space_and_host_PCI_command_register_following_init_Ethernet_002(void)
{
	bool is_pass = false;
	bool is_net_exist = false;
	uint32_t command1 = 0U;
	uint32_t command2 = 0U;
	is_net_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	if (is_net_exist) {
		command1 = first_net_new_command & SHIFT_LEFT(0x01, 1);
		command2 = second_net_command & SHIFT_LEFT(0x01, 1);
		is_pass = ((command1 == command2) && (command1 == 0x2)) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_PCI command register_following_start_up_USB_001
 *
 * Summary:ACRN hypervisor shall set initial guest PCI command register to 0406H following start-up.
 */
static __unused
void pci_rqmid_38105_PCIe_config_space_and_host_PCI_command_register_following_start_up_USB_001(void)
{
	bool is_pass = false;
	bool is_usb_exist = false;
	uint32_t bp_command = 0U;
	is_usb_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_usb_exist) {
		bp_command = *(uint32_t *)(BP_USB_STATUS_COMMAND_ADDR);
		bp_command &= SHIFT_MASK(15, 0);
		is_pass = (bp_command == BP_START_UP_COMMAND_VALUE) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_PCI command register_following_start_up_Ethernet_001
 *
 * Summary:ACRN hypervisor shall set initial guest PCI command register to 0406H following start-up.
 */
static __unused
void pci_rqmid_38106_PCIe_config_space_and_host_PCI_command_register_following_start_up_Ethernet_001(void)
{
	bool is_pass = false;
	bool is_exist = false;
	uint32_t bp_command = 0U;
	is_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	if (is_exist) {
		bp_command = *(uint32_t *)(BP_NET_STATUS_COMMAND_ADDR);
		bp_command &= SHIFT_MASK(15, 0);
		is_pass = (bp_command == BP_START_UP_COMMAND_VALUE) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_bar_control_function_net_01_following_startup_001
 *
 * Summary:ACRN hypervisor shall map the initial guest PCI configuration base address
 * to the control function corresponding to the guest BAR following start-up.
 */
static __unused
void pci_rqmid_38074_PCIe_config_space_and_host_bar_control_function_net_01_following_startup_001(void)
{
	bool is_pass = false;
	bool is_exist = false;
	uint32_t dev_status = 0U;
	is_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	if (is_exist) {
		dev_status = *(uint32_t *)(BP_NET_DEVICE_STATUS_ADDR);
		dev_status &= SHIFT_LEFT(0x1, 19);
		is_pass = (dev_status == SHIFT_LEFT(0x1, 19)) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}


/*
 * @brief case name:PCIe config space and host_bar_control_function_usb_01_following_startup_001
 *
 * Summary:ACRN hypervisor shall map the initial guest PCI configuration base address to
 * the control function corresponding to the guest BAR following start-up.
 */
static __unused
void pci_rqmid_38076_PCIe_config_space_and_host_bar_control_function_usb_01_following_startup_001(void)
{
	bool is_pass = false;
	bool is_exist = false;
	uint32_t hci_version = 0U;
	is_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_exist) {
		hci_version = *(uint32_t *)(BP_USB_DEVICE_HCIVERSION_ADDR);
		hci_version &= SHIFT_MASK(15, 0);
		is_pass = (hci_version == 0x100) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_bar_control_function_net_02_following_init_001
 *
 * Summary:ACRN hypervisor shall keep the initial guest physical base address
 * of guest BAR mapping unchanged following INIT.
 */
static __unused
void pci_rqmid_38075_PCIe_config_space_and_host_bar_control_function_net_02_following_init_001(void)
{
	bool is_pass = false;
	bool is_exist = false;
	uint32_t bp_dev_status = 0U;
	uint32_t ap_dev_status = 0U;
	is_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	if (is_exist) {
		bp_dev_status = *(uint32_t *)(BP_NET_DEVICE_STATUS_ADDR);
		bp_dev_status &= SHIFT_LEFT(0x1, 19);
		ap_dev_status = (first_net_status & SHIFT_LEFT(0x1, 19));
		is_pass = (bp_dev_status == ap_dev_status) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_bar_control_function_net_02_following_init_002
 *
 * Summary:ACRN hypervisor shall keep the initial guest physical base address
 * of guest BAR mapping unchanged following INIT.
 */
static __unused
void pci_rqmid_38266_PCIe_config_space_and_host_bar_control_function_net_02_following_init_002(void)
{
	bool is_pass = false;
	bool is_exist = false;
	is_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR);
	if (is_exist) {
		is_pass = (first_net_rctl == second_net_rctl) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_bar_control_function_usb_02_following_init_001
 *
 * Summary:ACRN hypervisor shall keep the initial guest physical base address
 * of guest BAR mapping unchanged following INIT.
 */
static __unused
void pci_rqmid_38080_PCIe_config_space_and_host_bar_control_function_usb_02_following_init_001(void)
{
	bool is_pass = false;
	bool is_exist = false;
	uint32_t bp_ver = 0U;
	uint32_t ap_ver = 0U;
	is_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_exist) {
		bp_ver = *(uint32_t *)(BP_USB_DEVICE_HCIVERSION_ADDR);
		bp_ver &= SHIFT_MASK(15, 0);
		ap_ver = (first_usb_hci_version & SHIFT_MASK(15, 0));
		is_pass = (bp_ver == ap_ver) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config space and host_bar_control_function_usb_02_following_init_002
 *
 * Summary:ACRN hypervisor shall keep the initial guest physical base address
 * of guest BAR mapping unchanged following INIT.
 */
static __unused
void pci_rqmid_38265_PCIe_config_space_and_host_bar_control_function_usb_02_following_init_002(void)
{
	bool is_pass = false;
	bool is_exist = false;
	is_exist = is_dev_exist_by_dev_vendor(pci_devs, nr_pci_devs, USB_DEV_VENDOR);
	if (is_exist) {
		is_pass = (first_usb_dn_ctrl == second_usb_dn_ctrl) ? true : false;
	}
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config_space_and_host PCI Configuration Space Header_001
 *
 * Summary:ACRN hypervisor shall expose PCI type 00H configuration space header layout
 * of any PCIe device to the VM which the PCIe device or hostbridge is assigned to,
 * in compliance with Chapter 6.1, PCI LBS and Chapter 7.2, PCIe BS.
 */
static __unused
void pci_rqmid_38288_PCIe_config_space_and_host_PCI_Configuration_Space_Header_001(void)
{
	int count = 0;
	bool is_exist = false;
	bool is_pass = false;
	struct pci_dev _pci_devs[MAX_PCI_DEV_NUM];
	uint32_t _nr_pci_devs = MAX_PCI_DEV_NUM;
	pci_pdev_enumerate_dev(_pci_devs, &_nr_pci_devs);
	is_exist = is_dev_exist_by_dev_vendor(_pci_devs, _nr_pci_devs, HOSTBRIDGE_DEV_VENDOR);
	if (is_exist) {
		count++;
	}
	is_exist = is_dev_exist_by_dev_vendor(_pci_devs, _nr_pci_devs, USB_DEV_VENDOR);
	if (is_exist) {
		count++;
	}
	is_exist = is_dev_exist_by_dev_vendor(_pci_devs, _nr_pci_devs, NET_DEV_VENDOR);
	if (is_exist) {
		count++;
	}
	is_pass = (count == 3) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:PCIe config_space_and_host Assumption of PCI devices assigned_001
 *
 * Summary:ACRN hypervisor shall guarantee that PCIe devices
 * under one sub-bridge or switch are assigned to the same VM.
 */
static __unused
void pci_rqmid_38297_PCIe_config_space_and_host_Assumption_of_PCI_devices_assigned_001(void)
{
	bool is_pass = false;
	struct pci_dev _pci_devs[MAX_PCI_DEV_NUM];
	uint32_t _nr_pci_devs = MAX_PCI_DEV_NUM;
	pci_pdev_enumerate_dev(_pci_devs, &_nr_pci_devs);
	is_pass = is_dev_exist_by_dev_vendor(_pci_devs, _nr_pci_devs, HOSTBRIDGE_DEV_VENDOR);
	report("%s", is_pass, __FUNCTION__);
}

static __unused void print_case_list(void)
{
	printf("\t\t PCI feature case list:\n\r");
#ifdef __x86_64__

//513:PCIe_MSI register start
	printf("\t\t Case ID:%d case name:%s\n\r", 26821u,
	"PCIe config space and host_MSI capability structure_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28884u,
	"PCIe config space and host_MSI message-address destination ID read-write_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28882u,
	"PCIe config space and host_MSI message-address destination ID read-write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28886u,
	"PCIe config space and host_MSI message-data vector read-write_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28909u,
	"PCIe config space and host_MSI message-data trigger mode read_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28908u,
	"PCIe config space and host_MSI message-data trigger mode read_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28889u,
	"PCIe_config_space_and_host_MSI message-data trigger mode write_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28888u,
	"PCIe_config_space_and_host_MSI message-data trigger mode write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28911u,
	"PCIe config space and host_MSI message-data delivery mode read_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28910u,
	"PCIe config space and host_MSI message-data delivery mode read_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28892u,
	"PCIe config space and host_MSI message-data delivery mode write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28891u,
	"PCIe config space and host_MSI message-data delivery mode write_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28914u,
	"PCIe config space and host_MSI message-address interrupt message address read_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28915u,
	"PCIe config space and host_MSI message-address interrupt message address read_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28894u,
	"PCIe config space and host_MSI message-address interrupt message address write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28916u,
	"PCIe config space and host_MSI message-address redirection hint read_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28917u,
	"PCIe config space and host_MSI message-address redirection hint read_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28896u,
	"PCIe config space and host_MSI message-address redirection hint write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28895u,
	"PCIe config space and host_MSI message-address redirection hint write_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28918u,
	"PCIe config space and host_MSI message-control read_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28920u,
	"PCIe config space and host_MSI message-control read_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28899u,
	"PCIe config space and host_MSI message-control write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28897u,
	"PCIe config space and host_MSI message-control write_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28923u,
	"PCIe config space and host_MSI Next-PTR_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28921u,
	"PCIe config space and host_MSI Next-PTR_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26821u,
	"PCIe config space and host_MSI capability structure_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26823u,
	"PCIe config space and host_MSI capability structure_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28836u,
	"PCIe config space and host_MSI capability structure_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 28835u,
	"PCIe config space and host_MSI capability structure_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 38091u,
	"PCIe config space and host_capability ID register of the PCI MSI capability_USB_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38094u,
	"PCIe config space and host_capability ID register of the PCI MSI capability_Ethernet_001");
//513:PCIe_MSI register end


/*****************************<The sample case part start>********************************/
	printf("\t\t Case ID:%d case name:%s\n\r", 28936u,
	"PCIe config space and host_2-byte BAR read_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28887u,
	"PCIe config space and host_MSI message-data vector read-write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28861u,
	"PCIe config space and host_Unmapping upon BAR writes_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28792u,
	"PCIe config space and host_write Reserved register_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 28829u,
	"PCIe config space and host_Status Register_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28893u,
	"PCIe config space and host_MSI message-address interrupt message address write_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 33824u,
	"PCIe config space and host_MSI message-address interrupt message address write_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 33822u,
	"PCIe config space and host_MSI message-address destination ID read-write_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 33823u,
	"PCIe_config_space_and_host_invalid destination ID_001");
/*****************************<The sample case part end>********************************/

/*****************************<The scaling part start>********************************/
//515: PCIe_Reserved register start
	printf("\t\t Case ID:%d case name:%s\n\r", 25295u,
	"PCIe config space and host_Write Reserved register_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 25298u,
	"PCIe config space and host_Write Reserved register_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28791u,
	"PCIe config space and host_Write Reserved register_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 28792u,
	"PCIe config space and host_Write Reserved register_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 28793u,
	"PCIe_config_space_and_host_Read_Reserved_register_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26040u,
	"PCIe_config_space_and_host_Read_Reserved_register_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26105u,
	"PCIe config space and host_Write register to no-exist device_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26095u,
	"PCIe config space and host_Write register to no-exist device_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26109u,
	"PCIe config space and host_Read register from no-exist device_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38097u,
	"PCIe config space and host_Write_PCI_cammand_register_USB_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38098u,
	"PCIe config space and host_Write_PCI_cammand_register_Ethernet_001");
//515: PCIe_Reserved register end

//514: PCIe_BAR memory access start
	printf("\t\t Case ID:%d case name:%s\n\r", 28858u,
	"PCIe config space and host_BAR range limitation_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28699u,
	"PCIe config space and host_BAR range limitation_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28780u,
	"PCIe config space and host_Unmapping upon BAR writes_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28741u,
	"PCIe config space and host_Mapping upon BAR writes_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28862u,
	"PCIe config space and host_Mapping upon BAR writes_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28863u,
	"PCIe config space and host_PCI hole range_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28743u,
	"PCIe config space and host_PCI hole range_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38089u,
	"PCIe config space and host_BAR memory map to none_USB_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38090u,
	"PCIe config space and host_BAR memory map to none_ethernet_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38095u,
	"PCIe config space and host_PCI configuration header register_USB_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38096u,
	"PCIe config space and host_PCI configuration header register_Ethernet_001");
//514: PCIe_BAR memory access end

//516: PCIe_ACRN shall expose PCI configuration register start
	printf("\t\t Case ID:%d case name:%s\n\r", 26245u,
	"PCIe_config_space_and_host_Device_Identification_Registers_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 28814u,
	"PCIe_config_space_and_host_Device_Identification_Registers_007");
	printf("\t\t Case ID:%d case name:%s\n\r", 26169u,
	"PCIe_config_space_and_host_Device_Identification_Registers_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28813u,
	"PCIe_config_space_and_host_Device_Identification_Registers_006");
	printf("\t\t Case ID:%d case name:%s\n\r", 28815u,
	"PCIe_config_space_and_host_Device_Identification_Registers_008");
	printf("\t\t Case ID:%d case name:%s\n\r", 28812u,
	"PCIe_config_space_and_host_Device_Identification_Registers_005");
	printf("\t\t Case ID:%d case name:%s\n\r", 26243u,
	"PCIe_config_space_and_host_Device_Identification_Registers_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26244u,
	"PCIe_config_space_and_host_Device_Identification_Registers_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 28816u,
	"PCIe_config_space_and_host_Capabilities_Pointer_Register_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26803u,
	"PCIe_config_space_and_host_Capabilities_Pointer_Register_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26817u,
	"PCIe config space and host_Interrupt Line Register_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28818u,
	"PCIe config space and host_Interrupt Line Register_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28820u,
	"PCIe_config_space_and_host_Cache_Line_Size_Register_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26818u,
	"PCIe_config_space_and_host_Cache_Line_Size_Register_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26793u,
	"PCIe config space and host_Status Register_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28830u,
	"PCIe_config_space_and_host_Command_Register_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26794u,
	"PCIe_config_space_and_host_Command_Register_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38288u,
	"PCIe config_space_and_host PCI Configuration Space Header_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38297u,
	"PCIe config_space_and_host Assumption of PCI devices assigned_001");
//516: PCIe_ACRN shall expose PCI configuration register end

//512: PCIe_ACRN shall guarantee that the vCPU gets fixed value start
	printf("\t\t Case ID:%d case name:%s\n\r", 29045u,
	"PCIe config space and host_Host bridge subsystem vendor ID_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29046u,
	"PCIe_config_space_and_host_Host_bridge_subsystem_ID_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29062u,
	"PCIe config space and host_Address register clear_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 29058u,
	"PCIe config space and host_Address register clear_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29060u,
	"PCIe config space and host_Address register clear_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 29063u,
	"PCIe config space and host_Address register clear_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 28928u,
	"PCIe config space and host_Hostbridge Interrupt Line_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28930u,
	"PCIe config space and host_Hostbridge command_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28929u,
	"PCIe config space and host_Hostbridge status_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28932u,
	"PCIe config space and host_2-byte BAR write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28933u,
	"PCIe config space and host_2-byte BAR write_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28935u,
	"PCIe config space and host_1-byte BAR write_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28934u,
	"PCIe config space and host_1-byte BAR write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28937u,
	"PCIe config space and host_2-byte BAR read_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28939u,
	"PCIe config space and host_1-byte BAR read_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28938u,
	"PCIe config space and host_1-byte BAR read_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29913u,
	"PCIe config space and host_Hostbridge read reserved or undefined location_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27486u,
	"PCIe config space and host_Hostbridge readonly_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27469u,
	"PCIe config space and host_Hostbridge BAR Size_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27470u,
	"PCIe config space and host_Hostbridge Interrupt Pin_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27471u,
	"PCIe config space and host_Hostbridge Capabilities Pointer_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27474u,
	"PCIe config space and host_Hostbridge Header Type_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27475u,
	"PCIe config space and host_Hostbridge Class Code_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27477u,
	"PCIe config space and host_Hostbridge Revision ID_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27478u,
	"PCIe config space and host_Hostbridge Device ID_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27480u,
	"PCIe config space and host_Hostbridge Vendor ID_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26141u,
	"PCIe config space and host_Write to RO bit in a RW register_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28794u,
	"PCIe config space and host_Write to RO bit in a RW register_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 28795u,
	"PCIe config space and host_Write to RO register_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 26142u,
	"PCIe config space and host_Write to RO register_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 26806u,
	"PCIe_config_space_and_host_Interrupt_Pin_Register_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28796u,
	"PCIe_config_space_and_host_Interrupt_Pin_Register_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 25232u,
	"PCIe_config_space_and_host_Control_of_INTx_interrupt_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 28797u,
	"PCIe_config_space_and_host_Control_of_INTx_interrupt_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 38068u,
	"PCIe config space and host_hostbridge_bdf_001");

	printf("\t\t Case ID:%d case name:%s\n\r", 38074u,
	"PCIe config space and host_bar_control_function_net_01_following_startup_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38076u,
	"PCIe config space and host_bar_control_function_usb_01_following_startup_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 38075u,
	"PCIe config space and host_bar_control_function_net_02_following_init_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38266u,
	"PCIe config space and host_bar_control_function_net_02_following_init_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 38080u,
	"PCIe config space and host_bar_control_function_usb_02_following_init_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38265u,
	"PCIe config space and host_bar_control_function_usb_02_following_init_002");
#endif
//512: PCIe_ACRN shall guarantee that the vCPU gets fixed value end

//517: PCIe_IO port access start
	printf("\t\t Case ID:%d case name:%s\n\r", 27735u,
	"PCIe config space and host_2-Byte CFEH Write with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27747u,
	"PCIe config space and host_1-Byte Config Data Port Write with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27742u,
	"PCIe config space and host_1-Byte CFEH Write with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27744u,
	"PCIe config space and host_1-Byte CFDH Write with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27741u,
	"PCIe config space and host_1-Byte CFFH Write with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27739u,
	"PCIe config space and host_2-Byte Config Data Port Write with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27759u,
	"PCIe config space and host_Normal Write with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27769u,
	"PCIe config space and host_1-Byte CFDH Write with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27767u,
	"PCIe config space and host 1-Byte CFFH Write with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27766u,
	"PCIe config space and host 2-Byte Config Data Port Write with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27765u,
	"PCIe config space and host_2-Byte CFEH Write with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27770u,
	"PCIe config space and host_1-Byte Config Data Port Write with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27768u,
	"PCIe config space and host_1-Byte CFEH Write with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27771u,
	"PCIe config space and host_Normal Write with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27774u,
	"PCIe config space and host_2-Byte Config Data Port Read with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27780u,
	"PCIe config space and host_Normal Config Data Port Read with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27775u,
	"PCIe config space and host_1-Byte CFFH Read with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27778u,
	"PCIe config space and host_1-Byte CFDH Read with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27773u,
	"PCIe config space and host_2-Byte CFEH Read with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27779u,
	"PCIe config space and host_1-Byte Config Data Port Read with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27776u,
	"PCIe config space and host_1-Byte CFEH Read with Disabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27785u,
	"PCIe config space and host_1-Byte CFFH Read with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27786u,
	"PCIe config space and host_1-Byte CFEH Read with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27789u,
	"PCIe config space and host_Normal Config Data Port Read with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27783u,
	"PCIe config space and host_2-Byte CFEH Read with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27788u,
	"PCIe config space and host_1-Byte Config Data Port Read with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27784u,
	"PCIe config space and host_2-Byte Config Data Port Read with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27787u,
	"PCIe config space and host_1-Byte CFDH Read with Enabled Config Address_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 30962u,
	"PCIe config space and host_Normal Config Address Port Write_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 30960u,
	"PCIe config space and host_Normal Config Address Port Write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27790u,
	"PCIe config space and host_2-Byte Config Address Port Write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27791u,
	"PCIe config space and host_1-Byte Config Address Port Write_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 31161u,
	"PCIe config space and host_Normal Config Address Port Read_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27792u,
	"PCIe_config_space_and_host_2-Byte_Config_Address_Port_Read_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 27793u,
	"PCIe_config_space_and_host_1-Byte_Config_Address_Port_Read_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 25099u,
	"PCIe config space and host_Config Data Port_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 25292u,
	"PCIe config space and host_Config Address Port_001");
//517: PCIe_IO port access end

//511:PCIe_start-up and init start
	printf("\t\t Case ID:%d case name:%s\n\r", 29069u,
	"PCIe config space and host_Address register start-up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29084u,
	"PCIe config space and host_device interrupt line register value start-up_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 29083u,
	"PCIe config space and host_device interrupt line register value start-up_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 38101u,
	"PCIe config space and host_msi_control_register_following_start_up_USB_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38105u,
	"PCIe config space and host_PCI command register_following_start_up_USB_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38106u,
	"PCIe config space and host_PCI command register_following_start_up_Ethernet_001");
#ifdef IN_NON_SAFETY_VM
	printf("\t\t Case ID:%d case name:%s\n\r", 37250u,
	"PCIe config space and host_Address register init_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37251u,
	"PCIe config space and host_Address register init_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 37255u,
	"PCIe config space and host_device interrupt line register value init_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37256u,
	"PCIe config space and host_device interrupt line register value init_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 37263u,
	"PCIe config space and host_device interrupt line register value init_003");
	printf("\t\t Case ID:%d case name:%s\n\r", 37264u,
	"PCIe config space and host_device interrupt line register value init_004");
	printf("\t\t Case ID:%d case name:%s\n\r", 37264u,
	"PCIe config space and BAR_Address register init_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37264u,
	"PCIe config space and BAR_Address register init_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 37661u,
	"PCIe config space and BAR_Address register init_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38204u,
	"PCIe config space and BAR_Address register init_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 38099u,
	"PCIe config space and host_msi_control_register_following_init_USB_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38217u,
	"PCIe config space and host_msi_control_register_following_init_USB_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 38100u,
	"PCIe config space and host_msi_control_register_following_init_Ethernet_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38222u,
	"PCIe config space and host_msi_control_register_following_init_Ethernet_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 38103u,
	"PCIe config space and host_PCI command register_following_init_USB_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38246u,
	"PCIe config space and host_PCI command register_following_init_USB_002");
	printf("\t\t Case ID:%d case name:%s\n\r", 38104u,
	"PCIe config space and host_PCI command register_following_init_Ethernet_001");
	printf("\t\t Case ID:%d case name:%s\n\r", 38249u,
	"PCIe config space and host_PCI command register_following_init_Ethernet_002");
#endif
//511:PCIe_start-up and init end
/*****************************<The scaling part end>********************************/
#endif
}

int main(void)
{
	setup_idt();
	set_log_level(PCI_DEBUG_LEVEL);

	print_case_list();
#ifdef __x86_64__
	/*Enumerate PCI devices to pci_devs*/
	pci_pdev_enumerate_dev(pci_devs, &nr_pci_devs);
	if (nr_pci_devs == 0) {
		DBG_ERRO("pci_pdev_enumerate_dev finds no devs!");
		return 0;
	}
	printf("\nFind PCI devs nr_pci_devs = %d \n", nr_pci_devs);

	//513:PCIe_MSI register start
	pci_rqmid_26821_PCIe_config_space_and_host_MSI_capability_structure_001();
	pci_rqmid_26823_PCIe_config_space_and_host_MSI_capability_structure_002();
	pci_rqmid_28836_PCIe_config_space_and_host_MSI_capability_structure_003();
	pci_rqmid_28835_PCIe_config_space_and_host_MSI_capability_structure_004();

	pci_rqmid_28884_PCIe_config_space_and_host_MSI_message_address_destination_ID_read_write_002();
	pci_rqmid_28882_PCIe_config_space_and_host_MSI_message_address_destination_ID_read_write_001();
	pci_rqmid_28886_PCIe_config_space_and_host_MSI_message_data_vector_read_write_002();
	pci_rqmid_28909_PCIe_config_space_and_host_MSI_message_data_trigger_mode_read_002();
	pci_rqmid_28908_PCIe_config_space_and_host_MSI_message_data_trigger_mode_read_001();
	pci_rqmid_28889_PCIe_config_space_and_host_MSI_message_data_trigger_mode_write_002();
	pci_rqmid_28888_PCIe_config_space_and_host_MSI_message_data_trigger_mode_write_001();
	pci_rqmid_28911_PCIe_config_space_and_host_MSI_message_data_delivery_mode_read_001();
	pci_rqmid_28910_PCIe_config_space_and_host_MSI_message_data_delivery_mode_read_002();
	pci_rqmid_28892_PCIe_config_space_and_host_MSI_message_data_delivery_mode_write_001();

	pci_rqmid_28891_PCIe_config_space_and_host_MSI_message_data_delivery_mode_write_002();
	pci_rqmid_28914_PCIe_config_space_and_host_MSI_message_address_interrupt_message_address_read_001();
	pci_rqmid_28915_PCIe_config_space_and_host_MSI_message_address_interrupt_message_address_read_002();
	pci_rqmid_28894_PCIe_config_space_and_host_MSI_message_address_interrupt_message_address_write_001();
	pci_rqmid_28916_PCIe_config_space_and_host_MSI_message_address_redirection_hint_read_002();
	pci_rqmid_28917_PCIe_config_space_and_host_MSI_message_address_redirection_hint_read_001();
	pci_rqmid_28896_PCIe_config_space_and_host_MSI_message_address_redirection_hint_write_001();
	pci_rqmid_28895_PCIe_config_space_and_host_MSI_message_address_redirection_hint_write_002();
	pci_rqmid_28918_PCIe_config_space_and_host_MSI_message_control_read_001();
	pci_rqmid_28920_PCIe_config_space_and_host_MSI_message_control_read_002();

	pci_rqmid_28899_PCIe_config_space_and_host_MSI_message_control_write_001();
	pci_rqmid_28897_PCIe_config_space_and_host_MSI_message_control_write_002();
	pci_rqmid_28923_PCIe_config_space_and_host_MSI_Next_PTR_001();
	pci_rqmid_28921_PCIe_config_space_and_host_MSI_Next_PTR_002();
	pci_rqmid_38091_PCIe_config_space_and_host_capability_ID_register_of_the_PCI_MSI_capability_USB_001();
	pci_rqmid_38094_PCIe_config_space_and_host_capability_ID_register_of_the_PCI_MSI_capability_Ethernet_001();
	//513:PCIe_MSI register end

	union pci_bdf bdf = {0};
	bool is = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is) {
		e1000e_init(bdf);
	}

/*****************************<The sample part start>********************************/
	pci_rqmid_28936_PCIe_config_space_and_host_2_byte_BAR_read_001();
	pci_rqmid_28887_PCIe_config_space_host_MSI_message_data_vector_read_write_001();
	pci_rqmid_28792_PCIe_config_space_and_host_write_Reserved_register_004();
	pci_rqmid_28829_PCIe_config_space_and_host_Status_Register_002();
	pci_rqmid_28893_PCIe_config_space_and_host_MSI_message_address_int_msg_addr_write_002();
	pci_rqmid_33824_PCIe_config_space_and_host_MSI_message_address_int_msg_addr_write_003();
	pci_rqmid_33822_PCIe_config_space_and_host_MSI_message_address_destination_ID_read_write_003();
	pci_rqmid_33823_PCIe_config_space_and_host_invalid_destination_ID_001();
	pci_rqmid_28861_PCIe_config_space_and_host_Unmapping_upon_BAR_writes_002();
/*****************************<The sample part end>**********************************/

/*****************************<The scaling part start>********************************/
//515: PCIe_Reserved register start
	pci_rqmid_25295_PCIe_config_space_and_host_Write_Reserved_register_001();
	pci_rqmid_25298_PCIe_config_space_and_host_Write_Reserved_register_002();
	pci_rqmid_28791_PCIe_config_space_and_host_Write_Reserved_register_003();
	pci_rqmid_28792_PCIe_config_space_and_host_Write_Reserved_register_004();
	pci_rqmid_28793_PCIe_config_space_and_host_Read_Reserved_register_002();
	pci_rqmid_26040_PCIe_config_space_and_host_Read_Reserved_register_001();
	pci_rqmid_26105_PCIe_config_space_and_host_Write_register_to_no_exist_device_002();
	pci_rqmid_26095_PCIe_config_space_and_host_Write_register_to_no_exist_device_001();
	pci_rqmid_26109_PCIe_config_space_and_host_Read_register_from_no_exist_device_001();
	pci_rqmid_38097_PCIe_config_space_and_host_Write_PCI_cammand_register_USB_001();
	pci_rqmid_38098_PCIe_config_space_and_host_Write_PCI_cammand_register_Ethernet_001();
//515: PCIe_Reserved register end

//514: PCIe_BAR memory access start
	pci_rqmid_28858_PCIe_config_space_and_host_BAR_range_limitation_002();
	pci_rqmid_28699_PCIe_config_space_and_host_BAR_range_limitation_001();
	pci_rqmid_28780_PCIe_config_space_and_host_Unmapping_upon_BAR_writes_001();
	pci_rqmid_28741_PCIe_config_space_and_host_Mapping_upon_BAR_writes_001();
	pci_rqmid_28862_PCIe_config_space_and_host_Mapping_upon_BAR_writes_002();
	pci_rqmid_28863_PCIe_config_space_and_host_PCI_hole_range_002();
	pci_rqmid_28743_PCIe_config_space_and_host_PCI_hole_range_001();
	pci_rqmid_38089_PCIe_config_space_and_host_BAR_memory_map_to_none_USB_001();
	pci_rqmid_38090_PCIe_config_space_and_host_BAR_memory_map_to_none_ethernet_001();
	pci_rqmid_38095_PCIe_config_space_and_host_PCI_configuration_header_register_USB_001();
	pci_rqmid_38096_PCIe_config_space_and_host_PCI_configuration_header_register_Ethernet_001();
//514: PCIe_BAR memory access end

//516: PCIe_ACRN shall expose PCI configuration register start
	pci_rqmid_26245_PCIe_config_space_and_host_Device_Identification_Registers_004();
	pci_rqmid_28814_PCIe_config_space_and_host_Device_Identification_Registers_007();
	pci_rqmid_26169_PCIe_config_space_and_host_Device_Identification_Registers_001();
	pci_rqmid_28813_PCIe_config_space_and_host_Device_Identification_Registers_006();
	pci_rqmid_28815_PCIe_config_space_and_host_Device_Identification_Registers_008();
	pci_rqmid_28812_PCIe_config_space_and_host_Device_Identification_Registers_005();
	pci_rqmid_26243_PCIe_config_space_and_host_Device_Identification_Registers_002();
	pci_rqmid_26244_PCIe_config_space_and_host_Device_Identification_Registers_003();
	pci_rqmid_28816_PCIe_config_space_and_host_Capabilities_Pointer_Register_002();
	pci_rqmid_26803_PCIe_config_space_and_host_Capabilities_Pointer_Register_001();

	pci_rqmid_26817_PCIe_config_space_and_host_Interrupt_Line_Register_001();
	pci_rqmid_28818_PCIe_config_space_and_host_Interrupt_Line_Register_002();
	pci_rqmid_28820_PCIe_config_space_and_host_Cache_Line_Size_Register_002();
	pci_rqmid_26818_PCIe_config_space_and_host_Cache_Line_Size_Register_001();
	pci_rqmid_26793_PCIe_config_space_and_host_Status_Register_001();
	pci_rqmid_28830_PCIe_config_space_and_host_Command_Register_002();
	pci_rqmid_26794_PCIe_config_space_and_host_Command_Register_001();
	pci_rqmid_38288_PCIe_config_space_and_host_PCI_Configuration_Space_Header_001();
	pci_rqmid_38297_PCIe_config_space_and_host_Assumption_of_PCI_devices_assigned_001();
//516: PCIe_ACRN shall expose PCI configuration register end

//512: PCIe_ACRN shall guarantee that the vCPU gets fixed value start
	pci_rqmid_29045_PCIe_config_space_and_host_Host_bridge_subsystem_vendor_ID_001();
	pci_rqmid_29046_PCIe_config_space_and_host_Host_bridge_subsystem_ID_001();
	pci_rqmid_29062_PCIe_config_space_and_host_Address_register_clear_003();
	pci_rqmid_29058_PCIe_config_space_and_host_Address_register_clear_001();
	pci_rqmid_29060_PCIe_config_space_and_host_Address_register_clear_002();
	pci_rqmid_29063_PCIe_config_space_and_host_Address_register_clear_004();
	pci_rqmid_28928_PCIe_config_space_and_host_Hostbridge_Interrupt_Line_001();
	pci_rqmid_28930_PCIe_config_space_and_host_Hostbridge_command_001();
	pci_rqmid_28929_PCIe_config_space_and_host_Hostbridge_status_001();
	pci_rqmid_28932_PCIe_config_space_and_host_2_byte_BAR_write_001();

	pci_rqmid_28933_PCIe_config_space_and_host_2_byte_BAR_write_002();
	pci_rqmid_28935_PCIe_config_space_and_host_1_byte_BAR_write_002();
	pci_rqmid_28934_PCIe_config_space_and_host_1_byte_BAR_write_001();
	pci_rqmid_28937_PCIe_config_space_and_host_2_byte_BAR_read_002();
	pci_rqmid_28939_PCIe_config_space_and_host_1_byte_BAR_read_002();
	pci_rqmid_28938_PCIe_config_space_and_host_1_byte_BAR_read_001();
	pci_rqmid_29913_PCIe_config_space_and_host_Hostbridge_read_reserved_or_undefined_location_001();
	pci_rqmid_27486_PCIe_config_space_and_host_Hostbridge_readonly_001();
	pci_rqmid_27469_PCIe_config_space_and_host_Hostbridge_BAR_Size_001();
	pci_rqmid_27470_PCIe_config_space_and_host_Hostbridge_Interrupt_Pin_001();

	pci_rqmid_27471_PCIe_config_space_and_host_Hostbridge_Capabilities_Pointer_001();
	pci_rqmid_27474_PCIe_config_space_and_host_Hostbridge_Header_Type_001();
	pci_rqmid_27475_PCIe_config_space_and_host_Hostbridge_Class_Code_001();
	pci_rqmid_27477_PCIe_config_space_and_host_Hostbridge_Revision_ID_001();
	pci_rqmid_27478_PCIe_config_space_and_host_Hostbridge_Device_ID_001();
	pci_rqmid_27480_PCIe_config_space_and_host_Hostbridge_Vendor_ID_001();
	pci_rqmid_26141_PCIe_config_space_and_host_Write_to_RO_bit_in_a_RW_register_001();
	pci_rqmid_28794_PCIe_config_space_and_host_Write_to_RO_bit_in_a_RW_register_002();
	pci_rqmid_28795_PCIe_config_space_and_host_Write_to_RO_register_002();
	pci_rqmid_26142_PCIe_config_space_and_host_Write_to_RO_register_001();

	pci_rqmid_26806_PCIe_config_space_and_host_Interrupt_Pin_Register_001();
	pci_rqmid_28796_PCIe_config_space_and_host_Interrupt_Pin_Register_002();
	pci_rqmid_25232_PCIe_config_space_and_host_Control_of_INTx_interrupt_001();
	pci_rqmid_28797_PCIe_config_space_and_host_Control_of_INTx_interrupt_002();
	pci_rqmid_38068_PCIe_config_space_and_host_hostbridge_bdf_001();
	pci_rqmid_38074_PCIe_config_space_and_host_bar_control_function_net_01_following_startup_001();
	pci_rqmid_38076_PCIe_config_space_and_host_bar_control_function_usb_01_following_startup_001();
#ifdef IN_NON_SAFETY_VM
	pci_rqmid_38075_PCIe_config_space_and_host_bar_control_function_net_02_following_init_001();
	pci_rqmid_38266_PCIe_config_space_and_host_bar_control_function_net_02_following_init_002();
	pci_rqmid_38080_PCIe_config_space_and_host_bar_control_function_usb_02_following_init_001();
	pci_rqmid_38265_PCIe_config_space_and_host_bar_control_function_usb_02_following_init_002();
#endif
//512: PCIe_ACRN shall guarantee that the vCPU gets fixed value end

//517: PCIe_IO port access start
	pci_rqmid_27735_PCIe_config_space_and_host_2_Byte_CFEH_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27747_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27742_PCIe_config_space_and_host_1_Byte_CFEH_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27744_PCIe_config_space_and_host_1_Byte_CFDH_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27741_PCIe_config_space_and_host_1_Byte_CFFH_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27739_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27759_PCIe_config_space_and_host_Normal_Write_with_Disabled_Config_Address_001();
	pci_rqmid_27769_PCIe_config_space_and_host_1_Byte_CFDH_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27767_PCIe_config_space_and_host_1_Byte_CFFH_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27766_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Write_with_Enabled_Config_Address_001();

	pci_rqmid_27765_PCIe_config_space_and_host_2_Byte_CFEH_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27770_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27768_PCIe_config_space_and_host_1_Byte_CFEH_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27771_PCIe_config_space_and_host_Normal_Write_with_Enabled_Config_Address_001();
	pci_rqmid_27774_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Read_with_Disabled_Config_Address_001();
	pci_rqmid_27780_PCIe_config_space_and_host_Normal_Config_Data_Port_Read_with_Disabled_Config_Address_001();
	pci_rqmid_27775_PCIe_config_space_and_host_1_Byte_CFFH_Read_with_Disabled_Config_Address_001();
	pci_rqmid_27778_PCIe_config_space_and_host_1_Byte_CFDH_Read_with_Disabled_Config_Address_001();
	pci_rqmid_27773_PCIe_config_space_and_host_2_Byte_CFEH_Read_with_Disabled_Config_Address_001();
	pci_rqmid_27779_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Read_with_Disabled_Config_Address_001();

	pci_rqmid_27776_PCIe_config_space_and_host_1_Byte_CFEH_Read_with_Disabled_Config_Address_001();
	pci_rqmid_27785_PCIe_config_space_and_host_1_Byte_CFFH_Read_with_Enabled_Config_Address_001();
	pci_rqmid_27786_PCIe_config_space_and_host_1_Byte_CFEH_Read_with_Enabled_Config_Address_001();
	pci_rqmid_27789_PCIe_config_space_and_host_Normal_Config_Data_Port_Read_with_Enabled_Config_Address_001();
	pci_rqmid_27783_PCIe_config_space_and_host_2_Byte_CFEH_Read_with_Enabled_Config_Address_001();
	pci_rqmid_27788_PCIe_config_space_and_host_1_Byte_Config_Data_Port_Read_with_Enabled_Config_Address_001();
	pci_rqmid_27784_PCIe_config_space_and_host_2_Byte_Config_Data_Port_Read_with_Enabled_Config_Address_001();
	pci_rqmid_27787_PCIe_config_space_and_host_1_Byte_CFDH_Read_with_Enabled_Config_Address_001();
	pci_rqmid_30962_PCIe_config_space_and_host_Normal_Config_Address_Port_Write_002();
	pci_rqmid_30960_PCIe_config_space_and_host_Normal_Config_Address_Port_Write_001();

	pci_rqmid_27790_PCIe_config_space_and_host_2_Byte_Config_Address_Port_Write_001();
	pci_rqmid_27791_PCIe_config_space_and_host_1_Byte_Config_Address_Port_Write_001();
	pci_rqmid_31161_PCIe_config_space_and_host_Normal_Config_Address_Port_Read_001();
	pci_rqmid_27792_PCIe_config_space_and_host_2_Byte_Config_Address_Port_Read_001();
	pci_rqmid_27793_PCIe_config_space_and_host_1_Byte_Config_Address_Port_Read_001();
	pci_rqmid_25099_PCIe_config_space_and_host_Config_Data_Port_001();
	pci_rqmid_25292_PCIe_config_space_and_host_Config_Address_Port_001();
//517: PCIe_IO port access end

//511:PCIe_start-up and init start
	pci_rqmid_29069_PCIe_config_space_and_host_Address_register_start_up_001();
	pci_rqmid_29084_PCIe_config_space_and_host_device_interrupt_line_register_value_start_up_001();
	pci_rqmid_29083_PCIe_config_space_and_host_device_interrupt_line_register_value_start_up_002();
	pci_rqmid_38101_PCIe_config_space_and_host_msi_control_register_following_start_up_USB_001();
	pci_rqmid_38105_PCIe_config_space_and_host_PCI_command_register_following_start_up_USB_001();
	pci_rqmid_38106_PCIe_config_space_and_host_PCI_command_register_following_start_up_Ethernet_001();
#ifdef IN_NON_SAFETY_VM
	pci_rqmid_37250_PCIe_config_space_and_host_Address_register_init_001();
	pci_rqmid_37251_PCIe_config_space_and_host_Address_register_init_002();
	pci_rqmid_37255_PCIe_config_space_and_host_device_interrupt_line_register_value_init_001();
	pci_rqmid_37256_PCIe_config_space_and_host_device_interrupt_line_register_value_init_002();
	pci_rqmid_37263_PCIe_config_space_and_host_device_interrupt_line_register_value_init_003();
	pci_rqmid_37264_PCIe_config_space_and_host_device_interrupt_line_register_value_init_004();
	pci_rqmid_37661_PCIe_config_space_and_BAR_Address_register_init_001();
	pci_rqmid_38204_PCIe_config_space_and_BAR_Address_register_init_002();
	pci_rqmid_38099_PCIe_config_space_and_host_msi_control_register_following_init_USB_001();
	pci_rqmid_38217_PCIe_config_space_and_host_msi_control_register_following_init_USB_002();
	pci_rqmid_38100_PCIe_config_space_and_host_msi_control_register_following_init_Ethernet_001();
	pci_rqmid_38222_PCIe_config_space_and_host_msi_control_register_following_init_Ethernet_002();
	pci_rqmid_38103_PCIe_config_space_and_host_PCI_command_register_following_init_USB_001();
	pci_rqmid_38246_PCIe_config_space_and_host_PCI_command_register_following_init_USB_002();
	pci_rqmid_38104_PCIe_config_space_and_host_PCI_command_register_following_init_Ethernet_001();
	pci_rqmid_38249_PCIe_config_space_and_host_PCI_command_register_following_init_Ethernet_002();
#endif
//511:PCIe_start-up and init end

/************************************<The scaling part end>***********************************/
#endif
	return report_summary();
}
