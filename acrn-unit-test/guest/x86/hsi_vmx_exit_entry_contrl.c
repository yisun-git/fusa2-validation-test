static void ia32_pat_exit_condition(__unused struct st_vcpu *vcpu)
{
	/* set VMCS VM-Exit controls bit18(save IA32_PAT) bit19(load IA32_PAT) to 1*/
	exec_vmwrite32_bit(VMX_EXIT_CONTROLS,
		VMX_EXIT_CTLS_SAVE_PAT | VMX_EXIT_CTLS_LOAD_PAT, BIT_TYPE_SET);
	DBG_INFO("VMX_EXIT_CONTROLS:0x%x\n", exec_vmread32(VMX_EXIT_CONTROLS));

	/*
	 * Set guest ia32_pat msr value in vmcs for vm-exit save.
	 */
	vmcs_write(VMX_GUEST_IA32_PAT_FULL, IA32_PAT_GUEST_VMCS_INIT_VALUE);
	DBG_INFO("VMX_GUEST_IA32_PAT_FULL:0x%lx\n", vmcs_read(VMX_GUEST_IA32_PAT_FULL));

	/* set host ia32_pat msr value in vmcs for vm-exit load */
	vmcs_write(VMX_HOST_IA32_PAT_FULL, IA32_PAT_VALUE1);
	DBG_INFO("VMX_HOST_IA32_PAT_FULL:0x%lx\n", vmcs_read(VMX_HOST_IA32_PAT_FULL));

	/* set IA32_PAT register passthrough */
	msr_passthroudh();

	/* set host msr ia32_pat value */
	wrmsr(MSR_IA32_PAT, IA32_PAT_HOST_VMCS_INIT_VALUE);

}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Exit_Control_Save_Load_IA32_PAT_001
 *
 * Summary: IN root operation, set VMCS VM-Exit controls bit18(save IA32_PAT) bit19(load IA32_PAT) to 1
 * execute VMWRITE set guest ia32_pat value to 0x7040600070406 with guest ia32_pat field,
 * execute VMWRITE set host ia32_pat value to 0x0000000070406 with host ia32_pat field,
 * execute WRMSR set host physical msr ia32_pat value to 0x7040600070406 different from 0x0000000070406.
 * set guest wrmsr ia32_pat register passthrough.
 * switch to non-root operation,
 * Execute WRMSR set guest physical msr ia32_pat value to 0x0000000001040506
 * switch to root operation with VM-exit.
 * Check guest ia32_pat value should be 0x0000000001040506 saved in VMCS guest ia32_pat fields.
 * Check host physical ia32_pat value should be 0x0000000070406 loaded from VMCS host ia32_pat fields.
 */
__unused static void hsi_rqmid_42066_virtualization_specific_features_ia32_pat_exit_001()
{
	vm_exit_info.result = RET_BUFF;
	wrmsr(MSR_IA32_PAT, IA32_PAT_VALUE2);

	/* check result in root operation mode */
	vmx_set_test_condition(CON_42066_GET_RESULT);
	vmcall();

	barrier();
	report("%s", (vm_exit_info.result == RET_42066_CHECK_MSR_IA32_PAT), __func__);
}

static void hypercall_check_ia32_pat_value(__unused struct st_vcpu *vcpu)
{
	u8 chk = 0;
	/**
	 * Check host physical ia32_pat value should be 0x0000000070406
	 * loaded from VMCS host ia32_pat fields.
	 */
	if (rdmsr(MSR_IA32_PAT) == IA32_PAT_VALUE1) {
		chk++;
		DBG_INFO("Host msr ia32 pat loaded success!");
	}

	/**
	 * Check guest ia32_pat value should be 0x0000000001040506
	 * saved in VMCS guest ia32_pat fields.
	 */
	if (vmcs_read(VMX_GUEST_IA32_PAT_FULL) == IA32_PAT_VALUE2) {
		chk++;
		DBG_INFO("Guest msr ia32 pat saved success!");
	}

	if (chk == 2) {
		vm_exit_info.result = RET_42066_CHECK_MSR_IA32_PAT;
	}
}

static void ia32_efer_exit_condition(__unused struct st_vcpu *vcpu)
{
	/* set VMCS VM-Exit controls bit20(save IA32_EFER) bit21(load IA32_EFER) to 1*/
	exec_vmwrite32_bit(VMX_EXIT_CONTROLS,
		VMX_EXIT_CTLS_SAVE_EFER | VMX_EXIT_CTLS_LOAD_EFER, BIT_TYPE_SET);
	DBG_INFO("VMX_EXIT_CONTROLS:0x%x", exec_vmread32(VMX_EXIT_CONTROLS));

	/*
	 * Set guest ia32_efer msr value in vmcs for vm-exit save.
	 */
	vmcs_write(VMX_GUEST_IA32_EFER_FULL, IA32_EFER_GUEST_VMCS_INIT_VALUE);
	DBG_INFO("VMX_GUEST_IA32_EFER_FULL:0x%lx", vmcs_read(VMX_GUEST_IA32_EFER_FULL));

	/* set host ia32_efer msr value in vmcs for vm-exit load */
	vmcs_write(VMX_HOST_IA32_EFER_FULL, IA32_EFER_HOST_VMCS_INIT_VALUE);
	DBG_INFO("VMX_HOST_IA32_EFER_FULL:0x%lx", vmcs_read(VMX_HOST_IA32_EFER_FULL));

	/* set host msr ia32_efer value */
	wrmsr(MSR_IA32_EFER, IA32_EFER_HOST_PYHSICAL_VALUE);

	msr_passthroudh();

}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Exit_Control_Save_Load_IA32_EFER_001
 *
 * Summary: IN root operation, set VMCS VM-Exit controls bit20(save IA32_EFER) bit21(load IA32_EFER) to 1
 * execute VMWRITE set guest IA32_EFER value to 0xD00 with guest IA32_EFER field,
 * execute VMWRITE set host IA32_EFER value to 0xD00 with host IA32_EFER field,
 * execute WRMSR set host physical msr IA32_EFER value to 0xD01 different from 0xD00.
 * set guest wrmsr IA32_EFER register passthrough.
 * switch to non-root operation,
 * Execute WRMSR set guest physical msr IA32_EFER value to 0x500.
 * switch to root operation with VM-exit.
 * Check guest IA32_EFER value should be 0x500 saved in VMCS guest ia32_pat fields.
 * Check host physical IA32_EFER value should be 0xD00 loaded from VMCS host IA32_EFER fields.
 */
__unused static void hsi_rqmid_42174_virtualization_specific_features_ia32_efer_exit_001()
{
	vm_exit_info.result = RET_BUFF;
	wrmsr(MSR_IA32_EFER, IA32_EFER_VALUE1);

	/* check result in root operation mode */
	vmx_set_test_condition(CON_42174_GET_RESULT);
	vmcall();

	barrier();
	report("%s", (vm_exit_info.result == RET_42174_CHECK_MSR_IA32_EFER), __func__);
}

static void hypercall_check_ia32_efer_value(__unused struct st_vcpu *vcpu)
{
	u8 chk = 0;
	/**
	 * Check host physical IA32_EFER value should be 0xD00
	 * loaded from VMCS host IA32_EFER fields.
	 */
	if (rdmsr(MSR_IA32_EFER) == IA32_EFER_HOST_VMCS_INIT_VALUE) {
		chk++;
		DBG_INFO("Host msr ia32 efer loaded success!");
	}

	/**
	 * Check guest IA32_EFER value should be 0x500
	 * saved in VMCS guest ia32_pat fields.
	 */
	if (vmcs_read(VMX_GUEST_IA32_EFER_FULL) == IA32_EFER_VALUE1) {
		chk++;
		DBG_INFO("Guest msr ia32 efer saved success!");
	}

	if (chk == 2) {
		vm_exit_info.result = RET_42174_CHECK_MSR_IA32_EFER;
	}
}

static void ia32_pat_entry_condition(__unused struct st_vcpu *vcpu)
{
	/* set VMCS VM-entry controls bit14(load IA32_PAT) to 1*/
	exec_vmwrite32_bit(VMX_ENTRY_CONTROLS,
		VMX_ENTRY_CTLS_LOAD_PAT, BIT_TYPE_SET);
	DBG_INFO("VMX_ENTRY_CONTROLS:0x%x", exec_vmread32(VMX_ENTRY_CONTROLS));

	/*
	 * Set guest ia32_pat msr value in vmcs for vm-entry load.
	 */
	vmcs_write(VMX_GUEST_IA32_PAT_FULL, IA32_PAT_VALUE1);
	DBG_INFO("VMX_GUEST_IA32_PAT_FULL:0x%lx", vmcs_read(VMX_GUEST_IA32_PAT_FULL));
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Entry_Control_Load_IA32_PAT_001
 *
 * Summary: IN root operation, set VMCS VM-entry bit14(load IA32_PAT) to 1
 * execute VMWRITE set guest ia32_pat value to 0x0000000070406 with guest ia32_pat field,
 * set guest read msr ia32_pat register passthrough.
 * switch to non-root operation which VM-entry is occurs,
 * Check ia32_pat value should be 0x0000000070406.
 * switch to root operation.
 * execute VMWRITE again set guest ia32_pat value to 0x0000000001040506 with guest ia32_pat field,
 * switch to non-root operation again which VM-entry is occurs,
 * Check ia32_pat value should be 0x0000000001040506.
 */
__unused static void hsi_rqmid_42116_virtualization_specific_features_ia32_pat_entry_001()
{
	bool chk = false;
	uint64_t first_ia32_pat = rdmsr(MSR_IA32_PAT);

	/* change guest msr ia32 pat vmcs in root operation */
	vmx_set_test_condition(CON_CHANGE_GUEST_MSR_PAT);
	vmcall();

	uint64_t second_ia32_pat = rdmsr(MSR_IA32_PAT);
	DBG_INFO("first_ia32_pat:0x%lx second_ia32_pat:0x%lx",
		first_ia32_pat, second_ia32_pat);

	if ((first_ia32_pat == IA32_PAT_VALUE1) &&
		(second_ia32_pat == IA32_PAT_VALUE2)) {
		chk = true;
	}

	report("%s", (chk == true), __func__);
}

static void hypercall_change_guest_msr_pat(__unused struct st_vcpu *vcpu)
{
	/*
	 * Set guest ia32_pat msr value in vmcs for vm-entry load.
	 */
	vmcs_write(VMX_GUEST_IA32_PAT_FULL, IA32_PAT_VALUE2);
	DBG_INFO("VMX_GUEST_IA32_PAT_FULL:0x%lx\n", vmcs_read(VMX_GUEST_IA32_PAT_FULL));
}

static void ia32_efer_entry_condition(__unused struct st_vcpu *vcpu)
{
	/* set VMCS VM-entry controls bit14(load IA32_PAT) to 1*/
	exec_vmwrite32_bit(VMX_ENTRY_CONTROLS,
		VMX_ENTRY_CTLS_LOAD_EFER, BIT_TYPE_SET);
	DBG_INFO("VMX_ENTRY_CONTROLS:0x%x", exec_vmread32(VMX_ENTRY_CONTROLS));

	/*
	 * Set guest IA32_EFER msr value in vmcs for vm-entry load.
	 */
	vmcs_write(VMX_GUEST_IA32_EFER_FULL, IA32_EFER_VALUE1);
	DBG_INFO("VMX_GUEST_IA32_EFER_FULL:0x%lx", vmcs_read(VMX_GUEST_IA32_EFER_FULL));
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Entry_Control_Load_IA32_EFER_001
 *
 * Summary: IN root operation, set VMCS VM-entry bit15(load IA32_EFER) to 1
 * execute VMWRITE set guest IA32_EFER value to 0x500 with guest IA32_EFER field,
 * set guest read msr IA32_EFER register passthrough.
 * switch to non-root operation which VM-entry is occurs,
 * Check IA32_EFER value should be 0x500.
 * switch to root operation.
 * execute VMWRITE again set guest IA32_EFER value to 0xD00 with guest IA32_EFER field,
 * switch to non-root operation again which VM-entry is occurs,
 * Check IA32_EFER value should be 0xD00.
 */
__unused static void hsi_rqmid_42175_virtualization_specific_features_ia32_efer_entry_001()
{
	bool chk = false;
	uint64_t first_ia32_efer = rdmsr(MSR_IA32_EFER);

	/* change guest msr ia32 efer vmcs in root operation */
	vmx_set_test_condition(CON_CHANGE_GUEST_MSR_EFER);
	vmcall();

	uint64_t second_ia32_efer = rdmsr(MSR_IA32_EFER);
	DBG_INFO("first_ia32_efer:0x%lx second_ia32_efer:0x%lx",
		first_ia32_efer, second_ia32_efer);

	if ((first_ia32_efer == IA32_EFER_VALUE1) &&
		(second_ia32_efer == IA32_EFER_VALUE2)) {
		chk = true;
	}

	report("%s", (chk == true), __func__);
}

static void hypercall_change_guest_msr_efer(__unused struct st_vcpu *vcpu)
{
	/*
	 * Set guest ia32_efer msr value in vmcs for vm-entry load.
	 */
	vmcs_write(VMX_GUEST_IA32_EFER_FULL, IA32_EFER_VALUE2);
	DBG_INFO("VMX_GUEST_IA32_EFER_FULL:0x%lx", vmcs_read(VMX_GUEST_IA32_EFER_FULL));
}

static void debug_controls_entry_condition(__unused struct st_vcpu *vcpu)
{
	/* clear VMCS VM-entry controls bit2(load debug controls) to 0 */
	exec_vmwrite32_bit(VMX_ENTRY_CONTROLS,
		VMX_ENTRY_CTLS_LOAD_DEBUG_REG, BIT_TYPE_CLEAR);
	DBG_INFO("VMX_ENTRY_CONTROLS:0x%x", exec_vmread32(VMX_ENTRY_CONTROLS));

	vmcs_clear_bits(CPU_EXEC_CTRL0, CPU_MOV_DR);

	/*
	 * Set guest DR7 value in vmcs for vm-entry load.
	 */
	vmcs_write(VMX_GUEST_DR7, GUEST_LOAD_DR7_VALUE1);
	DBG_INFO("VMX_GUEST_DR7:0x%lx", vmcs_read(VMX_GUEST_DR7));
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Entry_Control_Load_Debug_Controls_001
 *
 * Summary: IN root operation, clear VMCS VM-entry bit2(load debug controls) to 0,
 * execute VMWRITE set guest DR7 value to 0x401 with guest DR7 field,
 * switch to non-root operation which means VM-entry is occurs,
 * Check DR7 value should not be 0x401.
 */
__unused static void hsi_rqmid_42177_virtualization_specific_features_debug_controls_entry_001()
{
	uint64_t dr7 = read_dr7();
	DBG_INFO("guest_dr7:0x%lx", dr7);

	report("%s", (dr7 != GUEST_LOAD_DR7_VALUE1), __func__);
}

static void exit_load_perf_condition(__unused struct st_vcpu *vcpu)
{
	/* clear VMCS VM-Exit controls bit12(load IA32_PERF_GLOBAL_CTRL) to 0*/
	exec_vmwrite32_bit(VMX_EXIT_CONTROLS,
		VMX_EXIT_CTLS_LOAD_PERF_GLOBAL_CTRL, BIT_TYPE_CLEAR);
	DBG_INFO("VMX_EXIT_CONTROLS:0x%x", exec_vmread32(VMX_EXIT_CONTROLS));

	/*
	 * Set host IA32_PERF_GLOBAL_CTRL msr in vmcs for vm-exit load.
	 */
	vmcs_write(VMX_HOST_IA32_PERF_CTL_FULL, HOST_IA32_PERF_INIT_VALUE);
	DBG_INFO("VMX_HOST_IA32_PERF_CTL_FULL:0x%lx",
		vmcs_read(VMX_HOST_IA32_PERF_CTL_FULL));
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Exit_Control_Load_IA32_PERF_001
 *
 * Summary: IN root operation, clear VMCS VM-Exit controls bit12(load IA32_PERF_GLOBAL_CTRL) to 0,
 * execute VMWRITE set host IA32_PERF_GLOBAL_CTRL value to 0 with host IA32_PERF_GLOBAL_CTRL field,
 * switch to non-root operation,
 * Execute WRMSR set physical IA32_PERF_GLOBAL_CTRL value to 1.
 * switch to root operation with VM-exit.
 * Check physical IA32_PERF_GLOBAL_CTRL value should be 1 unchanged which is not loaded by processor.
 */
__unused static void hsi_rqmid_42178_virtualization_specific_features_exit_load_perf_001()
{
	vm_exit_info.result = RET_BUFF;
	wrmsr(MSR_IA32_PERF_GLOBAL_CTRL, IA32_PERF_GUEST_SET_VALUE);

	/* check result in root operation mode */
	vmx_set_test_condition(CON_42178_GET_RESULT);
	vmcall();

	barrier();
	report("%s", (vm_exit_info.result == RET_42178_CHECK_MSR_IA32_PERF), __func__);
}

static void hypercall_check_ia32_perf_value(__unused struct st_vcpu *vcpu)
{
	/* check vm-exit are not loaded MSR_IA32_PERF_GLOBAL_CTRL register
	 * MSR_IA32_PERF_GLOBAL_CTRL should changed by guest, the value should be
	 * IA32_PERF_GUEST_SET_VALUE rather than HOST_IA32_PERF_INIT_VALUE.
	 */
	uint64_t perf = rdmsr(MSR_IA32_PERF_GLOBAL_CTRL);
	if (perf == IA32_PERF_GUEST_SET_VALUE) {
		vm_exit_info.result = RET_42178_CHECK_MSR_IA32_PERF;
		DBG_INFO("host IA32_PERF not loaded by cpu!");
	}
}

static void entry_load_bndcfgs_condition(__unused struct st_vcpu *vcpu)
{
	/* clear VMCS VM-Entry controls bit16(load IA32_BNDCFGS ) to 0*/
	exec_vmwrite32_bit(VMX_ENTRY_CONTROLS,
		VMX_ENTRY_CTLS_LOAD_BNDCFGS, BIT_TYPE_CLEAR);
	DBG_INFO("VMX_ENTRY_CONTROLS:0x%x", exec_vmread32(VMX_ENTRY_CONTROLS));

	/*
	 * Set guest IA32_BNDCFGS msr in vmcs for vm-exit load.
	 */
	vmcs_write(VMX_GUEST_IA32_BNDCFGS_FULL, IA32_BNDCFGS_VALUE2);
	DBG_INFO("VMX_GUEST_IA32_BNDCFGS_FULL:0x%lx",
		vmcs_read(VMX_GUEST_IA32_BNDCFGS_FULL));

	/* init MSR_IA32_BNDCFGS value in host */
	wrmsr(MSR_IA32_BNDCFGS, IA32_BNDCFGS_VALUE1);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Entry_Control_Load_IA32_BNDCFGS_001
 *
 * Summary: IN root operation, clear VMCS VM-Entry controls bit16(load IA32_BNDCFGS) to 0,
 * execute VMWRITE set guest IA32_BNDCFGS value to 1 with guest IA32_BNDCFGS field,
 * execute WRMSR set physical MSR IA32_BNDCFGS value to 0.
 * switch to non-root operation,
 * Execute RDMSR get physical IA32_BNDCFGS which should be 0 rather than 1.
 */
__unused static void hsi_rqmid_42182_virtualization_specific_features_entry_load_bndcfgs_001()
{
	ulong bndcfgs = rdmsr(MSR_IA32_BNDCFGS);
	DBG_INFO("guest_bndcfgs:0x%lx", bndcfgs);

	report("%s", (bndcfgs == IA32_BNDCFGS_VALUE1), __func__);
}

static void exit_clear_bndcfgs_condition(__unused struct st_vcpu *vcpu)
{
	/* clear VMCS VM-Exit controls bit23(clear IA32_BNDCFGS ) to 0*/
	exec_vmwrite32_bit(VMX_EXIT_CONTROLS,
		VMX_EXIT_CTLS_CLEAR_BNDCFGS, BIT_TYPE_CLEAR);

	DBG_INFO("VMX_EXIT_CONTROLS:0x%x", exec_vmread32(VMX_EXIT_CONTROLS));
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Exit_Control_Clear_IA32_BNDCFGS_001
 *
 * Summary: IN root operation, clear VMCS VM-Exit controls bit23(clear IA32_BNDCFGS) to 0,
 * switch to non-root operation,
 * execute WRMSR set MSR IA32_BNDCFGS value to 1(non-zero value).
 * switch to root operation which means vm-exit occurs,
 * Execute RDMSR get physical IA32_BNDCFGS which should be 1 rather than 0.
 */
__unused static void hsi_rqmid_42183_virtualization_specific_features_exit_clear_bndcfgs_001()
{
	vm_exit_info.result = RET_BUFF;
	wrmsr(MSR_IA32_BNDCFGS, IA32_BNDCFGS_VALUE2);

	/* check result in root operation mode */
	vmx_set_test_condition(CON_42183_GET_RESULT);
	vmcall();

	barrier();
	report("%s", (vm_exit_info.result == RET_42183_CHECK_MSR_IA32_BNDCFGS), __func__);
}

static void hypercall_check_ia32_bndcfgs_value(__unused struct st_vcpu *vcpu)
{
	uint64_t ia32_bndcfgs = rdmsr(MSR_IA32_BNDCFGS);
	/* make sure MSR_IA32_BNDCFGS are not cleared on vm-exit */
	if (ia32_bndcfgs == IA32_BNDCFGS_VALUE2) {
		vm_exit_info.result = RET_42183_CHECK_MSR_IA32_BNDCFGS;
		DBG_INFO("host IA32_BNDCFGS not cleared by cpu!");
	}
}

static void exit_ack_inter_condition(__unused struct st_vcpu *vcpu)
{
	/* set VMCS VM-Exit controls bit15(acknowledge interrupt on exit) to 1 */
	exec_vmwrite32_bit(VMX_EXIT_CONTROLS,
		VMX_EXIT_CTLS_ACK_IRQ, BIT_TYPE_SET);
	DBG_INFO("VMX_EXIT_CONTROLS:0x%x", exec_vmread32(VMX_EXIT_CONTROLS));

	/*
	 * Enable VM_EXIT for external interrupt.
	 */
	vmcs_set_bits(PIN_CONTROLS, PIN_EXTINT);
	DBG_INFO("PIN_CONTROLS:0x%x", exec_vmread32(PIN_CONTROLS));
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Exit_Control_Ack_Inter_001
 *
 * Summary: IN root operation, set VMCS VM-Exit controls bit15(acknowledge interrupt on exit) to 1,
 * set Pin-base execute controls field bit0(External interrupt exiting) to 1,
 * switch to non-root operation,
 * Trigger an external 0x80 interrupt.
 * Check in VM-exit interruption information field bit[7:0]interrupt vector is 0x80,
 * bit31 is 1.
 */
__unused static void hsi_rqmid_42186_virtualization_specific_features_exit_ack_inter_001()
{
	vm_exit_info.is_vm_exit[VM_EXIT_EXTER_INTER_VALID] = EXTERNAL_INTER_INVALID;

	/* Trigger external interrupts in guest */
	apic_write(APIC_LVTT, APIC_LVT_TIMER_TSCDEADLINE | EXTEND_INTERRUPT_80);
	wrmsr(MSR_IA32_TSCDEADLINE, asm_read_tsc()+(0x1000));

	/* wait external interrupt occur */
	delay_tsc(0x2000);

	barrier();
	report("%s", (vm_exit_info.is_vm_exit[VM_EXIT_EXTER_INTER_VALID] == EXTERNAL_INTER_VALID), __func__);
}

static void exit_save_pree_timer_condition(__unused struct st_vcpu *vcpu)
{
	/* clear VMCS VM-Exit controls bit22(Save VMX preemption timer value on exit) to 0 */
	exec_vmwrite32_bit(VMX_EXIT_CONTROLS,
		VMX_EXIT_CTLS_SAVE_PREE_TIMER, BIT_TYPE_CLEAR);
	DBG_INFO("VMX_EXIT_CONTROLS:0x%x", exec_vmread32(VMX_EXIT_CONTROLS));

	/* Set value to VMX-preemption timer-value */
	exec_vmwrite32(VMX_PREEMPTION_TIMER, SAVE_PREEMPTION_TIME_VALUE);
	DBG_INFO("VMX_PREEMPTION_TIMER:0x%x", exec_vmread32(VMX_PREEMPTION_TIMER));

	/* enabe activate VMX preemption timer at pin-based execution control */
	vmcs_set_bits(PIN_CONTROLS, PIN_PREEMPT);
	DBG_INFO("PIN_CONTROLS:0x%x", exec_vmread32(PIN_CONTROLS));
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Exit_Control_Save_Pree_Timer_001
 *
 * Summary: IN root operation, clear VMCS VM-Exit controls bit22(Save VMX preemption timer value on exit) to 0,
 * set Pin-base execute controls field bit6(activate VMX preemption timer) to 1,
 * execute VMWRITE set preemption timer to a large value 0x10000000 with preemption timer fields,
 * switch to non-root operation,
 * Execute any instruction.
 * switch to root operation immediately which means vm-exit occurs,
 * Execute VMREAD get preemption timer value should be unchanged.
 */
__unused static void hsi_rqmid_42189_virtualization_specific_features_exit_save_pree_timer_001()
{
	vm_exit_info.result = RET_BUFF;
	/* wait for tsc time */
	delay_tsc(0x10000);

	/* check result in root operation mode */
	vmx_set_test_condition(CON_42189_GET_PREEM_TIME);
	vmcall();

	barrier();
	report("%s", (vm_exit_info.result == RET_42189_CHECK_PREE_TIME), __func__);
}

static void hypercall_check_preem_time(__unused struct st_vcpu *vcpu)
{
	uint64_t pree_timer = exec_vmread32(VMX_PREEMPTION_TIMER);
	/* make sure preemption timer not saved by processor on vm-exit */
	if (pree_timer == SAVE_PREEMPTION_TIME_VALUE) {
		vm_exit_info.result = RET_42189_CHECK_PREE_TIME;
	}
	DBG_INFO("pree_timer:0x%lx", pree_timer);
}

static u64 old_efer;
static void exit_host_addr64_condition(__unused struct st_vcpu *vcpu)
{
	/* set VMCS VM-Exit controls bit0(Host address space size) to 1 */
	exec_vmwrite32_bit(VMX_EXIT_CONTROLS,
		VMX_EXIT_CTLS_HOST_ADDR64, BIT_TYPE_SET);

	/* clear VMCS VM-Exit controls bit21(load IA32_EFER) to 0 */
	vmcs_clear_bits(VMX_EXIT_CONTROLS,
		VMX_EXIT_CTLS_LOAD_EFER);

	/* save old ia32_efer register */
	old_efer = rdmsr(MSR_IA32_EFER);
	DBG_INFO("VMX_EXIT_CONTROLS:0x%x old_efer:0x%lx", exec_vmread32(VMX_EXIT_CONTROLS), old_efer);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Exit_Control_Host_Address_Space_Size_001
 *
 * Summary: IN root operation, set VMCS VM-Exit controls bit9(Host address space size) to 1,
 * clear VMCS VM-Exit controls bit21(load IA32_EFER) to 0,
 * switch to non-root operation,
 * Execute any instruction.
 * switch to root operation which means vm-exit occurs,
 * Check IA32_EFER.LMA, IA32_EFER.LME, CS.L is loaded to 1.
 */
__unused static void hsi_rqmid_42192_virtualization_specific_features_exit_host_addr64_001()
{
	vm_exit_info.result = RET_BUFF;

	/* check result in root operation mode */
	vmx_set_test_condition(CON_42192_GET_RESULT);
	vmcall();

	barrier();
	report("%s", (vm_exit_info.result == RET_42192_CHECK_RESULT), __func__);
}

__unused static inline uint64_t hsi_sgdt(void)
{
	struct descriptor_table_ptr gdtb;
	asm volatile ("sgdt %0" : "=m"(gdtb) : : "memory");

	DBG_INFO("gdt_base=0x%lx, gdb_limit=0x%x\n", gdtb.base, gdtb.limit);
	return gdtb.base;
}

static void hypercall_check_host_addr64(__unused struct st_vcpu *vcpu)
{
	u8 chk = 0;
	uint64_t gdt_base;

	uint64_t phy_efer = rdmsr(MSR_IA32_EFER);
	/* make sure IA32_EFER.LMA and IA32_EFER.LME is set to 1
	 * because bit9(Host address space size) is set to 1 at vm-exit controls field
	 */
	if ((phy_efer & (MSR_IA32_EFER_LMA_BIT | MSR_IA32_EFER_LME_BIT))
		== (MSR_IA32_EFER_LMA_BIT | MSR_IA32_EFER_LME_BIT)) {
		chk++;
	}

	uint16_t cs = read_cs();
	gdt_base = hsi_sgdt();

	DBG_INFO("cs_des:0x%lx gdt_base:%lx cs:%x phy_efer:%lx", \
			((uint64_t *)gdt_base)[cs >> 3], \
			gdt_base, read_cs(), phy_efer);
	/* make sure CS.L is set to 1 */
	if ((((uint64_t *)gdt_base)[cs >> 3] & CS_L_BIT) != 0) {
		chk++;
	}

	if (chk == 2) {
		vm_exit_info.result = RET_42192_CHECK_RESULT;
	}
	/* resume environment */
	wrmsr(MSR_IA32_EFER, old_efer);
}

static void entry_ia32_mode_guest_condition(__unused struct st_vcpu *vcpu)
{
	/* set VMCS VM-Entry controls bit9(IA32E mode guest) to 1 */
	exec_vmwrite32_bit(VMX_ENTRY_CONTROLS,
		VMX_ENTRY_CTLS_IA32E_MODE, BIT_TYPE_SET);
	DBG_INFO("VMX_ENTRY_CONTROLS:0x%x", exec_vmread32(VMX_ENTRY_CONTROLS));
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Entry_Control_IA32E_Mode_Guest_001
 *
 * Summary: IN root operation, set VMCS VM-Entry controls bit9(IA32E mode guest) to 1,
 * switch to non-root operation,
 * Check IA32_EFER.LMA, IA32_EFER.LME, CS.L is loaded to 1.
 */
__unused static void hsi_rqmid_42223_virtualization_specific_features_entry_ia32_mode_guest_001()
{
	u64 efer = rdmsr(MSR_IA32_EFER);
	int chk = 0;
	/* check long mode is enable */
	if ((efer & (MSR_IA32_EFER_LMA_BIT | MSR_IA32_EFER_LME_BIT))
		== (MSR_IA32_EFER_LMA_BIT | MSR_IA32_EFER_LME_BIT)) {
		chk++;
	}

	u16  cs = read_cs();
	struct descriptor_table_ptr desc;
	sgdt(&desc);
	u64 *pdesc = (u64 *)desc.base;
	u64 code_desc = pdesc[cs >> 3];
	/* make sure CS.L is set to 1 */
	if ((code_desc & CS_L_BIT) != 0) {
		chk++;
	}

	DBG_INFO(" code_desc:0x%lx efer:0x%lx", code_desc, efer);

	report("%s", (chk == 2), __func__);
}

/* clear VMCS VM-Entry controls bit10(Entry to SMM) to 0 */
BUILD_CONDITION_FUNC(entry_to_smm, VMX_ENTRY_CONTROLS, \
	VMX_ENTRY_CTLS_ENTRY_TO_SMM, BIT_TYPE_CLEAR);

static int entry_to_smm_rsm_ins(void)
{
	asm volatile(ASM_TRY("1f")
		"rsm \n\t"
		"1:" : : : );
	return exception_vector();
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Entry_Control_to_SMM_001
 *
 * Summary: IN root operation, clear VMCS VM-Entry controls bit10(Entry to SMM) to 0,
 * switch to non-root operation,
 * Execute RSM instruction should generate #UD exception, because SMM mode not support.
 */
__unused static void hsi_rqmid_42271_virtualization_specific_features_entry_to_smm_001()
{
	report("%s", (entry_to_smm_rsm_ins() == UD_VECTOR), __func__);
}


