bp_have_init = 0x8000
ap_cpu_lock = 0x8004
ap_cpu_count = 0x8008
bp_init_rflags = 0x800c
ap_startup_rflags = 0x8010
.ap.1:
	mov $0, %eax
	movl $1, %ecx
	lock cmpxchgl %ecx, (ap_cpu_lock)
	jnz .ap.1
	incl (ap_cpu_count)
	mov (ap_cpu_count), %eax
	dec %eax
	mov $4, %ecx
	mul %ecx
	mov %eax, %ecx
	pushfl
	pop %eax
	mov %eax, ap_startup_rflags(%ecx)
	lock decl (ap_cpu_lock)
