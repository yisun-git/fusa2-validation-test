#include "processor.h"
#include "condition.h"
#include "condition.c"
#include "libcflat.h"
#include "desc.h"
#include "alloc.h"
#include "misc.h"
#include "fwcfg.h"
#include "vmalloc.h"
#include "alloc_page.h"
#include "asm/io.h"
#include "pci.h"
#include "smp.h"

#define MMCFG_SAFETY		(0xE0000000)
#define IOAPIC			(0xFEC00000)
#define IOAPIC_REGSEL		(IOAPIC)
#define IOAPIC_IOWINDOW		(IOAPIC + 0x10)
#define IVSHMEM_MSIX_ENTRY_NUM (0x08)

#define RSDP_SIGNATURE_LEN      8U
#define ACPI_OEM_ID_SIZE	6U
#define ACPI_NAME_SIZE		4U
#define ACPI_SIG_RSDP		"RSD PTR "
#define ACPI_SIG_MCFG		"MCFG"
#define OFFSET_MCFG_ENTRY0_BASE 44U

#define LAST_16_BYTES(p)	((p) + 0xff0)
static uint32_t ivshmem_bar1;
static void *remapped_ivshmem;
static void *remapped_mmcfg;
static void *msix_table;
static uint32_t msix_table_count;

/*
 * IVSHMEM BAR1 address range and IOAPCI address range only allow DWORD
 * and QWORD access. for rm8, rm16 access, we need find address range in
 * PCI MMCFG address range.
 * vm0 and vm2 have different number of pci devices, we only use devices those
 * have at least 16 writeable bytes at the end of their configuration space.
 */
static u32 pci_devices;
static uint64_t pci_device_configs[4];

/* A PCIE configuration space 4K bytes, there are 64K configuration spaces
 * in 256M memory
 */
#define PCIE_PAGE_SIZE (0x1000)
#define TOTAL_PAGES (0x10000)
static bool write_test(u8 *conf) {
	/* only check the last 16 bytes */
	volatile u8 *reg = (volatile u8 *)conf + 0xff0;

	u8 i, j, read_back, new;

	for (j = 0x0; j < 0x10; j++) {
		i = *reg;
		new = i ^ 0xff;

		*reg = new;
		read_back = *reg;

		if (read_back != new) {
			*reg = i;
			return false;
		}
		*reg = i;
		reg++;
	}
	return true;
}

static void collect_config_spaces()
{
	int i;
	u8 *mmcfg = (u8 *)remapped_mmcfg;
	pci_devices = 0;

	for(i = 0; i < TOTAL_PAGES; i++) {
		if (*mmcfg != 0xff) {
			if (write_test(mmcfg)) {
			pci_device_configs[pci_devices] = (uint64_t)mmcfg;
			pci_devices++;
			}
		}
		if (pci_devices >= 4)
			break;

		mmcfg += PCIE_PAGE_SIZE;
	}
	printf("use %d pci devices for mmcfg access\n", pci_devices);
}

#define IVSHMEM_VENDOR_ID (0x1af4U)
#define IVSHMEM_DEVICE_ID (0x1110U)
void find_ivshmem_bar1()
{
	int i;
	u32 *mmcfg = (u32 *)remapped_mmcfg;

	ivshmem_bar1 = 0;
	for(i = 0; i < TOTAL_PAGES; i++) {
		/* check vendor id and device id */
		if (*mmcfg == ((IVSHMEM_DEVICE_ID << 16) | IVSHMEM_VENDOR_ID)) {
			ivshmem_bar1 = *(mmcfg + 5);
			break;
		}
		mmcfg += PCIE_PAGE_SIZE;
	}
	if(ivshmem_bar1 == 0) {
		printf("No ivshmem device found\n");
	} else {
		printf("ivshmem BAR1: 0x%x\n", ivshmem_bar1);
	}
}

#define CAP_LIST	(0x34)
#define MSIX_CAP_ID (0x11)
void get_msix_table(u8 *cap, u8 *mmcfg)
{
	u32 *temp = (u32 *)cap;
	u32 bar_indicator;
	u32 offset;

	/* bit[26:16]: size of MSI-X table size */
	msix_table_count = *temp & 0x07ff0000;
	msix_table_count = (msix_table_count >> 16) + 1;

	offset = *(temp + 1);
	bar_indicator = offset & 0x7;
	offset = offset & ~(0x7);

	temp = (u32 *)(mmcfg + 0x10 + bar_indicator * 4);
	msix_table = (void *)(intptr_t)((*temp & ~0xf) + offset);

	printf("use msix table at 0x%p, count 0x%x\n", msix_table, msix_table_count);
}

u8 * find_msix_cap_structrue(u8 *mmcfg)
{
	u8 *pos = (u8 *)mmcfg;
	u8 next;

	next = *(pos + CAP_LIST);
	while (next != 0) {
		pos = mmcfg + next;
		if (*pos == MSIX_CAP_ID) {
			return pos;
		} else {
			next = *(pos + 1);
		}
	}
	return NULL;
}

void find_msix_table()
{
	int i;
	u8 *conf = (u8 *)remapped_mmcfg;
	u8 *cap;

	msix_table = NULL;
	for (i = 0; i < TOTAL_PAGES; i++) {
		cap = NULL;
		if (*conf != 0xff)
			cap = find_msix_cap_structrue(conf);

		/* only use the first found msix table */
		if (cap) {
			get_msix_table(cap, conf);
			break;
		}

		conf += PCIE_PAGE_SIZE;
	}
	if (cap == NULL)
		printf("No msi-x table found\n");
}

#ifdef IN_NON_SAFETY_VM
static uint64_t mmcfg_sos;
struct acpi_table_rsdp {
	/* ACPI signature, contains "RSD PTR " */
	char signature[RSDP_SIGNATURE_LEN];
	/* ACPI 1.0 checksum */
	uint8_t checksum;
	/* OEM identification */
	char oem_id[ACPI_OEM_ID_SIZE];
	/* Must be (0) for ACPI 1.0 or (2) for ACPI 2.0+ */
	uint8_t revision;
	/* 32-bit physical address of the RSDT */
	uint32_t rsdt_physical_address;
	/* Table length in bytes, including header (ACPI 2.0+) */
	uint32_t length;
	/* 64-bit physical address of the XSDT (ACPI 2.0+) */
	uint64_t xsdt_physical_address;
	/* Checksum of entire table (ACPI 2.0+) */
	uint8_t extended_checksum;
	/* Reserved, must be zero */
	uint8_t reserved[3];
} __attribute__((packed));

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
	/* ASCII ASL compiler vendor ID */
	char asl_compiler_id[4];
	/* ASL compiler version */
	uint32_t asl_compiler_revision;
} __attribute__((packed));

struct acpi_table_xsdt {
	/* Common ACPI table header */
	struct acpi_table_header header;
	/* Array of pointers to ACPI tables */
	uint64_t table_offset_entry[1];
} __attribute__((packed));

struct acpi_mcfg_allocation {
        uint64_t address;               /* Base address, processor-relative */
        uint16_t pci_segment;           /* PCI segment group number */
        uint8_t start_bus_number;       /* Starting PCI Bus number */
        uint8_t end_bus_number;         /* Final PCI Bus number */
        uint32_t reserved;
} __packed;

static bool probe_table(uint64_t address, const char *signature)
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

/* acpi tables are placed at 0xf0000-0x100000 by BIOS */
static void* get_acpi_table_addr(const char *sig)
{
	struct acpi_table_rsdp *rsdp;
	struct acpi_table_xsdt *xsdt;
	uint32_t count;
	unsigned long addr;
	int i;

	for (addr = 0xf0000; addr < 0x100000; addr += 16) {
		rsdp = (void *)addr;
		if (strncmp(rsdp->signature, ACPI_SIG_RSDP, 8) == 0) {
			printf("find rsdp\n");
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

	xsdt = (struct acpi_table_xsdt *)(uint64_t)rsdp->xsdt_physical_address;
	count = (xsdt->header.length - sizeof(struct acpi_table_header)) / sizeof(uint64_t);

	for (i = 0U; i < count; i++) {
		if (probe_table(xsdt->table_offset_entry[i], sig)) {
			return (void *)(uint64_t)xsdt->table_offset_entry[i];
		}
	}

	return NULL;
}

static void init_mmcfg_sos()
{
	unsigned long  addr;
	struct acpi_mcfg_allocation *mcfg_table = NULL;

	addr = (unsigned long)get_acpi_table_addr(ACPI_SIG_MCFG);
	printf("get mmcfg table: 0x%lx\n", addr);
	mcfg_table = (struct acpi_mcfg_allocation *)(addr + OFFSET_MCFG_ENTRY0_BASE);
	if (mcfg_table) {
		printf("mcfg address: 0x%lx\n", mcfg_table->address);
		mmcfg_sos = mcfg_table->address;
	} else {
		printf("Fail to init mmcfg_sos\n");
	}
	return;
}
#endif

static void pci_init(void)
{
	pcidevaddr_t devfn;
	struct pci_dev pci_dev;

	for (devfn = 0; devfn < PCI_DEVFN_MAX; ++devfn) {
		if (pci_dev_exists(devfn)) {
			pci_dev_init(&pci_dev, devfn);
		}
	}
}

static volatile u32 execption_inc_len = 0;
u8 xpt = 0xff;
static void handled_exception(struct ex_regs *regs)
{
	struct ex_record *ex;
	unsigned ex_val;

	ex_val = regs->vector | (regs->error_code << 16) |
			(((regs->rflags >> 16) & 1) << 8);
	asm("mov %0, %%gs:"xstr(EXCEPTION_ADDR)"" : : "r"(ex_val));
	for (ex = &exception_table_start; ex != &exception_table_end; ++ex) {
		if (ex->rip == regs->rip) {
			regs->rip = ex->handler;
			return;
		}
	}

	if (execption_inc_len == 0) {
		unhandled_exception(regs, false);
	} else {
		regs->rip += execption_inc_len;
	}
}

static void set_handle_exception(void)
{
	handle_exception(DE_VECTOR, &handled_exception);
	handle_exception(DB_VECTOR, &handled_exception);
	handle_exception(NMI_VECTOR, &handled_exception);
	handle_exception(BP_VECTOR, &handled_exception);
	handle_exception(OF_VECTOR, &handled_exception);
	handle_exception(BR_VECTOR, &handled_exception);
	handle_exception(UD_VECTOR, &handled_exception);
	handle_exception(NM_VECTOR, &handled_exception);
	handle_exception(DF_VECTOR, &handled_exception);
	handle_exception(TS_VECTOR, &handled_exception);
	handle_exception(NP_VECTOR, &handled_exception);
	handle_exception(SS_VECTOR, &handled_exception);
	handle_exception(GP_VECTOR, &handled_exception);
	handle_exception(PF_VECTOR, &handled_exception);
	handle_exception(MF_VECTOR, &handled_exception);
	handle_exception(AC_VECTOR, &handled_exception);
	handle_exception(MC_VECTOR, &handled_exception);
	handle_exception(XM_VECTOR, &handled_exception);
}

static __unused u64 *trigger_pgfault(void)
{
	static u64 *add1 = NULL;
	if (NULL == add1) {
		add1 = malloc(sizeof(u64));
		set_page_control_bit((void *)add1, PAGE_PTE, PAGE_P_FLAG, 0, true);
	}
	return (u64 *)add1;
}

__attribute__ ((aligned(64))) static u64 addr_64 = 0;
static __unused u64 *non_canon_align_64(void)
{
	u64 address = (unsigned long)&addr_64;

	if ((address >> 63) & 1) {
		address = (address & (~(1ull << 63)));
	} else {
		address = (address | (1UL << 63));
	}
	return (u64 *)address;
}

// for SS_VECTOR
#define NON_CANO_PREFIX_MASK (0xFFFFUL << 48) /*bits [63:48] */
#define EXCEPTION_IST 1
#define OFFSET_IST 5
#define AVL_SET 0x1
ulong old_rsp;
static jmp_buf jmpbuf;
static u8 test_vector = NO_EXCEPTION;
static char exception_stack[0x1000] __attribute__((aligned(0x10)));
static char rspx_stack[0x1000] __attribute__((aligned(0x10)));

/* construct TSS, set rsp0, rsp1, rsp2, used to load the stack when a privilege
 * level change occurs. set ist1 which will be used to load the stack when
 * interrupt occurs
 */
tss64_t *set_ist(ulong non_cano_rsp)
{
	u64 ex_rsp = (u64)(exception_stack + sizeof(exception_stack));
	u64 rspx = ((non_cano_rsp > 0) ? non_cano_rsp : (u64)(rspx_stack + sizeof(rspx_stack)));
	tss64_t *tss64 = &tss +  OFFSET_IST;
	tss64->ist1 = ex_rsp;
	tss64->ist2 = 0;
	tss64->ist3 = 0;
	tss64->ist4 = 0;
	tss64->ist5 = 0;
	tss64->ist6 = 0;
	tss64->ist7 = 0;
	tss64->rsp0 = rspx;
	tss64->rsp1 = rspx;
	tss64->rsp2 = rspx;
	tss64->res1 = 0;
	tss64->res2 = 0;
	tss64->res3 = 0;
	tss64->res4 = 0;
	tss64->iomap_base = (u16)sizeof(tss64_t);
	return tss64;
}

/* setup an entry in GDT for TSS, and load TSS register */
void setup_tss64(int dpl, ulong non_cano_rsp)
{
	tss64_t *tss64 = set_ist(non_cano_rsp);
	int tss_ist_sel = TSS_MAIN + (sizeof(struct segment_desc64)) * OFFSET_IST; //tss64
	struct descriptor_table_ptr old_gdt_desc;
	sgdt(&old_gdt_desc);
	set_gdt64_entry(tss_ist_sel, (u64)tss64, sizeof(tss64_t) - 1,
		SEGMENT_PRESENT_SET | DESCRIPTOR_TYPE_SYS | (dpl << 5) | SEGMENT_TYPE_CODE_EXE_ONLY_ACCESSED,
		GRANULARITY_SET | L_64_BIT_CODE_SEGMENT | AVL_SET);
	lgdt(&old_gdt_desc);
	ltr(tss_ist_sel);
}

void longjmp_func(void)
{
	asm("movb %0, %%gs:"xstr(EXCEPTION_ADDR)"" : : "r"(test_vector));
	longjmp(jmpbuf, 0xbeef);
}

void ring0_non_cano_esp_handler(void);
asm (
"ring0_non_cano_esp_handler:\n"
	"movq old_rsp, %rax\n"
	"pop %rcx\n" //pop error code
	"mov %rsp, %rcx\n"
	"mov %rax, 24(%rcx)\n" //change rsp
	"movq $longjmp_func, (%rcx)\n" //change rip
	"iretq\n"
    );

u8 ring0_run_with_non_cano_rsp(void (*func)(void))
{
	int ret = 0;
	u64 non_cano_rsp = 0;
	test_vector = SS_VECTOR;
	setup_tss64(0, 0);
	set_idt_entry_ist(test_vector, ring0_non_cano_esp_handler, 0, EXCEPTION_IST);
	ret = setjmp(jmpbuf);
	if (ret == 0) {
		asm("movb $" xstr(NO_EXCEPTION) ", %%gs:"xstr(EXCEPTION_ADDR)"\n" : );
		asm volatile("mov %%rsp, %0\n" : "=r"(old_rsp) : );
		non_cano_rsp = old_rsp ^ NON_CANO_PREFIX_MASK; // construct a non-canonical address
		asm volatile("movq %0, %%rsp\n" : : "r"(non_cano_rsp) : "memory");
		func();
		asm volatile("movq old_rsp, %rsp\n");
	}
	u8 vector = exception_vector();
	setup_idt(); //restore idt
	return vector;
}

static void cmp_m64_r64_non_canonical()
{
	volatile u64 *rm64 = (u64 *)remapped_ivshmem;
	u32 result = 1;

	*rm64 = 0xabcd0000abcd0000;
	asm volatile ( "MOV $0x8086, %%r8\n\t"
			"MOV %1, %%rax\n\t"
			"ADD %%rsp, %%rax\n\t"
			"CMPQ %%r8, (%%rax)\n\t"
			"JA 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm64):"rax");
	report("%s", result, __FUNCTION__);
}

static void cmp_m64_r64(const char *msg)
{
	u8 vector = ring0_run_with_non_cano_rsp(cmp_m64_r64_non_canonical);
	report("%s", (vector == SS_VECTOR), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r/m8, r8
 *
 * Summary: mov r8 to r/m8
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address + offset (Random)
 * Safety:  0xE0000000 + offset
 */
void instruction_emulation_ACRN_T13805_mov_r8_rm8()
{
	int index = get_random_value() % pci_devices;
	u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[index]);
	u8 i = 0x1f;
	u8 j;

	asm volatile ( "MOVB %1, %%al\n\t"
			"MOVB %%al, %0\n\t"
			:"=m"(*reg):"m"(i) :"rax");
	j = *reg;
	report("%s", (i == j), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r/m8, r8 with rex
 *
 * Summary:  mov r8 to r/m8
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address max
 * Safety: Random in [0xE0000000, 0xE0000000+256MB)
 */
void instruction_emulation_ACRN_T13806_mov_rex_r8_rm8()
{
	u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[pci_devices - 1]);
	u8 i = 0x1f;
	u8 j;

	asm volatile ( "MOVB %1, %%r8b\n\t"
			"MOV %%r8b, %0" :"=m" (*reg):"r"(i) :"r8");
	j = *reg;
	report("%s", (i == j), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r/m16, r16
 *
 * Summary: mov r16 to r/m16
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13807_mov_r16_rm16()
{
	volatile u16 *reg = (u16 *)LAST_16_BYTES(pci_device_configs[0]);
	u16 i = 0xabcd;
	u16 j;
	asm volatile ( "MOVW %1, %%ax\n\t"
			"MOVW %%ax, %0" :"=m"(*reg):"m"(i) :"rax");
	j = *reg;

	report("%s", (i == j), __FUNCTION__);
}

/** 
 * @brief Case Name:Instruction Emulation MOV r/m32, r32
 *
 * Summary: mov r32 to r/m32
 *
 * Address range:
 * VSHMEM virtual device’s BAR1 address range Random */
void instruction_emulation_ACRN_T13808_mov_r32_rm32()
{
	u32 *ivsh = (u32 *)(remapped_ivshmem +
		    (get_random_value() % IVSHMEM_MSIX_ENTRY_NUM) * 0x10);
	u32 i = 0xABCDABCD;

	asm volatile ( "MOV $0xABCDABCD, %%eax\n\t"
			"MOV %%eax, %0" :"=m" (*ivsh) : :"rax");
	report("%s", (i == *ivsh), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r/m64, r64
 *
 * Summary: mov r64 to r/m64
 *
 * Address range:
 * Non-safety:PT PCIe device's MSIx BAR address
 * Safety: PT PCIe device's MSIx BAR address
 */
void instruction_emulation_ACRN_T13809_mov_rex_r64_rm64()
{
	u64 *msi = (u64 *)msix_table;
	u64 i = 0xABCDABCDABCDABCD;

	asm volatile ( "MOV $0xABCDABCDABCDABCD, %%rax\n\t"
			"MOV %%rax, %0" :"=m" (*msi) : :"rax");
	report("%s", (i == *msi), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r8, r/m8
 *
 * Summary: mov r/m8 to r8
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address max
 * Safety: Random in [0xE0000000, 0xE0000000+256MB)
 */
void instruction_emulation_ACRN_T13810_mov_rm8_r8()
{
	volatile u8 *reg = (u8 *)pci_device_configs[pci_devices - 1];
	u8 i = 0;

	asm volatile ( "MOVB %1, %%al\n\t"
			"MOVB %%al, %0\n\t" :"=m"(i):"m" (*reg):"rax");

	report("%s", (i == *reg), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r8, r/m8 with rex
 *
 * Summary: mov r/m8 to r8
 *
 * Address range
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13811_mov_rex_rm8_r8()
{
	volatile u8 *reg = (u8 *)pci_device_configs[0];
	u8 i = 0;

	asm volatile ( "MOV %1, %%r8b\n\t"
			"MOV %%r8b, %0\n\t" :"=r"(i):"m" (*reg):"rax");
	report("%s", (i == *reg), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r16, r/m16
 *
 * Summary: mov r/m16 to r16
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13812_mov_rm16_r16()
{
	u16 *reg = (u16 *)pci_device_configs[0];
	u16 i = 0;

	asm volatile ( "MOV %1, %%ax\n\t"
			"MOV %%ax, %0\n\t" :"=r"(i):"m" (*reg):"rax");
	report("%s", (i == *reg), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r32, r/m32
 *
 * Summary: mov r/m32 to r32
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13813_mov_rm32_r32()
{
	u32 *reg = (u32 *)pci_device_configs[0];
	u32 i;

	asm volatile ( "MOV %1, %%eax\n\t"
			"MOV %%eax, %0\n\t" :"=r"(i):"m" (*reg):"rax");
	report("%s", (i == *reg), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r64, r/m64
 *
 * Summary: mov r/m64 to r64
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13814_mov_rex_rm64_r64()
{
	u64 *reg = (u64 *)pci_device_configs[0] + 2;
	u64 i = 0;

	asm volatile ( "MOV %1, %%rax\n\t"
			"MOV %%rax, %0\n\t" :"=r"(i):"m" (*reg):"rax");
	report("%s", (i == 0x00000000FFFFFFFF), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV AX, moffs16
 *
 * Summary: Move word at (seg:offset) to AX
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address max
 * Safety: Random in [0xE0000000, 0xE0000000+256MB)
 *
 * with/without 2E (CS)
 */
void instruction_emulation_ACRN_T13815_mov_offs16_AX()
{
	u16 *reg = (u16 *)pci_device_configs[pci_devices - 1];
	u16 i = 0;

	asm volatile ( "MOV %1, %%rax\n\t"
			"MOVW %%cs:(%%rax), %%ax\n\t"
			"MOV %%ax, %0\n\t"
			:"=r"(i): "m" (reg):"rax");
	report("%s", (i == *reg), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV EAX, moffs32
 *
 * Summary: Move doubleword at (seg:offset) to EAX
 *
 * Address range:
 * IVSHMEM virtual device's BAR1 address range Min
 *
 * with/without 64 (FS)
 */
void instruction_emulation_ACRN_T13816_mov_offs32_EAX()
{
	u32 *ivsh = (u32 *)remapped_ivshmem;
	u32 i;

	*ivsh = 0xABCDABCD;
	asm volatile ( "MOV %1, %%rax\n\t"
			"MOV %%ss:(%%rax), %0\n\t"
			:"=r"(i):"m" (ivsh):"rax");
	report("%s", (i == *ivsh), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV RAX, moffs64
 *
 * Summary: Move quadword at (offset) to RAX.
 *
 * Address range:
 * IVSHMEM virtual device's BAR1 address range Random
 */
void instruction_emulation_ACRN_T13817_mov_rex_offs64_RAX()
{
	u64 *ivsh = (u64 *)remapped_ivshmem +
		    (get_random_value() % IVSHMEM_MSIX_ENTRY_NUM) * 0x10 / 0x8;
	u64 i;

	*ivsh = 0xABCDABCDABCDABCD;
	asm volatile ( "MOV %1, %%rax\n\t"
			"MOV %%fs:(%%rax), %0\n\t"
			:"=r"(i):"m" (ivsh):"rax");
	report("%s", (i == *ivsh), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV moffs16, AX
 *
 * Summary: Move AX to (seg:offset)
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address max
 * Safety: Random in [0xE0000000, 0xE0000000+256MB)
 *
 * with/without 64 (FS)
 */
void instruction_emulation_ACRN_T13818_mov_AX_offs16()
{
	u16 *reg = (u16 *)LAST_16_BYTES(pci_device_configs[pci_devices - 1]);

	u16 i = *reg ^ 0xff;
	asm volatile ( "MOV %1, %%ax\n\t"
			"MOV %%ax, %%ds:%0\n\t"
			:"=m" (*reg):"r"(i):"rax");
	u16 j = *reg;

	report("%s", (i == j), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV moffs32, EAX
 *
 * Summary: Move EAX to (seg:offset)
 *
 * Address range:
 * IVSHMEM virtual device's BAR1 address range Min
 *
 * with/without 64 (FS)
 */
void instruction_emulation_ACRN_T13819_mov_EAX_offs32()
{
	u32 *ivsh = (u32 *)remapped_ivshmem;
	u32 i;

	i = 0xABCDABCD;
	asm volatile ( "MOV %1, %%eax\n\t"
			"MOV %%eax, %%es:%0\n\t"
			:"=m" (*ivsh): "r"(i):"rax");
	report("%s", (i == *ivsh), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV moffs64, RAX
 *
 * Summary: Move RAX to (offset)
 *
 * Address range:
 * VSHMEM virtual device's BAR1 address range Random
 *
 * with/without 64 (FS)
 */
void instruction_emulation_ACRN_T13820_mov_rex_RAX_offs64()
{
	u64 *ivsh = (u64 *)(remapped_ivshmem +
		    (get_random_value() % IVSHMEM_MSIX_ENTRY_NUM) * 0x10);
	u64 i;

	i = 0xABCDABCDABCDABCD;
	asm volatile ( "MOV %1, %%rax\n\t"
			"MOV %%rax, %%fs:%0\n\t"
			:"=m" (*ivsh): "r"(i):"rax");
	report("%s", (i == *ivsh), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r/m8, imm8
 *
 * Summary: Move imm8 to r/m8
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 *
 * with/without 65 (GS)
 */
void instruction_emulation_ACRN_T13821_mov_imm8_rm8()
{
	volatile u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[0]);
	*reg = 0x1f;

	asm volatile ( "MOVB $0x1f, %%cs:%0\n\t"
			:"=m" (*reg)::"memory");
	report("%s", (*reg == 0x1f), __FUNCTION__);
}


/**
 * @brief Case Name: Instruction Emulation MOV r/m8, imm8 with rex
 *
 * Summary: Move imm8 to r/m8
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address max
 * Safety: Random in [0xE0000000, 0xE0000000+256MB)
 */
void instruction_emulation_ACRN_T13822_mov_rex_imm8_rm8()
{
	volatile u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[pci_devices - 1]);
	u8 i;

	i = 0xAB;
	asm volatile ("MOVB $0xAB, %%r8b\n\t"
			"MOVB %%r8b, %0\n\t" :"=m" (*reg)::"r8");
	report("%s", (i == *reg), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r/m16, imm16
 *
 * Summary: Move imm16 to r/m16
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address + offset (random)
 * Safety:  0xE0000000 + offset
 */
void instruction_emulation_ACRN_T13823_mov_imm16_rm16()
{
	int index = get_random_value() % pci_devices;
	volatile u16 *reg = (u16 *)LAST_16_BYTES(pci_device_configs[index]);
	u16 i;

	*reg = 0xABCD;
	i = 0xABCD;
	report("%s", (i == *reg), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r/m32, imm32
 *
 * Summary: Move imm32 to r/m32
 *
 * Address range:
 * Non-safety:HECI MSIx BAR address Max
 * Safety: VGA MSIx BAR address Max
 */
void instruction_emulation_ACRN_T13824_mov_imm32_rm32()
{
	u32 *msi = (u32 *)(msix_table);
	u32 i;

	i = 0xABCDABCD;
	*msi = 0xABCDABCD;
	report("%s", (i == *msi), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOV r/m64, imm32
 *
 * Summary: Move imm32 to r/m64
 *
 * Address range:
 * IVSHMEM virtual device's BAR1 address range Min
 */
void instruction_emulation_ACRN_T13825_mov_rex_imm32_rm64()
{
	volatile u64 *ivsh = (u64 *)remapped_ivshmem;
	u64 i;

	*ivsh = 0;
	i = 0xABCDABCD;
	asm volatile ("MOVL $0xABCDABCD, %0\n\t" :"=m"(*ivsh)::"memory");
	report("%s", (i == *ivsh), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVS m8, m8
 *
 * Summary: For legacy mode, Move byte from address DS:(E)SI to ES:(E)DI.
 *          For 64-bit mode move byte from address (R|E)SI to (R|E)DI
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 *
 * with/without F2
 */
#define SIZEOF_HELLO 5
void instruction_emulation_ACRN_T13826_movs_m8_m8()
{
	u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[0]);
	char source[] = "hello";

	asm volatile ("MOV %0, %%rax\n\t"
			"LEA (%%rax), %%rdi\n\t"
			"LEA %1, %%rsi\n\t"
			"MOV $"xstr(SIZEOF_HELLO)", %%ecx\n\t"
			"CLD\n\t"
			"REPNZ/MOVSB (%%rsi), (%%rdi)\n\t"
			::"r"(reg),"m"(source):"rsi", "rdi", "rax", "ecx");
	report("%s", (strncmp(source, (const char*)reg, SIZEOF_HELLO) == 0),
		__FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVSB
 *
 * Summary: For legacy mode, Move byte from address DS:(E)SI to ES:(E)DI.
 *          For 64-bit mode move byte from address (R|E)SI to (R|E)DI
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address max
 * Safety: Random in [0xE0000000, 0xE0000000+256MB)
 *
 * with/without F3
 */

void instruction_emulation_ACRN_T13827_movsb()
{
	u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[pci_devices - 1]);
	char source[] = "hello";

	asm volatile ("MOV %0, %%rax\n\t"
			"LEA (%%rax), %%rdi\n\t"
			"LEA %1, %%rsi\n\t"
			"MOV $"xstr(SIZEOF_HELLO)", %%ecx\n\t"
			"CLD\n\t"
			"REP/MOVSB\n\t"
			::"r"(reg),"m"(source):"rsi", "rdi", "rax", "ecx");
	report("%s", (strncmp(source, (const char*)reg, SIZEOF_HELLO) == 0),
			__FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVS m16, m16
 *
 * Summary: For legacy mode, move word from address DS:(E)SI to ES:(E)DI.
 *          For 64-bit mode move word at address (R|E)SI to (R|E)DI.
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 *
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13828_movs_m16_m16()
{
	u16 *reg = (u16 *)LAST_16_BYTES(pci_device_configs[0]);
	char source[] = "hello";

	asm volatile ("MOV %0, %%rax\n\t"
			"LEA (%%rax), %%rdi\n\t"
			"LEA %1, %%rsi\n\t"
			"MOVSW\n\t"
			::"r"(reg),"m"(source):"rsi", "rdi", "rax");
	report("%s", (*(u16 *)source == *reg), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVS m32, m32
 *
 * Summary: For legacy mode, move dword from address DS:(E)SI to ES:(E)DI.
 *          For 64-bit mode move dword from address (R|E)SI to (R|E)DI
 *
 * Address range:
 * IVSHMEM virtual device's BAR1 address range Random
 */
void instruction_emulation_ACRN_T13829_movs_m32_m32()
{
	u32 *ivsh = (u32 *)remapped_ivshmem +
		    (get_random_value() % IVSHMEM_MSIX_ENTRY_NUM) * 0x10 / 0x4;
	char source[] = "hello";

	asm volatile ("MOV %0, %%rax\n\t"
			"LEA (%%rax), %%rdi\n\t"
			"LEA %1, %%rsi\n\t"
			"CLD\n\t"
			"MOVSL (%%rsi), (%%rdi)"
			::"r"(ivsh),"m"(source):"rsi", "rdi", "rax");
	report("%s", (*(u32 *)source == *ivsh), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVS m64, m64
 *
 * Summary: Move qword from address (R|E)SI to (R|E)DI
 *
 * Address range:
 * IVSHMEM virtual device's BAR1 address range Max
 */
void instruction_emulation_ACRN_T13830_movs_rex_m64_m64()
{
	u64 *ivsh = (u64 *)remapped_ivshmem + (IVSHMEM_MSIX_ENTRY_NUM - 1) * 0x10 / 0x8;
	char source[] = "hello world";

	memset(ivsh, 0, 0x10);
	asm volatile ("MOV %0, %%rax\n\t"
			"LEA (%%rax), %%rdi\n\t"
			"LEA %1, %%rsi\n\t"
			"CLD\n\t"
			"MOVSQ (%%rsi), (%%rdi)\n\t"
			::"r"(ivsh),"m"(source):"rsi", "rdi", "rax");

	report("%s", (*(u64 *)source == *(u64 *)ivsh), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVSW
 *
 * Summary: For legacy mode, move word from address DS:(E)SI to ES:(E)DI.
 *          For 64-bit mode move word at address (R|E)SI to (R|E)DI.
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13831_movsw()
{
	u16 *reg = (u16 *)LAST_16_BYTES(pci_device_configs[0]);
	char source[] = "hello";

	asm volatile ("MOV %0, %%rax\n\t"
			"LEA (%%rax), %%rdi\n\t"
			"LEA %1, %%rsi\n\t"
			"MOVSW\n\t"
			::"r"(reg),"m"(source):"rsi", "rdi", "rax");
	report("%s", (*(u16 *)source == *reg), __FUNCTION__);
}

/**
 * @brief Case Name:Instruction Emulation MOVSD
 *
 * Summary: For legacy mode, move dword from address DS:(E)SI to ES:(E)DI.
 *          For 64-bit mode move dword from address (R|E)SI to (R|E)DI
 *
 * Address range:
 * Non-safety:HECI MSIx BAR address range random
 * Safety: VGA MSIx BAR address range random
 */
void instruction_emulation_ACRN_T13832_movsd()
{
	u32 *msix = (u32 *)msix_table + get_random_value() % msix_table_count;
	char source[] = "hello";

	asm volatile ("MOV %0, %%rax\n\t"
			"LEA (%%rax), %%rdi\n\t"
			"LEA %1, %%rsi\n\t"
			"MOVSL\n\t"
			::"r"(msix),"m"(source):"rsi", "rdi", "rax");
	report("%s", (*(u32 *)source == *msix), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVSQ
 *
 * Summary: Move qword from address (R|E)SI to (R|E)DI
 *
 * Address range:
 * IVSHMEM virtual device's BAR1 address range Min
 */
void instruction_emulation_ACRN_T13833_rex_movsq()
{
	u8 *ivsh = (u8 *)remapped_ivshmem;
	char source[] = "hello world";

	memset(ivsh, 0, 0x10);
	asm volatile ("MOV %0, %%rax\n\t"
			"LEA (%%rax), %%rdi\n\t"
			"LEA %1, %%rsi\n\t"
			"MOVSQ\n\t"
			::"r"(ivsh),"m"(source):"rsi", "rdi", "rax");
	report("%s", (*(u64 *)source == *(u64 *)ivsh), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVZX r16, r/m8
 *
 * Summary: Move byte to word with zero-extension
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13850_movzx_rm8_r16()
{
	volatile u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[0]);
	u32 result = 1;

	*reg = 0x80;
	asm volatile ("MOVZX %1, %%ax\n\t"
			"BT $15, %%ax\n\t"
			"JNB 1f\n\t"
			"mov $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*reg):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVZX r32, r/m8
 *
 * Summary: Move byte to doubleword, zero-extension
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13851_movzx_rm8_r32()
{
	volatile u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[0]);
	u32 result = 1;

	*reg = 0x80;
	asm volatile ("MOVZX %1, %%eax\n\t"
			"BT $31, %%eax\n\t"
			"JNB 1f\n\t"
			"mov $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*reg):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVZX r64, r/m8
 *
 * Summary: Move byte to quadword, zero-extension
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13852_movzx_rex_rm8_r64()
{
	volatile u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[0]);
	u32 result = 1;

	*reg = 0x80;
	asm volatile ("MOVZX %1, %%rax\n\t"
			"BT $63, %%rax\n\t"
			"JNB 1f\n\t"
			"mov $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*reg):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVZX r32, r/m16
 *
 * Summary: Move word to doubleword, zero-extension
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13853_movzx_rm16_r32()
{
	volatile u16 *reg = (u16 *)LAST_16_BYTES(pci_device_configs[0]);
	u32 result = 1;

	*reg = 0x8000;
	asm volatile ("MOVZX %1, %%eax\n\t"
			"BT $31, %%eax\n\t"
			"JNB 1f\n\t"
			"mov $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*reg):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVZX r64, r/m16
 *
 * Summary: Move word to quadword, zero-extension
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13854_movzx_rex_rm16_r64()
{
	volatile u16 *reg = (u16 *)LAST_16_BYTES(pci_device_configs[0]);
	u32 result = 1;

	*reg = 0x8000;
	asm volatile ("MOVZX %1, %%rax\n\t"
			"BT $63, %%rax\n\t"
			"JNB 1f\n\t"
			"mov $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*reg):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVSX r16, r/m8
 *
 * Summary: Move byte to word with sign-extension
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13855_movsx_rm8_r16()
{
	volatile u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[0]);
	u32 result = 1;

	*reg = 0x80;
	asm volatile ("XOR %%ax, %%ax\n\t"
			"MOVSX %1, %%ax\n\t"
			"BT $15, %%ax\n\t"
			"JC 1f\n\t"
			"mov $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*reg):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVSX r32, r/m8
 *
 * Summary: Move byte to doubleword with sign extension
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address max
 * Safety: Random in [0xE0000000, 0xE0000000+256MB)
 */
void instruction_emulation_ACRN_T13856_movsx_rm8_r32()
{
	volatile u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[pci_devices - 1]);
	u32 result = 1;

	*reg = 0x80;
	asm volatile ("XOR %%eax, %%eax\n\t"
			"MOVSX %1, %%eax\n\t"
			"BT $31, %%eax\n\t"
			"JC 1f\n\t"
			"MOVL $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*reg):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation MOVSX r64, r/m8
 *
 * Summary: Move byte to quadword with sign-extension
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13857_movsx_rex_rm8_r64()
{
	volatile u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[0]);
	u32 result = 1;

	*reg = 0x80;
	asm volatile ("XOR %%rax, %%rax\n\t"
			"MOVSX %1, %%rax\n\t"
			"BT $63, %%rax\n\t"
			"JC 1f\n\t"
			"mov $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*reg):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation STOS m8
 *
 * Summary: For legacy mode, store AL at address ES:(E)DI;
 *          For 64-bit mode store AL at address RDI or EDI
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13834_stos_m8()
{
	register u8 *reg asm("edi") = (u8 *)LAST_16_BYTES(pci_device_configs[0]);
	u8 i = 0xab;

	asm volatile ("MOV $0xab, %%al\n\t"
			"STOSB (%0)\n\t" ::"r"(reg):"rax");
	report("%s", (i == *reg), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation STOSB
 *
 * Summary: For legacy mode, store AL at address ES:(E)DI;
 *          For 64-bit mode store AL at address RDI or EDI
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13835_stosb()
{
	u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[0]);
	u8 i = 0xab;

	asm volatile ("MOV $0xab, %%al\n\t"
			"MOV %0, %%rdi\n\t"
			"STOSB\n\t" ::"m"(reg):"rax","rdi");
	report("%s", (i == *reg), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation STOS m16
 *
 * Summary: For legacy mode, store AX at address ES:(E)DI;
 *          For 64-bit mode store AX at address RDI or EDI
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address max
 * Safety: Random in [0xE0000000, 0xE0000000+256MB)
 */
void instruction_emulation_ACRN_T13836_stos_m16()
{
	register u16 *dest asm("edi") =
		(u16 *)LAST_16_BYTES(pci_device_configs[pci_devices - 1]);
	u16 i = 0xabcd;

	asm volatile ("MOV $0xabcd, %%ax\n\t"
			"STOSW (%0)\n\t" ::"r"(dest):"rax");
	report("%s", (i == *dest), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation STOS m32
 *
 * Summary: For legacy mode, store EAX at address ES:(E)DI;
 *          For 64-bit mode store EAX at address RDI or EDI
 *
 * Address range:
 * IVSHMEM virtual device’s BAR1 address range Random
 * */
void instruction_emulation_ACRN_T13837_stos_m32()
{
	register u32 *ivsh asm("edi") = (u32 *)remapped_ivshmem +
		    (get_random_value() % IVSHMEM_MSIX_ENTRY_NUM) * 0x10 / 0x4;
	u32 i = 0xabcdabcd;

	/* in gcc, stosd is called stosl */
	asm volatile ("MOV $0xabcdabcd, %%eax\n\t"
			"STOSL (%0)\n\t" ::"r"(ivsh):"rax");
	report("%s", (i == *ivsh), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation STOS m64
 *
 * Summary: Store RAX at address RDI or EDI
 *
 * Address range:
 * IVSHMEM virtual device's BAR1 address range Max
 */
void instruction_emulation_ACRN_T13838_stos_rex_m64()
{
	register u64 *ivsh asm("edi") = (u64 *)remapped_ivshmem +
		(IVSHMEM_MSIX_ENTRY_NUM - 1) * 0x10 / 0x8;
	u64 i = 0xabcdabcdabcdabcd;

	asm volatile ("MOV $0xabcdabcdabcdabcd, %%rax\n\t"
			"STOSQ (%0)\n\t" ::"r"(ivsh):"rax");
	report("%s", (i == *ivsh), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation STOSW
 *
 * Summary: For legacy mode, store AX at address ES:(E)DI;
 *          For 64-bit mode store AX at address RDI or EDI
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address + offset(random)
 * Safety:  0xE0000000 + offset
 */
void instruction_emulation_ACRN_T13839_stosw()
{
	int index = get_random_value() % pci_devices;
	u16 *dest = (u16 *)LAST_16_BYTES(pci_device_configs[index]);
	u16 i = 0xabcd;

	asm volatile ("MOVW $0xabcd, %%ax\n\t"
			"MOVQ %0, %%rdi\n\t"
			"STOSW\n\t" ::"m"(dest):"rax","memory");
	report("%s", (i == *dest), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation STOSD
 *
 * Summary: For legacy mode, store EAX at address ES:(E)DI;
 *          For 64-bit mode store EAX at address RDI or EDI
 *
 * Address range:
 * IVSHMEM virtual device's BAR1 address range Min
 */
void instruction_emulation_ACRN_T13840_stosd()
{
	u32 *dest = (u32 *)remapped_ivshmem;
	u32 i = 0xf0f0f0f0;

	/* in gcc, stosd is called stosl */
	asm volatile ("MOV $0xf0f0f0f0, %%rax\n\t"
			"MOV %0, %%rdi\n\t"
			"STOSL \n\t" ::"m"(dest):"rax", "rdi");
	report("%s", (i == *dest), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation STOSQ
 *
 *
 * Summary: Store RAX at address RDI or EDI
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address  + offset(random)
 * Safety:  0xE0000000 + offset （Random)
 */
void instruction_emulation_ACRN_T13841_rex_stosq()
{
	int index = get_random_value() % pci_devices;
	volatile u64 *dest = (u64 *)LAST_16_BYTES(pci_device_configs[index]);
	u64 i = 0xabcdabcdabcdabcd;

	asm volatile ("MOVQ $0xabcdabcdabcdabcd, %%rax\n\t"
			"MOVQ %0, %%rdi\n\t"
			"STOSQ\n\t" ::"m"(dest):"rax", "rdi", "memory");
	report("%s", (i == *dest), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation SUB r16, r/m16
 *
 * Summary: Subtract r/m16 from r16
 *
 * Address range:
 * Non-safety: Random addr in PCI MMCFG Address range
 * Safety: 0xE0000000+offset
 * with/without prefix 0x66
 */
void instruction_emulation_ACRN_T13862_sub_rm16_r16()
{
	int index = get_random_value() % pci_devices;
	volatile u16 *rm16 = (u16 *)LAST_16_BYTES(pci_device_configs[index]);
	u32 result = 0;

	*rm16 = 0x8086;
	/* 0xffff - 0x8086 = 0x7f79 */
	asm volatile ("MOV $0xffff, %%ax\n\t"
			"SUB %1, %%ax\n\t"
			"MOV %%ax, %0\n\t" :"=m"(result):"m"(*rm16):"rax");
	report("%s", (result == 0x7f79), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation SUB r32, r/m32
 *
 * Summary: Subtract r/m32 from r32
 *
 * Address range:
 * IOAPIC’s registers address range Max Random in
 * [0xFEC00000, 0xFEC00000+0x40)
 */
void instruction_emulation_ACRN_T13789_sub_rm32_r32()
{
	u32 *rm32 = (u32 *)IOAPIC_IOWINDOW;
	u32 result = 0;
	int line = 0;//the first redirection table

	ioapic_set_redir(line, 0x80, TRIGGER_EDGE);
	*(volatile u32 *)IOAPIC_REGSEL = (0x10 + line * 2);

	asm volatile ("MOV $0xffffff80, %%eax\n\t"
			"SUB %1, %%eax\n\t"
			"MOV %%eax, %0\n\t" :"=r"(result):"m"(*rm32):"rax");

	/* 0xffffff80 - 0x00000080 = 0xffffff00 */
	report("%s", (result == 0xffffff00), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation SUB r64, r/m64
 *
 * Summary: Subtract r/m64 from r64
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13790_sub_rex_rm64_r64()
{
	u64 *rm64 = (u64 *)remapped_mmcfg + 2;/* BAR0 */
	u64 result = 0;

	/* 0xffffffffffffffff - 0x00000000ffffffff */
	asm volatile ("MOV $0xffffffffffffffff, %%r8\n\t"
			"SUB %1, %%r8\n\t"
			"MOV %%r8, %0\n\t" :"=r"(result):"m"(*rm64):"rax");
	report("%s", (result == 0xffffffff00000000), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r/m16, r16
 *
 * Summary: Compare r16 with r/m16
 *
 * Address range:
 * Non-Safety: PCI MMCFG Min
 * Safety:  0xE0000000
 *
 * with/without prefix prefix 0x67
 */
void instruction_emulation_ACRN_T13791_cmp_r16_rm16()
{
	volatile u16 *rm16 = (u16 *)LAST_16_BYTES(pci_device_configs[0]);
	u16 result = 1;

	*rm16 = 0x8080;
	asm volatile ( "MOVW $0x8080, %%ax\n\t"
			"MOVQ %1, %%rbx\n\t"
			"CMPW %%ax, (%%rbx)\n\t"
			"JE 1f\n\t"
			"MOVW $0, %0\n\t"
			"1:\n\t" :"=m"(result):"m"(rm16):"rax", "rbx");

	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r/m32, r32
 *
 * Summary: Compare r32 with r/m32
 *
 * Address range:
 * IOAPIC's registers address range random
 */
void instruction_emulation_ACRN_T13792_cmp_r32_rm32()
{
	volatile u32 *rm32 = (u32 *)IOAPIC_IOWINDOW;
	u32 result = 1;
	int line = get_random_value() % 24;

	ioapic_set_redir(line, 0x76, TRIGGER_EDGE);
	*(volatile u32 *)IOAPIC_REGSEL = (0x10 + line * 2);

	asm volatile ( "MOV $0x80868086, %%eax\n\t"
			"CMPL %%eax, %1\n\t"
			"JB 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm32):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r/m64, r64
 *
 * Summary: Compare r64 with r/m64
 *
 * Address range:
 * IVSHMEM virtual device's BAR1 address range Min
 */
void instruction_emulation_ACRN_T13793_cmp_rex_r64_rm64()
{
	volatile u64 *rm64 = (u64 *)remapped_ivshmem;
	u32 result = 1;

	*rm64 = 0xabcd0000abcd0000;
	asm volatile ( "MOV $0x8086, %%r8\n\t"
			"CMPQ %%r8, %1\n\t"
			"JA 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm64):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r16, r/m16
 *
 * Summary: Compare r/m16 with r16
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13794_cmp_rm16_r16()
{
	volatile u16 *rm16 = (u16 *)LAST_16_BYTES(pci_device_configs[0]);
	u16 result = 1;

	*rm16 = 0x8080;
	asm volatile ( "MOV $0x8080, %%ax\n\t"
			"CMPW %1, %%ax\n\t"
			"JE 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm16):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r32, r/m32
 *
 * Summary: Compare r/m32 with r32
 *
 * Address range:
 * Non-safety: Pt PCI device MSIx BAR address Min
 * Safety:  Pt PCI address MSIx BAR address Min
 */
void instruction_emulation_ACRN_T13795_cmp_rm32_r32()
{
	volatile u32 *rm32 = (u32 *)msix_table;
	u32 result = 1;

	*rm32 = 0x80;
	asm volatile ( "MOV $0x8086, %%eax\n\t"
			"CMPL %1, %%eax\n\t"
			"JA 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm32):"rax");
	report("%s", result, __FUNCTION__);

}

/**
 * @brief Case Name: Instruction Emulation CMP r64, r/m64
 *
 * Summary: Compare r/m64 with r64
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13796_cmp_rex_rm64_r64()
{
	u64 *rm64 = (u64 *)remapped_mmcfg;
	u32 result = 1;
	asm volatile ( "MOV $0x8086, %%r8\n\t"
			"CMPQ %1, %%r8\n\t"
			"JB 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm64):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r/m8, imm8
 *
 * Summary: Compare imm8 with r/m8
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13797_cmp_imm8_rm8()
{
	u8 *rm8 = (u8 *)remapped_mmcfg;
	u32 result = 1;

	asm volatile ( "CMPB $1, %1\n\t"
			"JA 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm8):);
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r/m8, imm8 with rex
 *
 * Summary: Compare imm8 with r/m8
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13798_cmp_rex_imm8_rm8()
{
	u8 *rm8 = (u8 *)remapped_mmcfg;
	u32 result = 1;
	asm volatile ( "MOV %1, %%r8\n\t"
			"CMP $1, %%r8\n\t"
			"JA 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm8):);
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r/m16, imm16
 *
 * Summary: Compare imm16 with r/m16
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13799_cmp_imm16_rm16()
{
	u16 *rm16 = (u16 *)remapped_mmcfg;
	u32 result = 1;

	asm volatile ( "CMPW $1, %1\n\t"
			"JA 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm16):);
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r/m32, imm32
 *
 * Summary: Compare imm32 with r/m32
 *
 * Address range:
 * IOAPIC's registers address range Max
 */
void instruction_emulation_ACRN_T13800_cmp_imm32_rm32()
{
	u32 *rm32 = (u32 *)IOAPIC_IOWINDOW;
	u32 result = 1;
	int line = 23;//the last redirection table

	ioapic_set_redir(line, 0x76, TRIGGER_EDGE);
	*(volatile u32 *)IOAPIC_REGSEL = (0x10 + line * 2);

	asm volatile ( "CMPL $1, %1\n\t"
			"JA 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm32):);
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r/m64, imm32
 *
 * Summary: Compare imm32 with r/m64
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13801_cmp_rex_imm32_rm64()
{
	u64 *rm64 = (u64 *)remapped_mmcfg;
	u32 result = 1;

	asm volatile ( "CMPQ $1, %1\n\t"
			"JA 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm64):);
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r/m16, imm8
 *
 * Summary: Compare imm8 with r/m16
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13802_cmp_imm8_rm16()
{
	u16 *rm16 = (u16 *)remapped_mmcfg;
	u32 result = 1;

	asm volatile ( "CMPW $1, %1\n\t"
			"JA 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm16):);
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r/m32, imm8
 *
 * Summary: Compare imm8 with r/m32
 *
 * Address range:
 * Random in IOAPIC’s registers address range
 * [0xFEC00000, 0xFEC00000+0x40)
 */
void instruction_emulation_ACRN_T13803_cmp_imm8_rm32()
{
	u32 *rm32 = (u32 *)IOAPIC_IOWINDOW;
	u32 result = 1;
	int line = get_random_value() % 24;

	ioapic_set_redir(line, 0x76, TRIGGER_EDGE);
	*(volatile u32 *)IOAPIC_REGSEL = (0x10 + line * 2);

	asm volatile ( "CMPL $1, %1\n\t"
			"JA 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm32):);
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation CMP r/m64, imm8
 *
 * Summary: Compare imm8 with r/m64
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13804_cmp_rex_imm8_rm64()
{
	u64 *rm64 = (u64 *)remapped_mmcfg;
	u32 result = 1;

	asm volatile ( "CMPQ $1, %1\n\t"
			"JA 1f\n\t"
			"MOV $0, %0\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm64):);
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation AND r16, r/m16
 *
 * Summary: r16 and r/m16
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address max
 * Safety: Random in [0xE0000000, 0xE0000000+256MB)
 */
void instruction_emulation_ACRN_T13842_and_rm16_r16()
{
	volatile u16 *rm16 = (u16 *)pci_device_configs[pci_devices - 1];
	u16 value = 0;
	u16 result = 0;

	value = *rm16 & 0x00ff;
	asm volatile ("MOVW $0x00ff, %%ax\n\t"
			"ANDW %1, %%ax\n\t"
			"MOVW %%ax, %0\n\t" :"=m"(result):"m"(*rm16):"rax");
	report("%s", (result == value), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation AND r32, r/m32
 *
 * Summary: r32 and r/m32
 *
 * Address range:
 * Non-safety:HECI MSIx BAR address Min
 * Safety: VGA MSIx BAR address Min
 */
void instruction_emulation_ACRN_T13843_and_rm32_r32()
{
	volatile u32 *rm32 = (u32 *)msix_table;
	*rm32 = 0xabcdabcd;
	u32 result;

	asm volatile ("MOVL $0xffff, %%eax\n\t"
			"ANDL %1, %%eax\n\t"
			"MOVL %%eax, %0\n\t" :"=m"(result):"m"(*rm32):"rax");

	report("%s", (result == 0xabcd), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation AND r64, r/m64
 *
 * Summary: r64 and r/m64
 *
 * Address range:
 * IVSHMEM virtual device’s BAR1 address range Random */
void instruction_emulation_ACRN_T13844_and_rex_rm64_r64()
{
	u64 *ivsh = (u64 *)remapped_ivshmem +
		(IVSHMEM_MSIX_ENTRY_NUM - 1) * 0x10 / 0x8;
	*ivsh = 0xabcdabcdabcdabcd;
	u64 result;

	asm volatile ("MOVQ $0xffffffff, %%r8\n\t"
			"ANDQ %1, %%r8\n\t"
			"MOVQ %%r8, %0\n\t" :"=m"(result):"m"(*ivsh):"rax");
	report("%s", (result == 0xabcdabcd), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation TEST r/m8, r8
 *
 * Summary: AND r8 with r/m8; set SF, ZF, PF according to result
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address + offset
 * Safety:  0xE0000000 + offset （Random）
 */
void instruction_emulation_ACRN_T13845_test_r8_rm8()
{
	int index = get_random_value() % pci_devices;
	volatile u8 *reg = (u8 *)LAST_16_BYTES(pci_device_configs[index]);
	u32 result = 1;

	*reg = 0x8f;
	asm volatile ( "MOVB $0x70, %%al\n\t"
			"TESTB %%al, %1\n\t"
			"JZ 1f\n\t"
			"MOVL $0, %0\n\t"
			"1:\n\t"	:"=m"(result) :"m"(*reg):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation TEST r/m8, r8 with rex
 *
 * Summary: AND r8 with r/m8; set SF, ZF, PF according to result
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13846_test_rex_r8_rm8()
{
	u8 *rm8 = (u8 *)remapped_mmcfg;
	u32 result = 1;

	/* 0x70 & 0x86 */
	asm volatile ( "MOVB $0x70, %%r8b\n\t"
			"TESTB %%r8b, %1\n\t"
			"JZ 1f\n\t"
			"MOVL $0, %0\n\t"
			"1:\n\t" :"=m"(result) :"m"(*rm8):);
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation TEST r/m16, r16
 *
 * Summary: AND r16 with r/m16; set SF, ZF, PF according to result
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13847_test_r16_rm16()
{
	u16 *rm16 = (u16 *)remapped_mmcfg;
	u16 result = 1;

	/* 0x70 & 0x8086 */
	asm volatile ( "MOVW $0x0070, %%ax\n\t"
			"TESTW %%ax, %1\n\t"
			"JZ 1f\n\t"
			"MOVW $0, %0\n\t"
			"1:\n\t" :"=m"(result):"m"(*rm16):"rax", "rbx");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation TEST r/m32, r32
 *
 * Summary: AND r32 with r/m32; set SF, ZF, PF according to result
 *
 * Address range:
 * IOAPIC's registers address range Min (0xFEC00000)
 */
void instruction_emulation_ACRN_T13848_test_r32_rm32()
{
	u32 *rm32 = (u32 *)IOAPIC_IOWINDOW;
	u32 result = 1;
	int line = 23;//the last redirection table

	ioapic_set_redir(line, 0x80, TRIGGER_EDGE);
	*(volatile u32 *)IOAPIC_REGSEL = (0x10 + line * 2);

	asm volatile ( "MOVL $0x70, %%eax\n\t"
			"TESTl %%eax, %1\n\t"
			"JZ 1f\n\t"
			"MOVL $0, %0\n\t"
			"1:\n\t" :"=m"(result) :"m"(*rm32):"rax");
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation TEST r/m64, r64
 *
 * Summary: AND r64 with r/m64; set SF, ZF, PF according to result
 *
 * Address range:
 * IVSHMEM virtual device’s BAR1 address range Min
 */
void instruction_emulation_ACRN_T13849_test_rex_r64_rm64()
{
	volatile u64 *rm64 = (u64 *)remapped_ivshmem;
	u32 result = 1;

	*rm64 = 0x8080abcd8080abcd;
	asm volatile ( "MOVQ $0x7070000000000000, %%r8\n\t"
			"TESTQ %%r8, %1\n\t"
			"JZ 1f\n\t"
			"MOVL $0, %0\n\t"
			"1:\n\t" :"=m"(result) :"m"(*rm64):);
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation BT r/m16, imm8
 *
 * Summary: Store selected bit in CF flag
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13858_bt_imm8_rm16()
{
	u16 *bit_base = (u16 *)remapped_mmcfg;
	u16 result = 1;

	asm volatile ( "BTW $15, %1\n\t"
			"JC 1f\n\t"
			"MOVW $0, %0\n\t"
			"1:\n\t" :"=m"(result) :"m"(*bit_base):);
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation BT r/m32, imm8
 *
 * Summary: Store selected bit in CF flag
 *
 * Address range:
 * IOAPIC's registers address range Max 0xFEC00040
 */
void instruction_emulation_ACRN_T13859_bt_imm8_rm32()
{
	u32 *bit_base = (u32 *)IOAPIC_IOWINDOW;
	u32 result = 1;
	int line = 23;//the last redirection table

	ioapic_set_redir(line, 0x76, TRIGGER_EDGE);
	*(volatile u32 *)IOAPIC_REGSEL = (0x10 + line * 2);

	asm volatile ( "BTL $6, %1\n\t"
			"JC 1f\n\t"
			"MOVL $0, %0\n\t"
			"1:\n\t" :"=m"(result) :"m"(*bit_base):);
	report("%s", result, __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation BT r/m64, imm8
 *
 * Summary: Store selected bit in CF flag
 *
 * Address range:
 * Non-Safety: PCI MMCFG Address Min
 * Safety:  0xE0000000
 */
void instruction_emulation_ACRN_T13860_bt_rex_imm8_rm64()
{
	u64 *bit_base = (u64 *)remapped_mmcfg;
	u16 rflag = 0;

	asm volatile ( "MOV %1, %%r8\n\t"
			"BTQ $7, %%r8\n\t"
			"PUSHF\n\t"
			"POP %0\n\t" :"=m"(rflag) :"m"(*bit_base):);
	report("%s", (rflag & 0x1), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation sub r8, rm8 exception #UD
 *
 * Summary: #UD if the LOCK prefix is used
 */
void instruction_emulation_ACRN_T13894_exception_sub_ud()
{
	u8 *rm8 = (u8 *)remapped_mmcfg; /* 0x86 */

	asm volatile ( ASM_TRY("1f") "lock\n\t"
			"SUBB %0, %%al\n\t"
			"1:\n\t" ::"m"(*rm8):"rax");
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation cmp AL, imm8  exception #UD
 *
 * Summary: #UD if the LOCK prefix is used
 */
void instruction_emulation_ACRN_T13895_exception_cmp_ud()
{
	asm volatile( ASM_TRY("1f") "lock \n\t"
			"CMPB $0, %%al\n\t"
			"1:\n\t" :::"rax");
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation and r8, rm8 exception #UD
 *
 * Summary: #UD if the LOCK prefix is used
 */
void instruction_emulation_ACRN_T13897_exception_and_8_ud()
{
	u8 *dest = (u8 *)remapped_ivshmem + 0x1000 - 1;

	asm volatile( ASM_TRY("1f")
			"lock \n\t"
			"ANDB %0, %%al\n\t"
			"1: \n\t" ::"m"(*dest):"rax");
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

/**
 * @brief Case Name Instruction Emulation and r64, rm64 exception #UD
 *
 * Summary: #UD if the LOCK prefix is used
 */
void instruction_emulation_ACRN_T13891_exception_and_64_ud()
{
	u64 *dest = (u64 *)remapped_ivshmem + 0x1000 - 1;

	asm volatile( ASM_TRY("1f")
			"lock \n\t"
			"ANDQ %0, %%rax\n\t"
			"1: \n\t" ::"m"(*dest):"rax");
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}
/*
 * @brief Case Name: Instruction Emulation test AL, imm8 exception #UD
 *
 * Summary: #UD if the LOCK prefix is used
 */
void instruction_emulation_ACRN_T13898_exception_test_ud()
{
	u8 *dest = (u8 *)remapped_ivshmem;

	asm volatile( ASM_TRY("1f")
			"lock \n\t"
			"TESTB %0, %%al\n\t"
			"1: \n\t" ::"m"(*dest):"rax");
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation movsx r32, rm16 exception #UD
 *
 * Summary: #UD if the LOCK prefix is used
 */
void instruction_emulation_ACRN_T13899_exception_movsx_ud()
{
	u16 *rm16 = (u16 *)remapped_mmcfg;
	u32 result = 1;

	asm volatile (ASM_TRY("1f")
			"lock \n\t"
			"MOVSX %1, %%eax\n\t"
			"1:\n\t" :"=r"(result):"m"(*rm16):"rax");
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation bt rm16, r16 exception #UD
 *
 * Summary: #UD if the LOCK prefix is used
 */
void instruction_emulation_ACRN_T13900_exception_bt_ud()
{
	u16 *dest = (u16 *)remapped_ivshmem + 0x800 - 1;

	asm volatile( ASM_TRY("1f")
			"lock \n\t"
			"BTW %%ax, %0\n\t"
			"1: \n\t" ::"m"(*dest):"rax");
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation mov rm16, sreg exception #UD
 *
 * Summary: #UD if attempt is made to load the CS register.
 */
void instruction_emulation_ACRN_T13896_exception_mov_ud()
{
	u16 *descriptor = (u16 *)remapped_mmcfg;

	asm volatile (ASM_TRY("1f") "MOVW %0, %%cs\n\t"
			"1: \n\t" ::"m"(*descriptor):);
	report("%s", (exception_vector() == UD_VECTOR), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation movs m64, m64 exception #GP
 *
 * Summary: Move qword from address (R|E)SI to (R|E)DI,
 *          destination address is non-canonical
 */
void instruction_emulation_ACRN_T13901_exception_movs_gp()
{
	u8 *ivsh = (u8 *)remapped_ivshmem + 0x1000 - 0x8;

	asm volatile (ASM_TRY("1f") "MOV %0, %%rax\n\t"
			"LEA (%%rax), %%rdi\n\t"
			"LEA %1, %%rsi\n\t"
			"MOVSQ\n\t"
			"1:\n\t"
			:"=m" (*(non_canon_align_64()))
			:"m"(*ivsh):"rsi", "rdi", "rax");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}

/*
 * @brief Case Name: Instruction Emulation mov r64, rm64 exception #GP
 *
 * Summary: #GP if the memory address is in a non-canonical form
 */
void instruction_emulation_ACRN_T13892_exception_mov_gp()
{
	u64 *ivsh = (u64 *)((u64)remapped_ivshmem & (~(1ull << 63)));

	asm volatile (ASM_TRY("1f") "MOVQ %%rax, %0\n\t"
			"1:\n\t" :"=m"(*ivsh):: "rax");
	report("%s", (exception_vector() == GP_VECTOR), __FUNCTION__);
}
/**
 * @brief Case Name: Instruction Emulation movs m64, m64 exception #PF
 *
 * Summary: Move qword from address (R|E)SI to (R|E)DI,
 *          destination address is not in page table 
 */
void instruction_emulation_ACRN_T13902_exception_movs_pf()
{
	char source[] = "hello world";

	asm volatile (ASM_TRY("1f") "MOV %0, %%rax\n\t"
			"LEA (%%rax), %%rdi\n\t"
			"LEA %1, %%rsi\n\t"
			"MOVSQ\n\t"
			"1:\n\t"
			::"m" (*(trigger_pgfault())),"m"(source)
			:"rsi", "rdi", "rax");
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation test r64, rm64 exception #PF
 *
 * Summary: #PF if a page fault occurs
 */
void instruction_emulation_ACRN_T13893_exception_test_pf()
{
	asm volatile (ASM_TRY("1f") "TEST %0, %%rax\n\t"
			"1:\n\t"
			::"m" (*(trigger_pgfault())):"rax");
	report("%s", (exception_vector() == PF_VECTOR), __FUNCTION__);
}

/**
 * @brief Case Name: Instruction Emulation cmp r64, m64 exception #SS
 *
 * Summary: memory address referencing the SS segment is in a non-canonical form
 */
void instruction_emulation_ACRN_T13889_exception_cmp_ss()
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_Ad_Cann_non_mem();
	execption_inc_len = 3;
	do_at_ring0(cmp_m64_r64, "");
	execption_inc_len = 0;
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

static void sub_m64_r64_unaligned(const char *msg)
{
	u64 *reg = (u64 *)LAST_16_BYTES(pci_device_configs[0] + 1);
	asm volatile (ASM_TRY("1f") "SUB %0, %%rax\n\t"
			"1:\n\t" ::"m"(*reg):"rax");
	xpt = exception_vector();
}

/**
 * @brief Case Name: Instruction Emulation sub r64, m64 exception #AC
 *
 * Summary: Alignment checking is enabled and an unaligned memory reference
 *          is made while the current privilege level is 3
 */
void instruction_emulation_ACRN_T13890_exception_sub_ac()
{
	u32 cr0 = read_cr0();
	u32 cr2 = read_cr2();
	u32 cr3 = read_cr3();
	u32 cr4 = read_cr4();
	u32 cr8 = read_cr8();
	condition_LOCK_not_used();
	condition_pgfault_not_occur();
	condition_cs_cpl_3();
	condition_EFLAGS_AC_set_to_1();
	condition_CR0_AM_1();
	condition_Alignment_unaligned();
	execption_inc_len = 3;
	do_at_ring3(sub_m64_r64_unaligned, "");
	condition_EFLAGS_AC_set_to_0();
	execption_inc_len = 0;
	report("%s", (xpt == AC_VECTOR), __FUNCTION__);
	write_cr0(cr0);
	write_cr2(cr2);
	write_cr3(cr3);
	write_cr4(cr4);
	write_cr8(cr8);
}

int main(void)
{
	setup_vm();
	setup_idt();
	setup_ring_env();
	set_handle_exception();
	pci_init();

#ifdef IN_NON_SAFETY_VM
	init_mmcfg_sos();
	remapped_mmcfg = ioremap(mmcfg_sos, PAGE_SIZE * TOTAL_PAGES);
#else
	remapped_mmcfg = ioremap(MMCFG_SAFETY, PAGE_SIZE * TOTAL_PAGES );
#endif

	collect_config_spaces();
	find_ivshmem_bar1();
	if (ivshmem_bar1 != 0)
		remapped_ivshmem = ioremap(ivshmem_bar1, PAGE_SIZE);
	else
		remapped_ivshmem = NULL;

	find_msix_table();

	if (pci_devices) {
		instruction_emulation_ACRN_T13805_mov_r8_rm8();
		instruction_emulation_ACRN_T13806_mov_rex_r8_rm8();
		instruction_emulation_ACRN_T13807_mov_r16_rm16();
		instruction_emulation_ACRN_T13810_mov_rm8_r8();
		instruction_emulation_ACRN_T13811_mov_rex_rm8_r8();
		instruction_emulation_ACRN_T13812_mov_rm16_r16();
		instruction_emulation_ACRN_T13813_mov_rm32_r32();
		instruction_emulation_ACRN_T13814_mov_rex_rm64_r64();
		instruction_emulation_ACRN_T13815_mov_offs16_AX();
		instruction_emulation_ACRN_T13818_mov_AX_offs16();
		instruction_emulation_ACRN_T13821_mov_imm8_rm8();
		instruction_emulation_ACRN_T13822_mov_rex_imm8_rm8();
		instruction_emulation_ACRN_T13823_mov_imm16_rm16();
		instruction_emulation_ACRN_T13826_movs_m8_m8();
		instruction_emulation_ACRN_T13827_movsb();
		instruction_emulation_ACRN_T13828_movs_m16_m16();
		instruction_emulation_ACRN_T13831_movsw();
		instruction_emulation_ACRN_T13850_movzx_rm8_r16();
		instruction_emulation_ACRN_T13851_movzx_rm8_r32();
		instruction_emulation_ACRN_T13852_movzx_rex_rm8_r64();
		instruction_emulation_ACRN_T13853_movzx_rm16_r32();
		instruction_emulation_ACRN_T13854_movzx_rex_rm16_r64();
		instruction_emulation_ACRN_T13855_movsx_rm8_r16();
		instruction_emulation_ACRN_T13856_movsx_rm8_r32();
		instruction_emulation_ACRN_T13857_movsx_rex_rm8_r64();
		instruction_emulation_ACRN_T13834_stos_m8();
		instruction_emulation_ACRN_T13835_stosb();
		instruction_emulation_ACRN_T13836_stos_m16();
		instruction_emulation_ACRN_T13839_stosw();
		instruction_emulation_ACRN_T13841_rex_stosq();
		instruction_emulation_ACRN_T13862_sub_rm16_r16();
		instruction_emulation_ACRN_T13790_sub_rex_rm64_r64();
		instruction_emulation_ACRN_T13791_cmp_r16_rm16();
		instruction_emulation_ACRN_T13794_cmp_rm16_r16();
		instruction_emulation_ACRN_T13796_cmp_rex_rm64_r64();
		instruction_emulation_ACRN_T13797_cmp_imm8_rm8();
		instruction_emulation_ACRN_T13798_cmp_rex_imm8_rm8();
		instruction_emulation_ACRN_T13799_cmp_imm16_rm16();
		instruction_emulation_ACRN_T13801_cmp_rex_imm32_rm64();
		instruction_emulation_ACRN_T13802_cmp_imm8_rm16();
		instruction_emulation_ACRN_T13804_cmp_rex_imm8_rm64();
		instruction_emulation_ACRN_T13842_and_rm16_r16();
		instruction_emulation_ACRN_T13845_test_r8_rm8();
		instruction_emulation_ACRN_T13846_test_rex_r8_rm8();
		instruction_emulation_ACRN_T13847_test_r16_rm16();
		instruction_emulation_ACRN_T13858_bt_imm8_rm16();
		instruction_emulation_ACRN_T13860_bt_rex_imm8_rm64();
		instruction_emulation_ACRN_T13894_exception_sub_ud();
		instruction_emulation_ACRN_T13895_exception_cmp_ud();
		instruction_emulation_ACRN_T13899_exception_movsx_ud();
		instruction_emulation_ACRN_T13896_exception_mov_ud();
		instruction_emulation_ACRN_T13902_exception_movs_pf();
		instruction_emulation_ACRN_T13893_exception_test_pf();
		instruction_emulation_ACRN_T13890_exception_sub_ac();
	} else {
		printf("No accessible PCIE devices, please check scenario file\n");
		return -1;
	}

	if (remapped_ivshmem) {
		instruction_emulation_ACRN_T13808_mov_r32_rm32();
		instruction_emulation_ACRN_T13816_mov_offs32_EAX();
		instruction_emulation_ACRN_T13817_mov_rex_offs64_RAX();
		instruction_emulation_ACRN_T13819_mov_EAX_offs32();
		instruction_emulation_ACRN_T13820_mov_rex_RAX_offs64();
		instruction_emulation_ACRN_T13825_mov_rex_imm32_rm64();
		instruction_emulation_ACRN_T13829_movs_m32_m32();
		instruction_emulation_ACRN_T13830_movs_rex_m64_m64();
		instruction_emulation_ACRN_T13833_rex_movsq();
		instruction_emulation_ACRN_T13837_stos_m32();
		instruction_emulation_ACRN_T13838_stos_rex_m64();
		instruction_emulation_ACRN_T13840_stosd();
		instruction_emulation_ACRN_T13793_cmp_rex_r64_rm64();
		instruction_emulation_ACRN_T13844_and_rex_rm64_r64();
		instruction_emulation_ACRN_T13849_test_rex_r64_rm64();
		instruction_emulation_ACRN_T13897_exception_and_8_ud();
		instruction_emulation_ACRN_T13891_exception_and_64_ud();
		instruction_emulation_ACRN_T13898_exception_test_ud();
		instruction_emulation_ACRN_T13900_exception_bt_ud();
		instruction_emulation_ACRN_T13901_exception_movs_gp();
		instruction_emulation_ACRN_T13892_exception_mov_gp();
		instruction_emulation_ACRN_T13889_exception_cmp_ss();
	} else {
		printf("No available ivshmem device, please check scenario file\n");
		return -1;
	}

	if (msix_table) {
		instruction_emulation_ACRN_T13809_mov_rex_r64_rm64();
		instruction_emulation_ACRN_T13824_mov_imm32_rm32();
		instruction_emulation_ACRN_T13832_movsd();
		instruction_emulation_ACRN_T13795_cmp_rm32_r32();
		instruction_emulation_ACRN_T13843_and_rm32_r32();
	} else {
		printf("There is no device has msi-x capability, ");
		printf("please check scenario file\n");
		return -1;
	}

	/* IOAPIC */
	instruction_emulation_ACRN_T13789_sub_rm32_r32();
	instruction_emulation_ACRN_T13792_cmp_r32_rm32();
	instruction_emulation_ACRN_T13800_cmp_imm32_rm32();
	instruction_emulation_ACRN_T13803_cmp_imm8_rm32();
	instruction_emulation_ACRN_T13848_test_r32_rm32();
	instruction_emulation_ACRN_T13859_bt_imm8_rm32();

	return report_summary();
}
