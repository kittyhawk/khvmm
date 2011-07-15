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

LINKADDR = 0x6000000;

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

	.preinit_array     :
	{
	  PROVIDE_HIDDEN (__preinit_array_start = .);
	  KEEP (*(.preinit_array))
	  PROVIDE_HIDDEN (__preinit_array_end = .);
	}
	.init_array     :
	{
	  PROVIDE_HIDDEN (__init_array_start = .);
	  KEEP (*(SORT(.init_array.*)))
	  KEEP (*(.init_array))
	  PROVIDE_HIDDEN (__init_array_end = .);
	}
	.fini_array     :
	{
	  PROVIDE_HIDDEN (__fini_array_start = .);
	  KEEP (*(SORT(.fini_array.*)))
	  KEEP (*(.fini_array))
	  PROVIDE_HIDDEN (__fini_array_end = .);
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
