#include "vm.h"
#include "libcflat.h"
#include "processor.h"
#include "misc.h"
#include "desc.h"
#include "apic.h"
#include "asm/spinlock.h"
#include "fwcfg.h"
#include "regdump.h"
#include "xsave.h"
#include "alloc.h"
#include "../../x86/instruction_common.h"
#include "register_op.h"
extern short cpu_online_count;
static struct spinlock lock;

u8 lapicid_map[UT_CORE_NUMS] = {0};

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
	} else {
		pd = (pteval_t *)(pdpte[pdpte_offset] & PAGE_MASK);
		if ((pd[pd_offset] & (1 << PAGE_PS_FLAG)) && level == PAGE_PTE) {
			level = PAGE_PDE;
		} else {
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
	} else {
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

	dest = lapicid_map[ap_cpus];
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

u8 check_cpu_online_status()
{
	/* wait for 5 pause() */
	u8 cnt = 5;
	while (cnt > 0) {
		pause();
		if (cpu_online_count >= fwcfg_get_nb_cpus()) {
			return 1;
		}
		cnt--;
	}
	return 0;
}

/* ap record its lapic id. should only be called from ap. */
void ap_record_lapicid()
{
	spin_lock(&lock);
	static u8 recorded = 1;
	if (recorded < sizeof(lapicid_map)) {
		lapicid_map[recorded++] = get_lapic_id();
	}
	spin_unlock(&lock);
}

int get_cpu_id()
{
	uint32_t lapic_id = get_lapic_id();
	for (int i = 0; i < sizeof(lapicid_map); i++) {
		if (lapicid_map[i] == lapic_id)
			return i;
	}
	return -1;
}

volatile bool ring1_ret = false;
/**
 * @brief execute void (*fn)(const char *) function at ring1; pass "const char *arg" to (*fn)
 *
 * @pre setup ring1 environment by call setup_ring_env()
 *
 */
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
/**
 * @brief execute void (*fn)(const char *) function at ring2; pass "const char *arg" to (*fn)
 *
 * @pre setup ring2 environment by call setup_ring_env()
 *
 */
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
/**
 * @brief execute void (*fn)(const char *) function at ring3; pass "const char *arg" to (*fn)
 *
 * @pre setup ring3 environment by call setup_ring_env()
 *
 */
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
/*-------------------------------------------------------*
 *	@ dump all supported msr value to memory;
 *	return the memory address and size
 *	pls remember free the memory
 *
 */
void *msr_reg_dump(u32 *size)
{
	u64 *msr_value;

	assert(size);

	*size = sizeof(u64) * SUPPORTED_MSR_NUM;
	msr_value = malloc(*size);
	assert(msr_value);
	memset(msr_value, 0, *size);

	for (u32 i = 0; i < SUPPORTED_MSR_NUM; i++) {
		rdmsr_checking((supported_msr_list[i]), (msr_value + i));
	}

	return msr_value;
}
int xsave_setbv(u32 index, u64 value)
{
	u32 eax = value;
	u32 edx = value >> 32;

	asm volatile("xsetbv\n\t" /* xsetbv */
				 : : "a" (eax), "d" (edx), "c" (index));
	return 0;
}
int xsave_getbv(u32 index, u64 *result)
{
	u32 eax, edx;

	asm volatile("xgetbv\n" /* xgetbv */
				 : "=a" (eax), "=d" (edx)
				 : "c" (index));
	*result = eax + ((u64)edx << 32);
	return 0;
}
int xsave_instruction(u64 *xsave_addr, u64 xcr0)
{
	u32 eax = xcr0;
	u32 edx = xcr0 >> 32;
	asm volatile(
		"xsave %[addr]\n"
		: : [addr]"m"(*xsave_addr), "a"(eax), "d"(edx)
		: "memory");
	return 0;
}

u64 get_xcr0(void)
{
	struct cpuid r;
	r = cpuid_indexed(0xd, 0);
	return r.a + ((u64)r.d << 32);
}
u64 enable_xsave()
{
	u64 xcr0;
	u64 supported_xcr0;

	/*enable xsave feature set by set cr4.18*/
	write_cr4(read_cr4() | (1 << 18)); /* osxsave */
	supported_xcr0 = get_xcr0();
	/*enable all xsave bitmap 0x3--we support until now!!
	 */
	xsave_getbv(0, &xcr0);
	xsave_setbv(0, supported_xcr0);

	return xcr0;
}
/*------------------------------------------------------*
 *   dump xsave reg to ptr
 *   TURE:sucess
 *   FALSE:failed
 */
bool xsave_reg_dump(void *ptr)
{
	void *mem;
	uintptr_t p_align;
	void *fpu_sse, *ymm_ptr, *bnd_ptr;
	xsave_dump_t *xsave_reg;
	xsave_area_t *xsave;
	size_t alignment;

	assert(ptr);
	memset(ptr, 0x0, sizeof(xsave_dump_t));


	/*allocate 2K memory to save xsave feature*/
	mem = malloc(1 << 11);
	assert(mem);
	memset(mem, 0, (1 << 11));
	/*mem base address must be 64 bytes aligned to excute "xsave". vol1 13.4 in SDM*/
	alignment = 64;
	p_align = (uintptr_t) mem;
	p_align = ALIGN(p_align, alignment);
	if (xsave_instruction((void *)p_align, STATE_X87 | STATE_SSE | STATE_AVX) != 0) {
		free(mem);
		return false;
	}
	/*copy to dump buffer*/
	xsave_reg = (xsave_dump_t *)ptr;
	xsave = (xsave_area_t *)p_align;
	fpu_sse = (void *)xsave;
	memcpy((void *) &(xsave_reg->fpu_sse), fpu_sse, sizeof(fpu_sse_t));
	ymm_ptr = (void *)&xsave->ymm[0];
	memcpy((void *) &(xsave_reg->ymm), ymm_ptr, sizeof(xsave_avx_t));
	bnd_ptr = (void *)&xsave->bndregs;
	memcpy((void *) &(xsave_reg->bndregs), bnd_ptr, \
		   sizeof(xsave_bndreg_t) + sizeof(xsave_bndcsr_t));
	free(mem);
	return true;
}

#ifdef __x86_64__
u64 *creat_non_canon_add(u64 addr)
{
	return (u64 *)(addr ^ (1ULL << 63));
}
#endif

