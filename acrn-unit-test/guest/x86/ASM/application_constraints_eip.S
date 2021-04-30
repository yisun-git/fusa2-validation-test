#define MULTIBOOT_STARTUP_EIP_ADDR	(0x6004)

	mov $stacktop, %esp
	call 11f
11:
	pop %edx
	mov %edx, MULTIBOOT_STARTUP_EIP_ADDR
