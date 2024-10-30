## Common patches

1. switch_apicv_mode_x2apic.patch (Workaround)
	JIRA issue ID: ACRN-10399 (Guest #GP when read x2APIC MSRs after vCPU receiving SIPI)

2. enable_local_apic.patch
	Fix acrn build error when enabling local apic.
3. enable_rdt.patch
	JIRA issue ID: ACRN-10719 (ACRN log show "Unhandled exception: 0 (Divide Error)" and generates coredump due to ENABLED RDT)
4. increase_mmap_entries.patch
	JIRA issue ID: ACRN-11130 (Acrn log show"Unhandled exception: 14 (Page Fault)" on mbl due to too few mmap defined by acrn)
5. increase_vuart_tx_buf_size.patch
	To resolve the issue of incomplete printing of feature case results when there are many cases.
## Features patches
Machine check
0001-MCA-expose-MCA-to-VM0-and-hide-from-others.patch
	JIRA issue ID: ACRN-10753 (Hide/expose some MSRs to meet machine check req)

