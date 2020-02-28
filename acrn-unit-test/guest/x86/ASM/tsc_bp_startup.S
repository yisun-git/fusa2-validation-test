#cr4
mov %cr4, %eax
mov %eax, (0x8130)	//bp_cr4_greg_long

#tscadj clear
efer = 0x0000003b
mov $efer, %edx

val = 0x700000001
mov $val,%ecx
wrmsr

#tscadj
efer = 0x0000003b
mov $efer, %ecx
rdmsr
mov %eax, (0x8100)		//bp_eax_tscadj_greg_64
mov %edx, (0x8108)	//bp_edx_tscadj_greg_64

#tscaux
efer = 0xC0000103
mov $efer, %ecx
rdmsr
mov %eax, (0x8110)	//bp_eax_tscaux_greg_64
mov %edx, (0x8118)	//bp_edx_tscaux_greg_64

#tsc
efer = 0x10
mov $efer, %ecx
rdmsr
mov %eax, (0x8120)	//bp_eax_tsc_greg_64
mov %edx, (0x8128)	//bp_edx_tsc_greg_64

