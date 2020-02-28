#save cr4 to 0x8008
mov %cr4, %eax
mov %eax, (0x8008)

#tscadj
efer = 0x0000003b
mov $efer, %ecx
rdmsr
mov %eax, (0x8010)	//ap_eax_tscadj_greg_64
mov %edx, (0x8018)	//ap_edx_tscadj_greg_64

#tscaux
efer = 0xC0000103
mov $efer, %ecx
rdmsr
mov %eax, (0x8020)	//ap_eax_tscaux_greg_64
mov %edx, (0x8028)	//ap_edx_tscaux_greg_64

#tsc
efer = 0x10
mov $efer, %ecx
rdmsr
mov %eax, (0x8030)	//ap_eax_tsc_greg_64
mov %edx, (0x8038)	//ap_edx_tsc_greg_64

