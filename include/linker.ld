/* Minimal linker script for RISC-V bare-metal programs */
OUTPUT_FORMAT("elf32-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 32M  /* Reduced to 128KB for safety */
}

SECTIONS {
    /* Text section at 0x0 */
    .text : {
        *(.text .text.*)
    } > RAM
    
    /* Read-only data */
    .rodata : {
        *(.rodata .rodata.*)
    } > RAM
    
    /* Data section */
    .data : {
        *(.data .data.*)
    } > RAM
    
    /* BSS section */
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } > RAM
    
    /* Stack grows down from 0x3FFFFFF */
    .stack (NOLOAD) : {
        . = 0x3FFFFFF;
        . += 4K;
    }
    /* Heap 
    
    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.eh_frame)
        *(.note.*)
        *(.comment)
    }
}