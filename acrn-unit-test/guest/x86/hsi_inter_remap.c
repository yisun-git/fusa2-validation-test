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
#include "pci_log.h"
#include "hsi_inter_remap.h"
#include "alloc_page.h"
#include "misc.h"

static struct pci_dev pci_devs[MAX_PCI_DEV_NUM];
static u32 nr_pci_devs = MAX_PCI_DEV_NUM;
static uint64_t g_get_msi_int = 0;
#define BSP_ID 0
#define RECORD_MSI_INT_VALUE 0x1UL
#define CACHE_LINE_SIZE		64UL

#define HOST_BRIDGE_IOMMU_ADDR  0xfed91000ULL
#define VTD_TEST_VECTOR_MSI (0xe8)
#define atomic_inc_fetch(ptr)  __sync_add_and_fetch(ptr, 1)
#define DMAR_IR_ENABLE_EIM_SHIFT	11UL
#define DMAR_IR_ENABLE_EIM		(1UL << DMAR_IR_ENABLE_EIM_SHIFT)
/* Hardware whether support interrupt remapping */
#define ECAP_IR_BIT (1UL << 3)
void *vtd_reg_base;

/* dmar unit runtime data */
struct dmar_drhd_rt {
	uint32_t index;
	uint64_t root_table_addr;
	uint64_t ir_table_addr;

	uint64_t cap;
	uint64_t ecap;
	uint32_t gcmd;  /* sw cache value of global cmd register */
};

extern void e1000e_init_flag_reset();
static int msi_int_handler(void *p_arg)
{
	DBG_INFO("[ int ]Run %s(%p)", __func__, p_arg);

	g_get_msi_int = RECORD_MSI_INT_VALUE;
	eoi();
	return 0;
}

static void msi_trigger_interrupt(void)
{
	// init e1000e net pci device, get ready for msi int
	msi_int_connect(msi_int_handler, (void *)1);
	msi_int_simulate();
	delay_loop_ms(500);
}

static int pci_probe_msi_cap(union pci_bdf bdf, uint32_t *msi_addr)
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
		/* Find MSI register group */
		if (PCI_CAP_ID_MSI == (reg_val & SHIFT_MASK(7, 0))) {
			break;
		}
		reg_addr = (reg_val & SHIFT_MASK(15, 8)) >> 8;
		reg_val = pci_pdev_read_cfg(bdf, reg_addr, 4);
	}
	if (repeat == i) {
		DBG_ERRO("no msi cap found!!!");
		return ERROR;
	}
	*msi_addr = reg_addr;
	return OK;
}

/*
 * @brief case name: HSI_Virtualization_Specific_Features_Interrupt_Remapping_001
 *
 * Summary: In 64-bit native mode, to enumerate the Capabilities Pointer Register for the NET devices in the root mode.
 * The first address of the Capabilities Register is 0x34. You recursively obtain the Capabilities information
 * for the PCIe device, based on the NEXT Pointer information in the Capabilities Register value.
 * If the Capabilities ID is 0x05, that is the Capabilities register set for the MSI.
 * Set the local APIC ID of value to the message address register bit[19:12] to 0(BP ID) and
 * set message control register bit0 to 1 enable MSI interrupt function.
 * Set interrupt vector 0x40 to the message data register .Trigger NET MAC MSI interrupt, 
 * CPU should be able to receive MSI interrupt, interrupt vector number 0x40.
 * Then clear message control register bit0 to 0 disable MSI interrupt function,
 * trigger NET MAC MSI interrupt,CPU should not be able to receive MSI interrupt.
 */
__unused static void hsi_rqmid_42668_virtualization_specific_features_inter_remapping_001(void)
{
	uint32_t reg_addr = 0U;
	uint32_t lapic_id = 0U;
	bool is_pass = false;
	int ret = OK;
	int cnt = 0;

	union pci_bdf bdf = {0};
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		/* resume enviroment */
		e1000e_init_flag_reset();
		e1000e_init(bdf);

		/* pci probe and check MSI capability register */
		ret = pci_probe_msi_cap(bdf, &reg_addr);

		if (ret == OK) {
		/* message address L register address = pointer + 0x4 */
			e1000e_msi_reset();
			lapic_id = BSP_ID;
			uint32_t msi_msg_addr_lo = MAKE_NET_MSI_MESSAGE_ADDR(0xFEE00000, lapic_id);
			uint32_t msi_msg_addr_hi = 0U;
			uint32_t msi_msg_data = MAKE_NET_MSI_MESSAGE_DATA(MSI_NET_IRQ_VECTOR);
			uint64_t msi_addr = msi_msg_addr_hi;
			msi_addr <<= 32;
			msi_addr &= 0xFFFFFFFF00000000UL;
			msi_addr |= msi_msg_addr_lo;
			e1000e_msi_config(msi_addr, msi_msg_data);

			g_get_msi_int = 0;
			/* Generate MSI intterrupt */
			e1000e_msi_enable();
			msi_trigger_interrupt();

			if (g_get_msi_int == RECORD_MSI_INT_VALUE) {
				cnt++;
			}

			g_get_msi_int = 0;
			e1000e_msi_config(msi_addr, msi_msg_data);
			e1000e_msi_disable();
			msi_trigger_interrupt();
			if (g_get_msi_int  == 0) {
				cnt++;
			}
		}
		is_pass = ((ret == OK) && (cnt == 2)) ? true : false;
	}
	DBG_INFO("g_get_msi_int:0x%lx cnt:%d", g_get_msi_int, cnt);
	report("%s", is_pass, __FUNCTION__);
}

struct dmar_drhd_rt st_drhd;

static uint64_t vtd_ir_table(void)
{
	return vtd_readq(DMAR_IRTA_REG) & PAGE_MASK;
}

static void dmar_gcmd_set_bit(uint32_t cmd)
{
	uint32_t status;

	/* We only allow set one bit for each time */
	assert(is_power_of_2(cmd));

	status = vtd_readl(DMAR_GSTS_REG);
	vtd_writel(DMAR_GCMD_REG, status | cmd);

	if (cmd & VTD_GCMD_ONE_SHOT_BITS) {
		/* One-shot bits are taking effect immediately */
		return;
	}

	/* Make sure IOMMU handled our command request */
	while (!(vtd_readl(DMAR_GSTS_REG) & cmd)) {
		cpu_relax();
	}
}

static void dma_gcmd_bit_clear(uint32_t cmd)
{
	uint32_t status;

	/* We only allow set one bit for each time */
	assert(is_power_of_2(cmd));

	status = vtd_readl(DMAR_GSTS_REG);
	vtd_writel(DMAR_GCMD_REG, status & (~cmd));

	delay_loop_ms(200);
	if (cmd & VTD_GCMD_ONE_SHOT_BITS) {
		/* One-shot bits are taking effect immediately */
		return;
	}

	/* Make sure IOMMU handled our command request */
	while (vtd_readl(DMAR_GSTS_REG) & cmd) {
		cpu_relax();
	}
}

static void vtd_dump_init_info(void)
{
	uint32_t version;

	version = vtd_readl(DMAR_VER_REG);

	/* Major version >= 1 */
	assert(((version >> 3) & 0xf) >= 1);

	DBG_INFO("VT-d version:   %#x", version);
	DBG_INFO("     cap:       %#018lx", vtd_readq(DMAR_CAP_REG));
	DBG_INFO("     ecap:      %#018lx", vtd_readq(DMAR_ECAP_REG));
}

static void vt_d_setup_ir_table(void)
{
	void *root = alloc_page();

	memset(root, 0, PAGE_SIZE);
	/* 0xf stands for table size (2^(0xf+1) == 65536) */
	vtd_writeq(DMAR_IRTA_REG, virt_to_phys(root) | 0xf | DMAR_IR_ENABLE_EIM);
	dmar_gcmd_set_bit(DMA_GCMD_SIRTP);
	st_drhd.ir_table_addr = vtd_ir_table();
	DBG_INFO("IR table address: %#018lx DMAR_IRTA_REG:%lx", st_drhd.ir_table_addr, vtd_readq(DMAR_IRTA_REG));
}

void vt_d_init(void)
{
	memset((void *)&st_drhd, 0, sizeof(st_drhd));
	vtd_reg_base = ioremap(HOST_BRIDGE_IOMMU_ADDR, PAGE_SIZE);

	vtd_dump_init_info();
	dmar_gcmd_set_bit(VTD_GCMD_QI); /* Enable QI */
	vt_d_setup_ir_table();
	dmar_gcmd_set_bit(DMA_GCMD_IRE);   /* Enable IR */

	st_drhd.ecap = vtd_readq(DMAR_ECAP_REG);
	DBG_INFO("22 gs_reg:%x ", vtd_readl(DMAR_GSTS_REG));
}

static void vt_d_disalbe()
{
	dma_gcmd_bit_clear(VTD_GCMD_QI); /* Disable QI */
	dma_gcmd_bit_clear(DMA_GCMD_IRE);   /* Disable IR */
	dma_gcmd_bit_clear(DMA_GCMD_SIRTP);   /* Disable IR */
	vtd_writeq(DMAR_IRTA_REG, 0);

	DBG_INFO("gsts:%x ", vtd_readl(DMAR_GSTS_REG));
}
struct vtd_msi_addr {
	union {
		u64 value;
		struct   {
			uint32_t __dont_care:2;
			uint32_t handle_15:1;	 /* handle[15] */
			uint32_t shv:1;
			uint32_t interrupt_format:1;
			uint32_t handle_0_14:15; /* handle[0:14] */
			uint32_t head:12;	 /* 0xfee */
			uint32_t addr_hi;	 /* not used except with x2apic */
		} __attribute__((__packed__));
	} __attribute__ ((packed));
};
typedef struct vtd_msi_addr vtd_msi_addr_t;

struct vtd_msi_data {
	union {
		u32 value;
		struct {
			uint16_t __reserved;
			uint16_t subhandle;
		} __attribute__ ((packed));
	};
} __attribute__ ((packed));
typedef struct vtd_msi_data vtd_msi_data_t;

struct vtd_irte {
	uint32_t present:1;
	uint32_t fault_disable:1;    /* Fault Processing Disable */
	uint32_t dest_mode:1;        /* Destination Mode */
	uint32_t redir_hint:1;       /* Redirection Hint */
	uint32_t trigger_mode:1;     /* Trigger Mode */
	uint32_t delivery_mode:3;    /* Delivery Mode */
	uint32_t __avail:4;          /* Available spaces for software */
	uint32_t __reserved_0:3;     /* Reserved 0 */
	uint32_t irte_mode:1;        /* IRTE Mode */
	uint32_t vector:8;           /* Interrupt Vector */
	uint32_t __reserved_1:8;     /* Reserved 1 */

	uint32_t dest_id;            /* Destination ID */

	uint64_t source_id:16;       /* Source-ID */
	uint64_t sid_q:2;            /* Source-ID Qualifier */
	uint64_t sid_vtype:2;        /* Source-ID Validation Type */
	uint64_t __reserved_2:44;    /* Reserved 2 */
} __attribute__ ((packed));
typedef struct vtd_irte vtd_irte_t;

static void vt_d_setup_irte(union pci_bdf *p_bdf, vtd_irte_t *irte,
			   int vector, int dest_id, trigger_mode_t trigger)
{
	assert(sizeof(vtd_irte_t) == 16);
	memset(irte, 0, sizeof(vtd_irte_t));
	DBG_INFO("irte size:%ld SID:%x", sizeof(vtd_irte_t), p_bdf->value);
	irte->fault_disable = 1;
	irte->dest_mode = 0;	 /* physical */
	irte->trigger_mode = trigger;
	irte->delivery_mode = 1; /* fixed */
	irte->irte_mode = 0;	 /* remapped */
	irte->vector = vector;
	irte->dest_id = dest_id;
	irte->source_id = p_bdf->value;
	irte->sid_q = 0;
	irte->sid_vtype = 1;     /* full-sid verify */
	irte->present = 1;
}

__unused static uint16_t vtd_intr_index_alloc(void)
{
	static volatile int index_ctr = 0;
	int ctr;

	assert(index_ctr < 65535);
	ctr = atomic_inc_fetch(&index_ctr);
	DBG_INFO("INTR: alloc IRTE index %d\n", ctr);
	return ctr;
}

static volatile bool net_intr_recved;

static void net_isr(isr_regs_t *regs)
{
	net_intr_recved = true;
	eoi();
	DBG_INFO("hanlder interrupt remapping!");
}

static inline void clflush(const volatile void *p)
{
	asm volatile ("clflush (%0)" :: "r"(p));
}

/*
 * Flush CPU cache when root table, context table or second-level translation teable updated
 */
__unused static void iommu_flush_cache(const void *p, uint32_t size)
{
	uint32_t i;

	/* if vtd support page-walk coherency, no need to flush cacheline */
	if (!(st_drhd.ecap & DMA_ECAP_C)) {
		DBG_INFO(" flush iommu cache!");
		for (i = 0U; i < size; i += CACHE_LINE_SIZE) {
			clflush((const char *)p + i);
		}
	}
}

/*
 * @brief case name: HSI_Virtualization_Specific_Features_Interrupt_Remapping_001
 *
 * Summary: In 64-bit native mode,
 * Check ecap register bit3 whether is 1 and make sure hardware supports interrupt remapping,
 * allocate ir_table_addr pointed to 4K free memory, set the value to the Interrupt
 * Remapping Table Address Register(offset is 0xb8), set bit24(SIRTP: Set Interrupt Remap Table Pointer),
 * bit25(Interrupt Remapping Enable) to 1 with global command register. set IRTE with vector 0xe8 and LAPIC
 * ID is 0 with Interrupt Remapping table's index is 1.
 * to enumerate the Capabilities Pointer Register for the NET devices in the root mode.
 * The first address of the Capabilities Register is 0x34. You recursively obtain the Capabilities information
 * for the PCIe device, based on the NEXT Pointer information in the Capabilities Register value.
 * If the Capabilities ID is 0x05, that is the Capabilities register set for the MSI.
 * Set the handler index to 1 and Interrupt format bit4 to 1(remapping format) with MSI address register.
 * set message control register bit0 to 1 enable MSI interrupt function.
 * Trigger NET MAC MSI interrupt, 
 * CPU should be able to receive MSI interrupt, interrupt vector number 0xe8 which is config in the IRTE.
 */
__unused static void hsi_rqmid_43872_virtualization_specific_features_inter_remapping_002(void)
{
	uint32_t reg_addr = 0U;
	bool is_pass = false;
	int ret = OK;
	int cnt = 0;
	union pci_bdf bdf = {0};
	vtd_msi_data_t msi_data = {0};
	vtd_msi_addr_t msi_addr = {0};

	vt_d_init();
	vtd_irte_t *irte = phys_to_virt(st_drhd.ir_table_addr);
	uint16_t index = vtd_intr_index_alloc();

	/* Mare supported interrupt remapping */
	if ((st_drhd.ecap & ECAP_IR_BIT) == 0) {
		goto test_report;
	}
	st_drhd.index = index;
	net_intr_recved = false;
	is_pass = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is_pass) {
		/* pci probe and check MSI capability register */
		ret = pci_probe_msi_cap(bdf, &reg_addr);

		if (ret == OK) {
		/* message address L register address = pointer + 0x4 */
			e1000e_msi_reset();
			/* Use edge irq as default */
			vt_d_setup_irte(&bdf, irte + index, VTD_TEST_VECTOR_MSI,
				BSP_ID, TRIGGER_EDGE);
			iommu_flush_cache(irte + index, sizeof(vtd_irte_t));
			/* set vector handler */
			cli();
			handle_irq(VTD_TEST_VECTOR_MSI, net_isr);
			sti();

			msi_addr.handle_15 = ((index >> 15) & 1);
			msi_addr.shv = 0;
			/* remapping format */
			msi_addr.interrupt_format = 1;
			msi_addr.handle_0_14 = index & 0x7fff;
			msi_addr.head = 0xfee;
			msi_data.subhandle = 0;
			e1000e_msi_config(msi_addr.value, msi_data.value);

			/* Generate MSI intterrupt */
			e1000e_msi_enable();
			msi_trigger_interrupt();

			if (net_intr_recved == true) {
				cnt++;
			}

		}
	}

	vt_d_disalbe();

test_report:
	report("%s", (cnt == 1), __FUNCTION__);
}

static void print_case_list(void)
{
	/*_x86_64__*/
	printf("\t HSI interrupt remapping test case list: \n\r");
#ifdef IN_NATIVE
	printf("\t Case ID: %d Case name: %s\n\r", 42668u, "HSI_Virtualization_Specific_Features" \
		"_Interrupt_Remapping_001");
	printf("\t Case ID: %d Case name: %s\n\r", 43872u, "HSI_Virtualization_Specific_Features" \
		"_Interrupt_Remapping_002");
#endif
	printf("\t \n\r \n\r");
}

int main(void)
{
	setup_vm();
	setup_idt();
	set_log_level(DBG_LVL_INFO);
	/* Enumerate PCI devices to pci_devs */
	pci_pdev_enumerate_dev(pci_devs, &nr_pci_devs);
	if (nr_pci_devs == 0) {
		DBG_ERRO("pci_pdev_enumerate_dev finds no devs!");
		return 0;
	}
	DBG_INFO("Find PCI devs nr_pci_devs = %d", nr_pci_devs);

	union pci_bdf bdf = {0};
	bool is = get_pci_bdf_by_dev_vendor(pci_devs, nr_pci_devs, NET_DEV_VENDOR, &bdf);
	if (is) {
		e1000e_init(bdf);
	}
#ifdef IN_NATIVE
	print_case_list();
	hsi_rqmid_43872_virtualization_specific_features_inter_remapping_002();
	hsi_rqmid_42668_virtualization_specific_features_inter_remapping_001();
#endif
	report_summary();
	return 0;
}

