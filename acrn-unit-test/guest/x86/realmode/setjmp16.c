asm(".code16gcc");
asm(".globl setjmp \n"
"setjmp: \n"
"				mov (%esp), %ecx \n"       // get return EIP \n
"				mov 4(%esp), %eax \n"      // get jmp_buf \n
"				mov %ecx, (%eax) \n"
"				mov %esp, 4(%eax) \n"
"				mov %ebp, 8(%eax) \n"
"				mov %ebx, 12(%eax) \n"
"				mov %esi, 16(%eax) \n"
"				mov %edi, 20(%eax) \n"
"				xor %eax, %eax \n"
"				ret \n"
);

asm(".globl longjmp\n"
"longjmp:\n"
"				mov 8(%esp), %eax \n"		// get return value\n
"				mov 4(%esp), %ecx \n"		// get jmp_buf\n
"				mov 20(%ecx), %edi \n"
"				mov 16(%ecx), %esi \n"
"				mov 12(%ecx), %ebx \n"
"				mov 8(%ecx), %ebp \n"
"				mov 4(%ecx), %esp \n"
"				mov (%ecx), %ecx \n"		// get saved EIP\n
"				mov %ecx, (%esp) \n"    // and store it on the stack\n
"				ret \n"
);
