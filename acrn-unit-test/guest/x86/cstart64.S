#include "apic-defs.h"
#ifdef IN_PAGING_TEST
#include "paging.h"
#endif
#ifdef IN_GP_TEST
#include "general_purpose.h"
#endif

#ifdef IN_TSC_TEST
#include "tsc.h"
#endif

.globl boot_idt

.globl idt_descr
.globl tss_descr
.globl gdt64_desc
.globl cpu_online_count
.globl smp_stacktop
/*Adding for GP at X64 environment*/
.globl gdt64

ipi_vector = 0x20

max_cpus = 128

.section stack

	. = . + 4096
	.align 16
stacktop:

	. = . + 4096 * max_cpus
	.align 16
ring0stacktop:

.data

.align 4096
.globl ptl2
ptl2:
i = 0
	.rept 512 * 4
	.quad 0x1e7 | (i << 21)
	i = i + 1
	.endr

.align 4096
ptl3:
	.quad ptl2 + 7 + 0 * 4096
	.quad ptl2 + 7 + 1 * 4096
	.quad ptl2 + 7 + 2 * 4096
	.quad ptl2 + 7 + 3 * 4096

.align 4096
ptl4:
	.quad ptl3 + 7

.align 4096
ptl5:
	.quad ptl4 + 7

.align 4096

gdt64_desc:
	.word gdt64_end - gdt64 - 1
	.quad gdt64

gdt64:
	.quad 0
	.quad 0x00af9b000000ffff // 64-bit code segment
	.quad 0x00cf93000000ffff // 32/64-bit data segment
	.quad 0x00af1b000000ffff // 64-bit code segment, not present
	.quad 0x00cf9b000000ffff // 32-bit code segment
	.quad 0x008f9b000000FFFF // 16-bit code segment
	.quad 0x008f93000000FFFF // 16-bit data segment
	.quad 0x00cffb000000ffff // 32-bit code segment (user)
	.quad 0x00cff3000000ffff // 32/64-bit data segment (user)
	.quad 0x00affb000000ffff // 64-bit code segment (user)

	.quad 0			 // 6 spare selectors
	.quad 0
	.quad 0
	.quad 0
	.quad 0
	.quad 0

tss_descr:
	.rept max_cpus
	.quad 0x000089000000ffff // 64-bit avail tss
	.quad 0                  // tss high addr
	.endr
gdt64_end:

i = 0
.globl tss
tss:
	.rept max_cpus
	.long 0
	.quad ring0stacktop - i * 4096
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
mb_boot_info:	.quad 0

pt_root:	.quad ptl4


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
#ifdef BP_HLT_TEST
	hlt
#endif

#ifdef BP_STARTUP_CHECK
	#include BP_STARTUP_CHECK
#endif
	mov %ebx, mb_boot_info
	mov $stacktop, %esp

/* get BP EIP starup value */
#ifdef IN_MULTIBOOT_TEST
#ifdef IN_SAFETY_VM
	call 11f
11:
	pop %edx
	mov %edx, (0x6004)
#endif
#endif

	setup_percpu_area
	call prepare_64
	jmpl $8, $start64

switch_to_5level:
	/* Disable CR4.PCIDE */
	mov %cr4, %eax
	btr $17, %eax
	mov %eax, %cr4

	mov %cr0, %eax
	btr $31, %eax
	mov %eax, %cr0

	mov $ptl5, %eax
	mov %eax, pt_root

	/* Enable CR4.LA57 */
	mov %cr4, %eax
	bts $12, %eax
	mov %eax, %cr4

	call enter_long_mode
	jmpl $8, $lvl5

prepare_64:
	lgdt gdt64_desc
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

enter_long_mode:
	mov %cr4, %eax
	bts $5, %eax  // pae
	mov %eax, %cr4

	mov pt_root, %eax
	mov %eax, %cr3

efer = 0xc0000080
	mov $efer, %ecx
	rdmsr
	bts $8, %eax
	wrmsr

	mov %cr0, %eax
	bts $0, %eax
	bts $31, %eax
	mov %eax, %cr0
	ret

smp_stacktop:	.long 0xa0000

.align 16

gdt32:
	.quad 0
	.quad 0x00cf9b000000ffff // flat 32-bit code segment
	.quad 0x00cf93000000ffff // flat 32-bit data segment
gdt32_end:

.code16
sipi_entry:
#ifdef AP_HLT_TEST
	hlt
#endif
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

.code32
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
	call prepare_64
	ljmpl $8, $ap_start64

.code64
ap_start64:
#ifdef IN_PAGING_TEST
	mov %cr3, %rax
	mov %rax, (INIT_64BIT_CR3_ADDR)
#endif
#ifdef IN_GP_TEST
	movq %r8, (INIT_R8_ADDR)
	movq %r9, (INIT_R9_ADDR)
	movq %r10, (INIT_R10_ADDR)
	movq %r11, (INIT_R11_ADDR)
	movq %r12, (INIT_R12_ADDR)
	movq %r13, (INIT_R13_ADDR)
	movq %r14, (INIT_R14_ADDR)
	movq %r15, (INIT_R15_ADDR)
#endif
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

start64:
#ifdef BP_STARTUP_CHECK_64
	#define RUN_IN_64BIT
	#include BP_STARTUP_CHECK
#endif
#ifdef IN_PAGING_TEST
	mov %cr3, %rax
	movq %rax, (STARTUP_64BIT_CR3_ADDR)
#endif
	call bss_init
	call enable_apic
	call load_tss
	call mask_pic_interrupts
	mov mb_boot_info(%rip), %rbx
	mov %rbx, %rdi
	call setup_smp_stacktop
	call smp_init
	mov mb_boot_info(%rip), %rbx
	mov %rbx, %rdi
	call setup_multiboot
	call setup_libcflat
#ifndef IN_SAFETY_VM
	mov mb_cmdline(%rbx), %eax
	mov %rax, __args(%rip)
	call __setup_args
#endif
	mov __argc(%rip), %edi
	lea __argv(%rip), %rsi
	lea __environ(%rip), %rdx
	call main
	mov %eax, %edi
	call exit

.globl setup_5level_page_table
setup_5level_page_table:
	/* Check if 5-level paging has already enabled */
	mov %cr4, %rax
	test $12, %eax
	jnz lvl5

	pushq $32
	pushq $switch_to_5level
	lretq
lvl5:
	retq

idt_descr:
	.word 16 * 256 - 1
	.quad boot_idt

load_tss:
	lidtq idt_descr

	movl $APIC_ID, %ecx
	shrl $4, %ecx
	addl $APIC_BASE_MSR, %ecx
	rdmsr
	mov %eax, %ebx
	shl $4, %ebx
	movl $0, tss_descr+8(%rbx)
	movl $0, tss_descr+12(%rbx)
	movl $0x00008900, tss_descr+4(%rbx)
	movl $0x0000ffff, tss_descr(%rbx)
	mov $((tss_end - tss) / max_cpus), %edx
	imul %edx
	add $tss, %rax
	mov %ax, tss_descr+2(%rbx)
	shr $16, %rax
	mov %al, tss_descr+4(%rbx)
	shr $8, %rax
	mov %al, tss_descr+7(%rbx)
	shr $8, %rax
	mov %eax, tss_descr+8(%rbx)
	lea tss_descr-gdt64(%rbx), %rax
	ltr %ax
	ret

smp_init:
	cld
	lea sipi_entry, %rsi
	xor %rdi, %rdi
	mov $(sipi_end - sipi_entry), %rcx
	rep/movsb

	lea sipi_entry, %rsi
	mov $4096, %rdi
	mov $(sipi_end - sipi_entry), %rcx
	rep/movsb

	lea sipi_entry, %rsi
	mov $8192, %rdi
	mov $(sipi_end - sipi_entry), %rcx
	rep/movsb

	// get local apic id of bp
	movl $0xb, %eax
	cpuid
	movl %edx, %ebx
	movl %ebx, lapicid_map

	movl $APIC_ICR, %ecx
	shrl $4, %ecx
	addl $APIC_BASE_MSR, %ecx

/* only for TSC initial value test */
#ifdef IN_TSC_TEST
		push %rcx
		push %rax
		push %rdx
		mov $MSR_TSC, %ecx
		rdmsr
		mov %eax, (BP_TSC_LOW_ADDR)
		mov %edx, (BP_TSC_HIGH_ADDR)
		pop %rdx
		pop %rax
		pop %rcx
#endif

	call fwcfg_get_nb_cpus
	cmp $1, %eax
	je  smp_init_done

	call bringup_ap

smp_init_done:
	movb $1, is_all_cpu_init
	ret

cpu_online_count:	.word 1
is_all_cpu_init:	.byte 0
