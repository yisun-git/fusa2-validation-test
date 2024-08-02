## Common patches

1. switch_apicv_mode_x2apic.patch (Workaround)
	JIRA issue ID: ACRN-10399 (Guest #GP when read x2APIC MSRs after vCPU receiving SIPI)

2. enable_local_apic.patch
	Fix acrn build error when enabling local apic.
## Features patches
Machine check
0001-MCA-expose-MCA-to-VM0-and-hide-from-others.patch
	JIRA issue ID: ACRN-10753 (Hide/expose some MSRs to meet machine check req)

