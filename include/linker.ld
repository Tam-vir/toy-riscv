OUTPUT_FORMAT("elf32-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 64M
}

SECTIONS {
    . = ORIGIN(RAM);

    /* Code */
    .text : {
        *(.text .text.*)
    } > RAM

    . = ALIGN(4);

    /* Read-only data */
    .rodata : {
        *(.rodata .rodata.*)
    } > RAM

    . = ALIGN(4);

    /* Initialized data */
    .data : {
        *(.data .data.*)
    } > RAM

    . = ALIGN(4);

    /* Uninitialized data */
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } > RAM

    . = ALIGN(4);

    /* Heap start (optional) */
    _heap_start = .;

    /* Stack at top of RAM */
    _stack_top = ORIGIN(RAM) + LENGTH(RAM);

    /DISCARD/ : {
        *(.eh_frame)
        *(.note.*)
        *(.comment)
    }
}