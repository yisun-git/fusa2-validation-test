#include "memory_type.h"
#include "debug_print.h"

void asm_mfence()
{
	asm volatile("mfence" ::: "memory");
}

void asm_read_access_memory(u64 *p)
{
#ifdef __x86_64__
	asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax");
#elif __i386__
	asm volatile("mov (%0), %%eax\n" : : "c"(p) : "eax");
#endif
}

void asm_clwb(void *target)
{
	/* clwb (%rbx): */
	asm volatile("clwb (%0)" : : "b"(&target));
}

void asm_wbinvd()
{
	asm volatile ("wbinvd\n" : : : "memory");
}

void asm_wbinvd_lock(void *data)
{
	asm volatile(".byte 0xF0\n\t" "wbinvd\n" : : : "memory");
}

void asm_invd()
{
	asm volatile ("invd\n" : : : "memory");
}

void asm_invd_lock(void *data)
{
	asm volatile(".byte 0xF0\n\t" "invd\n" : : : "memory");
}

void asm_write_access_memory(unsigned long address, unsigned long value)
{
	asm volatile("mov %[value], (%[address])"
		     :
		     : [value]"r"(value), [address]"r"(address)
		     : "memory");
}

void asm_clflush(long unsigned int addr)
{
	asm volatile("clflush (%0)" : : "b" (addr));
}

void asm_clflush_lock(long unsigned int addr)
{
	asm volatile(".byte 0xF0\n\t" "clflush (%0)" : : "b" (addr));
}

void asm_clflushopt_lock(long unsigned int addr)
{
	asm volatile(".byte 0xF0\n\t" "clflushopt (%0)" : : "b" (addr));
}

void asm_clflushopt_lock_f2(long unsigned int addr)
{
	asm volatile(".byte 0xF2\n\t" "clflushopt (%0)" : : "b" (addr));
}

void asm_clflushopt_lock_f3(long unsigned int addr)
{
	asm volatile(".byte 0xF3\n\t" "clflushopt (%0)" : : "b" (addr));
}

unsigned long long asm_read_tsc(void)
{
	long long r;
#ifdef __x86_64__
	unsigned a, d;

	asm volatile("mfence" ::: "memory");
	asm volatile ("rdtsc" : "=a"(a), "=d"(d));
	r = a | ((long long)d << 32);
#else
	asm volatile ("rdtsc" : "=A"(r));
#endif
	asm volatile("mfence" ::: "memory");
	return r;
}

void asm_mfence_wbinvd()
{
	asm_mfence();
	asm_wbinvd();
	asm_mfence();
}

void asm_mfence_invd()
{
	asm_mfence();
	asm_invd();
	asm_mfence();
}

void write_cr0_bybit(u32 bit, u32 bitvalue)
{
	u32 cr0 = read_cr0();
	if (bitvalue) {
		write_cr0(cr0 | (1 << bit));
	} else {
		write_cr0(cr0 & ~(1 << bit));
	}
}

/*Modify the PTE/PCD/PWT bit in paging table entry*/
void set_memory_type_pt(void *address, u64 type, u64 size)
{
	unsigned long *ptep;
	u64 *next_addr;
	int i;
	int j = 0;
	int pat = PT_PAT;

	for (i = 0; i < size; i += PAGE_SIZE) {
		j++;
		next_addr = (u64 *)((u8 *)address + i);

		ptep = get_pte_level(current_page_table(), next_addr, 1);
		if (ptep == NULL) {
			pat = PT_PAT_LARGE_PAGE;
		}
		switch (type) {
		case PT_MEMORY_TYPE_MASK0:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 0, 0);
			break;
		case PT_MEMORY_TYPE_MASK1:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 0, 0);
			break;
		case PT_MEMORY_TYPE_MASK2:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 0, 0);
			break;
		case PT_MEMORY_TYPE_MASK3:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 0, 0);
			break;
		case PT_MEMORY_TYPE_MASK4:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 1, 0);
			break;
		case PT_MEMORY_TYPE_MASK5:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 1, 0);
			break;
		case PT_MEMORY_TYPE_MASK6:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 0, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 1, 0);
			break;
		case PT_MEMORY_TYPE_MASK7:
			set_page_control_bit(next_addr, PAGE_PTE, PT_PWT, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, PT_PCD, 1, 0);
			set_page_control_bit(next_addr, PAGE_PTE, pat, 1, 0);
			break;
		default:
			printf("error type\n");
			break;
		}
	}
}

void flush_tlb()
{
	u32 cr3;
	cr3 = read_cr3();
	write_cr3(cr3);
}

void mem_cache_reflush_cache()
{
	u32 cr4;

	/*Disable interrupts;*/
	irq_disable();

	/*Save current value of CR4;*/
	cr4 = read_cr4();

	/*Disable and flush caches;*/
	write_cr0_bybit(CR0_BIT_CD, 1);
	write_cr0_bybit(CR0_BIT_NW, 0);
	asm_wbinvd();

	/*Flush TLBs;*/
	flush_tlb();

	/*Disable MTRRs;*/
	//disable_MTRR();

	/*Flush caches and TLBs;*/
	asm_wbinvd();
	flush_tlb();

	/*Enable MTRRs;*/
	//enable_MTRR();

	/*enable caches;*/
	write_cr0_bybit(CR0_BIT_CD, 0);
	write_cr0_bybit(CR0_BIT_NW, 0);

	/*Restore value of CR4;*/
	write_cr4(cr4);

	/*Enable interrupts;*/
	irq_enable();
}

void set_mem_cache_type_addr(void *address, u64 cache_type, u64 size)
{
	set_memory_type_pt(address, cache_type, size);
	debug_print("cache_test_array=%p\n", address);

	/* Flush caches and TLBs;*/
	asm_wbinvd();
	flush_tlb();
}

void set_mem_cache_type_all(u64 cache_type)
{
	u64 ia32_pat_test;

	wrmsr(IA32_PAT_MSR, cache_type);

	ia32_pat_test = rdmsr(IA32_PAT_MSR);

#ifdef __x86_64__
	debug_print("ia32_pat_test 0x%lx \n", ia32_pat_test);
	if (ia32_pat_test != cache_type) {
		debug_print("set pat type all error set=0x%lx, get=0x%lx\n", cache_type, ia32_pat_test);
	} else {
		debug_print("set pat type all sucess type=0x%lx\n", cache_type);
	}
#elif __i386__
	debug_print("ia32_pat_test 0x%llx \n", ia32_pat_test);
	if (ia32_pat_test != cache_type) {
		debug_print("set pat type all error set=0x%llx, get=0x%llx\n", cache_type, ia32_pat_test);
	} else {
		debug_print("set pat type all sucess type=0x%llx\n", cache_type);
	}
#endif
	asm_mfence_wbinvd();

	mem_cache_reflush_cache();

	debug_print("CR0: 0x%lx, CR3: 0x%lx, CR4: 0x%lx\n", read_cr0(), read_cr3(), read_cr4());
}

__attribute__((aligned(64))) u64 read_mem_cache_test_addr(u64 *address, u64 size)
{
	u64 index;
	u64 t[2] = {0};

	cli();
	t[0] = asm_read_tsc();
	for (index = 0; index < size; index++) {
		asm_read_access_memory(&address[index]);
	}
	t[1] = asm_read_tsc();
	sti();
#ifdef __x86_64__
	debug_print("%ld\n", (t[1] - t[0]));
#elif __i386__
	debug_print("%lld\n", (t[1] - t[0]));
#endif
	asm_mfence();
	return t[1] - t[0];
}

__attribute__((aligned(64))) u64 write_mem_cache_test_addr(u64 *address, u64 size)
{
	u64 index;
	u64 t[2] = {0};
	u64 t_total = 0;

	t[0] = asm_read_tsc();
	for (index = 0; index < size; index++) {
		asm_write_access_memory((unsigned long)&address[index], index);
	}
	t[1] = asm_read_tsc();
	t_total += t[1] - t[0];

#ifdef __x86_64__
	debug_print("%ld\n", t_total);
#elif __i386__
	debug_print("%lld\n", t_total);
#endif
	asm_mfence();
	return t_total;
}

