/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       khvmm.ld
 * Description:     Linker Script
 *
 * All rights reserved
 *
 ********************************************************************/

OUTPUT_FORMAT("elf32-powerpc", "elf32-powerpc", "elf32-powerpc")
OUTPUT_ARCH(powerpc:common)
ENTRY(_start)

LINKADDR = 0x600000;

SECTIONS
{
    .text LINKADDR :
    {
	_start = .;
        *(.text.hot)
        *(.text .text.* .gnu.linkonce.t.*)
    }

    .rodata :
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    .ctors :
    {
        PROVIDE (__CTOR_LST__ = .);
        KEEP (*(.ctors))
        KEEP (*(SORT(.ctors.*)))
        PROVIDE (__CTOR_END__ = .);
    }

    .data :
    {
        *(.data .data.* .gnu.linkonce.d.*)
    }

    .bss :
    {
	*(.bss .bss.* .sbss .sbss.* .gnu.linkonce.b.*)
    }

    _end = .;

    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.comment*)
    }
}
