OUTPUT_FORMAT("elf32-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 64M
}

SECTIONS {

    /* Place user program at 0x2000 */
    . = 0x00002000;

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

    /* Heap */
    _heap_start = .;

    /* Stack */
    _stack_top = ORIGIN(RAM) + LENGTH(RAM) - 0x2000;

    /DISCARD/ : {
        *(.eh_frame)
        *(.note.*)
        *(.comment)
    }
}