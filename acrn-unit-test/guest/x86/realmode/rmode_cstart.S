
#include "apic-defs.h"

.globl boot_idt
boot_idt = 0

.globl ap_end_life

ipi_vector = 0x20

max_cpus = 2

.data
	.rept 4096 * max_cpus
	.byte 0
	.endr

.align 16
stacktop:
	.rept 4096
	.byte 0
	.endr

.align 16
ring0stacktop:

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

boot_idt:
    .rept 256
    .quad 0
    .quad 0
    .endr
boot_idt_end:

idt_descr:
	.word 16 * 256 - 1
	.long boot_idt

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
	mov $stacktop, %esp
	push %ebx
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
	call smp_init
/*
*
*waiting ap finish test
*
*/
	mov $1, %bx
1:	pause
	cmpw %bx, ap_end_life
	jne 1b
2:	pause
	jmp 2b
	mov $1, %eax
	outl %eax, $0xf4

ap_end_life:	.word 0x0

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
	movl $0, tss_descr+8(%ebx)
	movl $0, tss_descr+12(%ebx)
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
.global smp_init
smp_init:
	cld
	lea sipi_entry, %esi
	xor %edi, %edi
	mov $(sipi_end - sipi_entry), %ecx
	rep/movsb
	movl $APIC_ICR, %ecx
	shrl $4, %ecx
	addl $APIC_BASE_MSR, %ecx

#ifdef IN_SAFETY_VM
	movl $1, %eax
#else
	movl $2, %eax
#endif

	movl %eax, %edx
#ifdef IN_NATIVE
	shll $1, %edx
#endif
	subl $1, %edx
1:
	movl $(APIC_DEST_PHYSICAL | APIC_DM_INIT | APIC_INT_ASSERT), %eax
	wrmsr
	movl $(APIC_DEST_PHYSICAL | APIC_DM_INIT), %eax
	wrmsr
	movl $(APIC_DEST_PHYSICAL | APIC_DM_STARTUP), %eax
	wrmsr
	subl $1, %edx
	cmp $0, %edx
	jnz 1b
smp_init_done:
	ret

cpu_online_count:	.word 1

.code16
gdt32_descr:
	.word gdt32_end - gdt32 - 1
	.long gdt32

	ap_stacktop = 0x2000
sipi_entry:
	mov $0x0, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	mov $ap_stacktop, %sp
	mov $(ap_stacktop - 4096), %ax
	mov %ax, %gs
	ljmpl $0, $ap_rmode
sipi_end:

ap_rmode:
	call setup_idt
	sti
	lock incw cpu_online_count
	ljmpl $0, $rmode_start
rmode_end:
/*
*
*realmode exception handler
*
*/
.align 16
excp_divide_error:
	push $0x0
	push $0x0
	jmp    __do_handle_rmode_excp

.align 8
excp_debug:
	push  $0x0
	push  $0x01
	jmp    __do_handle_rmode_excp

.align 8
excp_nmi:
	push $0x0
	push $0x02
	jmp    __do_handle_rmode_excp

.align 8
excp_breakpoint:
	push  $0x0
	push  $0x03
	jmp    __do_handle_rmode_excp

.align 8
excp_overflow:
	push  $0x0
	push  $0x04
	jmp    __do_handle_rmode_excp

.align 8
excp_bounds_check:
	push  $0x0
	push  $0x05
	jmp    __do_handle_rmode_excp

.align 8
excp_illegal_opcode:
	push  $0x0
	push  $0x06
	jmp    __do_handle_rmode_excp

.align 8
excp_device_not_available:
	push  $0x0
	push  $0x07
	jmp    __do_handle_rmode_excp

.align 8
excp_double_fault:
	push  $0x08
	jmp    __do_handle_rmode_excp

.align 8
excp_coprocessor_seg_overrun:
	push $0x0
	push $0x09
	jmp   __do_handle_rmode_excp

.align 8
excp_invalid_tss:
	push  $0x0A
	jmp    __do_handle_rmode_excp

.align 8
excp_segment_not_present:
	push  $0x0B
	jmp    __do_handle_rmode_excp

.align 8
excp_stack_fault:
	push  $0x0C
	jmp    __do_handle_rmode_excp

.align 8
excp_general_protection:
	push  $0x0D
	jmp    __do_handle_rmode_excp

.align 8
excp_page_fault:
	push  $0x0E
	jmp    __do_handle_rmode_excp

.align 8
excp_float_error:
	push  $0x0
	push  $0x10
	jmp    __do_handle_rmode_excp

.align 8
excp_alignment_check:
	push  $0x11
	jmp    __do_handle_rmode_excp

.align 8
expt_machine_check:
	push  $0x0
	push  $0x12
	jmp    __do_handle_rmode_excp

.align 8
excp_simd_fp_error:
	push  $0x0
	push  $0x13
	jmp    __do_handle_rmode_excp

.align 8
excp_virtualization:
	push  $0x0
	push  $0x14
	jmp    __do_handle_rmode_excp
/*
*
*realmode idt table
*
*/
rmode_excp:
	.word excp_divide_error
	.word 0

	.word excp_debug
	.word 0

	.word excp_nmi
	.word 0

	.word excp_breakpoint
	.word 0

	.word excp_overflow
	.word 0

	.word excp_bounds_check
	.word 0

	.word excp_illegal_opcode
	.word 0

	.word excp_device_not_available
	.word 0

	.word excp_double_fault
	.word 0

	.word excp_coprocessor_seg_overrun
	.word

	.word excp_invalid_tss
	.word 0

	.word excp_segment_not_present
	.word 0

	.word excp_stack_fault
	.word 0

	.word excp_general_protection
	.word 0

	.word excp_page_fault
	.word 0

	.word excp_float_error
	.word 0

	.word excp_alignment_check
	.word 0

	.word expt_machine_check
	.word 0

	.word excp_simd_fp_error
	.word 0

	.word excp_virtualization
	.word 0
excp_end:
/*
*
*setup realmode idt table
*
*/
setup_idt:
	cld
	lea rmode_excp, %si
	xor %di, %di
	mov $(excp_end - rmode_excp), %cx
	rep/movsb
	ret

