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

void set_page_control_bit(void *gva, page_level level, page_control_bit bit, u32 value, bool is_invalidate)
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

	pteval_t *pdpte;
	pteval_t *pd;
	pteval_t *pt;

	pdpte = (pteval_t *)(pml4[pml4_offset] & PAGE_MASK);
	if ((pdpte[pd_offset] & (1 << PAGE_PS_FLAG)) &&
		((level == PAGE_PDE) || (level == PAGE_PTE))) {
		level = PAGE_PDPTE;
	}
	else {
		pd = (pteval_t *)(pdpte[pdpte_offset] & PAGE_MASK);
		if ((pd[pd_offset] & (1 << PAGE_PS_FLAG)) && level == PAGE_PTE) {
			level = PAGE_PDE;
		}
		else {
			pt = (pteval_t *)(pd[pd_offset] & PAGE_MASK);
		}
	}

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
	pteval_t *pte;

	if ((pde[pde_offset] & (1 << PAGE_PS_FLAG)) && (level == PAGE_PTE)) {
		level = PAGE_PDE;
	}
	else {
		pte = (pteval_t *)(pde[pde_offset] & PAGE_MASK);
	}

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
 * ------------------------------------------------------------
 *
 * send init & startup to reset one ap which apic id is the biggest
 * ple ensure here are more than one cpu in system
 * -----------------------------------------------------------
 */
void send_sipi()
{
	u8 ap_cpus;
	u8 dest;
	u8 cpus_cnt;

	ap_cpus = fwcfg_get_nb_cpus() - 1;
	cpus_cnt = cpu_online_count;

	dest = ap_cpus;
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT, dest);
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_INIT, dest);
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_STARTUP, dest);
	/*waiting ap initilize completely*/
	while ((cpus_cnt + 1) > cpu_online_count) {
		asm volatile("nop");
	}

	/* set cpu_online_conunt to actual cpu core numbers*/
	spin_lock(&lock);
	cpu_online_count -= 1;
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

volatile bool ring1_ret = false;
int do_at_ring1(void (*fn)(const char *), const char *arg)
{
	static unsigned char user_stack[4096];
	int ret;

	asm volatile ("mov %[user_ds], %%" R "dx\n\t"
				  "mov %%dx, %%ds\n\t"
				  "mov %%dx, %%es\n\t"
				  "mov %%dx, %%fs\n\t"
				  "mov %%dx, %%gs\n\t"
				  "mov %%" R "sp, %%" R "cx\n\t"
				  "push" W " %%" R "dx \n\t"
				  "lea %[user_stack_top], %%" R "dx \n\t"
				  "push" W " %%" R "dx \n\t"
				  "pushf" W "\n\t"
				  "push" W " %[user_cs] \n\t"
				  "push" W " $1f \n\t"
				  "iret" W "\n"
				  "1: \n\t"
				  "push %%" R "cx\n\t"   /* save kernel SP */

#ifndef __x86_64__
				  "push %[arg]\n\t"
#endif
				  "call *%[fn]\n\t"
#ifndef __x86_64__
				  "pop %%ecx\n\t"
#endif

				  "pop %%" R "cx\n\t"
				  "mov $1f, %%" R "dx\n\t"
				  "int %[kernel_entry_vector]\n\t"
				  ".section .text.entry \n\t"
				  ".globl kernel_entry1 \n\t"
				  "kernel_entry1: \n\t"
				  "mov %%" R "cx, %%" R "sp \n\t"
				  "mov %[kernel_ds], %%cx\n\t"
				  "mov %%cx, %%ds\n\t"
				  "mov %%cx, %%es\n\t"
				  "mov %%cx, %%fs\n\t"
				  "mov %%cx, %%gs\n\t"
				  "jmp *%%" R "dx \n\t"
				  ".section .text\n\t"
				  "1:\n\t"
				  : [ret] "=&a" (ret)
				  : [user_ds] "i" (OSSERVISE1_DS),
				  [user_cs] "i" (OSSERVISE1_CS32),
				  [user_stack_top]"m"(user_stack[sizeof user_stack]),
				  [fn]"r"(fn),
				  [arg]"D"(arg),
				  [kernel_ds]"i"(KERNEL_DS),
				  [kernel_entry_vector]"i"(0x21)
				  : "rcx", "rdx");
	return ret;
}

volatile bool ring2_ret = false;
int do_at_ring2(void (*fn)(const char *), const char *arg)
{
	static unsigned char user_stack[4096];
	int ret;

	asm volatile ("mov %[user_ds], %%" R "dx\n\t"
				  "mov %%dx, %%ds\n\t"
				  "mov %%dx, %%es\n\t"
				  "mov %%dx, %%fs\n\t"
				  "mov %%dx, %%gs\n\t"
				  "mov %%" R "sp, %%" R "cx\n\t"
				  "push" W " %%" R "dx \n\t"
				  "lea %[user_stack_top], %%" R "dx \n\t"
				  "push" W " %%" R "dx \n\t"
				  "pushf" W "\n\t"
				  "push" W " %[user_cs] \n\t"
				  "push" W " $1f \n\t"
				  "iret" W "\n"
				  "1: \n\t"
				  "push %%" R "cx\n\t"   /* save kernel SP */

#ifndef __x86_64__
				  "push %[arg]\n\t"
#endif
				  "call *%[fn]\n\t"
#ifndef __x86_64__
				  "pop %%ecx\n\t"
#endif

				  "pop %%" R "cx\n\t"
				  "mov $1f, %%" R "dx\n\t"
				  "int %[kernel_entry_vector]\n\t"
				  ".section .text.entry \n\t"
				  ".globl kernel_entry2	\n\t"
				  "kernel_entry2: \n\t"
				  "mov %%" R "cx, %%" R "sp \n\t"
				  "mov %[kernel_ds], %%cx\n\t"
				  "mov %%cx, %%ds\n\t"
				  "mov %%cx, %%es\n\t"
				  "mov %%cx, %%fs\n\t"
				  "mov %%cx, %%gs\n\t"
				  "jmp *%%" R "dx \n\t"
				  ".section .text\n\t"
				  "1:\n\t"
				  : [ret] "=&a" (ret)
				  : [user_ds] "i" (OSSERVISE2_DS),
				  [user_cs] "i" (OSSERVISE2_CS32),
				  [user_stack_top]"m"(user_stack[sizeof user_stack]),
				  [fn]"r"(fn),
				  [arg]"D"(arg),
				  [kernel_ds]"i"(KERNEL_DS),
				  [kernel_entry_vector]"i"(0x22)
				  : "rcx", "rdx");
	return ret;
}

volatile bool ring3_ret = false;
int do_at_ring3(void (*fn)(const char *), const char *arg)
{
	static unsigned char user_stack[4096];
	int ret;

	asm volatile ("mov %[user_ds], %%" R "dx\n\t"
				  "mov %%dx, %%ds\n\t"
				  "mov %%dx, %%es\n\t"
				  "mov %%dx, %%fs\n\t"
				  "mov %%dx, %%gs\n\t"
				  "mov %%" R "sp, %%" R "cx\n\t"
				  "push" W " %%" R "dx \n\t"
				  "lea %[user_stack_top], %%" R "dx \n\t"
				  "push" W " %%" R "dx \n\t"
				  "pushf" W "\n\t"
				  "push" W " %[user_cs] \n\t"
				  "push" W " $1f \n\t"
				  "iret" W "\n"
				  "1: \n\t"
				  /* save kernel SP */
				  "push %%" R "cx\n\t"

#ifndef __x86_64__
				  "push %[arg]\n\t"
#endif
				  "call *%[fn]\n\t"
#ifndef __x86_64__
				  "pop %%ecx\n\t"
#endif

				  "pop %%" R "cx\n\t"
				  "mov $1f, %%" R "dx\n\t"
				  "int %[kernel_entry_vector]\n\t"
				  ".section .text.entry \n\t"
				  ".globl kernel_entry\n\t"
				  "kernel_entry: \n\t"
				  "mov %%" R "cx, %%" R "sp \n\t"
				  "mov %[kernel_ds], %%cx\n\t"
				  "mov %%cx, %%ds\n\t"
				  "mov %%cx, %%es\n\t"
				  "mov %%cx, %%fs\n\t"
				  "mov %%cx, %%gs\n\t"
				  "jmp *%%" R "dx \n\t"
				  ".section .text\n\t"
				  "1:\n\t"
				  : [ret] "=&a" (ret)
				  : [user_ds] "i" (USER_DS),
				  [user_cs] "i" (USER_CS),
				  [user_stack_top]"m"(user_stack[sizeof user_stack]),
				  [fn]"r"(fn),
				  [arg]"D"(arg),
				  [kernel_ds]"i"(KERNEL_DS),
				  [kernel_entry_vector]"i"(0x23)
				  : "rcx", "rdx");
	return ret;
}
/*
 *	@brief setup_ring
 *	Summary:setup ring environment:gdt entry and idt
 *		which will be used for CPU  switching to ring1, ring2,ring3
 */
void setup_ring_env()
{
	/*for setup ring1 ring2 ring3 environment*/
	extern unsigned char kernel_entry1;
	set_idt_entry(0x21, &kernel_entry1, 1);
	extern unsigned char kernel_entry2;
	set_idt_entry(0x22, &kernel_entry2, 2);
	extern unsigned char kernel_entry;
	set_idt_entry(0x23, &kernel_entry, 3);

#ifdef __x86_64__
	extern gdt_entry_t gdt64[];
	*(u64 *)&gdt64[11] = RING1_CS64_GDT_DESC;
	*(u64 *)&gdt64[12] = RING1_DS_GDT_DESC;
	*(u64 *)&gdt64[13] = RING2_CS64_GDT_DESC;
	*(u64 *)&gdt64[14] = RING2_DS_GDT_DESC;
#elif __i386__
	extern gdt_entry_t gdt32[];
	*(u64 *)&gdt32[11] = RING1_CS32_GDT_DESC;
	*(u64 *)&gdt32[12] = RING1_DS_GDT_DESC;
	*(u64 *)&gdt32[13] = RING2_CS32_GDT_DESC;
	*(u64 *)&gdt32[14] = RING2_DS_GDT_DESC;
#endif

}

