#include "apic-defs.h"

.globl boot_idt
.globl pt_root

.globl cpu_online_count
.globl smp_stacktop

ipi_vector = 0x20

max_cpus = 128

.data

	. = . + 4096
	.align 16
stacktop:

	. = . + 4096 * max_cpus
	.align 16
ring0stacktop:

.align 4096
.globl ptl2
ptl2:
i = 0
	.rept 512 * 4
	.long 0x1e7 | (i << 21)
	.long 0
	i = i + 1
	.endr

.align 4096
ptl3:
	.long ptl2 + 7 + 0 * 4096
	.long 0
	.long ptl2 + 7 + 1 * 4096
	.long 0
	.long ptl2 + 7 + 2 * 4096
	.long 0
	.long ptl2 + 7 + 3 * 4096
	.long 0

.align 4096
ptl4:
	.long ptl3 + 7
	.long 0

.align 4096
ptl5:
	.long ptl4 + 7
	.long 0

.align 4096
pt:
i = 0
	.rept 1024
	.long 0x1e7 | (i << 22)
	i = i + 1
	.endr

.globl gdt32
gdt32:
	.quad 0
	.quad 0x00cf9b000000ffff // flat 32-bit code segment
	.quad 0x00cf93000000ffff // flat 32-bit data segment
	.quad 0x00cf1b000000ffff // flat 32-bit code segment, not present
	.quad 0                  // TSS for task gates
	.quad 0x008f9b000000FFFF // 16-bit code segment
	.quad 0x008f93000000FFFF // 16-bit data segment
	.quad 0x00cffb000000ffff // 32-bit code segment (user)
	.quad 0x00cff3000000ffff // 32-bit data segment (user)
	.quad 0                  // unused

	.quad 0			 // 6 spare selectors
	.quad 0
	.quad 0
	.quad 0
	.quad 0
	.quad 0

tss_descr:
	.rept max_cpus
	.quad 0x000089000000ffff // 32-bit avail tss
	.endr
gdt32_end:

i = 0
.globl tss
tss:
	.rept max_cpus
	.long 0
	.long ring0stacktop - i * 4096
	.long 16
	.quad 0, 0
	.quad 0, 0, 0, 0, 0, 0, 0, 0
	.long 0, 0, 0
	i = i + 1
	.endr
tss_end:

.align 16
boot_idt:
	.rept 256
	.quad 0
	.quad 0
	.endr
boot_idt_end:
idt_descr:
	.word 16 * 256 - 1
	.long boot_idt

pt_root:
	.long ptl4
	.long 0

.section .init

.code32

mb_magic = 0x1BADB002
mb_flags = 0x2

	# multiboot header
	.long mb_magic, mb_flags, 0 - (mb_magic + mb_flags)
mb_cmdline = 16

MSR_GS_BASE = 0xc0000101

.macro setup_percpu_area
	lea -4096(%esp), %eax
	mov $0, %edx
	mov $MSR_GS_BASE, %ecx
	wrmsr
.endm

.globl start
start:
#ifdef BP_STARTUP_CHECK
	#include BP_STARTUP_CHECK
#endif
	mov $stacktop, %esp
	push %ebx
	call bss_init
	call setup_multiboot
	call setup_libcflat
#ifndef IN_SAFETY_VM
	mov mb_cmdline(%ebx), %eax
	mov %eax, __args
	call __setup_args
#endif
	setup_percpu_area
	call prepare_32
	jmpl $8, $start32

prepare_32:
	lgdtl gdt32_descr

	mov %cr4, %eax
	bts $4, %eax  // pse
	mov %eax, %cr4

	mov $pt, %eax
	mov %eax, %cr3

	mov %cr0, %eax
	bts $0, %eax
	bts $31, %eax
	mov %eax, %cr0
	ret

smp_stacktop:	.long 0xa0000

ap_start32:
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	mov $-4096, %esp
	lock/xaddl %esp, smp_stacktop
	setup_percpu_area
	call prepare_32
	call enable_apic
	call load_tss
	cmpb $1, is_all_cpu_init
	je 2f
	call ap_record_lapicid
2:
#ifdef AP_UNCHANGED_CHECK
	call save_unchanged_reg
#endif
	sti
	nop
	lock incw cpu_online_count
#ifdef AP_USE_MAIN
	call ap_main
#endif
1:	hlt
	jmp 1b

start32:
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	call enable_apic
	call load_tss
	call mask_pic_interrupts
	call setup_smp_stacktop
	call smp_init
	push $__environ
	push $__argv
	push __argc
	call main
	push %eax
	call exit

load_tss:
	lidt idt_descr
	mov $16, %eax
	mov %ax, %ss
	movl $APIC_ID, %ecx
	shrl $4, %ecx
	addl $APIC_BASE_MSR, %ecx
	rdmsr
	mov %eax, %ebx
	shl $3, %ebx
	movl $0x00008900, tss_descr+4(%ebx)
	movl $0x0000ffff, tss_descr(%ebx)
	mov $((tss_end - tss) / max_cpus), %edx
	imul %edx
	add $tss, %eax
	mov %ax, tss_descr+2(%ebx)
	shr $16, %eax
	mov %al, tss_descr+4(%ebx)
	shr $8, %eax
	mov %al, tss_descr+7(%ebx)
	lea tss_descr-gdt32(%ebx), %eax
	ltr %ax
	ret
smp_init:
	cld
	lea sipi_entry, %esi
	xor %edi, %edi
	mov $(sipi_end - sipi_entry), %ecx
	rep/movsb

	// get local apic id of bp
	movl $0xb, %eax
	cpuid
	movl %edx, %ebx
	movl %ebx, lapicid_map

	call fwcfg_get_nb_cpus
	cmp $1, %eax
	je  smp_init_done

	call bringup_ap

smp_init_done:
	movb $1, is_all_cpu_init
	ret

cpu_online_count:	.word 1
is_all_cpu_init:	.byte 0

.code16
sipi_entry:
#ifdef AP_INIT_CHECK
	#include AP_INIT_CHECK
#endif
	mov %cr0, %eax
	or $1, %eax
	mov %eax, %cr0
	lgdtl gdt32_descr - sipi_entry
	ljmpl $8, $ap_start32

gdt32_descr:
	.word gdt32_end - gdt32 - 1
	.long gdt32

sipi_end:
