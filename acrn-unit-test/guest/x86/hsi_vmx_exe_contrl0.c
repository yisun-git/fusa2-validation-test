/*
 * Disable VM_EXIT for invlpg execution.
 */
BUILD_CONDITION_FUNC(invlpg_exit, CPU_EXEC_CTRL0, \
	CPU_INVLPG, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_INVLPG_001
 *
 * Summary: IN root operation, clear Processor-Based VM-Execution Controls
 * INVLPG exiting bit to 0, switch to non-root operation,
 * execute instruction INVLPG, check vm-exit handler are not called for
 * this instruction in root operation.
 */
static void hsi_rqmid_40361_virtualization_specific_features_vm_exe_con_invlpg_001()
{
	void *gva = malloc(sizeof(u64));
	invlpg(gva);
	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);
	free(gva);
}

BUILD_CONDITION_FUNC(mwait_exit, CPU_EXEC_CTRL0, \
	CPU_MWAIT, BIT_TYPE_SET);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_MWAIT_001
 *
 * Summary: IN root operation, set Processor-Based VM-Execution Controls
 * MWAIT exiting bit10 to 1, switch to non-root operation,
 * execute MWAIT instruction, check vm-exit handler are called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40395_virtualization_specific_features_vm_exe_con_mwait_001()
{
	/* execute instruction in non-root operation , in root operation will skip this instruction */
	asm volatile("mwait" : : "a"(0), "c"(0) : );

	report("%s", (get_exit_reason() == VMX_MWAIT), __func__);
}

BUILD_CONDITION_FUNC(rdtsc_exit, CPU_EXEC_CTRL0, \
	CPU_RDTSC, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_RDTSC_001
 *
 * Summary: IN root operation, clear Processor-Based VM-Execution Controls
 * RDTSC exiting bit12 to 0, switch to non-root operation,
 * execute RDTSC instruction, check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40465_virtualization_specific_features_vm_exe_con_rdtsc_001()
{
	__unused unsigned a, d;
	asm volatile("rdtsc" : "=a"(a), "=d"(d));
	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);
}

BUILD_CONDITION_FUNC(rdpmc_exit, CPU_EXEC_CTRL0, \
	CPU_RDPMC, BIT_TYPE_SET);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_RDPMC_001
 *
 * Summary: IN root operation, set Processor-Based VM-Execution Controls
 * RDTSC exiting bit11 to 1, switch to non-root operation,
 * execute RDPMC instruction, check vm-exit handler are called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40466_virtualization_specific_features_vm_exe_con_rdpmc_001()
{
	__unused unsigned a, d;
	asm volatile("rdpmc" : "=a"(a), "=d"(d) : "c"(0) : );
	report("%s", (get_exit_reason() == VMX_RDPMC), __func__);
}

BUILD_CONDITION_FUNC(mov_dr_exit, CPU_EXEC_CTRL0, \
	CPU_MOV_DR, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_MOV_DR_001
 *
 * Summary: IN root operation, clear Processor-Based VM-Execution Controls
 * MOV-DR exiting bit23 to 0, switch to non-root operation,
 * execute [mov 0, dr4] instruction, check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40467_virtualization_specific_features_vm_exe_con_mov_dr_001()
{
	u64 tmp = 0;

	/* disable Debugging Extensions */
	write_cr4(read_cr4() & ~X86_CR4_DE);

	asm volatile ("movq %0, %%dr4" : : "r"(tmp) : );
	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);

	/* resume environment */
	write_cr4(read_cr4() | X86_CR4_DE);
}

u32 get_cr_access_reason(void)
{
	u32 reason;

	barrier();
	reason = vm_exit_info.cr_qualification;
	barrier();
	return reason;
}

BUILD_CONDITION_FUNC(cr8_store_exit, CPU_EXEC_CTRL0, \
	CPU_CR8_STORE, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_CR8_STORE_001
 *
 * Summary: IN root operation, clear Processor-Based VM-Execution Controls
 * CR8-STORE exiting bit20 to 0, switch to non-root operation,
 * mov from cr8, check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40485_virtualization_specific_features_vm_exe_con_cr8_store_001()
{
	__unused ulong cr8;

	asm volatile("mov %%cr8, %0" : "=r"(cr8) : : "memory");
	report("%s", (get_cr_access_reason() != VM_EXIT_STORE_CR8), __func__);
}

BUILD_CONDITION_FUNC(cr8_load_exit, CPU_EXEC_CTRL0, \
	CPU_CR8_LOAD, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_CR8_LOAD_001
 *
 * Summary: IN root operation, clear Processor-Based VM-Execution Controls
 * CR8-LOAD exiting bit19 to 0, switch to non-root operation,
 * mov to cr8, check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40486_virtualization_specific_features_vm_exe_con_cr8_load_001()
{
	__unused ulong cr8 = read_cr8();
	asm volatile("mov %0, %%cr8" : : "r"(cr8) : );

	report("%s", (get_cr_access_reason() != VM_EXIT_LOAD_CR8), __func__);
}

BUILD_CONDITION_FUNC(cr3_load_exit, CPU_EXEC_CTRL0, \
	CPU_CR3_LOAD, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_CR3_STORE_001
 *
 * Summary: IN root operation, clear Processor-Based VM-Execution Controls
 * CR3-LOAD exiting bit15 to 0, switch to non-root operation,
 * mov to cr3, check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40487_virtualization_specific_features_vm_exe_con_cr3_load_001()
{
	ulong cr3 = read_cr3();
	asm volatile("mov %0, %%cr3" : : "r"(cr3) : );

	report("%s", (get_cr_access_reason() != VM_EXIT_LOAD_CR3), __func__);
}

BUILD_CONDITION_FUNC(cr3_store_exit, CPU_EXEC_CTRL0, \
	CPU_CR3_STORE, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_CR3_STORE_001
 *
 * Summary: IN root operation, clear Processor-Based VM-Execution Controls
 * CR3-STORE exiting bit16 to 0, switch to non-root operation,
 * mov from cr3, check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40488_virtualization_specific_features_vm_exe_con_cr3_store_001()
{
	__unused ulong cr3;
	asm volatile("mov %%cr3, %0" : "=r"(cr3) : : "memory");

	report("%s", (get_cr_access_reason() != VM_EXIT_STORE_CR3), __func__);
}

BUILD_CONDITION_FUNC(pause_exit, CPU_EXEC_CTRL0, \
	CPU_PAUSE, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_PAUSE_001
 *
 * Summary: IN root operation, clear Processor-Based VM-Execution Controls
 * PAUSE exiting bit30 to 0, switch to non-root operation,
 * execute pause instruction, check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40519_virtualization_specific_features_vm_exe_con_pause_001()
{
	asm volatile("pause");

	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);
}

BUILD_CONDITION_FUNC(monitor_exit, CPU_EXEC_CTRL0, \
	CPU_MONITOR, BIT_TYPE_SET);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_MONITOR_001
 *
 * Summary: IN root operation, set Processor-Based VM-Execution Controls
 * MONITOR exiting bit29 to 1, switch to non-root operation,
 * execute MONITOR instruction, check vm-exit handler are called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40521_virtualization_specific_features_vm_exe_con_monitor_001()
{
	static u32 monitor_val = 0;
	asm volatile("monitor" :
		: "a"(&monitor_val), "c"(0ULL), "d"(0ULL)
		: "memory");

	report("%s", (get_exit_reason() == VMX_MONITOR), __func__);
}

static void unconditional_io_exit_condition(__unused struct st_vcpu *vcpu)
{
	/*
	 * Disable VM_EXIT for test condition.
	 */
	exec_vmwrite32_bit(CPU_EXEC_CTRL0, CPU_IO, BIT_TYPE_CLEAR);

	/* As sdm section 25.1.3 vol3 said: the I/O instruction
	 * causes a VM exit (the “unconditional I/O exiting”
	 * VM-execution control is ignored if the “use I/O bitmaps”
	 * VM-execution control is 1).So we disable I/O bitmaps exiting.
	 */
	vmcs_clear_bits(CPU_EXEC_CTRL0, CPU_IO_BITMAP);
	DBG_INFO("CPU_EXEC_CTRL0:0x%x", exec_vmread32(CPU_EXEC_CTRL0));
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Unconditional_IO_001
 *
 * Summary: IN root operation, clear Processor-Based VM-Execution Controls
 * Unconditional I/O exiting exiting bit24 and Use I/O bitmaps exiting bit25 to 0,
 * switch to non-root operation,
 * execute instruction [INL 0x71], check vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40492_virtualization_specific_features_vm_exe_con_unconditional_io_001()
{
	vm_exit_info.is_io_ins_exit = VM_EXIT_NOT_OCCURS;

	inl(IN_BYTE_TEST_PORT_ID);

	/* Make sure vm-exit not called in root operation */
	barrier();
	report("%s", (vm_exit_info.is_io_ins_exit != VM_EXIT_OCCURS), __func__);

}

static void io_bitmap_exit_condition(__unused struct st_vcpu *vcpu)
{
	/*
	 * Enable VM_EXIT for test condition.
	 */
	exec_vmwrite32_bit(CPU_EXEC_CTRL0, CPU_IO_BITMAP, BIT_TYPE_SET);

	DBG_INFO("CPU_EXEC_CTRL0:0x%x", exec_vmread32(CPU_EXEC_CTRL0));

	uint8_t *io_bitmap_a = vcpu->arch.io_bitmap;
	/* Set up IO bitmap register A for bit[0x71~0x74] to 1, enable io access vm exit */
	set_io_bitmap(io_bitmap_a, IN_BYTE_TEST_PORT_ID, BIT_TYPE_SET, io_byte4);

	uint64_t value64 = hva2hpa((void *)io_bitmap_a);
	vmcs_write(VMX_IO_BITMAP_A_FULL, value64);
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_IO_Bitmap_001
 *
 * Summary: IN root operation, set Processor-Based VM-Execution Controls
 * use I/O bitmaps exiting bit25 to 1, set I/O-bitmap A bit[0x71] to 1 enable vm exit
 * switch to non-root operation,
 * execute instruction [INL 0x71], check vm-exit handler are called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40495_virtualization_specific_features_vm_exe_con_io_bitmap_001()
{
	vm_exit_info.is_io_ins_exit = VM_EXIT_NOT_OCCURS;
	inl(IN_BYTE_TEST_PORT_ID);

	/* Make sure vm-exit are called in root operation */
	barrier();
	report("%s", (vm_exit_info.is_io_ins_exit == VM_EXIT_OCCURS), __func__);
}

static void msr_bitmap_rdmsr_exit_condition(__unused struct st_vcpu *vcpu)
{
	/*
	 * Enable VM_EXIT for test condition.
	 */
	exec_vmwrite32_bit(CPU_EXEC_CTRL0, CPU_MSR_BITMAP, BIT_TYPE_SET);

	DBG_INFO("CPU_EXEC_CTRL0:0x%x", exec_vmread32(CPU_EXEC_CTRL0));

	uint8_t *msr_bitmap = vcpu->arch.msr_bitmap;
	/* Set up read bitmap for low MSR bit[0x00000277U] to 1, enable msr access vm exit */
	set_msr_bitmap(msr_bitmap,
		TEST_MSR_BITMAP_MSR, BIT_TYPE_SET, MSR_READ);

	uint64_t value64 = hva2hpa((void *)msr_bitmap);
	vmcs_write(MSR_BITMAP, value64);

}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_MSR_Bitmap_RDMSR_001
 *
 * Summary: IN root operation, set Processor-Based VM-Execution Controls
 * use msr bitmaps exiting bit28 to 1, set read bitmap for low MSR bit[0x277] to 1 enable vm exit
 * switch to non-root operation,
 * execute instruction rdmsr with ECX = 0x00000277U, check vm-exit handler are called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40501_virtualization_specific_features_vm_exe_con_msr_bitmap_rdmsr_001()
{
	u8 chk = 0;
	vm_exit_info.is_msr_ins_exit = VM_EXIT_NOT_OCCURS;
	rdmsr(TEST_MSR_BITMAP_MSR);

	/* Make sure vm-exit are called in root operation */
	barrier();
	if (vm_exit_info.is_msr_ins_exit == VM_EXIT_OCCURS) {
		chk++;
	}

	if (get_exit_reason() == VMX_RDMSR) {
		chk++;
	}

	report("%s", (chk == 2), __func__);
}

static void msr_bitmap_wrmsr_exit_condition(__unused struct st_vcpu *vcpu)
{
	/*
	 * Enable VM_EXIT for test condition.
	 */
	exec_vmwrite32_bit(CPU_EXEC_CTRL0, CPU_MSR_BITMAP, BIT_TYPE_SET);

	DBG_INFO("CPU_EXEC_CTRL0:0x%x", exec_vmread32(CPU_EXEC_CTRL0));

	uint8_t *msr_bitmap = vcpu->arch.msr_bitmap;
	/* Set up write bitmap for low MSR for bit[0x00000277U] to 1, enable msr write vm exit */
	set_msr_bitmap(msr_bitmap,
		TEST_MSR_BITMAP_MSR, BIT_TYPE_SET, MSR_WRITE);

	uint64_t value64 = hva2hpa((void *)msr_bitmap);
	vmcs_write(MSR_BITMAP, value64);

}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_MSR_Bitmap_WRMSR_001
 *
 * Summary: IN root operation, set Processor-Based VM-Execution Controls
 * use msr bitmaps exiting bit28 to 1, set write bitmap for low MSR bit[2048*8 + 0x277] to 1 enable vm exit
 * switch to non-root operation,
 * execute instruction wrmsr with ECX = 0x00000277U, check vm-exit handler are called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_40503_virtualization_specific_features_vm_exe_con_msr_bitmap_wrmsr_001()
{
	u8 chk = 0;
	vm_exit_info.is_msr_ins_exit = VM_EXIT_NOT_OCCURS;
	wrmsr(TEST_MSR_BITMAP_MSR, MSR_CACHE_TYPE_WB_VALUE);

	/* Make sure vm-exit are called in root operation */
	barrier();
	if (vm_exit_info.is_msr_ins_exit == VM_EXIT_OCCURS) {
		chk++;
	}

	if (get_exit_reason() == VMX_WRMSR) {
		chk++;
	}
	report("%s", (chk == 2), __func__);
}

void set_msr_bitmap(uint8_t *msr_bitmap, uint32_t msr,
	uint32_t is_bit_set, uint8_t is_rd_wt)
{

	/**
	 * Clear high 19-bits of 'msr'.
	 * High 19-bits of 'msr' can be used to distinguish low MSRs and high MSRs. (This information has been
	 * detected with previous 'if' branch.)
	 * Low 13-bits of 'msr' can be used to further detect the location of the corresponding bits.
	 */
	uint32_t msr_offset = msr & 0x1FFFU;
	/* Set 'msr_index' to 1/8 of 'msr' (round down) */
	uint32_t test_msr_index = msr_offset / 8;
	uint8_t test_msr_bit = msr_offset % 8;

	/* if high msr, then set offset */
	if (HIGH_MSR_START_ADDR & msr) {
		/** Increment 'test_msr_index' by 1024, the new value is the offset of the read bitmap in the MSR bitmap
		 *  for high MSRs
		 */
		test_msr_index = test_msr_index + 1024U;
	}
	if ((is_rd_wt == MSR_READ) || (is_rd_wt == MSR_READ_WRITE))
	{
		if (is_bit_set == BIT_TYPE_SET) {
			msr_bitmap[test_msr_index] |= (0x1 << test_msr_bit);
		} else {
			msr_bitmap[test_msr_index] &= ~(0x1 << test_msr_bit);
		}
	}

	/* set wrmsr vm exit bitmap */
	if ((is_rd_wt == MSR_WRITE) || (is_rd_wt == MSR_READ_WRITE))
	{
		test_msr_index += MSR_WRITE_BITMAP_OFFSET;
		if (is_bit_set == BIT_TYPE_SET) {
			msr_bitmap[test_msr_index] |= (0x1 << test_msr_bit);
		} else {
			msr_bitmap[test_msr_index] &= ~(0x1 << test_msr_bit);
		}
	}
	DBG_INFO("msr_bitmap[%d]:0x%x  test_msr_bit:%d",\
		test_msr_index,\
		msr_bitmap[test_msr_index],\
		test_msr_bit);
}

static void activate_secon_con_condition(__unused struct st_vcpu *vcpu)
{
	/*
	 * Enable the secondary processor-based VM-execution controls
	 */
	exec_vmwrite32_bit(CPU_EXEC_CTRL0, CPU_SECONDARY, BIT_TYPE_SET);

	/**
	 * Disable vm exit for RDTSCP instruction,
	 * as SDM said: RDTSCP. Guest software attempted to execute
	 * RDTSCP and the “enable RDTSCP” and “RDTSC exiting” VM-execution controls were both 1
	 */
	vmcs_clear_bits(CPU_EXEC_CTRL0, CPU_RDTSC);
	DBG_INFO("CPU_EXEC_CTRL0:0x%x", exec_vmread32(CPU_EXEC_CTRL0));

	/* Enable RDTSCP at secondary processor-based */
	vmcs_set_bits(CPU_EXEC_CTRL1, CPU_RDTSCP);
	DBG_INFO("CPU_EXEC_CTRL1:0x%x", exec_vmread32(CPU_EXEC_CTRL1));
}

static unsigned rdtscp_check(void)
{
	__unused unsigned a, d, c;
	asm volatile (ASM_TRY("1f")
		"rdtscp \n\t"
		"1:"
		: "=a"(a), "=d"(d), "=c"(c));
	return exception_vector();
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Activate_Secondary_Controls_001
 *
 * Summary: IN root operation, set Processor-Based VM-Execution Controls
 * Activate secondary controls bit31 to 1, enable the secondary processor-based VM-execution controls,
 * set secondary processor-based VM-execution bit3 to 1, then enable execution of RDTSCP,
 * switch to non-root operation,
 * execute instruction RDTSCP, check this instruction executed successful and no #UD exception occurs.
 */
__unused static void hsi_rqmid_40621_virtualization_specific_features_vm_exe_con_activate_secon_con_001()
{
	u8 chk = 0;
	if (rdtscp_check() == NO_EXCEPTION) {
		chk++;
	}
	report("%s", (chk == 1), __func__);
}

BUILD_CONDITION_FUNC(nmi_window, CPU_EXEC_CTRL0, \
	CPU_NMI_WINDOW, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_NMI_Window_Exiting_001
 *
 * Summary: IN root operation, clear primary Processor-Based VM-Execution Controls
 * NMI-window exiting bit22 to 0, switch to non-root operation,
 * execute any instructions, check NMI-window vm-exit handler are not called for
 * this action in root operation.
 */
__unused static void hsi_rqmid_41088_virtualization_specific_features_vm_exe_con_nmi_window_001()
{

	/* do nothing at non-root operation make sure no nmi-window vm-exit occurs */
	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);

}

static void active_preemption_timer_condition(__unused struct st_vcpu *vcpu)
{
	/* Set value to VMX-preemption timer-value */
	exec_vmwrite32(VMX_PREEMPTION_TIMER, PREEMPTION_TIME_VALUE);

	/* Disable activate VMX preemption timer at pin-based execution control */
	exec_vmwrite32_bit(PIN_CONTROLS, PIN_PREEMPT, BIT_TYPE_CLEAR);
	DBG_INFO("PIN_CONTROLS:0x%x\n", exec_vmread32(PIN_CONTROLS));
}

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Activate_VMX_Preemption_Timer_001
 *
 * Summary: IN root operation, clear pin-based execution control
 * activate VMX preemption timer bit6 to 0,
 * set VMX-preemption timer value to 1000,
 * switch to non-root operation,
 * delay 2000 TSC, check preemption timer expired vm exit are not called.
 */
__unused static void hsi_rqmid_41091_virtualization_specific_features_vm_exe_con_active_preemption_timer_001()
{
	u64 now_tsc = rdtsc();
	while ((rdtsc() - now_tsc) < (2 * PREEMPTION_TIME_VALUE)) {
		nop();
	}

	/*
	 * check if there is preemption timer
	 * counted down causes vm exit.
	 */
	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);

}

BUILD_CONDITION_FUNC(monitor_trap_flag, CPU_EXEC_CTRL0, \
	CPU_MONITOR_TRAP_FLAG, BIT_TYPE_CLEAR);

/**
 * @brief Case name: HSI_Virtualization_Specific_Features_VM_Execution_Controls_Monitor_Trap_Flag_001
 *
 * Summary: IN root operation, clear primary Processor-Based VM-Execution Controls
 * Monitor trap flag bit27 to 0, switch to non-root operation,
 * execute any instruction, check vm-exit handler are not called for
 * monitor trap flag in root operation.
 */
__unused static void hsi_rqmid_41121_virtualization_specific_features_vm_exe_con_monitor_trap_flag_001()
{
	/* do nothing at non-root operation make sure no MTF vm-exit occurs */
	report("%s", (get_exit_reason() == VM_EXIT_REASON_DEFAULT_VALUE), __func__);
}

static void use_tsc_offset_condition(__unused struct st_vcpu *vcpu)
{
	/* Enable use TSC offset in primary processor-based */
	exec_vmwrite32_bit(CPU_EXEC_CTRL0,
		CPU_TSC_OFFSET, BIT_TYPE_SET);
	DBG_INFO("CPU_EXEC_CTRL0:0x%x", exec_vmread32(CPU_EXEC_CTRL0));

	/* Disable RDTSC exiting in primary processor-based */
	vmcs_clear_bits(CPU_EXEC_CTRL0, CPU_RDTSC);

	/* set TSC offset */
	vmcs_write(VMX_TSC_OFFSET_FULL, TSC_OFFSET_DEFAULT_VALUE);

	/* set real TSC to 0 */
	wrmsr(MSR_IA32_TIME_STAMP_COUNTER, 0);
	DBG_INFO("host_rdtsc:%llx", rdtsc());

}

/**
 * @brief Case name:HSI_Virtualization_Specific_Features_VM_Execution_Controls_Use_TSC_Offsetting_001
 *
 * Summary: IN root operation, set primary Processor-Based VM-Execution Controls
 * use TSC offsetting bit3 to 1 and RDTSC exiting bit12 to 0,
 * set TSC offsetting to a big value 0x100000000000, clear physical TSC to 0
 * switch to non-root operation,
 * execute RDTSC read TSC immediately, check the TSC value should more than
 * the TSC offsetting 0x100000000000, So the TSC offsetting is used in guest.
 */
__unused static void hsi_rqmid_41175_virtualization_specific_features_vm_exe_con_use_tsc_offset_001()
{
	u64 guest_tsc = (u64)rdtsc();
	u8 chk = 0;
	if (guest_tsc > TSC_OFFSET_DEFAULT_VALUE) {
		chk++;
	}
	DBG_INFO("guest_rdtsc:%lx", guest_tsc);

	report("%s", (chk == 1), __func__);
}

