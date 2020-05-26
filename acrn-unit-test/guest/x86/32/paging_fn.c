/**
 * @brief case name:32-Bit Paging Support_001
 *
 * Summary:	In 32-bit paging mode with 4K page,	the	mapping	between	GVA	and	GPA	shall be
 *		correct	by checking	if the value reading directly from GVA and GPA is same.
 */
static void	paging_rqmid_24415_32_bit_paging_support()
{
	volatile u8	*gva = malloc(PAGE_SIZE);
	volatile u8	*gpa;
	pgd_t *pgd = (pgd_t	*)read_cr3();
	u32	pte	= *get_pte(pgd,	(void *)gva);
	ulong cr0 =	read_cr0();
	u32	i;

	assert(gva);
	assert_msg(((pte>>7)&1)	== 0, "pte:	%#x", pte);

	/* setup no	paging mode, then set random value to 4k pages pointed by the gpa */
	write_cr0(cr0 &	~X86_CR0_PG);
	for	(i = 0;	i <	PAGE_SIZE; i++)	{
		gpa	= (volatile	u8 *)(uintptr_t)virt_to_pte_phys(pgd, (void	*)(gva + i));
		*gpa = i & 0xff;
	}

	/* setup 32-bit	paging mode	then compare the value in gva with gpa in whole	4k pages */
	write_cr0(cr0 |	X86_CR0_PG);
	for	(i = 0;	i <	PAGE_SIZE; i++)	{
		u8 got = gva[i];
		if (got	!= (i &	0xff)) {
			printf("%d:	%x != %x\n", i,	got, i & 0xff);
			break;
		}
	}

	report("%s", (i	== PAGE_SIZE), __func__);

	free((void *)gva);
}

static void	paging_rqmid_32355_32_bit_paging_4M_page_support()
{
	// 4M /	4K = 1024 =	2**10
	volatile u8	*gva = alloc_pages(10);
	volatile u8	*gpa = (volatile u8	*)(uintptr_t)virt_to_phys((void	*)gva);
	u32	pte	= *get_pte((pgd_t *)read_cr3(),	(void *)gva);
	ulong cr0 =	read_cr0();
	u32	i =	0;

	assert(gva);
	assert_msg(((pte>>7)&1)	== 1, "pte:	%#x", pte);

	/* setup no	paging mode, then set random value to 4M pages pointed by the gpa */
	write_cr0(cr0 &	~X86_CR0_PG);
	for	(i = 0;	i <	LARGE_PAGE_SIZE; i++) {
		gpa[i] = i & 0xff;
	}

	/* setup 32-bit	paging mode	then compare the value in gva with gpa in whole	4M pages */
	write_cr0(cr0 |	X86_CR0_PG);
	for	(i = 0;	i <	LARGE_PAGE_SIZE; i++) {
		u8 got = gva[i];
		if (got	!= (i &	0xff)) {
			printf("%d:	%x != %x\n", i,	got, i & 0xff);
			break;
		}
	}

	report("%s", (i	== LARGE_PAGE_SIZE), __func__);

	free_pages((void *)gva,	LARGE_PAGE_SIZE);
}

static void
paging_rqmid_28958_PSE_support()
{
	int	passed = 0;
	ulong cr4 =	read_cr4();

	if ((((cpuid(1).d)>>3)&1) == 1)	{
		passed++;
	}

	write_cr4(cr4 |	(1<<4));
	passed++;

	report("%s", passed	== 2, __func__);

	write_cr4(cr4);
}

static inline unsigned
get_pae_pe_offset(uintptr_t	addr, int level)
{
	switch (level) {
	case 3:
		return ((addr >> 30) & ((1<<2) - 1));
		break;
	case 2:
		return ((addr >> 21) & ((1<<9) - 1));
		break;
	case 1:
		return ((addr >> 12) & ((1<<9) - 1));
		break;
	default:
		assert_msg(1, "unknown level: %d", level);
		break;
	}
	return 0;
}

static u64 *install_pae_pte(u64	*cr3,
			  int pte_level,
			  u64 virt,
			  u64 pte,
			  u64 *pt_page)
{
	int	level;
	u64	*pt	= (u64 *)cr3;
	unsigned offset;

	assert_msg(pte_level ==	2 || pte_level == 1, "pte_level	= %d", pte_level);

	for	(level = 3;	level >	pte_level; --level)	{
		offset = get_pae_pe_offset((uintptr_t)virt,	level);
		if (!(pt[offset] & PT_PRESENT_MASK)) {
			u64	*new_pt	= pt_page;
				if (!new_pt)
					new_pt = alloc_page();
				else
					pt_page	= 0;
			memset(new_pt, 0, PAGE_SIZE);
			pt[offset] = virt_to_phys(new_pt) |	PT_PRESENT_MASK;
			if (level != 3)	{
				pt[offset] |=  PT_WRITABLE_MASK	| PT_USER_MASK;
			}
		}
		pt = phys_to_virt(pt[offset] & PT_ADDR_MASK);
	}
	offset = get_pae_pe_offset((uintptr_t)virt,	level);
	pt[offset] = pte;
	return &pt[offset];
}

static u64 *
install_pae_large_page(u64 *cr3, phys_addr_t phys, u64 virt)
{
	return install_pae_pte(cr3,	2, virt,
			   phys	| PT_PRESENT_MASK |	PT_WRITABLE_MASK | PT_USER_MASK	| PT_PAGE_SIZE_MASK, 0);
}

static u64 *
install_pae_page(u64 *cr3, phys_addr_t phys, u64 virt)
{
	return install_pae_pte(cr3,	1, virt, phys |	PT_PRESENT_MASK	| PT_WRITABLE_MASK | PT_USER_MASK, 0);
}

static void
install_pae_pages(u64 *cr3,	phys_addr_t	phys, size_t len, u64 virt)
{
	phys_addr_t	max	= (u64)len + (u64)phys;
	assert(phys	% PAGE_SIZE	== 0);
	assert((uintptr_t) virt	% PAGE_SIZE	== 0);
	assert(len % PAGE_SIZE == 0);

	while (phys	+ PAGE_SIZE	<= max)	{
		install_pae_page(cr3, phys,	virt);
		phys +=	PAGE_SIZE;
		virt = virt	+ PAGE_SIZE;
	}
}

static void
setup_pae_range(u64	*cr3, phys_addr_t start, size_t	len)
{
	u64	max	= (u64)len + (u64)start;
	u64	phys = start;

	while (phys	+ (2ul << 20) <= max) {
		install_pae_large_page(cr3,	phys, phys);
		phys +=	(2ul <<	20);
	}
	install_pae_pages(cr3, phys, max - phys, phys);
}

static ulong old_cr3;
static ulong old_cr4;
static u64 *
enter_pae()
{
	old_cr3	= read_cr3();
	old_cr4	= read_cr4();
	ulong cr0 =	read_cr0();
	u64	*cr3 = alloc_page();

	memset(cr3,	0, PAGE_SIZE);
	/* 0 - 2G memory, 2M page*/
	setup_pae_range(cr3, 0,	1ul<<31);

	write_cr0(cr0 &	~X86_CR0_PG);
	write_cr3(virt_to_phys(cr3));
	write_cr4(old_cr4 |	X86_CR4_PAE);
	write_cr0(cr0 |	X86_CR0_PG);

	return cr3;
}

static void
exit_pae()
{
	ulong cr0 =	read_cr0();

	write_cr0(cr0 &	~X86_CR0_PG);
	write_cr3(old_cr3);
	write_cr4(old_cr4);
	write_cr0(cr0 |	X86_CR0_PG);
}

static void
paging_rqmid_26999_PAE_4K_support()
{
	ulong cr0 =	read_cr0();
	u32	edx	= cpuid(1).d;
	u64	ia32_efer =	rdmsr(PAGING_IA32_EFER);
	int	phy_add_width =	(cpuid(0x80000008).a & 0xff);
	volatile u8	*gva = (volatile u8	*)(3ul << 30);
	u8 *gpa	= (u8 *)alloc_page();
	u64	*cr3;
	int	i;

	assert_msg(((edx >>	6)&1) == 1,	"cpuid(1).edx =	%#x", edx);
	assert_msg(((cr0 >>	31)&1) == 1, "paging not enable, cr0 = %#lx", cr0);
	assert_msg(((ia32_efer >> 8)&1)	== 0, "long	mode enabled, ia32_efer	= %#llx", ia32_efer);
	assert_msg(phy_add_width ==	39,	"physical address width	= %d", phy_add_width);

	cr3	= enter_pae();
	// setup mapping
	install_pae_page(cr3, (phys_addr_t)(uintptr_t)gpa, (u64)(uintptr_t)gva);

	// disable paging
	write_cr0(cr0 & ~X86_CR0_PG);

	// write random	bytes through gpa
	for	(i = 0;	i <	PAGE_SIZE; i++)	{
		gpa[i] = i & 0xff;
	}

	// enable paging again
	write_cr0(cr0 |	X86_CR0_PG);

	// verify written bytes	through	gva
	for	(i = 0;	i <	PAGE_SIZE; i++)	{
		u8 got = gva[i];
		if (got	!= (i &	0xff)) {
			printf("%d:	%x != %x\n", i,	got, i & 0xff);
			break;
		}
		gva[i] = 1;
	}

	exit_pae();
	free_page(gpa);
	report("%s", i == PAGE_SIZE, __func__);
}

static void
paging_rqmid_32327_PAE_2M_support()
{
	ulong cr0 =	read_cr0();
	u32	edx	= cpuid(1).d;
	u64	ia32_efer =	rdmsr(PAGING_IA32_EFER);
	int	phy_add_width =	(cpuid(0x80000008).a & 0xff);
	// 2M /	4K = 512 = 2**9
	u8 *gva	= (u8 *)alloc_pages(9);
	u8 *gpa	= (u8 *)virt_to_phys((void *)gva);
	int	i;

	assert_msg(((edx >>	6)&1) == 1,	"cpuid(1).edx =	%#x", edx);
	assert_msg(((cr0 >>	31)&1) == 1, "paging not enable, cr0 = %#lx", cr0);
	assert_msg(((ia32_efer >> 8)&1)	== 0, "long	mode enabled, ia32_efer	= %#llx", ia32_efer);
	assert_msg(phy_add_width ==	39,	"physical address width	= %d", phy_add_width);

	enter_pae();

	// disable paging
	write_cr0(cr0 & ~X86_CR0_PG);

	// write random	bytes through gpa
	for	(i = 0;	i <	(2ul <<	20); i++) {
		gpa[i] = i & 0xff;
	}

	// enable paging again
	write_cr0(cr0 |	X86_CR0_PG);

	// verify written bytes	through	gva
	for	(i = 0;	i <	(2ul <<	20); i++) {
		u8 got = gva[i];
		if (got	!= (i &	0xff)) {
			printf("%d:	%x != %x\n", i,	got, i & 0xff);
			break;
		}
		gva[i] = 1;
	}

	exit_pae();
	free_pages(gva,	(2ul <<	20));
	report("%s", i == (2ul << 20), __func__);
}

/**
 * @brief case name:Invalidate Paging Structure	Cache When vCPU	changes	CR0.PG
 *
 * Summary:Config 4-level paging structure,	define one GVA1	points to one 1B memory
 * block in	the	4K page. Write a random	value to gva1 and clear	P bit in PDE related
 * with	GVA1. we still can access GVA1 normally	for	paging-structure information is
 * cached in paging-structure cache. After changing	CR0.PG,	we will	get	#PF	because
 * paging-structure	cache is invalidated and get PDE directly from memory.
 *
 * NOTE: when we in	64bit mode,	trying to clear	PG will	trigger	#GP, so	we run this	case in	32bit mode.
 */
static void	paging_rqmid_37209_changes_cr0_pg_001()
{
	ulong cr0 =	read_cr0();
	volatile u8	*gva = (volatile u8	*)malloc(sizeof(u8));
	assert(gva != NULL);

	*gva = WRITE_INITIAL_VALUE;

	set_page_control_bit((void *)gva, PAGE_PDE,	PAGE_P_FLAG, 0,	false);

	u8 got = *gva;
	assert_msg(got == WRITE_INITIAL_VALUE, "got	%#x	!= %#x", got, WRITE_INITIAL_VALUE);

	write_cr0(cr0 ^	X86_CR0_PG);

	asm	volatile(ASM_TRY("1f")
		"mov %0, %%al\n"
		"1:"
		::"m"(*gva)
		);

	write_cr0(cr0);
	report("%s", exception_vector()	== PF_VECTOR, __FUNCTION__);

	/* resume environment */
	set_page_control_bit((void *)gva, PAGE_PDE,	PAGE_P_FLAG, 1,	true);
	free_gva((void *)gva);
}

static void
paging_rqmid_37208_changes_cr4_pae_001()
{
	volatile u8	*gva = (volatile u8	*)(3ul << 30);
	u8 *gpa	= (u8 *)alloc_page();
	u64	*cr3;
	u64	*pt;

	cr3	= enter_pae();
	// setup mapping
	install_pae_page(cr3, (phys_addr_t)(uintptr_t)gpa, (u64)(uintptr_t)gva);
	// reload pdptes
	write_cr3(virt_to_phys(cr3));

	gva[0] = WRITE_INITIAL_VALUE;

	// find	leaf pte
	pt = (u64 *)(uintptr_t)(cr3[get_pae_pe_offset((uintptr_t)gva, 3)] &	PT_ADDR_MASK);
	pt = (u64 *)(uintptr_t)(pt[get_pae_pe_offset((uintptr_t)gva, 2)] & PT_ADDR_MASK);
	pt = &(pt[get_pae_pe_offset((uintptr_t)gva,	1)]);
	// clear P bit
	*pt	= (*pt)	& ~PAGE_P_FLAG;

	u8 got = *gva;
	assert_msg(got == WRITE_INITIAL_VALUE, "got	%#x	!= %#x", got, WRITE_INITIAL_VALUE);

	exit_pae();

	asm	volatile(ASM_TRY("1f")
		"mov %0, %%al\n"
		"1:"
		::"m"(*gva)
		);
	free_page(gpa);
	report("%s", exception_vector()	== PF_VECTOR, __FUNCTION__);
}
