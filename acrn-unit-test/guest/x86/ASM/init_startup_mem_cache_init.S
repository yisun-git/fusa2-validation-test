mov $0xa1a1,%edx
/* save cr0 in memory during init */
mov %cr0, %eax
mov %eax, (0x8000)
/* save cr3 in memory during init */
mov %cr3,  %eax
mov %eax, (0x8000 + 0x4)
/* save pat in memory during init*/
mov $0x277, %ecx
rdmsr
mov %eax, (0x8000 + 0x8)
mov %edx, (0x8000 + 0xC)
/* save magic in memory during init.
 * not use, just test */
mov $0xa1a1, %eax
mov %eax, (0x8000 + 0x10)

