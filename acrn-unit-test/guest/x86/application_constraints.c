#include "libcflat.h"
#include "desc.h"
#include "processor.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"
#include "misc.h"
#include "pci_util.h"
#include "asm/spinlock.h"
#include "apic-defs.h"
#include "isr.h"
#include "apic.h"
#include "delay.h"

#include "acpi.h"

#include "register_op.h"
#include "regdump.h"
#include "atomic.h"

/* define the offset length in byte */
#define BYTE_1			(1U)
#define BYTE_2			(2U)
#define BYTE_4			(4U)

#define PCI_COMMAND_OFFSET					(0x4U)
#define PCI_COMMAND_SIZE					BYTE_2
#define PCI_STATUS_OFFSET					(0x6U)
#define PCI_STATUS_SIZE						BYTE_2
#define PCI_CACHE_LINE_OFFSET				(0xCU)
#define PCI_CACHE_LINE_SIZES				BYTE_1
#define PCI_MASTER_LATENCY_TIMER_OFFSET		(0x0D)
#define PCI_MASTER_LATENCY_TIMER_SIZE		BYTE_1
#define PCI_BIST_OFFSET						(0X0FU)
#define PCI_BIST_SIZE						BYTE_1
#define PCI_EROM_BA_OFFSET					(0x30)
#define PCI_EROM_BA_SIZE					BYTE_4
#define PCI_INTERRUPT_OFFSET				(0x3C)
#define PCI_INTERRUPT_SIZE					BYTE_1
#define PCI_MIN_GNT_OFFSET					(0x3E)
#define PCI_MIN_GNT_SIZE					BYTE_1
#define PCI_MAX_LAT_OFFSET					(0x3F)
#define PCI_MAX_LAT_SIZE					BYTE_1
#define PCI_XHCC1_OFFSET					(0x40)
#define PCI_XHCC1_SIZE						BYTE_4
#define PCI_XHCC2_OFFSET					(0x44)
#define PCI_XHCC2_SIZE						BYTE_4
#define PCI_XHCLKGTEN_OFFSET				(0x50)
#define PCI_XHCLKGTEN_SIZE					BYTE_4
#define PCI_AUDSYNC_OFFSET					(0x58)
#define PCI_AUDSYNC_SIZE					BYTE_4
#define PCI_SBRN_OFFSET						(0x60)
#define PCI_SBRN_SIZE						BYTE_1
#define PCI_FLADJ_OFFSET					(0x61)
#define PCI_FLADJ_SIZE						BYTE_1

#define PCI_BESL_OFFSET						(0x62)
#define PCI_BESL_SIZE						BYTE_1

#define PCI_VSHDR_OFFSET					(0x94)
#define PCI_VSHDR_SIZE						BYTE_4
#define PCI_PCE_REG_OFFSET					(0xA2)
#define PCI_PCE_REG_SIZE					BYTE_2
#define PCI_HSCFG2_OFFSET					(0xA4)
#define PCI_HSCFG2_SIZE						BYTE_2

#define PCI_XHCI_USB_OPM_SIZE				BYTE_4
#define PCI_XHCI_USB2_OPM0_OFFSET			(0xB0)
#define PCI_XHCI_USB2_OPM1_OFFSET			(0xB4)
#define PCI_XHCI_USB2_OPM2_OFFSET			(0xB8)
#define PCI_XHCI_USB2_OPM3_OFFSET			(0xBC)
#define PCI_XHCI_USB2_OPM4_OFFSET			(0xC0)
#define PCI_XHCI_USB2_OPM5_OFFSET			(0xC4)
#define PCI_XHCI_USB2_OPM6_OFFSET			(0xC8)
#define PCI_XHCI_USB2_OPM7_OFFSET			(0xCC)

#define PCI_XHCI_USB3_OPM0_OFFSET			(0xD0)
#define PCI_XHCI_USB3_OPM1_OFFSET			(0xD4)
#define PCI_XHCI_USB3_OPM2_OFFSET			(0xD8)
#define PCI_XHCI_USB3_OPM3_OFFSET			(0xDC)
#define PCI_XHCI_USB3_OPM4_OFFSET			(0xE0)
#define PCI_XHCI_USB3_OPM5_OFFSET			(0xE4)
#define PCI_XHCI_USB3_OPM6_OFFSET			(0xE8)
#define PCI_XHCI_USB3_OPM7_OFFSET			(0xEC)

#define PCI_LANDISCTRL_OFFSET				(0XA0)
#define PCI_LANDISCTRL_SIZE					BYTE_4
#define PCI_LOCKLANDIS_OFFSET				(0XA4)
#define PCI_LOCKLANDIS_SIZE					BYTE_4
#define PCI_LTRCAP_OFFSET					(0XA8)
#define PCI_LTRCAP_SIZE						BYTE_4
#define B_INDEX				0xB
#define RTC_INDEX_REG		0x70
#define RTC_TARGET_REG		0x71
#define B_DEFAULT_VALUE		0x2
#define X86_CR0_NE			(1UL << 5U)

#define DMAR_CAPAB_REG			0x8  /* Hardware supported capabilities */
#define DMAR_ECAPAB_REG			0x10 /* Extended capabilities supported */

#define IOMMU_BASE_ADDR				0xfed91000ULL
#define LAPIC_BASE_ADDR				0XFEE00000ULL

#define IOAPIC_VERSION_OFFSET			0x1

#define IOAPIC_VER_VALUE	0x20
#define IOAPIC_REGSEL 0x00U /**< IOAPIC I/O register select register */
#define IOAPIC_WINDOW 0x10U /**< IOAPIC I/O window register */
#define IO_APIC_BASE	0xFEC00000U
#define IOREGSEL	(IO_APIC_BASE + IOAPIC_REGSEL)
#define IOWIN		(IO_APIC_BASE + IOAPIC_WINDOW)
#define IOAPIC_VER_OFFSET  0x01U /**< Version of IOAPIC */

#define IOMMU_VER_OFFSET	0x00U /**< Version of IOMMU */
#define IOMMU_VER_VALUE		0x10

#define LAPIC_VER_OFFSET	0x30U /**< Version of IOMMU */
#define LAPIC_VER_VALUE		0x10


void *reg_base;

#define vtd_reg(reg) ({ assert(reg_base); \
						(volatile void *)(reg_base + reg); })

static inline uint64_t reg_readq(unsigned int reg)
{
	return __raw_readq(vtd_reg(reg));
}

static inline void mmio_write32(uint32_t value, void *addr)
{
	volatile uint32_t *addr32 = (volatile uint32_t *)addr;
	/** Write \a value to the specified memory addressed by the virtual address \a addr */
	*addr32 = value;
}

static inline uint32_t mmio_read32(const void *addr)
{
	/** Return a 32-bits value read from the virtual address \a addr */
	return *((volatile const uint32_t *)addr);
}

static bool check_port_available(unsigned long port)
{
	unsigned char value;

	asm volatile(ASM_TRY("1f")
				"inb %w1, %0\n\t"
				"1:"
				: "=a" (value) : "Nd" ((unsigned short)port));
	if (exception_vector() != NO_EXCEPTION) {
		return false;
	}

	return true;
}

static bool check_CPUID_leaf_support(u32 cpuid_leaf)
{
	struct cpuid r;
	u32 index = 0;

	asm volatile (ASM_TRY("1f"):::"memory");
	asm volatile ("cpuid"
					: "=a"(r.a), "=b"(r.b), "=c"(r.c), "=d"(r.d)
					: "0"(cpuid_leaf), "2"(index));
	asm volatile ("1:");
	if (exception_vector() != NO_EXCEPTION) {
		return false;
	}

	return true;
}

#define CHECK_REPORT() do {\
			if (exception_vector() != NO_EXCEPTION) {\
				report("\t\t %s %d", false, __FUNCTION__, __LINE__);\
				return;\
		} \
} while (0)

static uint8_t read_reg_al()
{
	uint8_t al;

	asm volatile(
		"mov %%al, %0\n\t"
		: "=r"(al));

	return al;
}

#define MAX_PCPU_NUM		4U
#define ACPI_MADT_TYPE_LOCAL_APIC   0U
#define ACPI_MADT_TYPE_IOAPIC       1U
#define ACPI_MADT_ENABLED           1U
#define ACPI_MADT_TYPE_LOCAL_APIC_NMI 4U
#define CONFIG_MAX_IOAPIC_NUM	2U
#define ACPI_NAME_SIZE 4U

struct acpi_table_header {
	/* ASCII table signature */
	char signature[4];
	/* Length of table in bytes, including this header */
	uint32_t length;
	/* ACPI Specification minor version number */
	uint8_t revision;
	/* To make sum of entire table == 0 */
	uint8_t checksum;
	/* ASCII OEM identification */
	char oem_id[6];
	/* ASCII OEM table identification */
	char oem_table_id[8];
	/* OEM revision number */
	uint32_t oem_revision;
#if 1
	/* ASCII ASL compiler vendor ID */
	char asl_compiler_id[4];
	/* ASL compiler version */
	uint32_t asl_compiler_revision;
#else
	/* Vendor ID of utility that created the table */
	char creator_id[4];
	/* Revision of utility that created the table */
	uint32_t creator_revision;
#endif
} __attribute__((packed));

struct acpi_subtable_header {
	uint8_t                   type;
	uint8_t                   length;
} __attribute__((packed));

struct acpi_table_xsdt {
	/* Common ACPI table header */
	struct acpi_table_header header;
	/* Array of pointers to ACPI tables */
	uint64_t table_offset_entry[1];
} __attribute__((packed));

struct acpi_table_rsdt {
	/* Common ACPI table header */
	struct acpi_table_header   header;
	/* Array of pointers to ACPI tables */
	uint32_t table_offset_entry[1];
} __packed;

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


struct acpi_table_madt {
	/* Common ACPI table header */
	struct acpi_table_header header;
	/* Physical address of local APIC */
	uint32_t address;
	uint32_t flags;
} __attribute__((packed));

struct acpi_madt_local_apic {
	struct acpi_subtable_header    header;
	/* ACPI processor id */
	uint8_t                        processor_id;
	/* Processor's local APIC id */
	uint8_t                        id;
	uint32_t                       lapic_flags;
} __attribute__((packed));

struct acpi_madt_ioapic {
	struct acpi_subtable_header    header;
	/* IOAPIC id */
	uint8_t   id;
	uint8_t   rsvd;
	uint32_t  addr;
	uint32_t  gsi_base;
} __attribute__((packed));

static bool
probe_table(uint64_t address, const char *signature)
{
	bool ret;
	struct acpi_table_header *table = (struct acpi_table_header *)address;

	if (strncmp(table->signature, signature, ACPI_NAME_SIZE) != 0) {
		ret = false;
	} else {
		ret = true;
	}

	return ret;
}


static void*
get_acpi_table_addr(const char *sig)
{
	unsigned long addr;
	struct rsdp_descriptor *rsdp;
	int i;

	for (addr = 0xf0000; addr < 0x100000; addr += 16) {
		rsdp = (void *)addr;
		if (rsdp->signature == 0x2052545020445352LL) {
			break;
		}
	}

	if (addr == 0x100000) {
		printf("Can't find RSDP\n");
		return 0;
	}

	if (strncmp(sig, "RSDP", ACPI_NAME_SIZE) == 0) {
		return rsdp;
	}

	if (rsdp->revision < 2) {
		struct acpi_table_rsdt *rsdt = (struct acpi_table_rsdt *)(uint64_t)rsdp->rsdt_physical_address;
		uint32_t count = (rsdt->header.length - sizeof(struct acpi_table_header)) / sizeof(uint32_t);

		for (i = 0U; i < count; i++) {
			if (probe_table(rsdt->table_offset_entry[i], sig)) {
				return (void *)(uint64_t)rsdt->table_offset_entry[i];
			}
		}
	} else {
		struct acpi_table_xsdt *xsdt = (struct acpi_table_xsdt *)(uint64_t)rsdp->xsdt_physical_address;
		uint32_t count = (xsdt->header.length - sizeof(struct acpi_table_header)) / sizeof(uint64_t);

		for (i = 0U; i < count; i++) {
			if (probe_table(xsdt->table_offset_entry[i], sig)) {
				return (void *)(uint64_t)xsdt->table_offset_entry[i];
			}
		}
	}

	return NULL;
}

static uint16_t
local_parse_madt(struct acpi_table_madt *madt, struct acpi_madt_local_apic lapic_array[MAX_PCPU_NUM])
{
	uint16_t pcpu_num = 0U;
	struct acpi_madt_local_apic *processor;
	struct acpi_table_madt *madt_ptr;
	void *first, *end, *iterator;
	struct acpi_subtable_header *entry;

	madt_ptr = madt;

	first = madt_ptr + 1;
	end = (void *)madt_ptr + madt_ptr->header.length;

	for (iterator = first; (iterator) < (end); iterator += entry->length) {
		entry = (struct acpi_subtable_header *)iterator;
		if (entry->length < sizeof(struct acpi_subtable_header)) {
			break;
		}

		if (entry->type == ACPI_MADT_TYPE_LOCAL_APIC) {
			processor = (struct acpi_madt_local_apic *)iterator;
			if ((processor->lapic_flags & ACPI_MADT_ENABLED) != 0U) {
				if (pcpu_num < MAX_PCPU_NUM) {
					lapic_array[pcpu_num].processor_id = processor->processor_id;
					lapic_array[pcpu_num].id = processor->id;
					lapic_array[pcpu_num].lapic_flags = processor->lapic_flags;
				}
				pcpu_num++;
			}
		}
	}

	return pcpu_num;
}

static uint8_t
parse_madt_ioapic(struct acpi_table_madt *madt, struct acpi_madt_ioapic ioapic_array[CONFIG_MAX_IOAPIC_NUM])
{
	uint8_t ioapic_idx = 0U;
	uint64_t entry, end;
	const struct acpi_madt_ioapic *ioapic;

	if (madt != NULL) {
		end = (uint64_t)madt + madt->header.length;

		for (entry = (uint64_t)(madt + 1); entry < end; entry += ioapic->header.length) {
			ioapic = (const struct acpi_madt_ioapic *)entry;

			if (ioapic->header.type == ACPI_MADT_TYPE_IOAPIC) {
				if (ioapic_idx < CONFIG_MAX_IOAPIC_NUM) {
					ioapic_array[ioapic_idx].id = ioapic->id;
					ioapic_array[ioapic_idx].addr = ioapic->addr;
					ioapic_array[ioapic_idx].gsi_base = ioapic->gsi_base;
				}
				ioapic_idx++;
			}
		}
	}

	return ioapic_idx;
}

struct multiboot_info {
	uint32_t	mi_flags;

	/* Valid if mi_flags sets MULTIBOOT_INFO_HAS_MEMORY. */
	uint32_t	mi_mem_lower;
	uint32_t	mi_mem_upper;

	/* Valid if mi_flags sets MULTIBOOT_INFO_HAS_BOOT_DEVICE. */
	uint8_t		mi_boot_device_part3;
	uint8_t		mi_boot_device_part2;
	uint8_t		mi_boot_device_part1;
	uint8_t		mi_boot_device_drive;

	/* Valid if mi_flags sets MULTIBOOT_INFO_HAS_CMDLINE. */
	uint32_t	mi_cmdline;

	/* Valid if mi_flags sets MULTIBOOT_INFO_HAS_MODS. */
	uint32_t	mi_mods_count;
	uint32_t	mi_mods_addr;

	/* Valid if mi_flags sets MULTIBOOT_INFO_HAS_{AOUT,ELF}_SYMS. */
	uint32_t	mi_elfshdr_num;
	uint32_t	mi_elfshdr_size;
	uint32_t	mi_elfshdr_addr;
	uint32_t	mi_elfshdr_shndx;

	/* Valid if mi_flags sets MULTIBOOT_INFO_HAS_MMAP. */
	uint32_t	mi_mmap_length;
	uint32_t	mi_mmap_addr;

	/* Valid if mi_flags sets MULTIBOOT_INFO_HAS_DRIVES. */
	uint32_t	mi_drives_length;
	uint32_t	mi_drives_addr;

	/* Valid if mi_flags sets MULTIBOOT_INFO_HAS_CONFIG_TABLE. */
	uint32_t	unused_mi_config_table;

	/* Valid if mi_flags sets MULTIBOOT_INFO_HAS_LOADER_NAME. */
	uint32_t	mi_loader_name;

	/* Valid if mi_flags sets MULTIBOOT_INFO_HAS_APM. */
	uint32_t	unused_mi_apm_table;

	/* Valid if mi_flags sets MULTIBOOT_INFO_HAS_VBE. */
	uint32_t	unused_mi_vbe_control_info;
	uint32_t	unused_mi_vbe_mode_info;
	uint16_t	unused_mi_vbe_mode;
	uint16_t	unused_mi_vbe_interface_seg;
	uint16_t	unused_mi_vbe_interface_off;
	uint16_t	unused_mi_vbe_interface_len;
} __attribute__((packed));

struct multiboot_mmap {
	uint32_t	size;
	uint64_t	baseaddr;
	uint64_t	length;
	uint32_t	type;
} __attribute__((packed));

struct multiboot_module {
	uint32_t	mm_mod_start;
	uint32_t	mm_mod_end;
	uint32_t	mm_string;
	uint32_t	mm_reserved;
} __attribute__((packed));

#define MAX_MMAP_ENTRIES		32U
#define MAX_MODULE_NUM			8U

extern struct mbi_bootinfo *g_bootinfo;
struct multiboot_mmap mi_mmap_entry[MAX_MMAP_ENTRIES];
uint32_t mi_mmap_entries = 0;
struct multiboot_module mi_mods[MAX_MODULE_NUM];
uint32_t mi_mods_count = 0;

static inline void memcpy_erms(void *d, const void *s, size_t slen)
{
	asm volatile ("rep; movsb"
		: "=&D"(d), "=&S"(s)
		: "c"(slen), "0" (d), "1" (s)
		: "memory");
}

static int32_t memcpy_s(void *d, size_t dmax, const void *s, size_t slen)
{
	int32_t ret = -1;

	if ((d != NULL) && (s != NULL) && (dmax >= slen) && ((d > (s + slen)) || (s > (d + dmax)))) {
		if (slen != 0U) {
			memcpy_erms(d, s, slen);
		}
		ret = 0;
	} else {
		(void)memset(d, 0U, dmax);
	}

	return ret;
}


static void get_mmap_and_mods_info(void)
{
	struct multiboot_info *p = (struct multiboot_info *)g_bootinfo;

	mi_mmap_entries = p->mi_mmap_length / sizeof(struct multiboot_mmap);
	void *mi_mmap_addr = (void *)(uint64_t)p->mi_mmap_addr;

	mi_mods_count = p->mi_mods_count;
	void *mi_mods_addr = (void *)(uint64_t)p->mi_mods_addr;

	if ((mi_mmap_entries != 0U) && (mi_mmap_addr != NULL)) {
		if (mi_mmap_entries > MAX_MMAP_ENTRIES) {
			printf("Too many E820 entries %d\n", mi_mmap_entries);
			mi_mmap_entries = MAX_MMAP_ENTRIES;
		}

		uint32_t mmap_entry_size = sizeof(struct multiboot_mmap);

		(void)memcpy_s((void *)(&mi_mmap_entry[0]),
			(mi_mmap_entries * mmap_entry_size),
			(const void *)mi_mmap_addr,
			(mi_mmap_entries * mmap_entry_size));

	}

	if (mi_mods_count > MAX_MODULE_NUM) {
		printf("Too many multiboot modules %d\n", mi_mods_count);
		mi_mods_count = MAX_MODULE_NUM;
	}

	if (mi_mods_count != 0U) {
		(void)memcpy_s((void *)(&mi_mods[0]),
			(mi_mods_count * sizeof(struct multiboot_module)),
			(const void *)mi_mods_addr,
			(mi_mods_count * sizeof(struct multiboot_module)));
	}
}

struct acpi_madt_local_apic lapic_array[MAX_PCPU_NUM];
struct acpi_madt_ioapic ioapic_array[CONFIG_MAX_IOAPIC_NUM];

uint32_t ap1_apicid = 0;
uint32_t ap2_apicid = 0;
uint32_t ap3_apicid = 0;
uint32_t ap1_x2apicid = 0;
uint32_t ap2_x2apicid = 0;
uint32_t ap3_x2apicid = 0;

uint64_t bp_l1d_flush_bit = 1;
uint64_t ap1_l1d_flush_bit = 0;
uint64_t ap2_l1d_flush_bit = 0;
uint64_t ap3_l1d_flush_bit = 0;

static uint64_t get_l1d_flush_bit(void)
{
	uint64_t check_bit = 0;
	check_bit = cpuid_indexed(0x07, 0x0).d;
	check_bit &= 1 << 28;

	return check_bit;
}

#define IA32_FLUSH_CMD_MSR	0x0000010B
uint32_t l1tf_flag = 0;
static struct spinlock lock;

static uint32_t x2apic_read(uint32_t reg)
{
	uint32_t a, d;

	asm volatile ("rdmsr" : "=a"(a), "=d"(d) : "c"(APIC_BASE_MSR + reg/16));

	return (a | ((uint64_t)d << 32));
}

static void l1tf_test()
{
	u32 index = IA32_FLUSH_CMD_MSR;
	u32 a = 0;
	u32 d = 0;

	asm volatile("rdmsr\n\t" : "=a"(a), "=d"(d) : "c"(index) : "memory");
}

void ap_main(void)
{
	spin_lock(&lock);
	uint32_t lapic_id = get_lapic_id();
	uint32_t x2apic_id = 0;
	uint64_t apicbase = rdmsr(MSR_IA32_APICBASE);
	if ((apicbase & (APIC_EN | APIC_EXTD)) == (APIC_EN | APIC_EXTD)) {
		x2apic_id = x2apic_read(APIC_ID);
	}

	if (lapic_id == 2) {
		ap1_apicid = lapic_id;
		ap1_x2apicid = x2apic_id;
		ap1_l1d_flush_bit = get_l1d_flush_bit();
	}

	if (lapic_id == 4) {
		ap2_apicid = lapic_id;
		ap2_x2apicid = x2apic_id;
		ap2_l1d_flush_bit = get_l1d_flush_bit();
	}

	if (lapic_id == 6) {
		ap3_apicid = lapic_id;
		ap3_x2apicid = x2apic_id;
		ap3_l1d_flush_bit = get_l1d_flush_bit();
	}
	spin_unlock(&lock);

	setup_idt();
	bool ret = test_for_exception(GP_VECTOR, l1tf_test, NULL);
	if (ret != true) {
		l1tf_flag++;
	}
}

#define MAX_PCI_DEV_NUM	(256)

#include "pci_util.h"
#include "pci_check.h"
#include "pci_regs.h"
#include "pci_native_config.h"


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
	}
	if (repeat == i) {
		DBG_ERRO("no msi cap found!!!");
		return ERROR;
	}
	*msi_addr = reg_addr;
	return OK;
}

#define PCI_ADDR_MEM_MASK	(~0x0fUL)

struct pci_vbar {
	uint64_t size;
	uint64_t base_hpa;
	uint64_t base;
	uint32_t bar_type;
	uint32_t mask;
	uint32_t fixed;
};

#define PCIE_CONFIG_SPACE_SIZE 0x1000U
union pci_cfgdata {
	uint8_t data_8[PCIE_CONFIG_SPACE_SIZE];
	uint16_t data_16[PCIE_CONFIG_SPACE_SIZE >> 1U];
	uint32_t data_32[PCIE_CONFIG_SPACE_SIZE >> 2U];
};

struct pci_barinfo {
	union pci_cfgdata cfgdata;
	uint32_t nr_bars;
	struct pci_vbar vbars[PCI_BAR_COUNT];
};

uint32_t msi_1f6 = 0;
uint32_t msi_146 = 0;
struct pci_barinfo barinfo[2];

static void pci_get_pci_configuration_space(void)
{
	uint32_t bar_id = PCI_BASE_ADDRESS_0;
	int ret = OK;
	union pci_bdf bdf = {0};
	bdf.bits.d = 0x14;
	ret = pci_probe_msi_capability(bdf, &msi_146);

	if (ret == OK) {
		uint32_t bar = pci_pdev_read_cfg(bdf, bar_id, 4);
		pci_pdev_write_cfg(bdf, bar_id, 4, ~0U);
		uint32_t mask = pci_pdev_read_cfg(bdf, bar_id, 4);
		mask &= PCI_ADDR_MEM_MASK;
		uint32_t size = (~mask) + 1;
		uint32_t bar1 = pci_pdev_read_cfg(bdf, bar_id + 4, 4);
		pci_pdev_write_cfg(bdf, bar_id + 4, 4, ~0U);
		uint32_t mask1 = pci_pdev_read_cfg(bdf, bar_id + 4, 4);
		uint32_t size1 = ~mask1;

		barinfo[0].vbars[0].base = bar & PCI_ADDR_MEM_MASK;
		barinfo[0].vbars[0].bar_type = ((bar & 0x7) == 0x0) ? 2 : 3;
		barinfo[0].vbars[0].size = size;

		barinfo[0].vbars[1].base = bar1 & PCI_ADDR_MEM_MASK;
		barinfo[0].vbars[1].bar_type = ((bar & 0x7) == 0x0) ? 2 : 4;
		barinfo[0].vbars[1].size = size1;
	}

	bdf.bits.d = 0x1f;
	bdf.bits.f = 0x6;
	ret = pci_probe_msi_capability(bdf, &msi_1f6);
	if (ret == OK) {

		uint32_t bar = pci_pdev_read_cfg(bdf, bar_id, 4);
		pci_pdev_write_cfg(bdf, bar_id, 4, ~0U);
		uint32_t mask = pci_pdev_read_cfg(bdf, bar_id, 4);
		uint32_t size = (~mask) + 1;

		barinfo[1].vbars[0].base = bar & PCI_ADDR_MEM_MASK;
		barinfo[1].vbars[0].bar_type = ((bar & 0x7) == 0x0) ? 2 : 3;
		barinfo[1].vbars[0].size = size;
	}
}

#define POSTED_INTR_VECTOR	(0xE3)
static u16 record_vector;
static void ipi_posted_inter_handle(__unused isr_regs_t *regs)
{
	record_vector = POSTED_INTR_VECTOR;
	eoi();
}

uint16_t lapic_num = 0;
uint16_t ioapic_num = 0;
uint32_t bp_apicid = 0xff;
extern uint8_t stext;
#define MULTIBOOT_STARTUP_EIP_ADDR	(0x6004)

#ifdef IN_NATIVE
/*
 * @brief case name:Check all entries of Processor Local APIC Structure in ACPI table_01
 *
 * Summary:The local APIC ID of the first entry of Processor Local APIC Structure in ACPI table
 * shall be equal to the value of physical LAPIC ID register of physical BP.
 * The local APIC ID of the other three entries of Processor Local APIC Structure in
 * ACPI table shall be equal to the value of the physical LAPIC ID register of three
 * physical APs. The physical platform shall guarantee that the physical processor number is 4H.
 *
 */
static void applicaton_constraints_rqmid_46556_check_entry_of_lapic_in_acpi_table_01(void)
{
	uint32_t ret;

	bp_apicid = get_lapic_id();
	if ((lapic_num == 4)
		&& (bp_apicid == lapic_array[0].id)
		&& (ap1_apicid == lapic_array[1].id)
		&& (ap2_apicid == lapic_array[2].id)
		&& (ap3_apicid == lapic_array[3].id)) {
		ret = true;
	} else {
		ret = false;
	}

	report("\t\t %s", ret, __FUNCTION__);
}

/*
 * @brief case name:Check all entries of IOAPIC Structure in ACPI table
 *
 * Summary:The physical platform shall guarantee that the physical I/O APIC number is 1H.
 * The physical platform shall guarantee that the physical I/O APIC base address is
 * FEC00000H.
 *
 */
static void applicaton_constraints_rqmid_46557_check_entry_of_ioapic_in_acpi_table_01(void)
{
	uint32_t ret;
	if ((ioapic_num == 1)
		&& (ioapic_array[0].addr == 0xfec00000)) {
		ret = true;
	} else {
		ret = false;
	}

	report("\t\t %s", ret, __FUNCTION__);
}

/*
 * @brief case name:Check the physical LAPIC mode_01
 *
 * Summary:The physical LAPIC shall be disabled mode, or xAPIC mode, or x2APIC mode.
 *
 */
static void applicaton_constraints_rqmid_46558_check_physical_lapic_mode_01(void)
{
	uint32_t ret;

	uint64_t apicbase = rdmsr(MSR_IA32_APICBASE);

	if (!(apicbase & APIC_EN)
		|| ((apicbase & (APIC_EN | APIC_EXTD)) == APIC_EN)
		|| ((apicbase & (APIC_EN | APIC_EXTD)) == (APIC_EN | APIC_EXTD))) {
		ret = true;
	} else {
		ret = false;
	}

	report("\t\t %s", ret, __FUNCTION__);
}

/*
 * @brief case name:Check the physical LAPIC ID_01
 *
 * Summary:The physical platform shall guarantee that the value of the LAPIC ID register of
 * physical BP is 0H.
 * The physical platform shall guarantee that the values of the LAPIC ID register of
 * three physical APs are 2H, 4H, 6H.
 *
 */
static void applicaton_constraints_rqmid_46559_check_physical_lapic_id_01(void)
{
	uint32_t ret;

	bp_apicid = get_lapic_id();
	if ((lapic_num == 4)
		&& (bp_apicid == 0)
		&& (ap1_apicid == 2)
		&& (ap2_apicid == 4)
		&& (ap3_apicid == 6)) {
		ret = true;
	} else {
		ret = false;
	}

	report("\t\t %s", ret, __FUNCTION__);
}

/*
 * @brief case name:Check all entries of the memory map buffer in multiboot_01
 *
 * Summary:All the size fields in the memory map buffer have a value of 20. The memory map
 * buffer is located at the host physical address in the mmap_addr field in the
 * physical Multiboot information structure.
 *
 */
static void applicaton_constraints_rqmid_46560_check_mmap_in_multiboot_01(void)
{
	uint32_t ret = true;

	for (int i = 0; i < mi_mmap_entries; i++) {
		if (mi_mmap_entry[i].size != 20) {
			ret = false;
			break;
		}
	}

	report("\t\t %s", ret, __FUNCTION__);
}

/*
 * @brief case name:Check the first instruction of program is executed on the physical BP_01
 *
 * Summary:The first entry of Processor Local APIC Structure in ACPI table shall be for the
 * physical BP.
 * The physical platform shall guarantee that first instruction of ACRN hypervisor is
 * executed on the physical BP.
 * The physical platform shall guarantee each physical AP waits for the first INIT IPI.
 *
 */
static void applicaton_constraints_rqmid_46561_check_first_instruction_in_bp_01(void)
{
	uint32_t ret;

	/* init vector */
	record_vector = 0;

	/* set handle */
	cli();
	/* set post-interrupt handler */
	handle_irq(POSTED_INTR_VECTOR, ipi_posted_inter_handle);
	sti();

	/* send a IPI to cpu with lapic id from the first entry of Processor Local APIC */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_FIXED | POSTED_INTR_VECTOR,
		lapic_array[0].id);

	/* wait ipi interrupt occurs and set record_vector, it will prove the lapic id is for BP */
	delay(10000);

	/* bit 8 set 1, it mean the processor is BSP */
	uint64_t apic_base = rdmsr(MSR_IA32_APICBASE);
	apic_base &= 0x100;

	/*
	 * &stext is image start addr
	 * 0x8 because the image is 16 byte aligned, we need to add 8 bytes here
	 * 0xc is the length of mulitboot header
	 * 0xa is the instruction length of code we insert to get eip
	 */
	uint32_t *eip = (uint32_t *)MULTIBOOT_STARTUP_EIP_ADDR;
	if ((*eip == (uint32_t)(uint64_t)&stext + 0x8 + 0xc + 0xa)
			&& (record_vector == POSTED_INTR_VECTOR) && apic_base != 0) {
		ret = true;
	} else {
		ret = false;
	}

	/* AP waits for the first INIT IPI can be proved by ap_main was executed correctly */
	report("\t\t %s", ret, __FUNCTION__);
}

/*
 * @brief case name:Check all entries of the module in multiboot_01
 *
 * Summary:The physical platform shall guarantee that each module load physical address is
 * higher than 100000H.
 * The physical platform shall guarantee that there is no overlap among hypervisor
 * image and boot module images.
 *
 */
static void applicaton_constraints_rqmid_46562_check_mods_in_multiboot_01(void)
{
	uint32_t ret;

	/*Because of the image is 16 byte alignment, we need to add 8 bytes here*/
	uint32_t start = (uint32_t)(uint64_t)&stext + 0x8;

	/* 0xc is the length of multiboot header */
	if ((mi_mods_count == 2)
		&& (mi_mods[0].mm_mod_start >= 0x100000)
		&& (mi_mods[1].mm_mod_start > mi_mods[0].mm_mod_end)
		&& (start > mi_mods[1].mm_mod_end)) {
		ret = true;
	} else {
		ret = false;
	}

	report("\t\t %s", ret, __FUNCTION__);
}

/*
 * @brief case name:The effect of L1TF on each physical processor shall be the same_01
 *
 * Summary:The effect of L1TF on each physical processor shall be the same.
 *
 */
static void applicaton_constraints_rqmid_46563_check_l1tf_effect_01(void)
{
	uint32_t ret;

	u32 index = IA32_FLUSH_CMD_MSR;
	u32 a = 0;
	u32 d = 0;
	asm volatile(ASM_TRY("1f")
		"rdmsr\n\t"
		"1:"
		: "=a"(a), "=d"(d) : "c"(index) : "memory");

	if (exception_vector() != GP_VECTOR) {
		l1tf_flag++;
	}

	bp_l1d_flush_bit = get_l1d_flush_bit();

	if ((l1tf_flag == 0)
		&& (bp_l1d_flush_bit == ap1_l1d_flush_bit)
		&& (bp_l1d_flush_bit == ap2_l1d_flush_bit)
		&& (bp_l1d_flush_bit == ap3_l1d_flush_bit)) {
		ret = true;
	} else {
		ret = false;
	}

	report("\t\t %s", ret, __FUNCTION__);
}

/*
 * @brief case name:Check x2APIC ID of each processor_01
 *
 * Summary:The physical platform shall guarantee that x2APIC ID of each processor is less
 * than 16.
 *
 */
static void applicaton_constraints_rqmid_46564_check_x2apic_id_01(void)
{
	uint32_t ret;

	uint32_t x2apic_id = 0;
	uint64_t apicbase = rdmsr(MSR_IA32_APICBASE);
	if ((apicbase & (APIC_EN | APIC_EXTD)) == (APIC_EN | APIC_EXTD)) {
		x2apic_id = x2apic_read(APIC_ID);
	}

	if ((x2apic_id < 16)
		&& (ap1_x2apicid != 0 && ap1_x2apicid < 16)
		&& (ap2_x2apicid != 0 && ap2_x2apicid < 16)
		&& (ap3_x2apicid != 0 && ap3_x2apicid < 16)) {
		ret = true;
	} else {
		ret = false;
	}

	report("\t\t %s", ret, __FUNCTION__);
}

/*
 * @brief case name:Check PCI MSI capability structure of the physical PCIe
 * device at physical BDF 00:14.0 or BDF 00:1F.6_01
 *
 * Summary:PCI MSI capability structure of the physical PCIe device at physical BDF 00:14.0 or
 * BDF 00:1F.6 shall be available on the physical platform.
 * PCI configuration space start offset of MSI capability structure of the physical PCIe
 * device at physical BDF 00:1F.6 shall be d0H on the physical platform.
 * PCI configuration space start offset of MSI capability structure of the physical PCIe
 * device at physical BDF 00:14.0 shall be 80H on the physical platform.
 *
 */
static void applicaton_constraints_rqmid_46565_check_pci_msi_01(void)
{
	uint32_t ret;

	if ((msi_1f6 == 0xd0) && (msi_146 == 0x80)) {
		ret = true;
	} else {
		ret = false;
	}

	report("\t\t %s", ret, __FUNCTION__);
}

/*
 * @brief case name:Check the bar of each physical PCI device specified in the configs module_01
 *
 * Summary:For each physical PCI device specified in the configs module, the BAR(including
 * base address, size and type) of the physical PCI devices shall be initialized as the
 * specified value in the table "Table of virtual PCI devices initialization" by external
 * application.
 *
 */
static void applicaton_constraints_rqmid_46566_check_pci_bar_01(void)
{
	uint32_t ret;

	if ((barinfo[0].vbars[0].size == 0x10000)
		&& (barinfo[0].vbars[1].size == 0)
		&& (barinfo[0].vbars[0].bar_type == 3)
		&& (barinfo[0].vbars[1].bar_type == 4)
		&& (barinfo[0].vbars[0].base == 0xdf230000)
		&& (barinfo[0].vbars[1].base == 0)
		&& (barinfo[1].vbars[0].size == 0x20000)
		&& (barinfo[1].vbars[0].bar_type == 2)
		&& (barinfo[1].vbars[0].base == 0xdf200000)) {
		ret = true;
	} else {
		ret = false;
	}

	report("\t\t %s", ret, __FUNCTION__);
}

/*
 * @brief case name:applicaton_constraints_cpuid_01
 *
 * Summary: Application constraints about CPUID on the physical platform derived from
 * ACRN hypervisor architecture design
 *
 */
static void applicaton_constraints_rqmid_46103_cpuid_01()
{
	struct cpuid cpuid_val;
	int a;
	int b;
	int c;
	int d;

	/* CPUID_00: EAX > 16H */
	cpuid_val = cpuid(0x0);
	if (cpuid_val.a < 0x16) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}
	/* CPUID_0H:EBX="Genu"*/
	if (strncmp((char *)&cpuid_val.b, "Genu", strlen("Genu")) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d\n", __LINE__);
		return;
	}
	/* CPUID_0H:EDX="ineI"*/
	if (strncmp((char *)&cpuid_val.d, "ineI", strlen("ineI")) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		d = cpuid_val.d;
		return;
	}

	cpuid_val = cpuid(0x1);
	/*CPUID_01:EAX[3:0]=AH*/
	if ((cpuid_val.a & SHIFT_MASK(3, 0)) != 0xA) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}
	/*CPUID_01:EAX[7:4]=EH*/
	if (((cpuid_val.a & SHIFT_MASK(7, 4)) >> 4) != 0xe) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}
	/*CPUID_01:EAX[11:8]=6H*/
	if (((cpuid_val.a & SHIFT_MASK(11, 8)) >> 8) != 0x6) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}
	/*CPUID_01:EAX[13:12]=0H*/
	if (((cpuid_val.a & SHIFT_MASK(13, 12)) >> 12) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}
	/*CPUID_01:EAX[19:16]=8H*/
	if (((cpuid_val.a & SHIFT_MASK(19, 16)) >> 16) != 0x8) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:EAX[27:20]=0H*/
	if ((cpuid_val.a & SHIFT_MASK(27, 20)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}
	/*CPUID_01:EBX[7:0]=0H*/
	if ((cpuid_val.b & SHIFT_MASK(7, 0)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:EBX[15:8]=8H*/
	if (((cpuid_val.b & SHIFT_MASK(15, 8)) >> 8) != 0x8) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:ECX[bit 10]=0H*/
	if ((cpuid_val.c & (1ul << 10)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	cpuid_val = cpuid(0x7);

	/*CPUID_01:EBX[bit 16]=0H*/
	if ((cpuid_val.b & (1ul << 16)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:EBX[bit 17]=0H*/
	if ((cpuid_val.b & (1ul << 17)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:EBX[bit 21]=0H*/
	if ((cpuid_val.b & (1ul << 21)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:EBX[bit 26]=0H*/
	if ((cpuid_val.b & (1ul << 26)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:EBX[bit 27]=0H*/
	if ((cpuid_val.b & (1ul << 27)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:EBX[bit 28]=0H*/
	if ((cpuid_val.b & (1ul << 28)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:EBX[bit 30]=0H*/
	if ((cpuid_val.b & (1ul << 30)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:EBX[bit 31]=0H*/
	if ((cpuid_val.b & (1ul << 31)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:ECX[bit 1]=0H*/
	if ((cpuid_val.c & (1ul << 1)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:ECX[bit 14]=0H*/
	if ((cpuid_val.c & (1ul << 14)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:ECX[21:17]=0H*/
	if ((cpuid_val.c & SHIFT_MASK(21, 17)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:EDX[bit 2]=0H*/
	if ((cpuid_val.d & (1ul << 2)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_01:EDX[bit 3]=0H*/
	if ((cpuid_val.d & (1ul << 3)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	cpuid_val = cpuid_indexed(0x0D, 0);
	/*CPUID_01:EAX[7:5]=0H*/
	if ((cpuid_val.a & SHIFT_MASK(7, 5)) != 0x0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	cpuid_val = cpuid_indexed(0x0D, 2);
	/*CPUID(EAX=DH,ECX=2):EAX=100H*/
	if (cpuid_val.a != 0x100) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID(EAX=DH,ECX=2):EBX=240H*/
	if (cpuid_val.b != 0x240) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}
	/*CPUID(EAX=DH,ECX=2):ECX[bit 0]=0H*/
	if ((cpuid_val.c & 0x1ul) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}
	/*CPUID(EAX=DH,ECX=2):ECX[bit 1]=0H*/
	if ((cpuid_val.c & (1ul << 1)) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	cpuid_val = cpuid(0x16);
	/*CPUID_16H:EAX[15:0]=834H*/
	if ((cpuid_val.a & SHIFT_MASK(15, 0)) != 0x834) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}
	/*CPUID_16H:EBX[15:0]=1068H*/
	if ((cpuid_val.b & SHIFT_MASK(15, 0)) != 0x1068) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}
	/*CPUID_16H:ECX[15:0]=64H*/
	if ((cpuid_val.c & SHIFT_MASK(15, 0)) != 0x64) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	cpuid_val = cpuid(0x80000000);
	/*CPUID_80000000H:EAX>=80000008H*/
	if (cpuid_val.a < 0x80000008) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	cpuid_val = cpuid(0x80000001);

	/*CPUID_80000001H:EAX=0H*/
	if (cpuid_val.a != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	cpuid_val = cpuid(0x80000002);
	a = cpuid_val.a;
	b = cpuid_val.b;
	c = cpuid_val.c;
	d = cpuid_val.d;

	/*CPUID_80000002H:EAX="Inte"*/
	if (strncmp((char *)&a, "Inte", strlen("Inte")) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d\n", __LINE__);
		return;
	}
	/*CPUID_80000002H:EBX="l(R)"*/
	if (strncmp((char *)&b, "l(R)", strlen("l(R)")) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d\n", __LINE__);
		return;
	}
	/*CPUID_80000002H:ECX=" Cor"*/
	if (strncmp((char *)&c, " Cor", strlen(" Cor")) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d\n", __LINE__);
		return;
	}
	/*CPUID_80000002H:EDX="e(TM"*/
	if (strncmp((char *)&d, "e(TM", strlen("e(TM")) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d\n", __LINE__);
		return;
	}

	cpuid_val = cpuid(0x80000003);
	a = cpuid_val.a;
	b = cpuid_val.b;
	c = cpuid_val.c;
	d = cpuid_val.d;
	/*CPUID_80000003H:EAX=") i7"*/
	if (strncmp((char *)&a, ") i7", strlen(") i7")) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d\n", __LINE__);
		return;
	}
	/*CPUID_80000003H:EBX="-865"*/
	if (strncmp((char *)&b, "-865", strlen("-865")) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d\n", __LINE__);
		return;
	}
	/*CPUID_80000003H:ECX="0U C"*/
	if (strncmp((char *)&c, "0U C", strlen("0U C")) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d\n", __LINE__);
		return;
	}
	/*CPUID_80000003H:ECX="PU @"*/
	if (strncmp((char *)&d, "PU @", strlen("PU @")) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d\n", __LINE__);
		return;
	}

	cpuid_val = cpuid(0x80000004);
	a = cpuid_val.a;
	b = cpuid_val.b;
	c = cpuid_val.c;
	d = cpuid_val.d;

	/*CPUID_80000004H:EAX=" 1.9"*/
	if (strncmp((char *)&a, " 1.9", strlen(" 1.9")) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d\n", __LINE__);
		return;
	}
	/*CPUID_80000004H:EBX="0GHz"*/
	if (strncmp((char *)&b, "0GHz", strlen("0GHz")) != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d\n", __LINE__);
		return;
	}

	/*CPUID_80000004H:ECX=0*/
	if (c != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*CPUID_80000004H:EDX=0*/
	if (d != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	cpuid_val = cpuid(0x80000006);
	/*CPUID_80000006H:ECX[7:0]=0*/
	if ((cpuid_val.c & 0xFF) != 0x40) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}
	/*CPUID_80000006H:ECX[15:12]=6H*/
	if (((cpuid_val.c & SHIFT_MASK(15, 12)) >> 12) != 0x6) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	cpuid_val = cpuid(0x80000008);
	/*CPUID_80000008H:ECX[7:0]=27H*/
	if ((cpuid_val.a & 0xFF) != 0x27) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}
	/*CPUID_80000008H:ECX[15:8]=30H*/
	if (((cpuid_val.a & 0xFF00) >> 8) != 0x30) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	report("\t\t %s", true, __FUNCTION__);
}

/*
 * @brief case name:applicaton constraints pci configuration space registers of ethernet controller_01
 *
 * Summary: Application constraints about PCI configuration space registers of Ethernet controller on the
 * physical platform derived from ACRN hypervisor architecture design
 *
 */
static void applicaton_constraints_rqmid_46503_pci_configuration_of_ethernet_controller_01()
{
	uint16_t command;

	command = pci_pdev_read_cfg(ethernet_bdf_native, PCI_COMMAND_OFFSET, PCI_COMMAND_SIZE);
	if (!(((command & SHIFT_MASK(15, 11)) == 0) && ((command & SHIFT_MASK(9, 0)) == 6)))	{
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint16_t status;
	status = pci_pdev_read_cfg(ethernet_bdf_native, PCI_STATUS_OFFSET, PCI_STATUS_SIZE);
	if (status != 0x10) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint8_t cache_line_size;
	cache_line_size = pci_pdev_read_cfg(ethernet_bdf_native, PCI_CACHE_LINE_OFFSET, PCI_CACHE_LINE_SIZES);
	if (cache_line_size != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint8_t master_latency_timer;
	master_latency_timer = pci_pdev_read_cfg(ethernet_bdf_native, PCI_MASTER_LATENCY_TIMER_OFFSET,
		PCI_MASTER_LATENCY_TIMER_SIZE);
	if (master_latency_timer != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint8_t bist;
	bist = pci_pdev_read_cfg(ethernet_bdf_native, PCI_BIST_OFFSET, PCI_BIST_SIZE);
	if (bist != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t erom_ba;
	erom_ba = pci_pdev_read_cfg(ethernet_bdf_native, PCI_EROM_BA_OFFSET, PCI_EROM_BA_SIZE);
	if (erom_ba != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint8_t interrupt_line;
	interrupt_line = pci_pdev_read_cfg(ethernet_bdf_native, PCI_INTERRUPT_OFFSET, PCI_INTERRUPT_SIZE);
	if (interrupt_line != 0xff) {
		printf("\t\t Error line is [%d] current value:%#x expected:0FF\n", __LINE__, interrupt_line);
		report("\t\t %s", false, __FUNCTION__);
		return;
	}

	uint8_t min_gnt;
	min_gnt = pci_pdev_read_cfg(ethernet_bdf_native, PCI_MIN_GNT_OFFSET, PCI_MIN_GNT_SIZE);
	if (min_gnt != 0) {
		printf("\t\t Error line is %d:\n", __LINE__);
		report("\t\t %s", false, __FUNCTION__);
		return;
	}

	uint8_t max_lat;
	max_lat = pci_pdev_read_cfg(ethernet_bdf_native, PCI_MAX_LAT_OFFSET, PCI_MAX_LAT_SIZE);
	if (max_lat != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t landisctrl;
	landisctrl = pci_pdev_read_cfg(ethernet_bdf_native, PCI_LANDISCTRL_OFFSET, PCI_LANDISCTRL_SIZE);
	if (landisctrl != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t locklandis;
	locklandis = pci_pdev_read_cfg(ethernet_bdf_native, PCI_LOCKLANDIS_OFFSET, PCI_LOCKLANDIS_SIZE);
	if (locklandis != 1) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t ltrcap;
	ltrcap = pci_pdev_read_cfg(ethernet_bdf_native, PCI_LTRCAP_OFFSET, PCI_LTRCAP_SIZE);
	if (ltrcap != 0x10031003) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	report("\t\t %s", true, __FUNCTION__);

}

/*
 * @brief case name:applicaton_constraints pci configuration space registers of usb controller_01
 *
 * Summary:Application constraints about PCI configuration space registers of Ethernet controller on the physical
 *platform derived from ACRN hypervisor architecture design
 *
 */
static void applicaton_constraints_rqmid_46104_pci_configuration_of_usb_controller_01()
{
	uint16_t command;
	uint16_t status;

	command = pci_pdev_read_cfg(usb_bdf_native, PCI_COMMAND_OFFSET, PCI_COMMAND_SIZE);
	if (!(((command & SHIFT_MASK(15, 11)) == 0) && ((command & SHIFT_MASK(9, 0)) == 6))) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	status = pci_pdev_read_cfg(usb_bdf_native, PCI_STATUS_OFFSET, PCI_STATUS_SIZE);
	if (status != 0x290) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint8_t cache_line_size;
	cache_line_size = pci_pdev_read_cfg(usb_bdf_native, PCI_CACHE_LINE_OFFSET, PCI_CACHE_LINE_SIZE);
	if (cache_line_size != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint8_t master_latency_timer;
	master_latency_timer = pci_pdev_read_cfg(usb_bdf_native, PCI_MASTER_LATENCY_TIMER_OFFSET,
		PCI_MASTER_LATENCY_TIMER_SIZE);
	if (master_latency_timer != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint8_t bist;
	bist = pci_pdev_read_cfg(usb_bdf_native, PCI_BIST_OFFSET, PCI_BIST_SIZE);
	if (bist != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t erom_ba;
	erom_ba = pci_pdev_read_cfg(usb_bdf_native, PCI_EROM_BA_OFFSET, PCI_EROM_BA_SIZE);
	if (erom_ba != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint8_t interrupt_line;
	interrupt_line = pci_pdev_read_cfg(usb_bdf_native, PCI_INTERRUPT_OFFSET, PCI_INTERRUPT_SIZE);
	if (interrupt_line != 0xff) {
		printf("\t\t Error line is [%d] current value:%#x expected:0FF\n", __LINE__, interrupt_line);
		report("\t\t %s", false, __FUNCTION__);
		return;
	}

	uint8_t min_gnt;
	min_gnt = pci_pdev_read_cfg(usb_bdf_native, PCI_MIN_GNT_OFFSET, PCI_MIN_GNT_SIZE);
	if (min_gnt != 0) {
		printf("\t\t Error line is %d:\n", __LINE__);
		report("\t\t %s", false, __FUNCTION__);
		return;
	}

	uint8_t max_lat;
	max_lat = pci_pdev_read_cfg(usb_bdf_native, PCI_MAX_LAT_OFFSET, PCI_MAX_LAT_SIZE);
	if (max_lat != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhhc1;
	xhhc1 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCC1_OFFSET, PCI_XHCC1_SIZE);
	if (xhhc1 != 0x803401FD) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhhc2;
	xhhc2 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCC2_OFFSET, PCI_XHCC2_SIZE);
	if (xhhc2 != 0x800FC688) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhclkgten;
	xhclkgten = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCLKGTEN_OFFSET, PCI_XHCLKGTEN_SIZE);
	if (xhclkgten != 0x0FCE6E5B) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t audsync;
	audsync = pci_pdev_read_cfg(usb_bdf_native, PCI_AUDSYNC_OFFSET, PCI_AUDSYNC_SIZE);
	if (audsync != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint8_t sbrn;
	sbrn = pci_pdev_read_cfg(usb_bdf_native, PCI_SBRN_OFFSET, PCI_SBRN_SIZE);
	if (sbrn != 0x30) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint8_t fladj;
	fladj = pci_pdev_read_cfg(usb_bdf_native, PCI_FLADJ_OFFSET, PCI_FLADJ_SIZE);
	if (fladj != 0x60) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint8_t besl;
	besl = pci_pdev_read_cfg(usb_bdf_native, PCI_BESL_OFFSET, PCI_BESL_SIZE);
	if (besl != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t vshdr;
	vshdr = pci_pdev_read_cfg(usb_bdf_native, PCI_VSHDR_OFFSET, PCI_VSHDR_SIZE);
	if (vshdr != 0x1400010) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint16_t pce_reg;
	pce_reg = pci_pdev_read_cfg(usb_bdf_native, PCI_PCE_REG_OFFSET, PCI_PCE_REG_SIZE);
	if (pce_reg != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint16_t hscfg2;
	hscfg2 = pci_pdev_read_cfg(usb_bdf_native, PCI_HSCFG2_OFFSET, PCI_HSCFG2_SIZE);
	if (hscfg2 != 0X1800) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb2_opm0;

	xhci_usb2_opm0 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB2_OPM0_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb2_opm0 != 6) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb2_opm1;
	xhci_usb2_opm1 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB2_OPM1_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb2_opm1 != 0x18) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb2_opm2;

	xhci_usb2_opm2 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB2_OPM2_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb2_opm2 != 1) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb2_opm3;
	xhci_usb2_opm3 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB2_OPM3_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb2_opm3 != 0x60) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d xhci_usb2_opm3=%x expected:0\n", __LINE__, xhci_usb2_opm3);
		return;
	}

	uint32_t xhci_usb2_opm4;
	xhci_usb2_opm4 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB2_OPM4_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb2_opm4 != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb2_opm5;
	xhci_usb2_opm5 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB2_OPM5_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb2_opm5 != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb2_opm6;
	xhci_usb2_opm6 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB2_OPM6_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb2_opm6 != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb2_opm7;
	xhci_usb2_opm7 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB2_OPM7_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb2_opm7 != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb3_opm0;
	xhci_usb3_opm0 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB3_OPM0_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb3_opm0 != 6) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb3_opm1;
	xhci_usb3_opm1 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB3_OPM1_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb3_opm1 != 0x18) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb3_opm2;
	xhci_usb3_opm2 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB3_OPM2_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb3_opm2 != 1) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb3_opm3;
	xhci_usb3_opm3 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB3_OPM3_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb3_opm3 != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb3_opm4;

	xhci_usb3_opm4 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB3_OPM4_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb3_opm4 != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb3_opm5;
	xhci_usb3_opm5 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB3_OPM5_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb3_opm5 != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb3_opm6;
	xhci_usb3_opm6 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB3_OPM6_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb3_opm6 != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	uint32_t xhci_usb3_opm7;
	xhci_usb3_opm7 = pci_pdev_read_cfg(usb_bdf_native, PCI_XHCI_USB3_OPM7_OFFSET, PCI_XHCI_USB_OPM_SIZE);
	if (xhci_usb3_opm7 != 0) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	report("\t\t %s", true, __FUNCTION__);
}

/*
 * @brief case name:The physical platform shall guarantee that specfic  CPUID leaves are supported_01
 * Summary: The physical platform shall guarantee that the following CPUID leaves are
 * supported 0H, 1H, 7H, 80000000H, 80000001H, 80000007H, and 80000008H.
 *
 */
static void applicaton_constraints_rqmid_46511_CPUID_support_leaves_check_01()
{
	u32 count = 0;
	bool is_pass = true;
	u32 cpuid_idx[] = {0, 1, 7, 0x80000000, 0x80000001, 0x80000007, 0x80000008};

	for (count = 0; count < (sizeof(cpuid_idx)/sizeof(cpuid_idx[0])); count++) {
		if (check_CPUID_leaf_support(cpuid_idx[count]) == false) {
			is_pass = false;
			break;
		}
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:The port range of pic rtc and  pci  configuration space is available_01
 *
 * Summary:The following port range shall be available on the physical platform:
 * 21H (legacy PIC)
 * 70H - 71H (RTC)
 * 0A1H (legacy PIC)
 * 0CF8H (for accessing PCI configuration space)
 * 0CFCH - 0CFFH (for accessing PCI configuration space)
 *
 */
static void applicaton_constraints_rqmid_46512_check_range_of_port_is_available_01()
{
	u32 count;
	bool is_pass = true;
	unsigned long port[] = {0x21, 0x70, 0x71, 0xA1, 0xCF8, 0xCFC, 0xCFD, 0xCFE, 0xCFF};

	for (count = 0; count < (sizeof(port)/sizeof(port[0])); count++) {
		if (check_port_available(port[count]) == false) {
			is_pass = false;
			break;
		}
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:The specfic MMIO range of IOAPIC  IOMMU and LAPIC is available_01
 *
 * Summary:The following MMIO range shall be available on the physical platform:
 * 0FEC00000H to 0FEC003FFH (for IOAPIC)
 * 0FED90000H to 0FED92000H (for IOMMU)
 * 0FEE00000H to 0FEE01000H (for LAPIC)
 *
 */
static void applicaton_constraints_rqmid_46513_check_range_of_MMIO_is_available_01()
{
	uint32_t tmp32 = 0;
	int chk_1 = 0;
	u8 lapic_ver;

	/* Write IOREGSEL */
	mmio_write32(IOAPIC_VER_OFFSET, (void *)IOREGSEL);
	/* Read  IOWIN */
	tmp32 = mmio_read32((void *)IOWIN);

	if ((tmp32 & 0xFFU) == IOAPIC_VER_VALUE) {
		chk_1++;
	}

	tmp32 = *(u32 *)(IOMMU_BASE_ADDR + IOMMU_VER_OFFSET);
	if ((tmp32 & 0xFFU) == IOMMU_VER_VALUE) {
		chk_1++;
	}

	reset_apic();
	tmp32 = *(u32 *)(LAPIC_BASE_ADDR + LAPIC_VER_OFFSET);
	lapic_ver = tmp32 & 0xFF;
	if ((lapic_ver >= 0x10) && (lapic_ver <= 0x15)) {
		chk_1++;
	}

	enable_apic();
	report("\t\t %s", chk_1 == 3, __FUNCTION__);
}

/*
 * @brief case name:The platform should support IA32_VMX_EPT_VPID_CAP_01
 *
 * Summary:The platform should support IA32_VMX_EPT_VPID_CAP.
 *
 */
static void applicaton_constraints_rqmid_46514_support_IA32_VMX_EPT_VPID_CAP01()
{
	struct cpuid cpuid_val;
	int c;
	u64 msr1;
	u64 msr2;
	bool is_pass = true;

	cpuid_val = cpuid(0x1);
	c = cpuid_val.c;
	msr1 = rdmsr(MSR_IA32_VMX_PROCBASED_CTLS);
	msr2 = rdmsr(MSR_IA32_VMX_PROCBASED_CTLS2);

	/*
	 * CPUID_01H.ECX[bit 5] A value of 1 indicates that the processor supports this technology
	 * The IA32_VMX_EPT_VPID_CAP MSR exists only on processors that support the 1-setting of the “activate secondary
	 * controls” VM-execution control (only if bit 63 of the IA32_VMX_PROCBASED_CTLS MSR is 1) and that support
	 * either the 1-setting of the “enable EPT” VM-execution control (only if bit 33 of the IA32_VMX_PROCBASED_CTLS2
	 * MSR is 1) or the 1-setting of the “enable VPID” VM-execution control (only if bit 37 of the
	 * IA32_VMX_PROCBASED_CTLS2 MSR is 1).
	 */
	if (!((c & (1ul << 5)) && (msr1 & (1ul << 63)) && ((msr2 & (1ul << 33)) || (msr2 & (1ul << 37))))) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:The EPT paging-structure memory type_wb_uc_2M_page_support_01
 *
 * Summary:The EPT paging-structure memory type write-back (WB) shall be available on the
 * physical platform.
 * The EPT paging-structure memory type uncacheable (UC) shall be available on
 * the physical platform.
 * The EPT 2-Mbyte page shall be available on the physical platform.
 *
 */
static void applicaton_constraints_rqmid_46515_memory_type_wb_uc_2M_page_support_01()
{
	u64 msr;

	msr = rdmsr(MSR_IA32_VMX_EPT_VPID_CAP);

	/*
	 * If bit 14 is read as 1, the logical processor allows software to configure
	 * the EPT paging-structure memory type  to be write-back (WB).
	 */
	if (!(msr & (1ul << 14))) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*
	 * If bit 8 is read as 1, the logical processor allows software to configure the EPT
	 * paging-structure memory type to be uncacheable (UC);
	 */
	if (!(msr & (1ul << 8))) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	/*
	 * If bit 16 is read as 1, the logical processor allows software to configure a EPT PDE to map
	 * a 2-Mbyte page (by setting bit 7 in the EPT PDE
	 */
	if (!(msr & (1ul << 16))) {
		report("\t\t %s", false, __FUNCTION__);
		printf("\t\t Error line is %d:\n", __LINE__);
		return;
	}

	report("\t\t %s", true, __FUNCTION__);
}

/*
 * @brief case name:processor base frequency in MHz in CPUID.16_01
 *
 * Summary:Physical platform shall report processor base frequency in MHz in CPUID.16H.
 *
 */
static void applicaton_constraints_rqmid_46516_processor_base_frequency_in_MHz_01()
{
	struct cpuid cpuid_val;
	int a;

	bool is_pass = true;

	cpuid_val = cpuid(0x16);
	a = cpuid_val.a;
	/*CPUID_16H:EAX Processor Base Frequency (in MHz).*/
	if (!((a >= 500) && (a <= 6000))) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:The physical IA32_SMM_MONITOR_CTL [bit 0] of the current processor shall be 0_01
 *
 * Summary:The physical IA32_SMM_MONITOR_CTL [bit 0] of the current processor shall be 0.
 *
 */
static void applicaton_constraints_rqmid_46517_IA32_SMM_MONITOR_CTL_bit_0_01()
{
	u64 msr;
	bool is_pass = true;

	msr = rdmsr(IA32_SMM_MONITOR_CTL);
	if ((msr & 1ul) != 0) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:The physical IA32_FEATURE_CONTROL[bit 2] shall be 1 which indicates that VMX is enabled_01
 *
 * Summary:The physical IA32_FEATURE_CONTROL[bit 2] shall be 1 which indicates that VMX is enabled.
 *
 */
static void applicaton_constraints_rqmid_46518_VMX_is_enabled_01()
{
	u64 msr;
	bool is_pass = true;

	msr = rdmsr(IA32_FEATURE_CONTROL);
	if (!(msr & (1ul << 2))) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:The IA32_FEATURE_CONTROL MSR is locked_01
 *
 * Summary:The physical IA32_FEATURE_CONTROL[bit 0] shall be 1 indicating that the
 * IA32_FEATURE_CONTROL MSR is locked
 *
 */
static void applicaton_constraints_rqmid_46519_IA32_FEATURE_CONTROL_is_locked_01()
{
	u64 msr;
	bool is_pass = true;

	msr = rdmsr(IA32_FEATURE_CONTROL);
	if ((msr & 1ul) != 1) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:VMX operation and related instructions shall be available on the physical platform_01
 * Summary:VMX operation and related instructions shall be available on the physical platform.
 *
 */
static void applicaton_constraints_rqmid_46520_VMX_oper_and_related_ins_shall_be_avail_01()
{
	struct cpuid cpuid_val;
	int c;
	bool is_pass = true;

	/*Just check CPUID_01h:ECX.VMX[bit5] == 1. others is  accomplished in HSI*/
	cpuid_val = cpuid(0x1);
	c = cpuid_val.c;
	if ((c & (1ul << 5)) == 0) {
		is_pass =  false;
	}
	report("\t\t %s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:The data read from Capability Register of DMAR unit except GPU is fixed_01
 *
 * The physical platform shall guarantee that the data read from Capability Register
 * of DMAR unit for all devices except GPU is d2008c40660462H. Refer to section
 *10.4.2 in Intel(R) Virtualization Technology for Direct I/O Architecture
 * Specification for a definition of bits in the Capability Register.
 *
 */
static void applicaton_constraints_rqmid_46521_Cap_Reg_of_DMAR_unit_is_fixed_01()
{
	uint64_t cap_dmar0;
	bool is_pass = true;

	reg_base = ioremap(IOMMU_BASE_ADDR, PAGE_SIZE);

	cap_dmar0 = reg_readq(DMAR_CAPAB_REG);
	if (cap_dmar0 != 0xd2008c40660462) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:The data read from Extend Capability Register of DMAR unit except GPU is fixed_01
 *
 * Summary:The physical platform shall guarantee that the data read from Extended Capability
 * Register of DMAR unit for all devices except GPU is f050daH. Refer to section
 * 10.4.3 in Intel(R) Virtualization Technology for Direct I/O Architecture
 * Specification for a definition of bits in the Extended Capability Register.
 *
 */
static void applicaton_constraints_rqmid_46522_Ext_Cap_Reg_of_DMAR_unit_is_fixed_01()
{
	uint64_t ecap_dmar0;
	bool is_pass = true;

	reg_base = ioremap(IOMMU_BASE_ADDR, PAGE_SIZE);
	ecap_dmar0 = reg_readq(DMAR_ECAPAB_REG);
	if (ecap_dmar0 != 0xf050da) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:The physical platform shall guarantee that the number of valid sub-leaves of CPUID leaf 4H is 4_01.
 *
 * Summary:The physical platform shall guarantee that the number of valid sub-leaves of  CPUID leaf 4H is 4.
 *
 */
static void applicaton_constraints_rqmid_46523_the_num_of_subleaves_of_CPUID_leaf_4_is_4_01()
{
	struct cpuid cpuid_val;
	int index = 0;
	int count = 0;

	for (index = 0; ; index++) {
		cpuid_val = cpuid_indexed(0x4, index);
		/*If ECX contains an invalid sub leaf index, EAX/EBX/ECX/EDX return 0*/
		if (!((cpuid_val.a == 0) && (cpuid_val.b == 0) && (cpuid_val.c == 0) && (cpuid_val.d == 0))) {
			count++;
		} else {
			break;
		}
	}
	report("\t\t %s", count == 4, __FUNCTION__);
}

/*
 * @brief case name:The physical platform shall guarantee that the number of extended function CPUID leaves is 9_01
 *
 * Summary:The physical platform shall guarantee that the number of extended function CPUID leaves is 9.
 *
 */
static void applicaton_constraints_rqmid_46524_the_num_of_ext_function_CPUID_leaf_is_9_01()
{
	struct cpuid cpuid_val;
	int a;
	bool is_pass = false;
	/*cpuid_80000000H:EAX Maximum Input Value for Extended Function CPUID Information*/
	cpuid_val = cpuid(0x80000000);
	a = cpuid_val.a;
	is_pass = (((a-0x80000000) + 1) == 9);
	report("\t\t %s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:The value of IA32_VMX_CR0_FIXED0_1 and _IA32_VMX_CR4_FIXED0_1 is fixed_01
 *
 * Summary:The physical platform shall guarantee that the value of the MSR
 * IA32_VMX_CR0_FIXED0 is 80000021H.
 * The physical platform shall guarantee that the value of the MSR
 * IA32_VMX_CR0_FIXED1 is FFFFFFFFH.
 * The physical platform shall guarantee that the value of the MSR
 * IA32_VMX_CR4_FIXED0 is 2000H.
 * The physical platform shall guarantee that the value of the MSR
 * IA32_VMX_CR4_FIXED1 is 3767FFH.
 *
 */
static void applicaton_constraints_rqmid_46525_01()
{
	bool is_pass = true;

	if (rdmsr(IA32_VMX_CR0_FIXED0) != 0x80000021) {
		is_pass = false;
	}

	if (rdmsr(IA32_VMX_CR0_FIXED1) != 0xFFFFFFFF) {
		is_pass = false;
	}

	if (rdmsr(IA32_VMX_CR4_FIXED0) != 0x2000) {
		is_pass = false;
	}

	if (rdmsr(IA32_VMX_CR4_FIXED1) != 0x3767FF) {
		is_pass = false;
	}

	report("\t\t %s", is_pass, __FUNCTION__);
}

/*
 * @brief case name:The physical platform shall guarantee that the value of IA32_BIOS_SIGN_ID is b400000000H_01
 * Summary:The physical platform shall guarantee that the value of IA32_BIOS_SIGN_ID is
 * b400000000H after executing CPUID instruction once.
 *
 */
static void applicaton_constraints_rqmid_46526_the_val_of_IA32_BIOS_SIGN_ID_is_b400000000H_01()
{
	cpuid(0);
	report("\t\t %s", rdmsr(IA32_BIOS_SIGN_ID) == 0xb400000000, __FUNCTION__);
}

/*
 * @brief case name:The value of physical RTC register BH of the physical platform shall be 2H_01
 *
 * Summary:The value of physical RTC register BH of the physical platform shall be 2H
 * indicating that the RTC reports time in twenty-four-hour mode in BCD data
 * representation.
 *
 */
static void applicaton_constraints_rqmid_46527_the_val_of_physical_RTC_register_BH_is_2H_01()
{
	uint8_t reg_b_value;

	outb(B_INDEX, RTC_INDEX_REG);
	(void)inb(RTC_TARGET_REG);
	reg_b_value = read_reg_al();

	report("\t\t %s", reg_b_value == B_DEFAULT_VALUE, __FUNCTION__);
}

/*
 * @brief case name:INVEPT instruction and  single-context INVEPT type shall be available_01
 *
 * Summary:INVEPT instruction shall be available on the physical platform.
 * The single-context INVEPT type shall be available on the physical platform.
 *
 */
static void applicaton_constraints_rqmid_46572_INVEPT_and_INVEPT_type_be_available_01()
{
	u64 msr;
	int chk = 0;
	msr = rdmsr(IA32_VMX_EPT_VPID_CAP);
	/*If bit 20 is read as 1, the INVEPT instruction is supported*/
	if (msr & (1ul << 20)) {
		chk++;
	}
	/*If bit 25 is read as 1, the single-context INVEPT type is supported*/

	if (msr & (1ul << 25)) {
		chk++;
	}

	report("\t\t %s", chk == 2, __FUNCTION__);
}


/*
 * @brief case name:INVVPID instruction shall be available on the physical platform_01
 *
 * Summary:INVVPID instruction shall be available on the physical platform.
 *
 */
static void applicaton_constraints_rqmid_46573_INVVPID_instruction_shall_be_available_01()
{
	u64 msr;
	int chk = 0;
	msr = rdmsr(IA32_VMX_EPT_VPID_CAP);
	/*If bit 32 is read as 1, the INVVPID instruction is supported*/
	if (msr & (1ul << 32)) {
		chk++;
	}

	report("\t\t %s", chk == 1, __FUNCTION__);

}

/*
 * @brief case name:Physical platform shall guarantee TURBO mode is disabled in BIOS_01
 *
 * Summary:Physical platform shall guarantee TURBO mode is disabled in BIOS.
 *
 */
static void applicaton_constraints_rqmid_46567_check_TURBO_mode_is_disabled()
{
	u64 msr;
	int chk = 0;
	u32 eax;

	msr = rdmsr(IA32_MISC_ENABLE);
	if (msr & (1ul << 38)) {
		chk++;
	} else {
		printf("\t\tIA32_MISC_ENABLE [bit 38] is not 1\n");
	}

	eax = cpuid(0x6).a;
	if ((eax & (1ul << 1)) == 0) {
		chk++;
	} else {
		printf("\t\tCPUID.06H:EAX[bit 1 is not 0\n");
	}

	report("\t\t %s", chk == 2, __FUNCTION__);
}
#endif

static void print_case_list(void)
{
	printf("\t\t applicaton constraints feature case list:\n\r");
#ifdef IN_NATIVE


	printf("\t\t Case ID:%d case name:%s\n\r", 46556,
			"Check all entries of Processor Local APIC Structure in ACPI table_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46557,
			"Check all entries of IOAPIC Structure in ACPI table_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46558,
			"Check the physical LAPIC mode_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46559,
			"Check the physical LAPIC ID_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46560,
			"Check all entries of the memory map buffer in multiboot_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46561,
			"Check the first instruction of program is executed on the physical BP_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46562,
			"Check all entries of the module in multiboot_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46563,
			"The effect of L1TF on each physical processor shall be the same_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46564,
			"Check x2APIC ID of each processor_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46565,
			"Check PCI MSI capability structure of the physical \
			PCIe device at physical BDF 00:14.0 or BDF 00:1F.6_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46566,
			"Check the bar of each physical PCI device specified in the configs module_01");

	printf("\t\t Case ID:%d case name:%s\n\r", 46013, "applicaton_constraints_cpuid_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46104,
			"applicaton_constraints pci configuration space registers of usb controller_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46503,
			"applicaton constraints pci configuration space registers of ethernet controller_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46511,
			"The physical platform shall guarantee that specfic CPUID leaves are supported_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46512,
			"The port range of pic rtc and pci configuration space is available_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46513,
			"The specfic MMIO range of IOAPIC  IOMMU and LAPIC is available_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46514,
			"The platform should support IA32_VMX_EPT_VPID_CAP_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46515,
			"The EPT paging-structure memory type_wb_uc_2M_page_support_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46516,
			"processor base frequency in MHz in CPUID.16_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46517,
			"The physical IA32_SMM_MONITOR_CTL [bit 0] of the current processor shall be 0_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46518,
			"The physical IA32_FEATURE_CONTROL[bit 2] shall be 1 which indicates that VMX is enabled_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46519,
			"The IA32_FEATURE_CONTROL MSR is locked_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46520,
			"VMX operation and related instructions shall be available on the physical platform_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46521,
			"The data read from Capability Register of DMAR unit except GPU is fixed_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46522,
			"The data read from Extend Capability Register of DMAR unit except GPU is fixed_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46523,
			"The physical platform shall guarantee that the number of valid sub-leaves of CPUID leaf 4H is 4_01.");
	printf("\t\t Case ID:%d case name:%s\n\r", 46524,
			"The physical platform shall guarantee that the number of extended function CPUID leaves is 9_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46525,
			"The value of IA32_VMX_CR0_FIXED0_1 and _IA32_VMX_CR4_FIXED0_1 is fixed_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46526,
			"The physical platform shall guarantee that the value of IA32_BIOS_SIGN_ID is b400000000H_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46527,
			"The value of physical RTC register BH of the physical platform shall be 2H_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46572,
			"INVEPT instruction and single-context INVEPT type shall be available_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46573,
			"INVVPID instruction shall be available on the physical platform_01");
	printf("\t\t Case ID:%d case name:%s\n\r", 46567,
			"Physical platform shall guarantee TURBO mode is disabled in BIOS_01");
#endif
	printf("\t\t \n\r \n\r");
}

int main(void)
{
	setup_vm();
	setup_idt();

	print_case_list();
#ifdef IN_NATIVE
	struct acpi_table_madt *madt = get_acpi_table_addr("APIC");
	lapic_num = local_parse_madt(madt, lapic_array);
	ioapic_num = parse_madt_ioapic(madt, ioapic_array);
	get_mmap_and_mods_info();
	pci_get_pci_configuration_space();

	applicaton_constraints_rqmid_46556_check_entry_of_lapic_in_acpi_table_01();
	applicaton_constraints_rqmid_46557_check_entry_of_ioapic_in_acpi_table_01();
	applicaton_constraints_rqmid_46558_check_physical_lapic_mode_01();
	applicaton_constraints_rqmid_46559_check_physical_lapic_id_01();
	applicaton_constraints_rqmid_46560_check_mmap_in_multiboot_01();
	applicaton_constraints_rqmid_46561_check_first_instruction_in_bp_01();
	applicaton_constraints_rqmid_46562_check_mods_in_multiboot_01();
	applicaton_constraints_rqmid_46563_check_l1tf_effect_01();
	applicaton_constraints_rqmid_46564_check_x2apic_id_01();
	applicaton_constraints_rqmid_46565_check_pci_msi_01();
	applicaton_constraints_rqmid_46566_check_pci_bar_01();

	applicaton_constraints_rqmid_46103_cpuid_01();
	applicaton_constraints_rqmid_46104_pci_configuration_of_usb_controller_01();
	applicaton_constraints_rqmid_46503_pci_configuration_of_ethernet_controller_01();
	applicaton_constraints_rqmid_46511_CPUID_support_leaves_check_01();
	applicaton_constraints_rqmid_46512_check_range_of_port_is_available_01();
	applicaton_constraints_rqmid_46513_check_range_of_MMIO_is_available_01();
	applicaton_constraints_rqmid_46514_support_IA32_VMX_EPT_VPID_CAP01();
	applicaton_constraints_rqmid_46515_memory_type_wb_uc_2M_page_support_01();
	applicaton_constraints_rqmid_46516_processor_base_frequency_in_MHz_01();
	applicaton_constraints_rqmid_46517_IA32_SMM_MONITOR_CTL_bit_0_01();
	applicaton_constraints_rqmid_46518_VMX_is_enabled_01();
	applicaton_constraints_rqmid_46519_IA32_FEATURE_CONTROL_is_locked_01();
	applicaton_constraints_rqmid_46520_VMX_oper_and_related_ins_shall_be_avail_01();
	applicaton_constraints_rqmid_46521_Cap_Reg_of_DMAR_unit_is_fixed_01();
	applicaton_constraints_rqmid_46522_Ext_Cap_Reg_of_DMAR_unit_is_fixed_01();
	applicaton_constraints_rqmid_46523_the_num_of_subleaves_of_CPUID_leaf_4_is_4_01();
	applicaton_constraints_rqmid_46524_the_num_of_ext_function_CPUID_leaf_is_9_01();
	applicaton_constraints_rqmid_46525_01();
	applicaton_constraints_rqmid_46526_the_val_of_IA32_BIOS_SIGN_ID_is_b400000000H_01();
	applicaton_constraints_rqmid_46527_the_val_of_physical_RTC_register_BH_is_2H_01();
	applicaton_constraints_rqmid_46572_INVEPT_and_INVEPT_type_be_available_01();
	applicaton_constraints_rqmid_46573_INVVPID_instruction_shall_be_available_01();
	applicaton_constraints_rqmid_46567_check_TURBO_mode_is_disabled();
#endif
	return report_summary();
}
