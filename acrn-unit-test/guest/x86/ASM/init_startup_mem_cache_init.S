mov $0xa1a1,%edx
/* save cr0 in memory during init */
mov %cr0, %eax
mov %eax, (0x8000)
/* save cr3 in memory during init */
mov %cr3,  %eax
mov %eax, (0x8000 + 0x4)
/* save magic in memory during init.
 * not use, just test */
mov $0xa1a1, %eax
mov %eax, (0x8000 + 0x8)

