SECTIONS
{
    . = 0x4000 + SIZEOF_HEADERS;
    stext = .;
    .text : { *(.init) *(.text) *(.text.*) }
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
    bss_start = .;
    .bss : { *(.bss) }
    . = ALIGN(4K);
    edata = .;

    /DISCARD/ : {
        *(.comment .comment.* .note .note.* .eh_frame)
    }
}

ENTRY(start)

