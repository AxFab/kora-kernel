
ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)
SECTIONS
{
    . = 2M;
    .text ALIGN(4K) :
    {
        code = .;
        *(.text.boot);
        *(.text);
    }
    .data ALIGN(4K) :
    {
        *(.data);
        *(.rodata);
    }
    .bss :
    {
        bss = .;
        *(.bss);
    }
    end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}
