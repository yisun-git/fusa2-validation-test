#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "pci_util.h"
#include "e1000e.h"
#include "delay.h"
#include "apic.h"
#include "smp.h"
#include "isr.h"
#include "fwcfg.h"

//#define PCI_DEBUG
#ifdef PCI_DEBUG
#define PCI_DEBUG_LEVEL		DBG_LVL_INFO
#else
#define PCI_DEBUG_LEVEL		DBG_LVL_ERROR
#endif

#define PCI_CONFIG_SPACE_SIZE		0x100U
#define PCI_CONFIG_RESERVE		0x36
#define BAR_REMAP_BASE		0xDFF00000
#define PCI_STATUS_VAL_NATIVE		(0x0010)
#define	MSI_NET_IRQ_VECTOR	(0x40)
#define VALID_APIC_ID_1 (0x01)
#define INVALID_APIC_ID_A (0x0A)
#define INVALID_APIC_ID_B (0xFFFF)
#define INVALID_REG_VALUE_U	(0xFFFFFFFFU)

static PCI_MAKE_BDF(usb, 0, 0x14, 0);//usb 00:14.0
static PCI_MAKE_BDF(ethernet, 0, 0x1F, 6);//ethernet 00:1f.6

static uint64_t apic_id_bitmap = 0UL;
void save_unchanged_reg(void)
{
	uint32_t id = 0U;
	id = apic_id();
	apic_id_bitmap |= SHIFT_LEFT(1UL, id);
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

	reg_val = pci_pdev_read_cfg(PCI_GET_BDF(usb), PCIR_BAR(0), 2);
	reg_val &= 0xFFFFU;
	/*Dump  the #BAR0 register*/
	printf("\nDump the #BAR0 register, #BAR0 = 0x%x \n", reg_val);
	ret |= pci_data_check(PCI_GET_BDF(usb), PCIR_BAR(0), 2, 0xFFFF, reg_val, false);
	/*Verify that the #BAR0 register value is 0xFFFF*/
	is_pass = (ret == OK) ? true : false;
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

	/*pci probe and check MSI capability register*/
	ret = pci_probe_msi_capability(PCI_GET_BDF(usb), &reg_addr);
	DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
	/*Check if probe to MSI CAP register*/
	if (ret == OK) {
		/*Set 1byte  to the #Message Data register*/
		reg_addr = reg_addr + 0xC;
		pci_pdev_write_cfg(PCI_GET_BDF(usb), reg_addr, 2, 0x30);
		reg_val = pci_pdev_read_cfg(PCI_GET_BDF(usb), reg_addr, 2);
		printf("\nDump #Message register = 0x%x \n", reg_val);
		ret |= pci_data_check(PCI_GET_BDF(usb), reg_addr, 2, 0x30, reg_val, false);
	}

	is_pass = (ret == OK) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

/*
 * @brief case name: PCIe config space and host_Unmapping upon BAR writes_002
 *
 * Summary: PCIe config space and host_Unmapping upon BAR writes_002.
 */
static __unused void pci_rqmid_28861_PCIe_config_space_and_host_Unmapping_upon_BAR_writes_002(void)
{
	uint32_t reg_val = INVALID_REG_VALUE_U;
	uint32_t bar_base = 0U;
	bool is_pass = false;

	/*Dump the #BAR0 register*/
	reg_val = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), PCIR_BAR(0), 4);
	bar_base = reg_val;
	printf("\nDump origin bar_base=%x \n", reg_val);

	/*Set the BAR0 register*/
	pci_pdev_write_cfg(PCI_GET_BDF(ethernet), PCIR_BAR(0), 4, BAR_REMAP_BASE);
	reg_val = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), PCIR_BAR(0), 4);

	printf("Dump new bar_base=%x \n", reg_val);

	/*Read the data from the address of the #BAR0 original value*/
	asm volatile(ASM_TRY("1f")
				 "mov (%%eax), %0\n\t"
				 "1:"
				: "=b" (reg_val)
				: "a" (bar_base)
				:);

	/*Verify that the #PF is triggered*/
	is_pass = (exception_vector() == PF_VECTOR) && (exception_error_code() == 0);
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

	/*Set 0xA53C to the #reserved register*/
	reg_val = 0xA53C;
	pci_pdev_write_cfg(PCI_GET_BDF(ethernet), PCI_CONFIG_RESERVE, 2, reg_val);

	reg_val = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), PCI_CONFIG_RESERVE, 2);

	/*	Dump the #reserved register*/
	printf("\nDump #PCIR_REV(0x36) = 0x%x \n", reg_val);

	/*Verify that the data from #reserved register is default value*/
	ret |= pci_data_check(PCI_GET_BDF(ethernet), PCI_CONFIG_RESERVE, 2, 0, reg_val, false);

	is_pass = (ret == OK) ? true : false;

	report("%s", is_pass, __FUNCTION__);

	return;
}

/*
 * @brief case name: PCIe config space and host_Status Register_002
 *
 * Summary: it is required by specification and commonly done in real-life PCIe bridges/devices.
 * ACRN hypervisor shall expose PCI configuration status register of any PCIe device to the VM
 * which the PCIe device is assigned to, in compliance with Chapter 7.5.1.2, PCIe BS.
 */
static __unused void pci_rqmid_28829_PCIe_config_space_and_host_Status_Register_002(void)
{
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;

	/*Dump the #status Register value for the NET devices in the hypervisor environment*/
	reg_val = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), PCI_STATUS, 2);
	printf("\nDump the status Register value [%xH] = [%xH]\n", PCI_STATUS, reg_val);

	is_pass = (PCI_STATUS_VAL_NATIVE == reg_val) ? true : false;
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
	uint32_t reg_val_ori = INVALID_REG_VALUE_U;
	uint32_t reg_addr = 0U;
	uint32_t reg_val = INVALID_REG_VALUE_U;
	bool is_pass = false;
	int ret = OK;

	/*pci probe and check MSI capability register*/
	ret = pci_probe_msi_capability(PCI_GET_BDF(ethernet), &reg_addr);
	DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
	if (ret == OK) {
		/*message address L register address = pointer + 0x4*/
		reg_addr = reg_addr + 4;

		reg_val_ori = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), reg_addr, 4);
		DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val_ori);

		uint32_t lapic_id = apic_id();
		uint32_t msg_addr = MAKE_NET_MSI_MESSAGE_ADDR(0xA5500000, lapic_id);
		/*write 0xA5500000 to Message address[20:64]*/
		pci_pdev_write_cfg(PCI_GET_BDF(ethernet), reg_addr, 4, msg_addr);
		DBG_INFO("msg_addr = %x", msg_addr);

		reg_val = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), reg_addr, 4);
		reg_val = reg_val & SHIFT_MASK(31, 20);
		DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
		/*The Message address should not be equal to 0xA5500000*/
		ret |= pci_data_check(PCI_GET_BDF(ethernet), reg_addr, 4, msg_addr, reg_val, true);

		/*The Message address should be equal to original val*/
		reg_val = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), reg_addr, 4);
		ret |= pci_data_check(PCI_GET_BDF(ethernet), reg_addr, 4, reg_val_ori, reg_val, false);

		/*Message address H register address = pointer + 0x8*/
		reg_addr = reg_addr + 4;

		reg_val_ori = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), reg_addr, 4);
		DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val_ori);

		pci_pdev_write_cfg(PCI_GET_BDF(ethernet), reg_addr, 4, 0x0000000A);

		reg_val = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), reg_addr, 4);
		DBG_INFO("R reg[%xH] = [%xH]", reg_addr, reg_val);
		/*The Message address should not be equal to 0xA5500000*/
		ret |= pci_data_check(PCI_GET_BDF(ethernet), reg_addr, 4, 0x0000000A, reg_val, true);

		/*The Message address should be equal to original val*/
		reg_val = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), reg_addr, 4);
		ret |= pci_data_check(PCI_GET_BDF(ethernet), reg_addr, 4, reg_val_ori, reg_val, false);
	}

	is_pass = (ret == OK) ? true : false;
	report("%s", is_pass, __FUNCTION__);
	return;
}

static uint64_t g_get_msi_int = 0;
static __unused int msi_int_handler(void *p_arg)
{
	uint32_t lapic_id = apic_id();
	printf("\n[ int ]Run %s(%d), lapic_id=%d \r\n", __func__, (uint32_t)(uint64_t)p_arg, lapic_id);
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

	/*pci probe and check MSI capability register*/
	ret = pci_probe_msi_capability(PCI_GET_BDF(ethernet), &reg_addr);
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
	reg_val = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), reg_addr + 4, 4);
	DBG_INFO("write MSI ADDR LOW reg[%xH] = [%xH]", reg_addr + 4, reg_val);
	reg_val = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), reg_addr + 0x0C, 2);
	DBG_INFO("write MSI ADDR DATA reg[%xH] = [%xH]", reg_addr + 0x0C, reg_val);

	is_pass = ((ret == OK) && (g_get_msi_int & SHIFT_LEFT(1UL, lapic_id))) ? true : false;
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
	unsigned nr_cpu = fwcfg_get_nb_cpus();
	nr_cpu = (nr_cpu > 2) ? 2 : nr_cpu;
	ap_lapic_id = find_first_apic_id();
	printf("\nnr_cpu=%d ap_lapic_id=%d\n", nr_cpu, ap_lapic_id);
	/*pci probe and check MSI capability register*/
	ret = pci_probe_msi_capability(PCI_GET_BDF(ethernet), &reg_addr);
	DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
	if (ret == OK) {
		/*message address L register address = pointer + 0x4*/
		for (i = 0; i < nr_cpu; i++) {
			e1000e_msi_reset();
			lapic_id = (i == 0) ? apic_id() : ap_lapic_id;
			printf("\nDump APIC id lapic_id = %x, ap_lapic_id=%x\n", lapic_id, ap_lapic_id);
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(MSI_NET_IRQ_VECTOR);
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_enable();

			reg_val = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), reg_addr + 4, 4);
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

	/*pci probe and check MSI capability register*/
	ret = pci_probe_msi_capability(PCI_GET_BDF(ethernet), &reg_addr);
	DBG_INFO("ret=%x, msi reg_addr=%x", ret, reg_addr);
	if (ret == OK) {
		/*message address L register address = pointer + 0x4*/
		e1000e_msi_reset();
		lapic_id = apic_id();
		printf("\nDump APIC id lapic_id = %x\n", lapic_id);
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

		reg_val = pci_pdev_read_cfg(PCI_GET_BDF(ethernet), reg_addr + 4, 4);
		DBG_INFO("write MSI ADDR LOW reg[%xH] = [%xH]", reg_addr + 4, reg_val);
		reg_val = (reg_val >> 12) & 0xFF;

		/*Generate MSI intterrupt*/
		msi_trigger_interrupt();
	}

	is_pass = ((ret == OK) && !(g_get_msi_int & SHIFT_LEFT(1UL, INVALID_APIC_ID_A))) ? true : false;
	report("%s", is_pass, __FUNCTION__);
}

static __unused void print_case_list(void)
{
	printf("\t\t PCI feature case list:\n\r");

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
}

int main(void)
{
	setup_idt();
	set_log_level(PCI_DEBUG_LEVEL);
	print_case_list();
#ifdef __x86_64__
	e1000e_init();
	pci_rqmid_28936_PCIe_config_space_and_host_2_byte_BAR_read_001();
	pci_rqmid_28887_PCIe_config_space_host_MSI_message_data_vector_read_write_001();
	pci_rqmid_28792_PCIe_config_space_and_host_write_Reserved_register_004();
	pci_rqmid_28829_PCIe_config_space_and_host_Status_Register_002();
	pci_rqmid_28893_PCIe_config_space_and_host_MSI_message_address_int_msg_addr_write_002();
	pci_rqmid_33824_PCIe_config_space_and_host_MSI_message_address_int_msg_addr_write_003();
	pci_rqmid_33822_PCIe_config_space_and_host_MSI_message_address_destination_ID_read_write_003();
	pci_rqmid_33823_PCIe_config_space_and_host_invalid_destination_ID_001();
	pci_rqmid_28861_PCIe_config_space_and_host_Unmapping_upon_BAR_writes_002();
#endif

	return report_summary();
}
