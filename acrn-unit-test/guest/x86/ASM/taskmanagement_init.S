#include "../taskmanagement_boot_logbuf.h"

/*save cr0 value to INIT_CR0_ADDR*/
mov %cr0, %eax
mov %eax, INIT_CR0_ADDR

/*enable protected mode*/
mov %cr0, %eax
or $1, %eax
mov %eax, %cr0

/*set temporary stack esp for eflags read.*/
mov $smp_stacktop, %esp

/* push 32bit eflags to eax */
pushfl
pop %eax
mov %eax, INIT_EFLAGS_ADDR

/*Store TR value to  INIT_STR_SEL_ADDR*/
str %eax
mov %eax, INIT_TR_SEL_ADDR
