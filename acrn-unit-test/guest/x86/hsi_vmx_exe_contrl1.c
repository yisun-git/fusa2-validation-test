BUILD_CONDITION_FUNC(enable_rdtscp, CPU_EXEC_CTRL1, \
	CPU_RDTSCP, BIT_TYPE_SET);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Enable_RDTSCP_001
 *
 * Summary: IN root operation, set Secondary Processor-Based VM-Execution Controls
 * enable RDTSCP bit3 to 1
 * switch to non-root operation,
 * execute instruction RDTSCP, check this instruction executed successful and no #UD exception occurs.
 */
__unused static void hsi_rqmid_40622_virtualization_specific_features_vm_exe_con_enable_rdtscp_001()
{
	u8 chk = 0;
	if (rdtscp_check() == NO_EXCEPTION) {
		chk++;
	}
	report("%s", (chk == 1), __func__);
}

BUILD_CONDITION_FUNC(descriptor_table, CPU_EXEC_CTRL1, \
	CPU_DESC_TABLE, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Descriptor_Table_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * Descriptor-table exiting bit2 to 0, switch to non-root operation,
 * execute SGDT instruction, check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40625_virtualization_specific_features_vm_exe_con_descriptor_table_001()
{
	__unused struct descriptor_table_ptr desc;
	sgdt(&desc);

	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);
}

BUILD_CONDITION_FUNC(rdrand, CPU_EXEC_CTRL1, \
	CPU_RDRAND, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_RDRAND_Exiting_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * RDRAND exiting bit11 to 0, switch to non-root operation,
 * execute RDRAND instruction, check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40626_virtualization_specific_features_vm_exe_con_rdrand_001()
{
	__unused u64 rand_num = 0;
	asm volatile("rdrand %0" : "+r"(rand_num) : : "memory");
	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);
}

BUILD_CONDITION_FUNC(rdseed, CPU_EXEC_CTRL1, \
	CPU_RDSEED, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_RDSEED_Exiting_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * RDSEED exiting bit16 to 0, switch to non-root operation,
 * execute RDSEED instruction, check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40627_virtualization_specific_features_vm_exe_con_rdseed_001()
{
	__unused u64 rand_num = 0;
	asm volatile("rdseed %0" : "+r"(rand_num) : : "memory");

	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);
}

BUILD_CONDITION_FUNC(xsaves, CPU_EXEC_CTRL1, \
	CPU_XSAVES_XRSTORS, BIT_TYPE_CLEAR);

__attribute__((aligned(64))) u8 xsave_area_array[XSAVE_AREA_SIZE];
static __unused int xsaves_checking(u8 *xsave_array, u64 xcr0)
{
	u32 eax = xcr0;
	u32 edx = xcr0 >> 32;

	asm volatile(ASM_TRY("1f")
		"xsaves %[addr]\n\t"
		"1:"
		: : [addr]"m"(*xsave_array), "a"(eax), "d"(edx)
		: "memory");

	return exception_vector();
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Enable_XSAVES_XRSTORS_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * Enable XSAVES/XRSTORS bit20 to 0, switch to non-root operation,
 * execute XSAVES instruction cause #UD exception, then check vm-exit handler are not called
 * for this action in root operation.
 */
__unused static void hsi_rqmid_40628_virtualization_specific_features_vm_exe_con_xsaves_001()
{
	u8 chk = 0;
	/* enable xsave feature set */
	write_cr4(read_cr4() | X86_CR4_OSXSAVE);
	memset(xsave_area_array, 0, sizeof(xsave_area_array));

	if (xsaves_checking(xsave_area_array, (STATE_X87 | STATE_SSE)) == UD_VECTOR) {
		chk++;
	}

	report("%s", (chk == 1), __func__);
}

BUILD_CONDITION_FUNC(wbinvd, CPU_EXEC_CTRL1, \
	CPU_WBINVD, BIT_TYPE_SET);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_WBINVD_Exiting_001
 *
 * Summary: IN root operation, set Secondary Processor-Based VM-Execution Controls
 * WBINVD exiting bit6 to 1, switch to non-root operation,
 * execute WBINVD instruction, then check vm-exit handler are called
 * for this action in root operation.
 */
__unused static void hsi_rqmid_40636_virtualization_specific_features_vm_exe_con_wbinvd_001()
{
	asm volatile("wbinvd");

	report("%s", (get_exit_reason() == VMX_WBINVD), __func__);
}

/* Disable INVPCID instruction in second processor-based */
BUILD_CONDITION_FUNC(enable_invpcid, CPU_EXEC_CTRL1, \
	CPU_INVPCID, BIT_TYPE_CLEAR);

typedef struct {
	u64 pcid : 12;
	u64 rsv : 52;
	u64 linear_addr;
} st_invpcid_des;
static int invpcid_checking(unsigned long type, void *desc)
{
	asm volatile(ASM_TRY("1f")
					"invpcid %0, %1\n\t" /* invpcid (%rax), %rbx */
					"1:" : : "m" (desc), "r" (type));
	return exception_vector();
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Enable_INVPCID_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * enable invpcid bit12 to 0, switch to non-root operation,
 * execute INVPCID instruction, then check execution of INVPCID causes a #UD.
 */
__unused static void hsi_rqmid_41079_virtualization_specific_features_vm_exe_con_enable_invpcid_001()
{
	st_invpcid_des desc;
	desc.pcid = 1;

	int chk = invpcid_checking(2, (void *)&desc);

	report("%s", (chk == UD_VECTOR), __func__);
}

BUILD_CONDITION_FUNC(vmcs_shadowing, CPU_EXEC_CTRL1, \
	CPU_SHADOW_VMCS, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_VMCS_Shadowing_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * VMCS shadowing bit14 to 0, switch to non-root operation,
 * execute VMREAD instruction, then check vm-exit handler are called
 * for this action in root operation beacause VMCS shadowing is disable.
 */
__unused static void hsi_rqmid_41084_virtualization_specific_features_vm_exe_con_vmcs_shadowing_001()
{
	u64 value;

	/** Execute 'vmread' instruction in order to read a specified field from the current processor's VMCS.
	 *  - Input operands: \a field_full which is the register source operand.
	 *  - Output operands: 'value' which is the destination operand.
	 *  - Clobbers: flags register.
	 */
	asm volatile("vmread %%rdx, %%rax "
		: "=a"(value)
		: "d"(HSI_VMX_PROC_VM_EXEC_CONTROLS2)
		: "cc");

	report("%s", (get_exit_reason() == VMX_VMREAD), __func__);
}

static void enable_pml_condition(__unused struct st_vcpu *vcpu)
{
	/* Disable PML in second processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL1, CPU_PML, BIT_TYPE_CLEAR);
	DBG_INFO("CPU_EXEC_CTRL1:0x%x", exec_vmread32(CPU_EXEC_CTRL1));

	/* Set page-modification log first address */
	u8 *pml = vcpu->arch.pml;
	memset((void *)pml, 0, PAGE_SIZE);
	uint64_t value64 = hva2hpa(pml);
	vmcs_write(VMX_PAGE_MODIFICATION_LOG_ADDRESS, value64);
	/* Set PML index */
	vmcs_write(VMX_PAGE_MODIFICATION_LOG_INDEX, DEFAULT_PML_ENTRY);

	/* Refer SDM table 24-8: set eptp bit6
	 * enables accessed and dirty flags for EPT
	 */
	vmcs_write(EPTP, vmcs_read(EPTP) | (1UL << 6U));
	DBG_INFO("VMX_EPT_POINTER_FULL:0x%lx\n", vmcs_read(EPTP));
}

static void hypercall_get_pml_index(__unused struct st_vcpu *vcpu)
{
	vm_exit_info.result = exec_vmread32(VMX_PAGE_MODIFICATION_LOG_INDEX);
	DBG_INFO("pml index:%d", (u32)vm_exit_info.result);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Enable_PML_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * Enable PML bit17 to 0, execute VMWRITE set PML first address and index,
 * switch to non-root operation,
 * Access guest physical address and invalid TLB , then check vm exit for PML full not called.
 * and the PML index value not change by processor.
 */
__unused static void hsi_rqmid_41094_virtualization_specific_features_vm_exe_con_enable_pml_001()
{
	u8 chk = 0;
	u32 *gva = (u32 *)(384 * 1024 * 1024UL);
	*(gva) = 2;
	invlpg(gva);

	/* in root operation Make sure PML index not change by processor */
	vmx_set_test_condition(CON_GET_PML_INDEX);
	vmcall();
	barrier();
	if ((vm_exit_info.result & 0xFFFF) == DEFAULT_PML_ENTRY) {
		chk++;
	}

	if (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE) {
		chk++;
	}

	report("%s", (chk == 2), __func__);
}

static void virt_apic_access_condition(__unused struct st_vcpu *vcpu)
{
	/* Disable virtualize APIC access at second processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL1, CPU_VIRT_APIC_ACCESSES, BIT_TYPE_CLEAR);

	/*APIC-v, config APIC virtualized page address*/
	uint64_t value64 = hva2hpa(&vcpu->arch.vlapic.apic_page);
	vmcs_write(VMX_VIRTUAL_APIC_PAGE_ADDR_FULL, value64);
	DBG_INFO("VMX_VIRTUAL_APIC_PAGE_ADDR_FULL:%lx", vmcs_read(VMX_VIRTUAL_APIC_PAGE_ADDR_FULL));

	u64 start_hpa = HOST_PHY_ADDR_START;
	u64 start_gpa = APIC_MMIO_BASE_ADDR;
	u32 size = GUEST_MEMORY_MAP_SIZE;

	/* map apic page cause vm-exit */
	ept_gpa2hpa_map(vcpu, start_hpa, start_gpa, size,
		EPT_EA, false);

	u64 eptp = vcpu->arch.eptp;
	vmcs_write(EPTP, eptp);

	invept(INVEPT_TYPE_ALL_CONTEXTS, eptp);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Virturalize_APIC_Accesses_001
 *
 * Summary: IN root operation, clear second Processor-Based VM-Execution Controls
 * virtual APIC access bit0 to 0, install the APIC-access page.
 * switch to non-root operation,
 * write memory pointed by APIC-access page(0xfee00000), check EPT violation vm-exit handler are called for
 * this action in root operation instead of APIC-access VM exit.
 */
__unused static void hsi_rqmid_41122_virtualization_specific_features_vm_exe_con_virt_apic_access_001()
{
	uint32_t *apic_base = ((u32 *)APIC_MMIO_BASE_ADDR);
	*apic_base = 3;

	report("%s", (get_exit_reason() == VMX_EPT_VIOLATION), __func__);
}

static void virt_x2apic_mode_condition(__unused struct st_vcpu *vcpu)
{
	/* disable Virtualize x2APIC mode for test condition. */
	exec_vmwrite32_bit(CPU_EXEC_CTRL1, CPU_VIRT_X2APIC, BIT_TYPE_CLEAR);

	set_msr_bitmap(vcpu->arch.msr_bitmap,
		MSR_IA32_EXT_APIC_ICR, BIT_TYPE_SET, MSR_READ_WRITE);
	DBG_INFO("X2APIC MODE CONDITION");
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Virtualize_X2APIC_Mode_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * Virtualize x2APIC mode bit4 to 0, switch to non-root operation,
 * execute WRMSR with index 0x830(MSR_IA32_EXT_APIC_ICR ) instruction,
 * then check vm-exit handler for MSR write are called
 * in root operation.
 */
__unused static void hsi_rqmid_41123_virtualization_specific_features_vm_exe_con_virt_x2apic_mode_001()
{
	vm_exit_info.is_wrmsr_apic_icr_exit = VM_EXIT_NOT_OCCURS;

	wrmsr(MSR_IA32_EXT_APIC_ICR, MSR_APIC_ICR_VALUE);

	barrier();
	report("%s", (vm_exit_info.is_wrmsr_apic_icr_exit == VM_EXIT_OCCURS), __func__);
}

BUILD_CONDITION_FUNC(apic_reg_virt, CPU_EXEC_CTRL1, \
	CPU_APIC_REG_VIRT, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_APIC-register_virtualization_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * APIC-register virtualization bit8 to 0, switch to non-root operation,
 * execute RDMSR with index 0x830(MSR_IA32_EXT_APIC_ICR ) instruction,
 * then check vm-exit handler for MSR read are called
 * in root operation rather than APIC access vm-exit.
 */
__unused static void hsi_rqmid_41124_virtualization_specific_features_vm_exe_con_apic_reg_virt_001()
{
	vm_exit_info.is_vm_exit[VM_EXIT_RDMSR_APIC_ICR] = VM_EXIT_NOT_OCCURS;
	rdmsr(MSR_IA32_EXT_APIC_ICR);

	barrier();
	report("%s", (vm_exit_info.is_vm_exit[VM_EXIT_RDMSR_APIC_ICR] == VM_EXIT_OCCURS), __func__);
}

static void virt_inter_delivery_condition(__unused struct st_vcpu *vcpu)
{
	/* Set virtual apic-access page address to incorrent value */
	u64 value64 = ILLEGAL_VIRTUAL_APIC_PAGE_ADDR;
	/* Install apic-access page */
	vmcs_write(VMX_VIRTUAL_APIC_PAGE_ADDR_FULL, value64);

	/* Disable Virtual-interrupt delivery at second processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL1,
		CPU_VINTD, BIT_TYPE_CLEAR);

	DBG_INFO("CPU_EXEC_CTRL1:0x%x", exec_vmread32(CPU_EXEC_CTRL1));
	DBG_INFO("pin_base:0x%x ---CPU_EXEC_CTRL0:0x%x", exec_vmread32(PIN_CONTROLS), exec_vmread32(CPU_EXEC_CTRL0));
}

/**
 * @brief Case name:HSI_Virtualization_Specific_Features_VM_Execution_Controls_Virtual-interrupt_Delivery_001
 *
 * Summary: IN root operation, clear second Processor-Based VM-Execution Controls
 * Virtual-interrupt delivery bit9 to 0, initialize 4K apic-access memory,
 * set the APIC-access address to incorrect value, install the APIC-access page.
 * Execute VMRESUME to entry non-root operation should be successful.
 */
__unused static void hsi_rqmid_41173_virtualization_specific_features_vm_exe_con_virt_inter_delivery_001()
{
	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);
}

static void use_tsc_scaling_condition(__unused struct st_vcpu *vcpu)
{
	msr_passthroudh();
	/* Disable RDTSC exiting in primary processor-based */
	vmcs_clear_bits(CPU_EXEC_CTRL0, CPU_RDTSC);

	/* Disable Use TSC scaling at second processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL1,
		CPU_TSC_SCALLING, BIT_TYPE_CLEAR);

	/* set TSC scaling */
	vmcs_write(VMX_TSC_MULTIPLIER_FULL, 2);
	DBG_INFO("CPU_EXEC_CTRL1:0x%x", exec_vmread32(CPU_EXEC_CTRL1));
}

#define TSC_DEADLINE_TIMER_VECTOR 0xef
/* compare local apic timer with tsc times */
#define CMP_TIMES 10
#define TSC_1000MS 0x7D2B74E0
#define TSC_10US 0x5208

static int enable_tsc_deadline_timer(void)
{
	u32 lvtt;

	lvtt = APIC_LVT_TIMER_TSCDEADLINE | TSC_DEADLINE_TIMER_VECTOR;
	apic_write(APIC_LVTT, lvtt);
	return 1;
}

u32 inter_count;
u64 tsc_now;
static void tsc_deadline_timer_handler(isr_regs_t *reg)
{
	/* record current TSC for compare */
	tsc_now = rdtsc();
	++inter_count;
	eoi();
}
static void start_apic_deadline_timer(u64 test_timer)
{
	wrmsr(MSR_IA32_TSCDEADLINE, rdtsc() + test_timer);
	nop();
}

/**
 * @brief Case name:HSI_Virtualization_Specific_Features_VM_Execution_Controls_Use_TSC_Scanling_001
 *
 * Summary: IN root operation, clear second Processor-Based VM-Execution Controls
 * use TSC scaling bit25 to 0 and RDTSC exiting bit12 to 0, set TSC scaling to 2.
 * switch to non-root operation,
 * compare TSC with lapic's clock, check the frequency of TSC keep unchanged.
 */
__unused static void hsi_rqmid_41176_virtualization_specific_features_vm_exe_con_use_tsc_scaling_001()
{
	u8 chk = 0;
	u64 tsc_start[CMP_TIMES];
	/* compare apic timer with TSC */
	u64 tsc_diff[CMP_TIMES];
	u32 i;
	inter_count = 0;
	tsc_now = 0;
	if (enable_tsc_deadline_timer() == 0) {
		printf("not supprot apic tsc.\n");
	}
	cli();
	handle_irq(TSC_DEADLINE_TIMER_VECTOR, tsc_deadline_timer_handler);
	sti();

	for (i = 0; i < CMP_TIMES; i++) {
		/* get start test TSC */
		tsc_start[i] = rdtsc();
		/* start APIC dealine timer */
		start_apic_deadline_timer(TSC_1000MS);
		/* wait for interrupt occurs */
		while (inter_count < (i + 1)) {
			nop();
		}
		/* get code run TSC */
		u64 singer_tsc = (tsc_now - tsc_start[i]);
		tsc_diff[i] = singer_tsc - TSC_1000MS;
		DBG_INFO("\n times:%d tsc_diff:0x%lx \n", i, tsc_diff[i]);
		/*
		 * compare apic timer with TSC
		 */
		if ((0 < tsc_diff[i]) && (tsc_diff[i] < TSC_10US)) {
			chk++;
		}
	}

	DBG_INFO("chk:%u", chk);
	report("%s", (chk == CMP_TIMES), __func__);
}

