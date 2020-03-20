#include "vm.h"
#include "libcflat.h"
#include "processor.h"
#include "misc.h"
#include "desc.h"
#include "apic.h"
#include "asm/spinlock.h"
#include "fwcfg.h"

extern short cpu_online_count;
static struct spinlock lock;

/**
 * @brief set paging-structure control bit
 *
 * change paging-structure control bit only for 4-level paging mode and 32-bit paging mode
 *
 * @param param_1: the address guest virtual address
 * @param param_2: paging level
 * @param param_3: paging control bit
 * @param param_4: the set value
 * @param param_5: whether invalidate TLB and paging-structure caches
 *
 */

void set_page_control_bit(void *gva,
	page_level level, page_control_bit bit, u32 value, bool is_invalidate)
{
	if (gva == NULL) {
		printf("this address is NULL!\n");
		return;
	}

	ulong cr3 = read_cr3();
#ifdef __x86_64__
	u32 pdpte_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_PDPTE);
	u32 pml4_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_PML4);
	u32 pd_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_PDE);
	u32 pt_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_PTE);
	pteval_t *pml4 = (pteval_t *)cr3;

	pteval_t *pdpte = (pteval_t *)(pml4[pml4_offset] & PAGE_MASK);
	pteval_t *pd = (pteval_t *)(pdpte[pdpte_offset] & PAGE_MASK);
	pteval_t *pt = (pteval_t *)(pd[pd_offset] & PAGE_MASK);

	switch (level) {
	case PAGE_PML4:
		if (value == 1) {
			pml4[pml4_offset] |= (1ull << bit);
		} else {
			pml4[pml4_offset] &= ~(1ull << bit);
		}
		break;
	case PAGE_PDPTE:
		if (value == 1) {
			pdpte[pdpte_offset] |= (1ull << bit);
		} else {
			pdpte[pdpte_offset] &= ~(1ull << bit);
		}
		break;
	case PAGE_PDE:
		if (value == 1) {
			pd[pd_offset] |= (1ull << bit);
		} else {
			pd[pd_offset] &= ~(1ull << bit);
		}
		break;
	default:
		if (value == 1) {
			pt[pt_offset] |= (1ull << bit);
		} else {
			pt[pt_offset] &= ~(1ull << bit);
		}
		break;
	}

#else
	u32 pde_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_PDE);
	u32 pte_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_PTE);
	pteval_t *pde = (pgd_t *)cr3;

	pteval_t *pte = (pteval_t *)(pde[pde_offset] & PAGE_MASK);

	if (level == PAGE_PDE) {
		if (value == 1) {
			pde[pde_offset] |= (1u << bit);
		} else {
			pde[pde_offset] &= ~(1u << bit);
		}
	} else {
		if (value == 1) {
			pte[pte_offset] |= (1u << bit);
		} else {
			pte[pte_offset] &= ~(1u << bit);
		}
	}
#endif
	if (is_invalidate) {
		invlpg(gva);
	}

}

int write_cr4_exception_checking(unsigned long val)
{
	asm volatile(ASM_TRY("1f")
		"mov %0,%%cr4\n\t"
		"1:" : : "r" (val));
	return exception_vector();
}

int rdmsr_checking(u32 MSR_ADDR, u64 *result)
{
	u32 eax;
	u32 edx;

	asm volatile(ASM_TRY("1f")
		"rdmsr \n\t"
		"1:"
		: "=a"(eax), "=d"(edx) : "c"(MSR_ADDR));
	*result = eax + ((u64)edx << 32);
	return exception_vector();
}

int wrmsr_checking(u32 MSR_ADDR, u64 value)
{
	u32 edx = value >> 32;
	u32 eax = value;

	asm volatile(ASM_TRY("1f")
		"wrmsr \n\t"
		"1:"
		: : "c"(MSR_ADDR), "a"(eax), "d"(edx));
	return exception_vector();
}
/**
 * -----------------------------------------------
 *
 * send init & startup to all APs
 *
 * -----------------------------------------------
 */
void send_sipi()
{
	u8 ap_cpus;
	u8 dest;
	u8 cpus_cnt;

	ap_cpus = fwcfg_get_nb_cpus() - 1;
	cpus_cnt = cpu_online_count;

	dest = ap_cpus;
	while (dest != 0) {
		/*issue sipi to awake AP */
		apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, dest);
		apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT, dest);
		apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_STARTUP, dest);
		dest--;
	}
	/*waiting all aps initilize completely*/
	while ((cpus_cnt + ap_cpus) > cpu_online_count) {
		asm volatile("nop");
	}

	/* set cpu_online_conunt to actual cpu core numbers*/
	spin_lock(&lock);
	cpu_online_count -= ap_cpus;
	spin_unlock(&lock);
}
/**
 * -------------------------------------------------------
 *
 * get_case_id : get case_id string from acrn-shell
 * case_id: NULL string---->test all cases
 *		  special string---->test single one
 *
 * -------------------------------------------------------
 */
void get_case_id(int ac, char **av, char *case_id)
{
	assert(case_id);
	assert(av);

	/*just handle ac=1 ,others return NULL string*/
	if (ac == 1) {
		if (strlen(av[0]) > (MAX_CASE_ID_LEN - 1)) {
			*case_id = '\0';
			printf("Error:input case-id is too much longer than %d\n\r", MAX_CASE_ID_LEN);
		}
		strcpy(case_id, av[0]);
	} else {
		*case_id = '\0';
	}
}

/*get cpu id*/
uint32_t get_lapic_id(void)
{
	uint32_t ebx;
	asm volatile("cpuid" : "=b"(ebx)
		: "a" (1)
		: "memory");
	return (ebx >> 24);
}

