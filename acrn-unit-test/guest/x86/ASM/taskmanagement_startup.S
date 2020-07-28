#include "../taskmanagement_boot_logbuf.h"

/*save cr0 to STARTUP_CR0_ADDR*/
mov %cr0, %eax
mov %eax,STARTUP_CR0_ADDR

/*set temporary stack esp for eflags read.*/
mov $stacktop, %esp

/* push 32bit eflags to eax */
pushfl
pop %eax
mov %eax, STARTUP_EFLAGS_ADDR

/*Store TR value to STARTUP_STR_SEL_ADDR*/
str %eax
mov %eax, STARTUP_TR_SEL_ADDR
