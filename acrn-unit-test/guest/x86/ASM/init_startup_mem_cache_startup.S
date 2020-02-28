/*save cr0 in memory during startup*/
mov %cr0, %eax
mov %eax, (0x6000)
/*save cr3 in memory during startup*/
mov %cr3,  %eax
mov %eax, (0x6000 + 4)
/*save ia32_pat in memory during startup*/
mov $0x277,%ecx
rdmsr
mov %eax, (0x6000 + 0x8)
mov %edx, (0x6000 + 0xC)
/* save magic in memory during startup.
 * not use, just test */
mov $0xafaf, %eax
mov %eax, (0x6000 + 0x10)
