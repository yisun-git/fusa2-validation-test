#include "apic-defs.h"
#include "hsi_16_64_cpumode.h"

.globl boot_idt

.globl idt_descr
.globl tss_descr
.globl gdt64_desc
.globl cpu_online_count
/*Adding for GP at X64 environment*/
.globl gdt64

max_cpus = 64
AP_CODE_OFFSET = 0x4000
IPI_VECTOR = (AP_CODE_OFFSET >> 12)

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
/*only  safte vm image does not have multiboot header*/
#ifndef IN_SAFETY_VM
	# multiboot header
	.long mb_magic, mb_flags, 0 - (mb_magic + mb_flags)
#endif
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
	mov %ebx, mb_boot_info
	mov $stacktop, %esp

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

smp_stacktop:	.long 0x10000

.align 16

gdt32:
	.quad 0
	.quad 0x00cf9b000000ffff // flat 32-bit code segment
	.quad 0x00cf93000000ffff // flat 32-bit data segment
gdt32_end:

.section .multiboot_header, "a"
mb_magic = 0x1BADB002
mb_flags = 0x2
	# multiboot header
	.long mb_magic, mb_flags, 0 - (mb_magic + mb_flags)

.section .trampoline_reset, "ax"
.align 4
.code16
sipi_entry:

	cli

	/* save CR0 */
	mov %cr0, %eax
	mov %eax, (INIT_CR0_ADDR)

	mov %cs, %ax
	mov %ax, %ds

	/* Set DE, PAE, MCE and OS support bits in CR4 */
	/* 0x00000668 =
	 *	  (CR4_DE | CR4_PAE | CR4_MCE | CR4_OSFXSR | CR4_OSXMMEXCPT) */
	movl	$0x00000668, %eax
	mov 	%eax, %cr4

	/* Set CR3 to PML4 table address */
	movl	$cpu_boot_page_tables_ptr, %ebx
	subl $AP_CODE_OFFSET, %ebx
	mov 	(%ebx), %eax
	mov 	%eax, %cr3

	/* Set LME bit in EFER */
	/* 0xc0000080 = MSR_IA32_EFER */
	movl	$0xc0000080, %ecx
	rdmsr
	/* 0x00000100 = MSR_IA32_EFER_LME_BIT */
	orl 	$0x00000100, %eax
	wrmsr

	/* 0xc0000080 = MSR_IA32_EFER */
	movl	$0xc0000080, %ecx
	rdmsr
	/* 0x00000800 = MSR_IA32_EFER_NXE_BIT */
	orl 	$0x00000800, %eax
	wrmsr

	/* Enable paging, protection, numeric error and co-processor
	   monitoring in CR0 to enter long mode */

	mov 	%cr0, %ebx
	/* 0x80000023 = (CR0_PG | CR0_PE | CR0_MP | CR0_NE) */
	orl 	$0x80000023, %ebx
	mov 	%ebx, %cr0

	/* Load temportary GDT pointer value */
	lgdt	(trampoline_gdt_ptr - sipi_entry)

	/* Perform a long jump based to start executing in 64-bit mode */
	movl	$trampoline_start64_fixup, %ebx
	subl  $AP_CODE_OFFSET, %ebx
	ljmpl	*(%ebx)

	.align 8
	.global trampoline_start64_fixup
trampoline_start64_fixup:
	.long	trampoline_start64
	/* 0x0008 = HOST_GDT_RING0_CODE_SEL */
	.word	0x0008

	.code64
trampoline_start64:
	/* Set up all other data segment registers */
	/* 0x0010 = HOST_GDT_RING0_DATA_SEL */
	lock incw cpu_online_count

	movl	$0x0010, %eax
	mov 	%eax, %ss
	mov 	%eax, %ds
	mov 	%eax, %es
	mov 	%eax, %fs
	mov 	%eax, %gs

	mov $-4096, %esp
	lock/xaddl %esp, smp_stacktop

	/* Jump to C entry */
	cmpb $1, is_all_cpu_init
	je 2f
	call ap_record_lapicid
2:	call ap_main
#	movq	main_entry(%rip), %rax
#	jmp 	*%rax
0:	hlt
	jmp 0b

/* main entry */
	.align	8
	.global main_entry
main_entry:
	.quad	ap_main /* default entry is AP start entry */

	.align	8
	.global secondary_cpu_stack
secondary_cpu_stack:
	.rept	256
	.byte	0
	.endr

/* GDT table */
	.align	4
trampoline_gdt:
	.quad	0x0000000000000000
	.quad	0x00af9b000000ffff
	.quad	0x00cf93000000ffff
trampoline_gdt_end:

/* GDT pointer */
	.align	2
	.global trampoline_gdt_ptr
trampoline_gdt_ptr:
	.short	(trampoline_gdt_end - trampoline_gdt) - 1
	.quad	trampoline_gdt

/* PML4 and PDPT tables initialized to map first 4 GBytes of memory */
/* Assumes CPU supports 1GB large pages */
    .align  4
    .global cpu_boot_page_tables_ptr
cpu_boot_page_tables_ptr:
    .long cpu_boot_page_tables_start

    /*0x1000 = PAGE_SIZE*/
    .align  0x1000
    .global cpu_boot_page_tables_start
cpu_boot_page_tables_start:
    /* 0x3 = (PAGE_PRESENT | PAGE_RW) */
    .quad   trampoline_pdpt_addr + 0x3
    /*0x1000 = PAGE_SIZE*/
    .align  0x1000
    .global trampoline_pdpt_addr
trampoline_pdpt_addr:
    address = 0
    .rept   4
    /* 0x83 = (PAGE_PRESENT | PAGE_PS | PAGE_RW) */
    .quad   address + 0x83
    address = address + 0x40000000
    .endr
sipi_end:

start64:

	call bss_init
	call enable_apic
	call load_tss
	call mask_pic_interrupts
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
	mov $AP_CODE_OFFSET, %rdi
	mov $(sipi_end - sipi_entry), %rcx
	rep/movsb

	movl $APIC_ICR, %ecx
	shrl $4, %ecx
	addl $APIC_BASE_MSR, %ecx

	call fwcfg_get_nb_cpus
	movl %eax, %edx
	cmp $1, %edx
	je  2f
#ifdef IN_NATIVE
	shll $1, %edx
#endif
	subl $1, %edx
	#mov %edx, (0x9000)
1:
	movl $(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT), %eax
	wrmsr
	movl $(APIC_DEST_PHYSICAL | APIC_DM_INIT), %eax
	wrmsr
	movl $(APIC_DEST_PHYSICAL | APIC_DM_STARTUP | IPI_VECTOR), %eax
	wrmsr
	subl $1, %edx
	cmp $0, %edx

	jnz 1b
	call fwcfg_get_nb_cpus
2:	pause
	cmpw %ax, cpu_online_count
	jne 2b

smp_init_done:
	movb $1, is_all_cpu_init
	ret

cpu_online_count:	.word 1
is_all_cpu_init:	.byte 0
