
ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)
SECTIONS
{
    . = 0x20000;
    .text ALIGN(0x1000) :
    {
        code = .;
        *(.text.boot);
        *(.text);
    }
    .data ALIGN(0x1000) :
    {
        *(.data);
    }
    .bss :
    {
        bss = .;
        *(.bss);
    }
    end = .;
}
