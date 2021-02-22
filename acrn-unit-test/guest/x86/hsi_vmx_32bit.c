
#define ADDR_GUEST_CR0 (0x8000UL)
#define ADDR_GUEST_EFER (0x8008UL)
tss32_t tss32;
static uint64_t init_vgdt[] = {
	0x0UL, 0x00cf9b000000ffffUL, 0x00cf93000000ffffUL,
	0x0000890000000000UL, 0x000089000000ffffUL,
};
static uint64_t boot_idt32[512] ALIGNED(16);
static uint8_t ring0stacktop[PAGE_SIZE] ALIGNED(PAGE_SIZE);

extern void *guest_entry_32;

/* guest_entry_32 */
/* save cr0, ia32_efer register for make sure guest
 * run in unpaged protected mode
 */
asm(
	".align	4, 0x90\n\t"
	".globl	guest_entry_32\n\t"
	".code32\n\t"
	"guest_entry_32:\n\t"
	"mov %cr0, %edx\n\t"
	"mov %edx, "xstr(ADDR_GUEST_CR0)"\n\t"
	"efer = 0xc0000080\n\t"
	"mov $efer, %ecx\n\t"
	"rdmsr\n\t"
	"mov %eax, "xstr(ADDR_GUEST_EFER)"\n\t"
	"vmcall\n\t"
);

asm(".code64\n\t");
static void setup_32bit_guest_vmcs(void)
{
	u64 guest_cr0 = 0x30;
	guest_cr0 |= X86_CR0_PE;
	vmcs_write(GUEST_CR0, guest_cr0);
	vmcs_write(GUEST_CR4, 0x2000);
	vmcs_write(GUEST_CR3, 0);
	vmcs_write(GUEST_EFER, 0);

	/* 26.3.1.2 */
	vmcs_write(GUEST_SEL_CS, KERNEL_CS);
	vmcs_write(GUEST_SEL_SS, KERNEL_DS);
	vmcs_write(GUEST_SEL_DS, KERNEL_DS);
	vmcs_write(GUEST_SEL_ES, KERNEL_DS);
	vmcs_write(GUEST_SEL_FS, KERNEL_DS);
	vmcs_write(GUEST_SEL_GS, KERNEL_DS);
	vmcs_write(GUEST_SEL_TR, 0x20);
	vmcs_write(GUEST_SEL_LDTR, 0);

	vmcs_write(GUEST_BASE_CS, 0);
	vmcs_write(GUEST_BASE_ES, 0);
	vmcs_write(GUEST_BASE_SS, 0);
	vmcs_write(GUEST_BASE_DS, 0);
	vmcs_write(GUEST_BASE_FS, 0);
	vmcs_write(GUEST_BASE_GS, 0);
	vmcs_write(GUEST_BASE_TR, (uint64_t)&tss32);
	vmcs_write(GUEST_BASE_LDTR, 0);

	vmcs_write(GUEST_LIMIT_CS, 0xFFFFFFFF);
	vmcs_write(GUEST_LIMIT_DS, 0xFFFFFFFF);
	vmcs_write(GUEST_LIMIT_ES, 0xFFFFFFFF);
	vmcs_write(GUEST_LIMIT_SS, 0xFFFFFFFF);
	vmcs_write(GUEST_LIMIT_FS, 0xFFFFFFFF);
	vmcs_write(GUEST_LIMIT_GS, 0xFFFFFFFF);
	vmcs_write(GUEST_LIMIT_LDTR, 0xffff);
	vmcs_write(GUEST_LIMIT_TR, 0xffff);

	vmcs_write(GUEST_AR_CS, 0xc09b);
	vmcs_write(GUEST_AR_DS, 0xc093);
	vmcs_write(GUEST_AR_ES, 0xc093);
	vmcs_write(GUEST_AR_FS, 0xc093);
	vmcs_write(GUEST_AR_GS, 0xc093);
	vmcs_write(GUEST_AR_SS, 0xc093);
	vmcs_write(GUEST_AR_LDTR, 0x82);
	vmcs_write(GUEST_AR_TR, 0x8b);

	/* 26.3.1.3 */
	vmcs_write(GUEST_BASE_GDTR, (uint64_t)init_vgdt);
	vmcs_write(GUEST_BASE_IDTR, (uint64_t)boot_idt32);
	vmcs_write(GUEST_LIMIT_GDTR, sizeof(init_vgdt) - 1U);
	vmcs_write(GUEST_LIMIT_IDTR, ((16 * 256) - 1));

	/* 26.3.1.4 */
	vmcs_write(GUEST_RIP, (u64)(&guest_entry_32));
	vmcs_write(GUEST_RSP, (u64)(guest_stack + PAGE_SIZE - 1));
	vmcs_write(GUEST_RFLAGS, 0x2);

	exec_vmwrite32(VMX_ENTRY_CONTROLS, 0xd1ff);
}


/**
 * @brief Case name: HSI_Virtualization_Specific_Features_Unrestricted_Guests_001
 *
 * Summary: IN root operation, execute VMWRITE instruction to set
 * the Unrestricted guests bit in the VM-execution controls.
 * execute VMWRITE set guest cr0 to 0x31 which means set CR0.PE to 1, clear
 * cr0.PG to 0 with guest cr0 vmcs field.
 * execute VMWRITE set guest msr ia32 efer to 0 which means IA32_EFER.LME clear to 0.
 * switch to non-root operation,
 * execute MOV save guest cr0 to memory,
 * execute RDMSR get ia32_efer rigseter in eax, then save eax value to memory.
 * switch to root operation again.
 * make sure guest run in unpaged protected mode,
 * Check guest cr0.pg is 0, cr0.pe is 1 and ia32_efer.lme is 0.
 */
__unused static void hsi_rqmid_42243_virtualization_specific_features_unrestricted_guest_001()
{
	u32 guest_cr0 = *((u32 *)ADDR_GUEST_CR0);
	u32 guest_efer = *((u32 *)ADDR_GUEST_EFER);
	u8 chk = 0;

	DBG_INFO("guest_cr0:%x guest_efer:0x%x", guest_cr0, guest_efer);
	/* make sure paging disabled */
	if ((guest_cr0 & X86_CR0_PG) == 0) {
		chk++;
	}

	/* make sure guest in protect mode */
	/* According SDM Figure 2-3 */
	if (((guest_cr0 & X86_CR0_PE) == X86_CR0_PE) &&
		((guest_efer & IA32_EFER_LME) == 0)) {
		chk++;
	}

	report("%s", (chk == 2), __func__);
}

/* make test condition for unrestricted guests test */
static void unrestricted_guest_condition(struct vmcs *vmcs)
{
	u32 *pguest_cr0 = ((u32 *)ADDR_GUEST_CR0);
	u32 *pguest_efer = ((u32 *)ADDR_GUEST_EFER);

	*pguest_cr0 = 0;
	*pguest_efer = 0;

	/* init primary execute control field and ept table */
	vm_exec_init(vmcs);
	/*
	 * Set unrestricted guests for test condition on ap1.
	 */
	exec_vmwrite32_bit(CPU_EXEC_CTRL1, \
		CPU_URG, \
		BIT_TYPE_SET);

	vmx_set_test_condition(CON_42243_REPORT_CASE);

	DBG_INFO("CPU_EXEC_CTRL1:0x%x", exec_vmread32(CPU_EXEC_CTRL1));

	DBG_INFO("guest_cr0:%x guest_efer:0x%x", *pguest_cr0, *pguest_efer);

}
static int vmx_exec_urg_init(struct vmcs *vmcs)
{
	uint64_t val;
	uint64_t tss_descr32;

	unrestricted_guest_condition(vmcs);

	/* setup vCPU tss32 */
	tss32.esp0 = (uint64_t)ring0stacktop;
	tss32.esp0 += PAGE_SIZE;
	tss32.ss0 = 0x10;

	/* setup vCPU gdt for tss32 */
	val = init_vgdt[4];
	tss_descr32 = (uint64_t)&tss32;
	val |= (tss_descr32 & 0xffff)<<16;
	val |= (tss_descr32 & 0xff0000)<<16;
	val |= (tss_descr32 & 0xff000000)<<32;
	init_vgdt[4] = val;

	/* setup vCPU IDT table */
	for (u32 i = 0; i < 512; i++) {
		boot_idt32[i] = 0x850000180000;
	}

	setup_32bit_guest_vmcs();
	return VMX_TEST_START;
}

static int vm_unrestricted_guest_exit_handler(void)
{
	u64 guest_rip;
	ulong reason;
	u32 insn_len;
	u32 exit_qual;
	u32 condition = 0;

	guest_rip = vmcs_read(GUEST_RIP);
	reason = vmcs_read(EXI_REASON) & 0xff;
	insn_len = vmcs_read(EXI_INST_LEN);
	exit_qual = vmcs_read(EXI_QUALIFICATION);
	/* init vcpu exit info */
	struct st_vcpu *vcpu = get_bp_vcpu();
	vcpu->arch.exit_reason = reason;
	vcpu->arch.guest_rip = guest_rip;
	vcpu->arch.guest_ins_len = insn_len;
	vcpu->arch.exit_qualification = exit_qual;
	switch (reason) {
	case VMX_VMCALL:
		condition = vmx_get_test_condition();
		if (condition == CON_42243_REPORT_CASE) {
			hsi_rqmid_42243_virtualization_specific_features_unrestricted_guest_001();
			return VMX_TEST_VMEXIT;
		}
		/* Should not reach here */
		print_vmexit_info();
		DBG_ERRO("Unkown vmcall for make condition:%d", condition);
		return VMX_TEST_VMEXIT;
	default:
		DBG_ERRO("Unknown exit reason, %ld", reason);
		print_vmexit_info();
		break;
	}
	return VMX_TEST_VMEXIT;
}

