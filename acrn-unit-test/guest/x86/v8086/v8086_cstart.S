#include "apic-defs.h"

.globl boot_idt
.globl prepare_v8086
.globl v8086_iopl
.globl shared_data
.globl ring0stacktop
.globl ptl2
.globl pt

max_cpus = 1
stack_size = 0x1400
ipi_vector = 0x20

.data
	. = . + stack_size
	.align 16
stacktop:

	. = . + stack_size * max_cpus
	.align 16
ring0stacktop:

	. = . + stack_size
	.align 16
v8086_stacktop:

v8086_iopl:
	.long 0x23246

shared_data:
	.fill 128,1,0x00

.align 4096
ptl2:
	i = 0
	.rept 1024
	.long 0x177 | (i << 12)
	i = i + 1
	.endr

.align 4096
pt:
	.long 0x177 + ptl2      //4K page
	i = 1
	.rept 1023
	.long 0x1e7 | (i << 22) //4M pages
	i = i + 1
	.endr

.globl gdt32
gdt32:
	.quad 0
	.quad 0x00cf9b000000ffff // flat 32-bit code segment
	.quad 0x00cf93000000ffff // flat 32-bit data segment
	.quad 0x00cf1b000000ffff // flat 32-bit code segment, not present
	.quad 0                  // TSS for task gates
	.quad 0x008f9b000000ffff // 16-bit code segment
	.quad 0x008f93000000ffff // 16-bit data segment
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
	.quad 0			 //32-bit avail tss
	.endr
gdt32_end:

i = 0
.globl tss
tss:
	.rept max_cpus
	.long 0
	.long ring0stacktop - i * stack_size
	.long 16
	.quad 0, 0
	.quad 0, 0, 0, 0, 0, 0, 0, 0
	.long 0, 0, 0
	.rept 256
	.byte 0
	.endr
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

gdt32_descr:
	.word gdt32_end - gdt32 - 1
	.long gdt32

.section .init

.code32
mb_magic = 0x1BADB002
mb_flags = 0x2

#if (defined(IN_NON_SAFETY_VM) || defined(IN_NATIVE))
	# multiboot header
	.long mb_magic, mb_flags, 0 - (mb_magic + mb_flags)
#endif

MSR_GS_BASE = 0xc0000101
.macro setup_percpu_area
	lea -stack_size(%esp), %eax
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
	call main
	push %eax
	hlt

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

prepare_v8086:
	/* push segment*/
	mov $0,%eax
	push %eax  /*gs */
	push %eax  /*fs */
	push %eax  /*ds */
	push %eax  /*es */

	/* ss */
	mov $0,%eax
	push %eax

	/* esp */
	mov $v8086_stacktop,%eax
	push %eax

	/* eflags.vm=1,IOPL 3,IF =1 */
	mov v8086_iopl,%eax
	/* mov $0x23246,%eax */
	push %eax

	/* cs,*/
	mov $0x0,%eax
	push %eax

	/* eip  */
	mov $v8086_main,%eax
	push %eax
	iret
