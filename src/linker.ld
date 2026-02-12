ENTRY(_start)

SECTIONS
{
    . = 0x1000;

    .text :
    {
        *(.multiboot)
        *(.text)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
        *(COMMON)
    }
}
