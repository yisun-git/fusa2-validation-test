SECTIONS
{
	/* multiboot header */
	. = 0x1000 ;
    . = ALIGN(4) ;

    ld_multiboot_load = .;
	.boot :
	{
		*(.multiboot_header) ;
	}

	/* ap start data and code start at 4k */
	. = 0x4000 ;
    . = ALIGN(4) ;

    ld_trampoline_load = .;
    .trampoline :
    {
        ld_trampoline_start = .;
        *(.trampoline_reset);
        . = ALIGN(4);
        ld_trampoline_end = .;

    }
    . = 0x1000800 + SIZEOF_HEADERS;
    stext = .;
    .text : { KEEP(*(.init)) *(.text) *(.text.*) }
    . = ALIGN(4K);
    .data : {
          *(.data)
          exception_table_start = .;
          *(.data.ex)
          exception_table_end = .;
          }
    . = ALIGN(16);
    .rodata : { *(.rodata) }
    . = ALIGN(16);
    .stack : { *(stack) }
    . = ALIGN(16);
    bss_start = .;
    .bss : { *(.bss) }
    . = ALIGN(4K);
    edata = .;

    /DISCARD/ : {
        *(.comment .comment.* .note .note.*)
    }
}

ENTRY(start)

