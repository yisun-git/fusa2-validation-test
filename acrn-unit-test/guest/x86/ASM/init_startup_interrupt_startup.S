bp_have_init = 0x8000
ap_cpu_lock = 0x8004
ap_cpu_count = 0x8008
bp_init_rflags = 0x800c
ap_startup_rflags = 0x8010
    mov $ap_startup_rflags, %edi
    mov $max_cpus, %ecx
    sub %eax, %eax
    mov %eax, (bp_have_init)
    mov %eax, (ap_cpu_lock)
    mov %eax, (ap_cpu_count)
    mov %eax, (bp_init_rflags)
    rep stosl
	movl $0x0f0fa55a, (bp_have_init)
	pushfl
	pop %eax
	mov %eax, (bp_init_rflags)

