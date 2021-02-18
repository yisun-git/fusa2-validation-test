
int vm_exec_init(struct vmcs *vmcs);
void ept_gpa2hpa_map(struct st_vcpu *vcpu, u64 hpa, u64 gpa,
		u32 size, u64 permission, bool is_clear);
/* define for posted interrupt */
static u16 record_vector;
/* start_run_id is used to identify which test case is running. */
static volatile int start_run_id = 0;
static volatile u32 nmi_count;
static u64 time_tsc[TSC_NMI_BUFF];
static volatile u8 ap_send_nmi_flag;
static volatile u8 bp_hand_flag;

static void msr_passthroudh()
{
	u8 *msr_bitmap = get_bp_vcpu()->arch.msr_bitmap;
	memset(msr_bitmap, 0x0, PAGE_SIZE);
	vmcs_write(MSR_BITMAP, (u64)msr_bitmap);
	vmcs_set_bits(CPU_EXEC_CTRL0, CPU_MSR_BITMAP);
}

static void handler_inter80_irq(__unused isr_regs_t *regs)
{
	DBG_INFO(" interrupt occurs!");
	eoi();
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VMX_Instruction_001
 *
 * Summary: Verify that below instructions clear/load VMCS
 * operation is successful and normal execution with no exception
 *		 1.  Execute VMXON instruction to enable VMX operation.
 *		 2.  In VMX root operation, execute VMPTRLD instruction to load VMCS pointer.
 *		 3.  In VMX root operation, execute VMREAD/VMWRITE instructions to read/write VMCS.
 *		 4.  In VMX root operation, execute VMCLEAR instruction to clear VMCS.
 *		 5.  Execute VMXOFF instruction to leave VMX operation.
 */
__unused void hsi_rqmid_40291_virtualization_specific_features_VMX_instruction_001()
{
	uint32_t chk = 0;
	u32 vmcs_revision;
	struct vmcs *hsi_vmcs = alloc_page();

	/* init vmcs data */
	memset(hsi_vmcs, 0, sizeof(PAGE_SIZE));
	vmcs_revision = (u32)rdmsr(MSR_IA32_VMX_BASIC);
	hsi_vmcs->hdr.revision_id = vmcs_revision;

	/* execute vmxon */
	*vmxon_region = vmcs_revision;
	vmx_on();

	/* execute VMCLEAR instruction to clear VMCS */
	if (vmcs_clear(hsi_vmcs)) {
		DBG_ERRO("%s : vmcs_clear error\n", __func__);
	} else {
		chk++;
	}

	/* execute VMPTRLD load VMCS pointer */
	if (make_vmcs_current(hsi_vmcs)) {
		DBG_ERRO("%s : make_vmcs_current error\n", __func__);
	} else {
		chk++;
	}

	/* execute VMWRITE instructions to write guest es limit */
	exec_vmwrite32(VMX_GUEST_ES_LIMIT, GUEST_ES_LIMIT_TEST_VALUE);

	/* execute VMREAD instructions to read */
	if (exec_vmread32(VMX_GUEST_ES_LIMIT) == GUEST_ES_LIMIT_TEST_VALUE) {
		chk++;
	}

	vmx_off();
	report("%s", (chk == 3), __func__);
}


/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VMX_Instruction_002
 *
 * Summary: Config 4k size memory for VMCS, enable VMX operation, load VMCS.
 * Then verify execute instructions VMLAUNCH/VMRESUME to swith from VMX root
 * opertaion to VMX non-root operation successful.
 */
__unused static void hsi_rqmid_41182_virtualization_specific_features_VMX_instruction_002()
{
	uint8_t chk = 0;

	/* this case should run the first case in test suit */
	/* if reach here host execute VMLANUCH
	 * to non-root operation success
	 */
	chk++;
	vmx_set_test_condition(CON_SECOND_VM_EXIT_ENTRY);
	/* execute vmcall cause vm-exit then execute host execute VMRESUME
	 * to non-root operation.
	 */
	vmcall();
	chk++;
	/* if reach here which means resume success */
	report("%s", (chk == 2), __func__);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_External_Interrupt_001
 *
 * Summary: IN root operation, clear Pin-Based VM-Execution Controls
 * external-interrupt exiting bit0 to 0, switch to non-root operation,
 * trigge an external interrupt, check vm-exit handler are not called for
 * this external interrupt in root operation.
 */
static void hsi_rqmid_40364_virtualization_specific_features_vm_exe_con_external_inter_001()
{
	irq_disable();
	handle_irq(EXTEND_INTERRUPT_80, handler_inter80_irq);

	sti();

	/* Trigger external interrupts in guest */
	apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_80);
	wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+(0x10000));

	/* wait external interrupt occur */
	asm volatile("hlt" : : :);
	report("%s", get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE, __func__);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Interrupt_Window_001
 *
 * Summary: IN root operation, clear Processor-Based VM-Execution Controls
 * interrupt-window exiting bit2 to 0, switch to non-root operation,
 * set EFLAGS.IF = 1, check vm-exit handler are not called for
 * this action in root operation.
 */
static void hsi_rqmid_40391_virtualization_specific_features_vm_exe_con_irq_window_001()
{
	sti();
	report("%s", get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE, __func__);
}

static u32 nmi_cout;

void handle_nmi_inter(struct ex_regs *regs)
{
	/* mark NMI interrupt is executed */
	nmi_cout++;
	DBG_INFO("---------NMI isr running rip=%lx nmi_cout:%d", regs->rip, nmi_cout);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_NMI_Interrupt_001
 *
 * Summary: IN root operation, clear Pin-Based VM-Execution Controls
 * NMI exiting bit3 to 0, switch to non-root operation,
 * trigge an NMI interrupt, check vm-exit handler are not called for
 * this NMI interrupt in root operation.
 */
static void hsi_rqmid_40369_virtualization_specific_features_vm_exe_con_nmi_inter_001()
{
	nmi_cout = 0;
	cli();
	handle_exception(NMI_VECTOR, &handle_nmi_inter);
	sti();

	/* Trigger non-maskable interrupts through write lapic icr register in guest */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_NMI, 0);

	/* wait NMI interrupt occurs */
	delay(1000);

	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE) &&\
		(nmi_cout == 1), __func__);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Use_TRP_Shadow_001
 *
 * Summary: IN root operation, clear primary Processor-Based VM-Execution Controls
 * Use TPR shadow bit21 to 0, install the APIC-access page then initialize the virtual TPR to the val2,
 * Set real APIC register TPR to val1.
 * switch to non-root operation,
 * access APIC's TPR via CR8, check CR8 value
 * should be val1 rather than val2.
 */
__unused static void hsi_rqmid_41089_virtualization_specific_features_vm_exe_con_use_tpr_shadow_001()
{
	/**
	 * Accroding SDM 10.8.6.1 Interaction of Task Priorities between CR8 and APIC
	 * APIC.TPR[bits 7:4] = CR8[bits 3:0], APIC.TPR[bits 3:0] = 0.
	 * CR8[bits 63:4] is 0.
	 */
	u64 tpr = (read_cr8() << 4U);
	DBG_INFO("\n cr8:0x%lx tpr_msr:0x%lx\n", read_cr8(), rdmsr(MSR_IA32_EXT_APIC_TPR));

	report("%s", (tpr == APIC_TPR_VAL1), __func__);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_HLT_001
 *
 * Summary: IN root operation, clear Processor-Based VM-Execution Controls
 * HLT exiting bit7 to 0, switch to non-root operation,
 * execute HLT instruction, check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40394_virtualization_specific_features_vm_exe_con_hlt_001()
{
	irq_disable();
	handle_irq(EXTEND_INTERRUPT_80, handler_inter80_irq);

	sti();

	/* Trigger external interrupts in guest to wake up hlt */
	apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_80);
	wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+(0x10000));

	/* test hlt not cause vm-exit */
	asm volatile("hlt" : : :);
	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);

}

static void ipi_posted_inter_handle(__unused isr_regs_t *regs)
{
	record_vector = POSTED_INTR_VECTOR;
	eoi();
}

static void pi_request_inter_handle(__unused isr_regs_t *regs)
{
	record_vector = POSTED_INTR_REQUEST_VECTOR;
	eoi();
}

/**
 * @brief Case name:HSI_Virtualization_Specific_Features_VM_Execution_Controls_Process_Posted_interrupts_001
 *
 * Summary: IN root operation, clear Pin-Based VM-Execution Controls
 * Process posted interrupts bit7 to 0, writethe posted-interrupt notification
 * vector field with 0xe3, write posted-interrupt descriptor bit[255:0]'s
 * bit[0xa0] to 1.
 * switch to non-root operation,
 * prepare two interrupt gate with vector 0xa0 and 0xe3, BP send an IPI to self
 * with the vector 0xe3, check vector 0xe3 handler are called rather than 0xa0.
 */
__unused static void hsi_rqmid_41181_virtualization_specific_features_vm_exe_con_posted_inter_001()
{
	/* init vector */
	record_vector = 0;

	/* set handle */
	cli();
	/* set post-interrupt handler */
	handle_irq(POSTED_INTR_VECTOR, ipi_posted_inter_handle);
	/* set post-interrupt request handler in posted-interrupt descriptor */
	handle_irq(POSTED_INTR_REQUEST_VECTOR, pi_request_inter_handle);
	sti();
	/* send a IPI to slef with the same vector */
	apic_icr_write(APIC_DEST_DESTFLD | APIC_DEST_PHYSICAL | APIC_DM_FIXED | POSTED_INTR_VECTOR,
		CPU_BP_ID);
	/* wait ipi interrupt occurs */
	delay(10000);
	report("%s", (record_vector == POSTED_INTR_VECTOR), __func__);
}

__unused static void delay_tsc(u64 count)
{
	u64 tsc_now = asm_read_tsc();
	while (asm_read_tsc() - tsc_now < count) {
		nop();
	}
}

static void handled_nmi_exception(__unused struct ex_regs *regs)
{
	nmi_count++;
	/*first nmi*/
	if (nmi_count == NMI_FIRST_TIME) {
		time_tsc[TSC_FIRST_NMI_START] = asm_read_tsc();
		ap_send_nmi_flag = 1;
		delay_tsc(HANDLE_FIRST_NMI_TIME);
		time_tsc[TSC_FIRST_NMI_END] = asm_read_tsc();
	}
	/*second nmi*/
	else if (nmi_count == NMI_SECOND_TIME) {
		time_tsc[TSC_SECOND_NMI_START] = asm_read_tsc();
		bp_hand_flag = BP_HANDLE_SECOND_NMI_END;
	}
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Virtual_NMIS_001
 *
 * Summary: IN root operation on BP, execute VMWRITE instruction to clear
 * the Virtual NMIs bit5 to 0 with Pin-Based Execution controls.
 * switch to non-root operation,
 * AP1 sends the first non-maskable IPI to BP,
 * when BP in NMI_handler(in NMI_handler implement a delay() function)
 * AP1 send the second non-maskable IPI to BP,
 * The second NMI will be processed after the first NMI processing is completed,
 * rather than nested processing.
 */
__unused static void hsi_rqmid_42244_virtualization_specific_features_vm_exe_con_virt_nmis_001()
{
	u32 chk = 0;
	nmi_count = 0;
	ap_send_nmi_flag = 0;
	bp_hand_flag = 0;
	memset(time_tsc, 0, sizeof(time_tsc));
	cli();
	handle_exception(NMI_VECTOR, &handled_nmi_exception);
	sti();

	/* ap start send NMI */
	start_run_id = CASE_ID_42244;

	/* bp wait ap send first nmi, and handle the interrupt,
	 * then we can check the result
	 */
	while (bp_hand_flag != BP_HANDLE_SECOND_NMI_END) {
		nop();
	}
	/* check the nmi is blocked by processor */
	if ((nmi_count == NMI_SECOND_TIME) &&
		(time_tsc[TSC_SECOND_NMI_START] > time_tsc[TSC_FIRST_NMI_END])) {
		chk++;
	}
	DBG_INFO("nmi_cout:%d tsc_fir_end:%lx tsc_sen_start:%lx\n", \
		nmi_count,\
		time_tsc[TSC_FIRST_NMI_END], \
		time_tsc[TSC_SECOND_NMI_START]);
	report("%s", (chk == 1), __func__);

}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Enable_EPT_001
 *
 * Summary: IN root operation, set Secondary Processor-Based VM-Execution Controls
 * enable EPT bit1 to 1, map a 4M memory from GPA to HPA by EPT,
 * switch to non-root operation,
 * Write random data to the 4M memory which pointed by GPA,
 * then check vm exit for EPT misconfiguration are not called
 * and check the random data to the 4M memory pointed by HPA.
 */
__unused static void hsi_rqmid_41120_virtualization_specific_features_vm_exe_con_enable_ept_001()
{
	vm_exit_info.ept_map_flag = 0;
	/* Because this address is one one mapping, so GVA is GPA */
	u32 *gva = (u32 *)GUEST_PHYSICAL_ADDRESS_START;
	u32 size = GUEST_MEMORY_MAP_SIZE / sizeof(u32);
	u32 i;
	for (i = 0; i < size; i++) {
		*gva++ = i;
	}
	/* at host check hpa pointed memory whether is set by gpa */
	vmx_set_test_condition(CON_SECOND_CHECK_EPT_ENABLE);
	vmcall();
	report("%s", (vm_exit_info.ept_map_flag == EPT_MAPPING_RIGHT), __func__);

}

static int write_mem_check(void *gva)
{
	assert(gva != NULL);
	u8 temp = 0x1;
	asm volatile(ASM_TRY("1f")
		"mov %[value], (%[gva])\n\t"
		"1:"
		: : [value]"r"(temp), [gva]"r"(gva) : "memory");

	return exception_vector();
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Enable_VPID_001
 *
 * Summary: IN root operation, set Secondary Processor-Based VM-Execution Controls
 * enable VPID bit5 to 1, switch to non-root operation,
 * Define a new address GVA points to 8bytes memory, cache the TLB for this GVA,
 * clear the P flag for this page table, execute VMCALL, it would cause vm exit/entry,
 * then swith to non-root operation again, access the GVA pointed memory, there is no #PF exception
 * becasue the VPID enabled.
 */
__unused static void hsi_rqmid_40638_virtualization_specific_features_vm_exe_con_enable_vpid_001()
{
	u64 *gva = (u64 *)malloc(sizeof(u64));
	ulong cr3 = read_cr3();
	u32 chk = 0;
	/* cache Page Table to TLB */
	*gva = 1;
	/* Clear PML4 P flag in memory */
	pteval_t *pml4 = (pteval_t *)cr3;
	u32 pml4_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_PML4);
	pml4[pml4_offset] &= ~PT_PRESENT_MASK;

	/* Call vmcall, cause vm exit and entry */
	vmx_set_test_condition(CON_SECOND_VM_EXIT_ENTRY);
	vmcall();
	/**
	 * Make sure TLB not invalid after vm exit/entry occurs,
	 * so the VPID function is used. Otherwise is will cause a #PF exception.
	 */
	if (write_mem_check((void *)gva) == NO_EXCEPTION) {
		chk++;
	}

	report("%s", (chk == 1), __func__);
	/* resume environment */
	pml4[pml4_offset] |= PT_PRESENT_MASK;
	free((void *)gva);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_PAUSE_Loop_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * PAUSE-Loop exiting bit10 to 0, execute VMWRITE set PLE_GAP and PLE_WINDOW value,
 * switch to non-root operation,
 * Loop PAUSE instruction, then check vm exit for pause-loop not called.
 */
__unused static void hsi_rqmid_41078_virtualization_specific_features_vm_exe_con_pause_loop_001()
{
	int i;
	for (i = 0; i < (2 * VMX_PLE_WINDOW_TSC); i++) {
		pause();
	}
	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);
}

static int vmfunc_checking(u8 func_id)
{
	asm volatile(ASM_TRY("1f")
					"vmfunc \n\t"
					"1:" : : "a"(func_id));
	return exception_vector();
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Enable_VM_Functions_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * enable VM functions bit13 to 0, switch to non-root operation,
 * execute VMFUNC instruction, then check execution of VMFUNC causes a #UD.
 */
__unused static void hsi_rqmid_41081_virtualization_specific_features_vm_exe_con_enable_vm_func_001()
{
	int chk = 0;
	/* disable vmfunc bit so execute vmfunc should casue #UD */
	if (vmfunc_checking(1) == UD_VECTOR) {
		chk++;
	}
	report("%s", (chk == 1), __func__);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_EPT_Violation_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * EPT-violation #VE bit18 to 0, map a 4M non-writable memory from HPA to GPA by EPT,
 * set EPT paging-structure bit1 to 0.
 * switch to non-root operation,
 * Write data to the momory which pointed by GPA, then check vm exit for EPT violation are called
 * instead of EPT violations may cause virtualization exceptions.
 */
__unused static void hsi_rqmid_41110_virtualization_specific_features_vm_exe_con_ept_violation_001()
{
	u32 *gva = (u32 *)GUEST_PHYSICAL_ADDRESS_START;
	u8 chk = 0;
	/* Because this address is one one mapping, so GVA is GPA */
	*(gva) = 2;

	u32 qualifi = get_bp_vcpu()->arch.exit_qualification;
	/* ept violation vm exit occurs and caused by data write rather than #VE exception */
	if ((get_exit_reason() == VMX_EPT_VIOLATION) &&
		((qualifi & EPT_VIOLATION_WRITE) == EPT_VIOLATION_WRITE)) {
		chk++;
	}
	DBG_INFO("qualification:%x exit reason:%d", qualifi, get_exit_reason());
	report("%s", (chk == 1), __func__);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Mode-based_Execute_Control_For_EPT_001
 *
 * Summary: IN root operation, clear Secondary Processor-Based VM-Execution Controls
 * Mode-based execute control for EPT bit22 to 0,
 * config 4M memory EPT paging-structrue table map GPA(384 * 1024 * 1024) to HPA,
 * clear EPT paging-structure bit2 to 0, disallow execute access,
 * set EPT paging-structure bit10 to 1, allow user-mode paging execute access.
 * switch to non-root operation,
 * Config paging-structure table, make GVA to GPA one one mapping,
 * set paging-structure table bit2 to 1(user-mode paging),
 * Write 0xc3(RET opcode) to the momory which pointed by GPA,
 * execute CALL instruction with the memory pointed by GVA,
 * then check vm exit for EPT violation are called becasue GVA pointed memory
 * can't executable even if set EPT paging-structure bit10 to 1, allow user-mode paging execute access.
 */
__unused static void hsi_rqmid_41113_virtualization_specific_features_vm_exe_con_excute_for_ept_001()
{
	const char *temp = "\xC3";
	u8 chk = 0;
	start_run_id = CASE_ID_41113;
	void (*gva)(void) = (void *)GUEST_PHYSICAL_ADDRESS_START;
	memcpy(gva, temp, strlen(temp));
	/*
	 * Because this address is one one mapping, so GVA is GPA.
	 * this instruction shall cause ept violation exit, because
	 * this memory disallow instruction fetched.
	 */
	asm volatile("call *%[address]\n\t"
		: : [address]"a"(gva));

	u32 qualifi = get_bp_vcpu()->arch.exit_qualification;
	/*
	 * ept violation vm exit occurs and caused by instruction fetch
	 * even if set ept use mode executable bit10.
	 * because disable Mode-based execute control for EPT
	 * in second processor-based
	 */
	if ((get_exit_reason() == VMX_EPT_VIOLATION) &&
		((qualifi & EPT_VIOLATION_INST) == EPT_VIOLATION_INST)) {
		chk++;
	}

	DBG_INFO("qualification:%x exit reason:%d", qualifi, get_exit_reason());
	report("%s", (chk == 1), __func__);
	start_run_id = CASE_ID_BUTT;
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Exit_Control_Save_Debug_001
 *
 * Summary: IN root operation, clear VMCS VM-Exit controls bit2(save debug controls) to 0,
 * execute VMWRITE set guest DR7 value to 0x400 with guest DR7 field,
 * switch to non-root operation,
 * Execute MOV set guest physical DR7 value to 0x2400.
 * switch to root operation with VM-exit.
 * Check guest DR7 value should be x400 unchanged in VMCS guest DR7 fields.
 */
__unused static void hsi_rqmid_42176_virtualization_specific_features_save_debug_exit_001()
{
	/*set DR7.GD to 1*/
	write_dr7(GUEST_DR7_VALUE2);
	/* hypercall cause vm-exit check guest dr7 unchanged at host */
	vmx_set_test_condition(CON_SECOND_CHECK_DEBUG_REG);
	vmcall();

	DBG_INFO("vm_exit_info.result:%ld", vm_exit_info.result);
	/* check vm-exit save DR7 register
	 * guest DR7 field should do not changed by processor, the value should be
	 * GUEST_DR7_VALUE1 rather than GUEST_DR7_VALUE2.
	 */
	report("%s", (vm_exit_info.result == RET_EXIT_SAVE_DEBUG), __func__);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Entry_Control_Load_IA32_PERF_001
 *
 * Summary: IN root operation, clear VMCS VM-Entry controls bit13(load IA32_PERF_GLOBAL_CTRL) to 0,
 * execute VMWRITE set guest IA32_PERF_GLOBAL_CTRL value to 0 with guest IA32_PERF_GLOBAL_CTRL field,
 * execute WRMSR set physical MSR IA32_PERF_GLOBAL_CTRL value to 1.
 * switch to non-root operation,
 * Execute RDMSR get physical IA32_PERF_GLOBAL_CTRL which should be 1 rather than 0.
 */
__unused static void hsi_rqmid_42179_virtualization_specific_features_entry_load_perf_001()
{
	u8 chk = 0;
	ulong perf = rdmsr(MSR_IA32_PERF_GLOBAL_CTRL);

	if (perf == IA32_PERF_HOST_SET_VALUE) {
		chk++;
	}
	report("%s", (chk == 1), __func__);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_Guest_Host_Masks_For_CR0_001
 *
 * Summary: IN root operation, set CR0's guest/host masks which set CR0.CD to 1
 * CR0.MP to 0, clear CR0's read shadow CR0.CD and CR0.MP to 0.
 * switch to non-root operation,
 * execute MOV set CR0.CD from 0 to 1, check vm-exit handler are called for
 * this action in root operation because CR0.CD owned by host.
 * execute MOV set CR0.MP from 0 to 1, check vm-exit handler are not called for
 * this action in root operation becasue CR0.MP owned by guest.
 */
__unused static void hsi_rqmid_41946_virtualization_specific_features_cr0_masks_001()
{
	u8 chk = 0;
	vm_exit_info.is_vm_exit[VM_EXIT_WRITE_CR0_CD] = VM_EXIT_NOT_OCCURS;
	vm_exit_info.is_vm_exit[VM_EXIT_WRITE_CR0_MP] = VM_EXIT_NOT_OCCURS;

	/*
	 * Because CR0.CD shadow is 0, so set to 1
	 */
	write_cr0(read_cr0() | X86_CR0_CD);

	/*
	 * Because CR0.MP shadow is 0, so set to 1
	 */
	write_cr0(read_cr0() | X86_CR0_MP);

	/* Make sure write CR0.CD vm-exit are called */
	if (vm_exit_info.is_vm_exit[VM_EXIT_WRITE_CR0_CD] == VM_EXIT_OCCURS) {
		chk++;
	}

	/* Make sure write CR0.MP vm-exit are not called */
	if (vm_exit_info.is_vm_exit[VM_EXIT_WRITE_CR0_MP] == VM_EXIT_NOT_OCCURS) {
		chk++;
	}
	DBG_INFO("cr0:%lx", read_cr0());
	report("%s", (chk == 2), __func__);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_MSR_Bitmap_001
 *
 * Summary: IN root operation, set Processor-Based VM-Execution Controls
 * use msr bitmaps exiting bit28 to 1, set MSR_IA32_PAT read cause vm-exit,
 * set MSR_PLATFORM_INFO read not cause vm-exit,
 * switch to non-root operation,
 * execute instructoin rdmsr with ECX = 0x00000277, check vm-exit handler are called for
 * this action in root operation.
 * execute instructoin rdmsr with ECX = 0x000000CE, check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_41085_virtualization_specific_features_vm_exe_con_bitmap_msr_001()
{
	u8 chk = 0;
	vm_exit_info.is_vm_exit[VM_EXIT_RDMSR_PAT] = VM_EXIT_NOT_OCCURS;
	vm_exit_info.is_vm_exit[VM_EXIT_RDMSR_PLATFROM] = VM_EXIT_NOT_OCCURS;

	/* Create test condition in non-root operation
	 * with execute instruction RDMSR with msr MSR_IA32_PAT, MSR_PLATFORM_INFO.
	 */
	rdmsr(MSR_IA32_PAT);
	rdmsr(MSR_PLATFORM_INFO);

	/* Make sure read msr MSR_IA32_PAT vm-exit are called */
	if (vm_exit_info.is_vm_exit[VM_EXIT_RDMSR_PAT] == VM_EXIT_OCCURS) {
		chk++;
	}

	/* Make sure read msr MSR_PLATFORM_INFO vm-exit are not called */
	if (vm_exit_info.is_vm_exit[VM_EXIT_RDMSR_PLATFROM] == VM_EXIT_NOT_OCCURS) {
		chk++;
	}
	report("%s", (chk == 2), __func__);
}

/**
 * @brief Case name:HSI_Virtualization_Specific_Features_EPT_Construct_001
 *
 * Summary: IN root operation, alloc host address EPTP pointed to 4k memory
 * map memory form gpa to hpa with the EPTP, then execute VMWRITE set the EPTP
 * to VMCS, check instructions execution with no exception
 * and value read from VMCS is EPTP.
 */
__unused static void hsi_rqmid_42209_virtualization_specific_features_ept_construct_001()
{
	/* check the ept construct config right */
	report("%s", (vm_exit_info.result == RET_CHECK_EPT_CONSTRUCT), __func__);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_EPT_Invalid_001
 *
 * Summary: IN root operation, map memory from GPA to HPA1 in the EPT,
 * switch to non-root operation,
 * Write magic number to the 8bytes memory which pointed by GPA,
 * switch to root operation,
 * change the mapping from GPA to HPA2 in the EPT
 * switch to non-root operatoin,
 * check the 8bytes memory should be the magic number,
 * switch to root operation again,
 * execute invept instruction,
 * check the 8bytes memory should not be the magic number, because the old ept TLB is invalid.
 */
__unused static void hsi_rqmid_42224_virtualization_specific_features_ept_invalid_001()
{
	u8 chk = 0;
	/* vm one one mapping, so gva is gpa */
	u64 *gva = (u64 *)GUEST_PHYSICAL_ADDRESS_TEST;
	*gva = INIT_MAGIC_NUMBER;

	/* change memory mapping from gpa to hpa in host */
	vmx_set_test_condition(CON_CHANGE_MEMORY_MAPPING);
	vmcall();

	/* check the value not change */
	if (*gva == INIT_MAGIC_NUMBER) {
		chk++;
	}
	DBG_INFO(" change mapping gva value:%lx", *gva);

	/* invalid ept in host */
	vmx_set_test_condition(CON_INVEPT);
	vmcall();

	/* check memory mapping are changed because invept executed in host */
	if (*gva != INIT_MAGIC_NUMBER) {
		chk++;
	}
	DBG_INFO(" after invept value:0x%lx", *gva);

	report("%s", (chk == 2), __func__);

}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VPID_Invalid_001
 *
 * Summary: IN root operation, set Secondary Processor-Based VM-Execution Controls
 * enable VPID bit5 to 1, set VPID to 1 with VMX VPID field.
 * switch to non-root operation,
 * Define a new address GVA points to 8bytes memory, cache the TLB for this GVA,
 * clear the P flag for this page table.
 * switch to root operation and do nothing at host,
 * swith to non-root operation,
 * write GVA pointed memory, there is no exception,
 * swith to root operation,
 * execute INVVPID instruction,
 * swith to non-root operation,
 * write GVA pointed memory, there is #PF exception,
 * becasue the cached VPID mappings is invalidated.
 */
__unused static void hsi_rqmid_42226_virtualization_specific_features_vpid_invalid_001()
{
	int chk = 0;
	int result1, result2;
	u64 *gva = (u64 *)malloc(sizeof(u64));

	/* cache Page Table to TLB */
	*gva = 1;
	/* Clear PML4 P flag in memory */
	pteval_t *pml4 = (pteval_t *)read_cr3();
	u32 pml4_offset = PGDIR_OFFSET((uintptr_t)gva, PAGE_PML4);
	pml4[pml4_offset] &= ~PT_PRESENT_MASK;

	/* Call vmcall, cause vm exit and entry */
	vmx_set_test_condition(CON_SECOND_VM_EXIT_ENTRY);
	vmcall();

	/* because guest TLBS is unchage, gva is still can access */
	result1 = write_mem_check((void *)gva);
	if (result1 == NO_EXCEPTION) {
		chk++;
	}

	/* Call vmcall, vm exit to invalid VPID in host */
	vmx_set_test_condition(CON_INVVPID);
	vmcall();

	/* page fault , because TLB is invalid */
	result2 = write_mem_check((void *)gva);
	if (result2 == PF_VECTOR) {
		chk++;
	}

	DBG_INFO("\n result1:%d result2:%d \n", result1, result2);
	/* resume environment */
	pml4[pml4_offset] |= PT_PRESENT_MASK;
	free((void *)gva);

	report("%s", (chk == 2), __func__);
}

/**
 * @brief Case name:HSI_Virtualization_Specific_Features_VM_Entry_MSR_Control_Guest_Load_001
 *
 * Summary: IN root operation, initialize 16 bytes memory guest msr area,
 * write IA32_TSC_AUX value to msr index, write 0 to the msr,
 * execute VMWRITE set msr cout to 1 with VM entry MSR load count control field in the VMCS,
 * execute VMWRITE set msr addr to guest msr area address with VM-entry MSR-load address control field in the VMCS,
 * switch to non-root operation which means VM-entry occurs,
 * check IA32_TSC_AUX value should be 0 loaded by processor.
 * swith to root operation,
 * set guest IA32_TSC_AUX to 1 with guest msr area,
 * switch to non-root operation which means VM-entry occurs,
 * check IA32_TSC_AUX value should be 1 loaded by processor.
 */
__unused static void hsi_rqmid_42236_virtualization_specific_features_entry_guest_msr_control_001()
{
	int chk = 0;
	u64 tsc_aux1, tsc_aux2;

	tsc_aux1 = rdmsr(MSR_IA32_TSC_AUX);
	/* check guest msr load first time */
	if (tsc_aux1 == TSC_AUX_VALUE1) {
		chk++;
	}

	/* enter host set entry guest msr area */
	vmx_set_test_condition(CON_CHANGE_GUEST_MSR_AREA);
	vmcall();

	tsc_aux2 = rdmsr(MSR_IA32_TSC_AUX);
	/* check guest msr load second time */
	if (tsc_aux2 == TSC_AUX_VALUE2) {
		chk++;
	}

	DBG_INFO("tsc_aux1:%lx tsc_aux2:%lx", tsc_aux1, tsc_aux2);
	report("%s", (chk == 2), __func__);
}

/**
 * @brief Case name:HSI_Virtualization_Specific_Features_VM_Execution_Controls_TSC_Offsetting_Multiplier_001
 *
 * Summary: IN root operation, set primary Processor-Based VM-Execution Controls
 * use TSC offsetting bit3 to 1 and RDTSC exiting bit12 to 0,
 * clear second Processor-Base VM-Execution controls use TSC scaling bit25 to 0,
 * set TSC offsetting to a big value 0x100000000000, clear physical TSC to 0,
 * set TSC scailng value to 4.
 * switch to non-root operation,
 * execute RDTSC read TSC immediately, check the TSC value should more than
 * the TSC offsetting 0x100000000000 and much less than 4*0x100000000000
 */
__unused static void hsi_rqmid_42201_virtualization_specific_features_tsc_offset_multi_001()
{
	u64 guest_tsc = (u64)rdtsc();
	u8 chk = 0;
	/* make sure TSC offset are used in guest and multiplier are not used */
	if ((guest_tsc > TSC_OFFSET_DEFAULT_VALUE) &&
		(guest_tsc < ((TSC_OFFSET_DEFAULT_VALUE) * (TSC_SCALING_DEFAULT_VALUE / 2)))) {
		chk++;
	}
	report("%s", (chk == 1), __func__);
}

void vcpu_retain_rip(struct st_vcpu *vcpu)
{
	vcpu->arch.guest_ins_len = 0;
}

static void handler_exit_common(struct st_vcpu *vcpu)
{
	set_exit_reason(vcpu->arch.exit_reason);

	DBG_INFO("VMX_EXTINT exit reason:%d guest_rip:0x%lx ins_len:%d qualification:0x%x", \
		vcpu->arch.exit_reason,\
		vcpu->arch.guest_rip,
		vcpu->arch.guest_ins_len,
		vcpu->arch.exit_qualification);
}

static void handler_exit_ept_violation(struct st_vcpu *vcpu)
{
	set_exit_reason(vcpu->arch.exit_reason);

	switch (start_run_id) {
	case CASE_ID_41113:
		/* resume environment for case 41113 */
		ept_gpa2hpa_map(vcpu,
			HOST_PHY_ADDR_START,
			GUEST_PHYSICAL_ADDRESS_START,
			GUEST_MEMORY_MAP_SIZE,
			EPT_RA | EPT_WA | EPT_EA,
			false);
		vcpu_retain_rip(vcpu);
		invept(INVEPT_TYPE_ALL_CONTEXTS, vcpu->arch.eptp);
		DBG_INFO("resume environment success!");
		break;
	default:
		break;
	}

	DBG_INFO("<ept violation>exit reason:%d guest_rip:0x%lx ins_len:%d qualification:0x%x", \
		vcpu->arch.exit_reason,\
		vcpu->arch.guest_rip,
		vcpu->arch.guest_ins_len,
		vcpu->arch.exit_qualification);
	/** According SDM Table 27-7. Exit Qualification for EPT Violations
	 * bit0: Set if the access causing the EPT violation was a data read.
	 * bit1: Set if the access causing the EPT violation was a data write.
	 * bit3: Set if the access causing the EPT violation was an instruction fetch.
	 */
}

static void handler_exit_cr_access(struct st_vcpu *vcpu)
{
	set_exit_reason(vcpu->arch.exit_reason);
	DBG_INFO("exit reason:%d guest_rip:0x%lx ins_len:%d qualification:0x%x", \
		vcpu->arch.exit_reason,\
		vcpu->arch.guest_rip,
		vcpu->arch.guest_ins_len,
		vcpu->arch.exit_qualification);

	uint32_t idx = 0xff;
	vm_exit_info.exit_qualification = vcpu->arch.exit_qualification;
	/* According SDM:
	 * Table 27-3. Exit Qualification for Control-Register Accesses
	 * bit[3:0]: Number of control register
	 * bit[5:4]: Access type
	 * bit[11:8]: For MOV CR, the general-purpose register
	 */
	uint64_t cr_num_and_access = hsi_vm_exit_cr_access_cr_num(vm_exit_info.exit_qualification) |
		hsi_vm_exit_cr_access_type(vm_exit_info.exit_qualification);
	if ((hsi_vm_exit_cr_access_cr_num(vm_exit_info.exit_qualification) == VM_EXIT_CR3_NUM) ||
		(hsi_vm_exit_cr_access_cr_num(vm_exit_info.exit_qualification) == VM_EXIT_CR8_NUM)) {
		DBG_INFO("[%s]Exit Reason: 0x%x--exit_qualification: 0x%x",
			__func__, vcpu->arch.exit_reason, vcpu->arch.exit_qualification);

		DBG_INFO("cr_num_and_access:0x%lx\n", cr_num_and_access);
	}
	/**
	 * decode the VM-Exit reason to get the info which general-purpose register is used
	 * and assign the result to idx
	 */
	idx = (uint32_t)hsi_vm_exit_cr_access_reg_idx(vm_exit_info.exit_qualification);

	/** The idx should never be bigger than 15 */
	if (idx >= CPU_REG_BUFF) {
		DBG_ERRO("index out of range");
		return;
	}
	/** For MOV CR, set reg to the value from the general-purpose register indicated by idx */
	uint64_t reg = vcpu_get_gpreg(idx);

	switch (cr_num_and_access) {
	case (VM_EXIT_CR3_NUM | (ACCESS_TYPE_MOV_FROM_CR << 4)):
		vm_exit_info.cr_qualification = VM_EXIT_STORE_CR3;
		break;
	case (VM_EXIT_CR3_NUM | (ACCESS_TYPE_MOV_TO_CR << 4)):
		vm_exit_info.cr_qualification = VM_EXIT_LOAD_CR3;
		break;
	case (VM_EXIT_CR8_NUM | (ACCESS_TYPE_MOV_FROM_CR << 4)):
		vm_exit_info.cr_qualification = VM_EXIT_STORE_CR8;
		break;
	case (VM_EXIT_CR8_NUM | (ACCESS_TYPE_MOV_TO_CR << 4)):
		vm_exit_info.cr_qualification = VM_EXIT_LOAD_CR8;
		break;
	case (VM_EXIT_CR0_NUM | (ACCESS_TYPE_MOV_TO_CR << 4)):
		/* guest set CR0.CD */
		if (reg & X86_CR0_CD) {
			/* record the vm exit */
			vm_exit_info.is_vm_exit[VM_EXIT_WRITE_CR0_CD] = VM_EXIT_OCCURS;
			DBG_INFO("set cr0.CD reg:0x%lx\n", reg);
		} else if (reg & X86_CR0_MP) {
			vm_exit_info.is_vm_exit[VM_EXIT_WRITE_CR0_MP] = VM_EXIT_OCCURS;
		}
		break;
	case (VM_EXIT_CR4_NUM | (ACCESS_TYPE_MOV_TO_CR << 4)):
		/* guest set CR4.SMEP */
		if (reg & X86_CR4_SMEP) {
			/* record the vm exit */
			vm_exit_info.is_vm_exit[VM_EXIT_WRITE_CR4_SMEP] = VM_EXIT_OCCURS;
			debug_print("set cr0.CD reg:0x%lx\n", reg);
		} else if (reg & X86_CR4_PVI) {
			vm_exit_info.is_vm_exit[VM_EXIT_WRITE_CR4_PVI] = VM_EXIT_OCCURS;
		}
		break;
	default:
		vm_exit_info.cr_qualification = VM_EXIT_QUALIFICATION_OTHERS_CR;
		break;
	}
}

__unused static void handler_exit_read_msr(struct st_vcpu *vcpu)
{
	set_exit_reason(vcpu->arch.exit_reason);
	DBG_INFO("exit reason:%d guest_rip:0x%lx ins_len:%d qualification:0x%x", \
		vcpu->arch.exit_reason,\
		vcpu->arch.guest_rip,
		vcpu->arch.guest_ins_len,
		vcpu->arch.exit_qualification);

	/* Read the msr value */
	/**
	 * Set 'msr' to the return value of '(uint32_t)vcpu_get_gpreg(vcpu, CPU_REG_RCX)',
	 * which is the contents of ECX register associated with \a vcpu and it
	 * specifies the MSR to be read from.
	 */
	uint32_t msr = (uint32_t)vcpu_get_gpreg(CPU_REG_RCX);
	if (msr == TEST_MSR_BITMAP_MSR) {
		DBG_INFO("msr: 0x%x", msr);
		vm_exit_info.is_msr_ins_exit = VM_EXIT_OCCURS;
	}
	switch (msr) {
	case MSR_IA32_PAT:
		vm_exit_info.is_vm_exit[VM_EXIT_RDMSR_PAT] = VM_EXIT_OCCURS;
		DBG_INFO("[MSR_IA32_PAT]");
		break;
	case MSR_PLATFORM_INFO:
		vm_exit_info.is_vm_exit[VM_EXIT_RDMSR_PLATFROM] = VM_EXIT_OCCURS;
		DBG_INFO("[MSR_PLATFORM_INFO]");
		break;
	case MSR_IA32_EXT_APIC_ICR:
		vm_exit_info.is_vm_exit[VM_EXIT_RDMSR_APIC_ICR] = VM_EXIT_OCCURS;
		DBG_INFO("[MSR_IA32_EXT_APIC_ICR]");
		break;
	default:
		break;
	}
}

__unused static void handler_exit_write_msr(struct st_vcpu *vcpu)
{
	set_exit_reason(vcpu->arch.exit_reason);
	DBG_INFO("exit reason:%d guest_rip:0x%lx ins_len:%d qualification:0x%x", \
		vcpu->arch.exit_reason,\
		vcpu->arch.guest_rip,
		vcpu->arch.guest_ins_len,
		vcpu->arch.exit_qualification);

	/* Read the msr value */
	/**
	 * Set 'msr' to the return value of '(uint32_t)vcpu_get_gpreg(vcpu, CPU_REG_RCX)',
	 * which is the contents of ECX register associated with \a vcpu and
	 * it specifies the MSR to be write from.
	 */
	uint32_t msr = (uint32_t)vcpu_get_gpreg(CPU_REG_RCX);
	if (msr == TEST_MSR_BITMAP_MSR) {
		DBG_INFO("write msr TEST_MSR_BITMAP_MSR!");
		vm_exit_info.is_msr_ins_exit = VM_EXIT_OCCURS;
	} else if (msr == MSR_IA32_EXT_APIC_ICR) {
		DBG_INFO("write msr TEST_MSR_BITMAP_MSR!");
		vm_exit_info.is_wrmsr_apic_icr_exit = VM_EXIT_OCCURS;
	}

}

/* vm exit handler hook function */
__unused static void handler_exit_pio_instruction(struct st_vcpu *vcpu)
{
	DBG_INFO("exit reason:%d guest_rip:0x%lx ins_len:%d qualification:0x%x", \
		vcpu->arch.exit_reason,\
		vcpu->arch.guest_rip,
		vcpu->arch.guest_ins_len,
		vcpu->arch.exit_qualification);

	/* save exit information for test */
	vm_exit_info.exit_qualification = vcpu->arch.exit_qualification;
	set_exit_reason(vcpu->arch.exit_reason);
	/* According SDM:
	 * Table 27-5. Exit Qualification for I/O Instructions
	 * bit[2:0]: Size of access:
	 *			 0 = 1-byte
	 *			 1 = 2-byte
	 *			 3 = 4-byte
	 *			 Other values not used
	 * bit[3]: Direction of the attempted access (0 = OUT, 1 = IN)
	 * bit[31:16]: Port number (as specified in DX or in an immediate operand)
	 */
	uint64_t access_port_num = hsi_vm_exit_io_access_port_num(vm_exit_info.exit_qualification);
	uint8_t access_size = (vm_exit_info.exit_qualification & 0x7);
	uint8_t access_type = ((vm_exit_info.exit_qualification >> 3) & 0x1);
	if ((access_size == IO_ACCESS_SIZE_4BYTE) &&
		(access_type == INS_TYPE_IN)) {
		switch (access_port_num) {
		case IN_BYTE_TEST_PORT_ID:
			vm_exit_info.is_io_ins_exit = VM_EXIT_OCCURS;
			break;
		case IO_BITMAP_TEST_PORT_NUM1:
			vm_exit_info.is_io_num1_exit = VM_EXIT_OCCURS;
			break;
		case IO_BITMAP_TEST_PORT_NUM2:
			vm_exit_info.is_io_num2_exit = VM_EXIT_OCCURS;
			break;
		default:
			break;
		}
	}
}

/* create condition function */
/* pin base */
BUILD_CONDITION_FUNC(virt_nmi, PIN_CONTROLS, \
	PIN_VIRT_NMI, BIT_TYPE_CLEAR);

static void exter_inter_condition(__unused struct st_vcpu *vcpu)
{
	exec_vmwrite32_bit(PIN_CONTROLS, PIN_EXTINT, BIT_TYPE_CLEAR);
	msr_passthroudh();
}

BUILD_CONDITION_FUNC(nmi_exit, PIN_CONTROLS, \
	PIN_NMI, BIT_TYPE_CLEAR);

static void posted_inter_condition(__unused struct st_vcpu *vcpu)
{
	/* Disable process posted interrupts exiting in pin-based execute controls */
	exec_vmwrite32_bit(PIN_CONTROLS,
		PIN_POST_INTR, BIT_TYPE_CLEAR);
	DBG_INFO("condition:PIN_CONTROLS:%lx", vmcs_read(PIN_CONTROLS));

	/* set posted vector to 0xe3 */
	vmcs_write(VMX_POSTED_INTERRUPT_NOTIFICATON_VECTOR_FULL,
		POSTED_INTR_VECTOR);

	struct st_pi_desc *pi_desc = &vcpu->arch.pid;
	memset(pi_desc, 0, sizeof(struct st_pi_desc));
	/* init notification vector according
	 * SDM Table 29-1. Format of Posted-Interrupt Descriptor
	 * bit[255:0] One bit for each interrupt vector.
	 * There is a posted-interrupt request for a vector if
	 * the corresponding bit is 1
	 */
	/* set posted-interrupt descriptor bitmap , vector is 0xa0 */
	uint32_t pi_req_index = POSTED_INTR_REQUEST_VECTOR / 64;
	pi_desc->pi_req_bitmap[pi_req_index] |=
		(1ULL << (POSTED_INTR_REQUEST_VECTOR % 64));
	DBG_INFO("pi_req_bitmap[%d]:0x%lx\n", pi_req_index, pi_desc->pi_req_bitmap[pi_req_index]);

	/* set posted interrupt descriptor table address */
	vmcs_write(POSTED_INTR_DESC_ADDR, hva2hpa(pi_desc));
}

/* vm execute control1 */
BUILD_CONDITION_FUNC(irq_window, CPU_EXEC_CTRL0, \
	CPU_INTR_WINDOW, BIT_TYPE_CLEAR);

static void use_tpr_condition(__unused struct st_vcpu *vcpu)
{
	/*APIC-v, config APIC virtualized page address*/
	vcpu->arch.vlapic.apic_page.tpr.v = APIC_TPR_VAL2;
	u64 value64 = hva2hpa(&vcpu->arch.vlapic.apic_page);
	vmcs_write(VMX_VIRTUAL_APIC_PAGE_ADDR_FULL, value64);

	/* Disable Use TPR shadow at primary processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL0, CPU_TPR_SHADOW, BIT_TYPE_CLEAR);
	DBG_INFO("CPU_EXEC_CTRL0:0x%x\n", exec_vmread32(CPU_EXEC_CTRL0));

	/**
	 * set APIC TPR register through MSR
	 */
	 wrmsr(MSR_IA32_EXT_APIC_TPR, APIC_TPR_VAL1);
}

BUILD_CONDITION_FUNC(hlt_exit, CPU_EXEC_CTRL0, \
	CPU_HLT, BIT_TYPE_CLEAR);

void ept_gpa2hpa_map(struct st_vcpu *vcpu, u64 hpa, u64 gpa,
		u32 size, u64 permission, bool is_clear)
{
	if (is_clear == true) {
		/* clear the mapping memory */
		memset((void *)hpa, 0, size);
	}

	u64 max = hpa + size;
	/* Map 4M memory from gpa to hpa */
	while ((hpa + PAGE_SIZE) <= max) {
		install_ept(vcpu->arch.pml4, hpa, gpa, permission);
		hpa += PAGE_SIZE;
		gpa += PAGE_SIZE;
	}
}
/* vm execute control2 */
static void ept_enable_condition(__unused struct st_vcpu *vcpu)
{
	/* Enable EPT in second processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL1, CPU_EPT, BIT_TYPE_SET);
	DBG_INFO("condition:exec_ctrl1:%lx", vmcs_read(CPU_EXEC_CTRL1));

	u64 start_hpa = HOST_PHY_ADDR_START;
	u64 start_gpa = GUEST_PHYSICAL_ADDRESS_START;
	u32 size = GUEST_MEMORY_MAP_SIZE;

	ept_gpa2hpa_map(vcpu, start_hpa, start_gpa, size,
		EPT_RA | EPT_WA | EPT_EA, true);

	/* According SDM Table 24-8. Format of Extended-Page-Table Pointer.
	 * set eptp to vmcs, WB memory type.
	 */
	u64 eptp = hva2hpa(vcpu->arch.pml4) | (3UL << 3U) | 6UL;
	vmcs_write(EPTP, eptp);

	invept(INVEPT_TYPE_ALL_CONTEXTS, eptp);
}

static void vpid_enable_condition(__unused struct st_vcpu *vcpu)
{
	/* Enable VPID at secondary processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL1, CPU_VPID, BIT_TYPE_SET);
	DBG_INFO("condition:exec_ctrl1:%lx", vmcs_read(CPU_EXEC_CTRL1));

	/* Set VPID to VMCS */
	vcpu->arch.vpid = CPU_BP_ID + 1;
	vmcs_write(VPID, vcpu->arch.vpid);
}

static void pause_loop_condition(__unused struct st_vcpu *vcpu)
{
	/* Disable pause exiting at primary processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL0, CPU_PAUSE, BIT_TYPE_SET);

	/* Disable PAUSE-loop exiting in second processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL1, CPU_PAUSE_LOOP, BIT_TYPE_CLEAR);
	DBG_INFO("condition:exec_ctrl1:%lx", vmcs_read(CPU_EXEC_CTRL1));

	/* Set PAUSE-loop exiting gap and window TSC time
	 * ple gap TSC time is 100 big enough for two pause instruction execute time.
	 * ple window TSC time is 1000 small enough for execute pause loop.
	 */
	exec_vmwrite32(VMX_PAUSE_LOOP_PLE_GAP, VMX_PLE_GAP_TSC);
	exec_vmwrite32(VMX_PAUSE_LOOP_PLE_WINDOW, VMX_PLE_WINDOW_TSC);
}

static void ept_violation_condition(__unused struct st_vcpu *vcpu)
{
	/* Disable EPT-violation #VE in second processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL1, CPU_EPT_VIOLATION, BIT_TYPE_CLEAR);
	DBG_INFO("condition:exec_ctrl1:%lx", vmcs_read(CPU_EXEC_CTRL1));

	u64 start_hpa = HOST_PHY_ADDR_START;
	u64 start_gpa = GUEST_PHYSICAL_ADDRESS_START;
	u32 size = GUEST_MEMORY_MAP_SIZE;

	/* map 4M memory from gpa to hpa */
	ept_gpa2hpa_map(vcpu, start_hpa, start_gpa, size,
		EPT_RA | EPT_EA, true);

	u64 eptp = vcpu->arch.eptp;
	vmcs_write(EPTP, eptp);

	invept(INVEPT_TYPE_ALL_CONTEXTS, eptp);
}

static void ept_exe_contrl_condition(__unused struct st_vcpu *vcpu)
{
	/* Disable Mode-based execute control for EPT in second processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL1, CPU_MODE_BASE_EXE_CON_FOR_EPT, BIT_TYPE_CLEAR);
	DBG_INFO("condition:exec_ctrl1:%lx", vmcs_read(CPU_EXEC_CTRL1));

	u64 start_hpa = HOST_PHY_ADDR_START;
	u64 start_gpa = GUEST_PHYSICAL_ADDRESS_START;
	u32 size = GUEST_MEMORY_MAP_SIZE;

	/* map 4M memory from gpa to hpa */
	/**
	 * Clear execute access bit(bit 2) to 0,
	 * set execute access for user-mode address bit(bit 10) to 1
	 * if Mode-based execute control is 1, then instruction fetches
	 * are allowed from user-mode address.
	 * otherwise, instruction fetches are not allowed.
	 */
	ept_gpa2hpa_map(vcpu, start_hpa, start_gpa, size,
		(EPT_RA | EPT_WA | EPT_EXE_FOR_USER_MODE) & ~EPT_EA, true);

	u64 eptp = vcpu->arch.eptp;
	vmcs_write(EPTP, eptp);

	invept(INVEPT_TYPE_ALL_CONTEXTS, eptp);
}

BUILD_CONDITION_FUNC(enable_vmfunc, CPU_EXEC_CTRL1, \
	CPU_VMFUNC, BIT_TYPE_CLEAR);

/* vm exit condition function */
static void vm_exit_save_debug_condition(__unused struct st_vcpu *vcpu)
{
	/* clear VMCS VM-Exit controls bit2(save debug controls) to 0*/
	exec_vmwrite32_bit(EXI_CONTROLS,
		EXI_SAVE_DBGCTLS, BIT_TYPE_CLEAR);
	DBG_INFO("EXI_CONTROLS:0x%x\n", exec_vmread32(EXI_CONTROLS));

	exec_vmwrite32_bit(CPU_EXEC_CTRL0,
		CPU_MOV_DR, BIT_TYPE_CLEAR);

	/*
	 * Set guest DR7 value in vmcs for vm-exit save.
	 */
	vmcs_write(VMX_GUEST_DR7, GUEST_DR7_VALUE1);
	DBG_INFO("VMX_GUEST_DR7:0x%lx\n", vmcs_read(VMX_GUEST_DR7));
}

/* vm entry condition function */
static void vm_entry_load_perf_condition(__unused struct st_vcpu *vcpu)
{
	/* clear VMCS VM-Entry controls bit13(load IA32_PERF_GLOBAL_CTRL) to 0*/
	exec_vmwrite32_bit(ENT_CONTROLS,
		ENT_LOAD_PERF, BIT_TYPE_CLEAR);
	DBG_INFO("ENT_CONTROLS:0x%x\n", exec_vmread32(ENT_CONTROLS));

	/*
	 * Set guest IA32_PERF_GLOBAL_CTRL msr in vmcs for vm-entry load.
	 */
	vmcs_write(VMX_GUEST_IA32_PERF_CTL_FULL, GUEST_IA32_PERF_INIT_VALUE);
	DBG_INFO("VMX_HOST_IA32_PERF_CTL_FULL:0x%lx\n",
		vmcs_read(VMX_HOST_IA32_PERF_CTL_FULL));

	msr_passthroudh();
	/* init IA32_PERF_GLOBAL_CTRL value in host */
	wrmsr(MSR_IA32_PERF_GLOBAL_CTRL, IA32_PERF_HOST_SET_VALUE);
}

/* cr mask and shadow */
static void cr0_mask_condition(__unused struct st_vcpu *vcpu)
{
	/*
	 * Set CR0.CD to 1 and clear CR0.MP to 0
	 * in Guest/Host masks for CR0 for test condition.
	 */
	uint64_t default_mask = vmcs_read(VMX_CR0_GUEST_HOST_MASK);
	default_mask = (default_mask | X86_CR0_CD) & ~X86_CR0_MP;
	vmcs_write(VMX_CR0_GUEST_HOST_MASK, default_mask);
	DBG_INFO("VMX_CR0_GUEST_HOST_MASK:0x%lx\n", vmcs_read(VMX_CR0_GUEST_HOST_MASK));

	/* clear CR0.CD, CR0.MP in read shadow for CR0 */
	uint64_t default_shadow = vmcs_read(VMX_CR0_READ_SHADOW);
	default_shadow = (default_shadow & ~X86_CR0_CD) & ~X86_CR0_MP;
	vmcs_write(VMX_CR0_READ_SHADOW, default_shadow);
	DBG_INFO("VMX_CR0_READ_SHADOW:0x%lx\n", vmcs_read(VMX_CR0_READ_SHADOW));
}

void set_rdmsr_low_msr_bitmap(uint8_t *msr_bitmap, uint32_t msr, uint32_t is_bit_set)
{
	/* Set 'msr_index' to 1/8 of 'msr' (round down) */
	uint32_t test_msr_index = msr / 8;
	uint8_t test_msr_bit = msr % 8;
	if (is_bit_set == BIT_TYPE_SET) {
		msr_bitmap[test_msr_index] |= (0x1 << test_msr_bit);
	} else {
		msr_bitmap[test_msr_index] &= ~(0x1 << test_msr_bit);
	}
	DBG_INFO("msr_bitmap[%d]:0x%x  test_msr_bit:%d",\
		test_msr_index,\
		msr_bitmap[test_msr_index],\
		test_msr_bit);
}

static void msr_bitmap_condition(__unused struct st_vcpu *vcpu)
{
	/*
	 * Enable VM_EXIT for test condition.
	 */
	exec_vmwrite32_bit(CPU_EXEC_CTRL0, CPU_MSR_BITMAP, BIT_TYPE_SET);
	DBG_INFO("CPU_EXEC_CTRL0:0x%x\n", exec_vmread32(CPU_EXEC_CTRL0));

	uint8_t *msr_bitmap = vcpu->arch.msr_bitmap;
	memset(msr_bitmap, 0x0, PAGE_SIZE);

	/* Set up read bitmap for low MSR_IA32_PAT, enable msr access vm exit */
	set_rdmsr_low_msr_bitmap(msr_bitmap, MSR_IA32_PAT, BIT_TYPE_SET);

	/* Clear read bitmap for low MSR_PLATFORM_INFO, disable msr access vm exit */
	set_rdmsr_low_msr_bitmap(msr_bitmap, MSR_PLATFORM_INFO, BIT_TYPE_CLEAR);

	uint64_t value64 = hva2hpa((void *)msr_bitmap);
	vmcs_write(VMX_MSR_BITMAP_FULL, value64);

}

static void ept_construct_condition(__unused struct st_vcpu *vcpu)
{
	u64 start_hpa = HOST_PHY_ADDR_START;
	u64 start_gpa = GUEST_PHYSICAL_ADDRESS_START;
	u32 size = GUEST_MEMORY_MAP_SIZE;
	vm_exit_info.result = RET_BUFF;

	ept_gpa2hpa_map(vcpu, start_hpa, start_gpa, size,
		EPT_RA | EPT_WA | EPT_EA, true);

	/* According SDM Table 24-8. Format of Extended-Page-Table Pointer.
	 * set eptp to vmcs, WB memory type.
	 */
	u64 eptp = hva2hpa(vcpu->arch.pml4) | (3UL << 3U) | 6UL;
	vmcs_write(EPTP, eptp);

	/* check EPTP in VMCS */
	if (vmcs_read(EPTP) == eptp) {
		vm_exit_info.result = RET_CHECK_EPT_CONSTRUCT;
	}

}

static void vpid_invalid_condition(__unused struct st_vcpu *vcpu)
{
	/* Set init VPID(value is 1) to VMCS */
	vmcs_write(VPID, vcpu->arch.vpid);
	DBG_INFO("vpid:%d", vcpu->arch.vpid);
}

static void entry_guest_msr_control_condition(__unused struct st_vcpu *vcpu)
{
	/**
	 * Set vcpu->arch.msr_area.guest[MSR_AREA_TSC_AUX].msr_index to MSR_IA32_TSC_AUX, which is the MSR index
	 * in the MSR Entry to be loaded on VM entry
	 */
	vcpu->arch.msr_area.guest[MSR_AREA_TSC_AUX].msr_index = MSR_IA32_TSC_AUX;
	/**
	 * Set vcpu->arch.msr_area.guest[MSR_AREA_TSC_AUX].value to TSC_AUX_VALUE1, which is the virtual CPU ID
	 * associated with \a vcpu and the MSR data in the MSR Entry to be loaded on VM entry
	 */
	vcpu->arch.msr_area.guest[MSR_AREA_TSC_AUX].value = TSC_AUX_VALUE1;

	/* Set up VMX entry MSR load count - pg 2908 24.8.2 Tells the number of
	 * MSRs on load from memory on VM entry from mem address provided by
	 * VM-entry MSR load address field
	 */
	vmcs_write(VMX_ENTRY_MSR_LOAD_COUNT, MSR_AREA_COUNT);
	vmcs_write(VMX_ENTRY_MSR_LOAD_ADDR_FULL, hva2hpa((void *)vcpu->arch.msr_area.guest));
	DBG_INFO(" set vm-entry guest msr");
}

static void tsc_offset_multi_condition(__unused struct st_vcpu *vcpu)
{
	/* Disable RDTSC exiting in primary processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL0,
		CPU_RDTSC, BIT_TYPE_CLEAR);

	/* Enable use TSC offset in primary processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL0,
		CPU_TSC_OFFSET, BIT_TYPE_SET);
	DBG_INFO("CPU_EXEC_CTRL0:0x%x\n", exec_vmread32(CPU_EXEC_CTRL0));

	/* set TSC offset */
	vmcs_write(VMX_TSC_OFFSET_FULL, TSC_OFFSET_DEFAULT_VALUE);

	/* Disable Use TSC scaling at second processor-based */
	vmcs_clear_bits(CPU_EXEC_CTRL1, CPU_TSC_SCALLING);
	DBG_INFO("CPU_EXEC_CTRL1:0x%x\n", exec_vmread32(CPU_EXEC_CTRL1));

	/* set TSC scaling */
	vmcs_write(VMX_TSC_MULTIPLIER_FULL, TSC_SCALING_DEFAULT_VALUE);

	/* clear real TSC to 0 */
	wrmsr(MSR_IA32_TIME_STAMP_COUNTER, 0);

}

/* at host check ept mapping right */
static void ept_enable_check(__unused struct st_vcpu *vcpu)
{
	u32 i;
	uint32_t *test_hpa = (uint32_t *)HOST_PHY_ADDR_START;
	bool chk_val = true;
	uint32_t size = GUEST_MEMORY_MAP_SIZE / sizeof(uint32_t);
	/* Check guest write memory is success */
	for (i = 0; i < size; i++) {
		if (test_hpa[i] != i) {
			chk_val = false;
			DBG_ERRO("test_hpa[%d]:%d\n", i, test_hpa[i]);
			break;
		}
	}
	if (chk_val) {
		vm_exit_info.ept_map_flag = EPT_MAPPING_RIGHT;
		DBG_INFO("check ept enable success!");
	}
}

/* at host check guest dr7 vmcs field unchange */
static void exit_save_debug_check(__unused struct st_vcpu *vcpu)
{
	vm_exit_info.result = RET_BUFF;
	if (vmcs_read(VMX_GUEST_DR7) == GUEST_DR7_VALUE1) {
		vm_exit_info.result = RET_EXIT_SAVE_DEBUG;
		DBG_INFO(" vm exit guest dr7 check success!");
	}
}

/* at host check ept mapping right */
static void vm_exit_entry_do_nothing(__unused struct st_vcpu *vcpu)
{
	DBG_INFO("do nothing!");
}

static void hypercall_change_mem_mapping(__unused struct st_vcpu *vcpu)
{
	/* map gpa(448M) to hpa(480M)*/
	ept_gpa2hpa_map(vcpu,
		HOST_PHYSICAL_ADDRESS_OFFSET,
		GUEST_PHYSICAL_ADDRESS_TEST,
		GUEST_MEMORY_MAP_SIZE,
		EPT_RA | EPT_WA | EPT_EA,
		false);
}

static void hypercall_invept(__unused struct st_vcpu *vcpu)
{
	invept(INVEPT_TYPE_ALL_CONTEXTS, vcpu->arch.eptp);
}

static void hypercall_invvpid(__unused struct st_vcpu *vcpu)
{
	invvpid(VMX_VPID_TYPE_ALL_CONTEXT, vcpu->arch.vpid, 0);
}

static void hypercall_change_guest_msr_area(__unused struct st_vcpu *vcpu)
{
	/*
	 * Set guest msr tsc_aux value in vmcs for vm-entry load.
	 */
	vcpu->arch.msr_area.guest[MSR_AREA_TSC_AUX].value = TSC_AUX_VALUE2;
	DBG_INFO("set guest msr for vm-entry load\n");
}

static void ap1_virt_nmi_test(void)
{
	debug_print("\n ap1 send nmi ipi to bp %s\n", __func__);

	/* send nmi ipi to bp */
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_NMI, CPU_BP_ID);

	/*wait bp in nmi handler*/
	while (ap_send_nmi_flag == 0) {
		nop();
	}

	/* send nmi ipi to bp again*/
	apic_icr_write(APIC_DEST_PHYSICAL | APIC_DM_NMI, CPU_BP_ID);
}

int ap_main(void)
{
	/***********************************/
	/* in native ap1,ap2,ap3 lapic id is 2,4,6 */
	u32 cpu_id = get_lapic_id() / 2;
	printf("###########ap %d starts running \n", cpu_id);
	while (start_run_id == 0) {
		test_delay(5);
	}

	/* ap test */
	while (start_run_id != CASE_ID_42244) {
		test_delay(5);
	}

	if (cpu_id == CPU_AP1_ID) {
		ap1_virt_nmi_test();
	}
	return 0;
}

st_vm_exit vm_exit[VMX_EXIT_REASON_NUM] = {
	[VMX_EXTINT] = {
		.exit_func = handler_exit_common},
	[VMX_INTR_WINDOW] = {
		.exit_func = handler_exit_common},
	[VMX_EXC_NMI] = {
		.exit_func = handler_exit_common},
	[VMX_HLT] = {
		.exit_func = handler_exit_common},
	[VMX_PAUSE] = {
		.exit_func = handler_exit_common},
	[VMX_INVLPG] = {
		.exit_func = handler_exit_common},
	[VMX_MWAIT] = {
		.exit_func = handler_exit_common},
	[VMX_RDTSC] = {
		.exit_func = handler_exit_common},
	[VMX_RDPMC] = {
		.exit_func = handler_exit_common},
	[VMX_DR] = {
		.exit_func = handler_exit_common},
	[VMX_MONITOR] = {
		.exit_func = handler_exit_common},
	[VMX_NMI_WINDOW] = {
		.exit_func = handler_exit_common},
	[VMX_PREEMPT] = {
		.exit_func = handler_exit_common},
	[VMX_MTF] = {
		.exit_func = handler_exit_common},

	[VMX_EPT_VIOLATION] = {
		.exit_func = handler_exit_ept_violation},
	[VMX_CR] = {
		.exit_func = handler_exit_cr_access},
	[VMX_RDMSR] = {
		.exit_func = handler_exit_read_msr},
	[VMX_WRMSR] = {
		.exit_func = handler_exit_write_msr},
	[VMX_IO] = {
		.exit_func = handler_exit_pio_instruction},
};

/* define for make test condition in root operation mode */
st_vm_exit vmcall_exit[] = {
	[CON_PIN_CLEAR_EXTER_INTER_BIT] = {
		.exit_func = exter_inter_condition},
	[CON_CPU_CTRL1_IRQ_WIN] = {
		.exit_func = irq_window_condition},
	[CON_PIN_CLEAR_NMI_EXIT_BIT] = {
		.exit_func = nmi_exit_condition},
	[CON_CPU_CTRL1_TPR_SHADOW] = {
		.exit_func = use_tpr_condition},
	[CON_CPU_CTRL1_HLT_EXIT] = {
		.exit_func = hlt_exit_condition},
	[CON_CPU_CTRL1_INVLPG] = {
		.exit_func = invlpg_exit_condition},
	[CON_CPU_CTRL1_MWAIT] = {
		.exit_func = mwait_exit_condition},
	[CON_CPU_CTRL1_RDTSC] = {
		.exit_func = rdtsc_exit_condition},
	[CON_CPU_CTRL1_RDPMC] = {
		.exit_func = rdpmc_exit_condition},
	[CON_CPU_CTRL1_MOV_DR] = {
		.exit_func = mov_dr_exit_condition},
	[CON_CPU_CTRL1_UNCONDITIONAL_IO] = {
		.exit_func = unconditional_io_exit_condition},
	[CON_CPU_CTRL1_USE_IO_BITMAP] = {
		.exit_func = io_bitmap_exit_condition},
	[CON_CPU_CTRL1_USE_MSR_BITMAP_READ] = {
		.exit_func = msr_bitmap_rdmsr_exit_condition},
	[CON_CPU_CTRL1_USE_MSR_BITMAP_WRITE] = {
		.exit_func = msr_bitmap_wrmsr_exit_condition},
	[CON_CPU_CTRL1_ACTIVATE_SECOND_CONTROL] = {
		.exit_func = activate_secon_con_condition},
	[CON_CPU_CTRL1_NMI_WIN] = {
		.exit_func = nmi_window_condition},
	[CON_CPU_CTRL1_PREEMPTION_TIMER] = {
		.exit_func = active_preemption_timer_condition},
	[CON_CPU_CTRL1_MONITOR_TRP_FLAG] = {
		.exit_func = monitor_trap_flag_condition},
	[CON_CPU_CTRL1_TSC_OFFSETTING] = {
		.exit_func = use_tsc_offset_condition},

	[CON_CPU_CTRL1_CR8_STORE] = {
		.exit_func = cr8_store_exit_condition},
	[CON_CPU_CTRL1_CR8_LOAD] = {
		.exit_func = cr8_load_exit_condition},
	[CON_CPU_CTRL1_CR3_STORE] = {
		.exit_func = cr3_store_exit_condition},
	[CON_CPU_CTRL1_CR3_LOAD] = {
		.exit_func = cr3_load_exit_condition},
	[CON_CPU_CTRL1_PAUSE] = {
		.exit_func = pause_exit_condition},
	[CON_CPU_CTRL1_MONITOR] = {
		.exit_func = monitor_exit_condition},

	[CON_PIN_POST_INTER] = {
		.exit_func = posted_inter_condition},
	[CON_PIN_VIRT_NMI] = {
		.exit_func = virt_nmi_condition},
	[CON_CPU_CTRL2_ENABLE_EPT] = {
		.exit_func = ept_enable_condition},
	[CON_CPU_CTRL2_ENABLE_VPID] = {
		.exit_func = vpid_enable_condition},
	[CON_CPU_CTRL2_PAUSE_LOOP] = {
		.exit_func = pause_loop_condition},
	[CON_CPU_CTRL2_ENABLE_VM_FUNC] = {
		.exit_func = enable_vmfunc_condition},
	[CON_CPU_CTRL2_EPT_VIOLATION] = {
		.exit_func = ept_violation_condition},
	[CON_CPU_CTRL2_EPT_EXE_CONTRL] = {
		.exit_func = ept_exe_contrl_condition},
	[CON_EXIT_SAVE_DEBUG] = {
		.exit_func = vm_exit_save_debug_condition},
	[CON_ENTRY_LOAD_PERF] = {
		.exit_func = vm_entry_load_perf_condition},
	[CON_CR0_MASK] = {
		.exit_func = cr0_mask_condition},
	[CON_MSR_BITMAP] = {
		.exit_func = msr_bitmap_condition},
	[CON_EPT_CONSTRUCT] = {
		.exit_func = ept_construct_condition},
	[CON_VPID_INVALID] = {
		.exit_func = vpid_invalid_condition},
	[CON_ENTRY_GUEST_MSR_CONTRL] = {
		.exit_func = entry_guest_msr_control_condition},
	[CON_EXIT_GUEST_MSR_STORE] = {
		.exit_func = exit_guest_msr_store_condition},
	[CON_EXIT_HOST_MSR_CONTRL] = {
		.exit_func = exit_host_msr_control_condition},
	[CON_TSC_OFFSET_MULTI] = {
		.exit_func = tsc_offset_multi_condition},
	[CON_VM_EXIT_INFO] = {
		.exit_func = vm_exit_info_condition},
};


/* define for check result or make other test second condition */
st_vm_exit vmcall_exit_second[] = {
	[(CON_SECOND_CHECK_EPT_ENABLE & ~VMCALL_EXIT_SECOND)] = {
		.exit_func = ept_enable_check},
	[(CON_SECOND_VM_EXIT_ENTRY & ~VMCALL_EXIT_SECOND)] = {
		.exit_func = vm_exit_entry_do_nothing},
	[(CON_SECOND_CHECK_DEBUG_REG & ~VMCALL_EXIT_SECOND)] = {
		.exit_func = exit_save_debug_check},
	[(CON_CHANGE_MEMORY_MAPPING & ~VMCALL_EXIT_SECOND)] = {
		.exit_func = hypercall_change_mem_mapping},
	[(CON_INVEPT & ~VMCALL_EXIT_SECOND)] = {
		.exit_func = hypercall_invept},
	[(CON_INVVPID & ~VMCALL_EXIT_SECOND)] = {
		.exit_func = hypercall_invvpid},
	[(CON_CHANGE_GUEST_MSR_AREA & ~VMCALL_EXIT_SECOND)] = {
		.exit_func = hypercall_change_guest_msr_area},
	[(CON_CHECK_TSC_AUX_GUEST_VMCS_FIELD & ~VMCALL_EXIT_SECOND)] = {
		.exit_func = hypercall_check_guest_tsc_aux_vmcs},
	[(CON_CHECK_TSC_AUX_VALUE & ~VMCALL_EXIT_SECOND)] = {
		.exit_func = hypercall_check_tsc_aux_load},
	[(CON_ENTRY_EVENT_INJECT & ~VMCALL_EXIT_SECOND)] = {
		.exit_func = hypercall_entry_event_inject},
};
st_case_suit case_suit[] = {
	{
		.condition = CON_BUFF,
		.func =
		hsi_rqmid_41182_virtualization_specific_features_VMX_instruction_002,
		.case_id = 41182,
		.pname = "HSI_Virtualization_Specific_Features_VMX_Instruction_002",
	},
	GET_CASE_INFO(CON_CPU_CTRL2_ENABLE_VM_FUNC, 41081, enable_vm_func, Enable_VM_Functions),
	GET_CASE_INFO(CON_CPU_CTRL2_PAUSE_LOOP, 41078, pause_loop, PAUSE_Loop),
	GET_CASE_INFO(CON_PIN_CLEAR_EXTER_INTER_BIT, 40364, external_inter, External_Interrupt),
	GET_CASE_INFO(CON_CPU_CTRL1_IRQ_WIN, 40391, irq_window, Interrupt_Window),
	GET_CASE_INFO(CON_PIN_CLEAR_NMI_EXIT_BIT, 40369, nmi_inter, NMI_Interrupt),
	GET_CASE_INFO(CON_CPU_CTRL1_TPR_SHADOW, 41089, use_tpr_shadow, Use_TRP_Shadow),
	GET_CASE_INFO(CON_CPU_CTRL1_HLT_EXIT, 40394, hlt, HLT),
	GET_CASE_INFO(CON_CPU_CTRL1_INVLPG, 40361, invlpg, INVLPG),
	GET_CASE_INFO(CON_CPU_CTRL1_MWAIT, 40395, mwait, MWAIT),
	GET_CASE_INFO(CON_CPU_CTRL1_RDTSC, 40465, rdtsc, RDTSC),
	GET_CASE_INFO(CON_CPU_CTRL1_RDPMC, 40466, rdpmc, RDPMC),
	GET_CASE_INFO(CON_CPU_CTRL1_MOV_DR, 40467, mov_dr, MOV_DR),
	GET_CASE_INFO(CON_CPU_CTRL1_CR8_STORE, 40485, cr8_store, CR8_STORE),
	GET_CASE_INFO(CON_CPU_CTRL1_CR8_LOAD, 40486, cr8_load, CR8_LOAD),
	GET_CASE_INFO(CON_CPU_CTRL1_CR3_STORE, 40488, cr3_store, CR3_STORE),
	GET_CASE_INFO(CON_CPU_CTRL1_CR3_LOAD, 40487, cr3_load, CR3_LOAD),
	GET_CASE_INFO(CON_CPU_CTRL1_PAUSE, 40519, pause, PAUSE),
	GET_CASE_INFO(CON_CPU_CTRL1_MONITOR, 40521, monitor, MONITOR),
	GET_CASE_INFO(CON_CPU_CTRL1_UNCONDITIONAL_IO, 40492, unconditional_io, Unconditional_IO),
	GET_CASE_INFO(CON_CPU_CTRL1_USE_IO_BITMAP, 40495, io_bitmap, IO_Bitmap),
	GET_CASE_INFO(CON_CPU_CTRL1_USE_MSR_BITMAP_READ, 40501, msr_bitmap_rdmsr, MSR_Bitmap_RDMSR),
	GET_CASE_INFO(CON_CPU_CTRL1_USE_MSR_BITMAP_WRITE, 40503, msr_bitmap_wrmsr, MSR_Bitmap_WRMSR),
	GET_CASE_INFO(CON_CPU_CTRL1_ACTIVATE_SECOND_CONTROL, 40621, activate_secon_con, Activate_Secondary_Controls),
	GET_CASE_INFO(CON_CPU_CTRL1_NMI_WIN, 41088, nmi_window, NMI_Window_Exiting),
	GET_CASE_INFO(CON_CPU_CTRL1_PREEMPTION_TIMER, 41091, active_preemption_timer, Activate_VMX_Preemption_Timer),
	GET_CASE_INFO(CON_CPU_CTRL1_MONITOR_TRP_FLAG, 41121, monitor_trap_flag, Monitor_Trap_Flag),
	GET_CASE_INFO(CON_CPU_CTRL1_TSC_OFFSETTING, 41175, use_tsc_offset, Use_TSC_Offsetting),

	GET_CASE_INFO(CON_PIN_POST_INTER, 41181, posted_inter, Process_Posted_interrupts),
	GET_CASE_INFO(CON_PIN_VIRT_NMI, 42244, virt_nmis, Virtual_NMIS),
};

st_case_suit case_mem_suit[] = {
	GET_CASE_INFO(CON_CPU_CTRL2_ENABLE_EPT, 41120, enable_ept, Enable_EPT),
	GET_CASE_INFO(CON_CPU_CTRL2_ENABLE_VPID, 40638, enable_vpid, Enable_VPID),
	GET_CASE_INFO(CON_CPU_CTRL2_EPT_EXE_CONTRL, 41113, excute_for_ept, Mode-based_Execute_Control_For_EPT),
	GET_CASE_INFO(CON_CPU_CTRL2_EPT_VIOLATION, 41110, ept_violation, EPT_Violation),
	GET_CASE_INFO_GENERAL(CON_EPT_CONSTRUCT, 42209, ept_construct, EPT_Construct),
	GET_CASE_INFO_GENERAL(CON_BUFF, 42224, ept_invalid, EPT_Invalid),
	GET_CASE_INFO_GENERAL(CON_VPID_INVALID, 42226, vpid_invalid, VPID_Invalid),
};

st_case_suit case_exit_entry[] = {
	GET_CASE_INFO_GENERAL(CON_EXIT_SAVE_DEBUG, 42176, save_debug_exit, VM_Exit_Control_Save_Debug),
	GET_CASE_INFO_GENERAL(CON_ENTRY_LOAD_PERF, 42179, entry_load_perf, VM_Entry_Control_Load_IA32_PERF),
	GET_CASE_INFO_GENERAL(CON_ENTRY_GUEST_MSR_CONTRL, 42236, \
		entry_guest_msr_control, VM_Entry_MSR_Control_Guest_Load),
	GET_CASE_INFO_GENERAL(CON_EXIT_HOST_MSR_CONTRL, 42237, \
		exit_host_msr_load, VM_Exit_MSR_Control_Host_Load),
	GET_CASE_INFO_GENERAL(CON_EXIT_GUEST_MSR_STORE, 42238, \
		exit_guest_msr_store, VM_Exit_MSR_Control_Guest_Store),
};

st_case_suit case_general[] = {
	GET_CASE_INFO_GENERAL(CON_CR0_MASK, 41946, cr0_masks, Guest_Host_Masks_For_CR0),
	GET_CASE_INFO(CON_MSR_BITMAP, 41085, bitmap_msr, MSR_Bitmap),
	GET_CASE_INFO_GENERAL(CON_TSC_OFFSET_MULTI, 42201,\
		tsc_offset_multi, TSC_Offsetting_Multiplier),
	GET_CASE_INFO_GENERAL(CON_VM_EXIT_INFO, 42242,\
		vm_exit_info, VM_Exit_Information),
	GET_CASE_INFO_GENERAL(CON_BUFF, 42250,\
		entry_event_inject, VM_Entry_Control_For_Event_Injection),

};

typedef struct {
	st_case_suit *plist;
	u32 size;
} st_case_list;
static st_case_list case_list[] = {
	{case_suit, sizeof(case_suit)},
	{case_mem_suit, sizeof(case_mem_suit)},
	{case_exit_entry, sizeof(case_exit_entry)},
	{case_general, sizeof(case_general)},
};

void print_case_list()
{
	printf("HSI virtualization specific feature case list:\n\r");
	int i, j;
	st_case_suit *pcase;
	u32 size;
	printf("\t Case ID: %d Case name: %s\n\r", 40291U,
		"HSI_Virtualization_Specific_Features_VMX_Instruction_001");

	for (j = 0; j < ARRAY_SIZE(case_list); j++) {
		pcase = case_list[j].plist;
		size = case_list[j].size;
		for (i = 0; i < (size / sizeof(st_case_suit)); i++) {
			printf("\t Case ID: %d Case name: %s\n\r", pcase[i].case_id,
				pcase[i].pname);
		}
	}
}

static int vm_exec_con_host_exit_handler(void)
{
	u64 guest_rip;
	ulong reason;
	u32 insn_len;
	u32 exit_qual;
	u32 condition = 0;
	u32 index = 0;

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
		if ((condition < CON_BUFF) && (vmcall_exit[condition].exit_func != NULL)) {
			vmcall_exit[condition].exit_func(vcpu);
			vmcs_write(GUEST_RIP, guest_rip + insn_len);
			return VMX_TEST_RESUME;
		} else if (condition & VMCALL_EXIT_SECOND) {
			/* get vmcall function index */
			index = (condition & ~VMCALL_EXIT_SECOND);
			if ((index < ARRAY_SIZE(vmcall_exit_second)) && (vmcall_exit_second[index].exit_func)) {
				vmcall_exit_second[index].exit_func(vcpu);
				vmcs_write(GUEST_RIP, guest_rip + insn_len);
				return VMX_TEST_RESUME;
			}
		}

		/* Should not reach here */
		print_vmexit_info();
		DBG_ERRO("Unkown vmcall for make condition:%d", condition);
		return VMX_TEST_VMEXIT;
	default:
		if ((reason < VMX_EXIT_REASON_NUM) && (vm_exit[reason].exit_func != NULL)) {
			vm_exit[reason].exit_func(vcpu);
			/* skip guest rip */
			vmcs_write(GUEST_RIP, vcpu->arch.guest_rip + vcpu->arch.guest_ins_len);
			return VMX_TEST_RESUME;
		}
		debug_print("Unknown exit reason, %ld", reason);
		print_vmexit_info();
		break;
	}
	return VMX_TEST_VMEXIT;
}

BUILD_GUEST_MAIN_FUNC(vmx_exec_con, case_suit);
BUILD_GUEST_MAIN_FUNC(vmx_memory, case_mem_suit);
BUILD_GUEST_MAIN_FUNC(vmx_exit_entry, case_exit_entry);
BUILD_GUEST_MAIN_FUNC(vmx_general, case_general);

/* name/init/guest_main/exit_handler/syscall_handler/guest_regs */
static struct vmx_test vmx_tests[] = {
	{ "VMX general", vm_exec_init, vmx_general_guest_main,
		vm_exec_con_host_exit_handler, NULL, {0} },
	{ "VMX exit entry contrl", vm_exec_init, vmx_exit_entry_guest_main,
		vm_exec_con_host_exit_handler, NULL, {0} },
	{ "VMX memory manage", vm_exec_init, vmx_memory_guest_main,
		vm_exec_con_host_exit_handler, NULL, {0} },
	{ "VMX exe contrl", vm_exec_init, vmx_exec_con_guest_main,
		vm_exec_con_host_exit_handler, NULL, {0} },
	{ NULL, NULL, NULL, NULL, NULL, {0} },
};

