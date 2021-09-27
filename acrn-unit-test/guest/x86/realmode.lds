SECTIONS
{
    . = 16K;
    stext = .;
    .text : { *(.init) *(.text) *(.text.*) }
    . = ALIGN(4K);
    .data : { *(.data) *(.rodata*) }
    . = ALIGN(16);
    .stack : { *(stack) }
    . = ALIGN(16);
    bss_start = .;
    .bss : { *(.bss) }

    edata = .;
    /DISCARD/ : {
        *(.comment .comment.* .note .note.*)
    }
}
ENTRY(start)
