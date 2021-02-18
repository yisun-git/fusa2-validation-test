
static void exit_host_msr_control_condition(__unused struct st_vcpu *vcpu)
{
	/**
	 * Set vcpu->arch.msr_area.host[MSR_AREA_TSC_AUX].msr_index to MSR_IA32_TSC_AUX, which is the MSR index
	 * in the MSR Entry to be loaded on VM exit
	 */
	vcpu->arch.msr_area.host[MSR_AREA_TSC_AUX].msr_index = MSR_IA32_TSC_AUX;
	/**
	 * Set vcpu->arch.msr_area.host[MSR_AREA_TSC_AUX].value to the return value of TSC_AUX_VALUE1,
	 * which is the physical CPU ID associated with \a vcpu and the MSR data in the MSR Entry to be loaded
	 * on VM exit
	 */
	vcpu->arch.msr_area.host[MSR_AREA_TSC_AUX].value = TSC_AUX_VALUE1;

	/* Set up VMX entry MSR load count - pg 2908 24.8.2 Tells the number of
	 * MSRs on load from memory on VM entry from mem address provided by
	 * VM-entry MSR load address field
	 */
	vmcs_write(VMX_EXIT_MSR_LOAD_COUNT, MSR_AREA_COUNT);
	vmcs_write(VMX_EXIT_MSR_LOAD_ADDR_FULL, hva2hpa((void *)vcpu->arch.msr_area.host));
	/* set physical MSR_AREA_TSC_AUX value to BP_TSC_AUX_VALUE3 different from TSC_AUX_VALUE1 */
	wrmsr(MSR_IA32_TSC_AUX, BP_TSC_AUX_VALUE3);
	DBG_INFO("vm-entry bp tsc_aux:%lx\n", rdmsr(MSR_IA32_TSC_AUX));
}

static void hypercall_check_tsc_aux_load(__unused struct st_vcpu *vcpu)
{
	uint64_t host_tsc_aux = rdmsr(MSR_IA32_TSC_AUX);
	/* make sure host msr are loaded by processor */
	if (host_tsc_aux == TSC_AUX_VALUE1) {
		vm_exit_info.result = RET_CHECK_TSC_AUX;
		DBG_INFO("check tsc value at host success!");
	}
}

/**
 * @brief Case name:HSI_Virtualization_Specific_Features_VM_Exit_MSR_Control_Host_Load_001
 *
 * Summary: IN root operation, initialize 16 bytes memory host msr area,
 * write IA32_TSC_AUX value to msr index, write 0 to the msr,
 * execute VMWRITE set msr cout to 1 with VM-exit MSR-load count control field in the VMCS,
 * execute VMWRITE set msr address to host msr area address with VM-exit MSR-load address control field in the VMCS,
 * set physical MSR_AREA_TSC_AUX value to 0xff different from 0
 * switch to non-root operation,
 * execute any instruction.
 * swith to root operation which means VM exit occurs,
 * check host IA32_TSC_AUX value should be 0 loaded by processor.
 */
__unused static void hsi_rqmid_42237_virtualization_specific_features_exit_host_msr_load_001()
{
	vm_exit_info.result = RET_BUFF;
	/* vmcall enter host check host IA32_TSC_AUX value is loaded by processor */
	vmx_set_test_condition(CON_CHECK_TSC_AUX_VALUE);
	vmcall();

	report("%s", (vm_exit_info.result == RET_CHECK_TSC_AUX), __func__);
}

/**
 * @brief Case name:HSI_Virtualization_Specific_Features_VM_Exit_MSR_Control_Guest_Store_001
 *
 * Summary: IN root operation, initialize 16 bytes memory guest msr area,
 * set IA32_TSC_AUX value to msr index, initialize 0 to the msr,
 * execute VMWRITE set msr cout to 1 with VM-Exit MSR-Store count control field in the VMCS,
 * execute VMWRITE set msr address to guest msr area address with VM-Exit MSR-Store address control field in the VMCS,
 * switch to non-root operation,
 * set msr IA32_TSC_AUX value to 1.
 * swith to root operation which means VM-exit occurs,
 * check IA32_TSC_AUX value should be 1 in guest msr area memory.
 */
__unused static void hsi_rqmid_42238_virtualization_specific_features_exit_guest_msr_store_001()
{
	vm_exit_info.result = RET_BUFF;
	/* change guest IA32_TSC_AUX value to value2 */
	wrmsr(MSR_IA32_TSC_AUX, TSC_AUX_VALUE2);
	DBG_INFO("guest_tsc:%ld", rdmsr(MSR_IA32_TSC_AUX));

	/* vmcall enter host check guest IA32_TSC_AUX vmcs field value is TSC_AUX_VALUE2*/
	vmx_set_test_condition(CON_CHECK_TSC_AUX_GUEST_VMCS_FIELD);
	vmcall();

	report("%s", (vm_exit_info.result == RET_CHECK_GUEST_TSC_AUX_VMCS), __func__);

}

static void exit_guest_msr_store_condition(__unused struct st_vcpu *vcpu)
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
	vmcs_write(VMX_EXIT_MSR_STORE_COUNT, MSR_AREA_COUNT);
	vmcs_write(VMX_EXIT_MSR_STORE_ADDR_FULL, hva2hpa((void *)vcpu->arch.msr_area.guest));
	DBG_INFO(" set vm-entry guest msr");
}

/* define for case 42238 */
static void hypercall_check_guest_tsc_aux_vmcs(__unused struct st_vcpu *vcpu)
{
	uint64_t guest_msr_tsc = vcpu->arch.msr_area.guest[MSR_AREA_TSC_AUX].value;
	/* make sure guest ia32_tsc_aux value is saved by processor */
	if (guest_msr_tsc == TSC_AUX_VALUE2) {
		vm_exit_info.result = RET_CHECK_GUEST_TSC_AUX_VMCS;
	}
	DBG_INFO("  test after guest_msr_tsc:%ld", \
		vcpu->arch.msr_area.guest[MSR_AREA_TSC_AUX].value);
}

static const uint8_t io_bitmap_size[io_byte_buff] = {
	0x1,
	0x3,
	0xf,
};

void set_io_bitmap(uint8_t *io_bitmap, uint32_t port_io, uint32_t is_bit_set, uint32_t bytes)
{
	uint32_t io_index = port_io / 8;
	uint8_t io_bit = port_io % 8;
	if (is_bit_set == BIT_TYPE_SET) {
		io_bitmap[io_index] |= (io_bitmap_size[bytes] << io_bit);
	} else {
		io_bitmap[io_index] &= ~(io_bitmap_size[bytes] << io_bit);
	}
	DBG_INFO("io_bitmap[%d]:0x%x  test_io_bit:%d",\
		io_index,\
		io_bitmap[io_index],\
		io_bit);
}

static void vm_exit_info_condition(__unused struct st_vcpu *vcpu)
{
	/*
	 * Enable VM_EXIT for port io read.
	 */
	exec_vmwrite32_bit(CPU_EXEC_CTRL0, CPU_IO_BITMAP, BIT_TYPE_SET);
	DBG_INFO("CPU_EXEC_CTRL0:0x%x", exec_vmread32(CPU_EXEC_CTRL0));

	uint8_t *io_bitmap_a = vcpu->arch.io_bitmap;
	/* Set up IO bitmap register A for bit[0xCFC~0xCFF] to 1, enable io access vm exit */
	set_io_bitmap(io_bitmap_a, IO_BITMAP_TEST_PORT_NUM1, BIT_TYPE_SET, io_byte4);

	uint64_t value64 = hva2hpa((void *)io_bitmap_a);
	vmcs_write(VMX_IO_BITMAP_A_FULL, value64);

}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Exit_Information_001
 *
 * Summary: IN root operation, Execute VMWRITE instruction to configure I/O bitmap
 * in the VM-execution controls field, to make IO read of specified port cause vm-exit.
 * Enter VMX non-root operation.
 * IO read from VMX non-root operation.
 * then cause vm-exit enter root operation
 * Execute VMREAD instruction to read VM-exit information from VMCS.
 * Verify that the VM-exit reason is IO read.
 */
__unused static void hsi_rqmid_42242_virtualization_specific_features_vm_exit_info_001()
{
	u8 chk = 0;
	/* init vm exit information */
	vm_exit_info.exit_reason = VM_EXIT_REASON_DEFAULT_VALUE;
	vm_exit_info.exit_qualification = INVALID_EXIT_QUALIFICATION_VALUE;

	inw(IO_BITMAP_TEST_PORT_NUM1);

	/* According SDM:
	 * Table C-1. Basic Exit Reasons
	 * exit reasons 30 is I/O instruction vm exit.
	 */

	if (get_exit_reason() == VMX_IO) {
		chk++;
	}

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
	uint32_t qualification = vm_exit_info.exit_qualification;
	if (((qualification & 0x7) == IO_ACCESS_SIZE_2BYTE) &&
		(((qualification >> 3) & 0x1) == INS_TYPE_IN) &&
		(((qualification >> 16) & 0xffff) == IO_BITMAP_TEST_PORT_NUM1)) {
		chk++;
	}
	DBG_INFO("exit_reason_val:0x%lx exit_qualification:0x%x",\
		vm_exit_info.exit_reason,\
		vm_exit_info.exit_qualification);

	report("%s", (chk == 2), __func__);
}

static void hypercall_entry_event_inject(__unused struct st_vcpu *vcpu)
{
	/* for test case vm-entry event inject */
	DBG_INFO("inject UD exception");
	/* According SDM Table 24-13 set as below */
	exec_vmwrite32(VMX_ENTRY_INT_INFO_FIELD,
		VMX_INT_INFO_VALID | (VMX_INT_TYPE_HW_EXP << 8U) | (IDT_UD));
}

static u8 count_ud_exception;
static void ud_handler(__unused struct ex_regs *regs)
{
	count_ud_exception++;
}

/**
 * @brief Case name:HSI_Virtualization_Specific_Features_VM_Entry_Control_For_Event_Injection_001
 *
 * Summary: IN root operation, execute VMWRITE config vm-entry interruption-information field as below:
 * set vection to #UD exception, interruption type to hardware exception, valid interruption.
 * switch to non-root operation,
 * check #UD exception handler function are called once.
 */
__unused static void hsi_rqmid_42250_virtualization_specific_features_entry_event_inject_001()
{
	u8 chk = 0;
	count_ud_exception = 0;
	handler old_fn = NULL;
	old_fn = handle_exception(UD_VECTOR, &ud_handler);
	/* inject a UD exception */
	vmx_set_test_condition(CON_ENTRY_EVENT_INJECT);
	vmcall();

	/* make sure UD handler function are called */
	if (count_ud_exception == UD_HANDLER_CALL_ONE_TIMES) {
		chk++;
	}
	/* resume environment */
	handle_exception(UD_VECTOR, old_fn);
	DBG_INFO("count_ud_exception:%u", count_ud_exception);
	report("%s", (chk == 1), __func__);
}


