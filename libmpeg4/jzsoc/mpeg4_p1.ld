/*
 * Copyright (C) 2006-2014 Ingenic Semiconductor CO.,LTD.
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* Script for -z combreloc: combine and sort reloc sections */
OUTPUT_FORMAT("elf32-tradlittlemips", "elf32-tradbigmips",
	      "elf32-tradlittlemips")
/*OUTPUT_FORMAT("elf32-tradlittlemips", "elf32-tradbigmips",
	      "elf32-tradlittlemips")*/
OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(mips)
/*ENTRY(__start)*/
SEARCH_DIR("/icdev/toolchain/linux/mipseltools-gcc412-lnx26/mipsel-linux/lib");
SECTIONS
{

/* Reset handler position */
  . = 0xF4000000;			/* reserved memory */
  _boot_text = .;			/* Sys_text and read-only data */
  .p1_main : {
	*(.boot_text.head)
	*(.p1_main)
	*(.fixup)
	*(.gnu.warning)
  }
  .p1_text : {
	*(.boot_text.head)
	*(.p1_text)
	*(.fixup)
	*(.gnu.warning)
  }

  _text = .;			/* Text and read-only data */
  .text : {
	*(.text.head)
	*(.text)
	*(.fixup)
	*(.gnu.warning)
  } 

  _etext = .;			/* End of text section */


  _srodata = .;			        /* start of rodata section */
  .rodata : { *(.rodata); _erodata = ABSOLUTE (.); }
  .kstrtab : { *(.kstrtab) }

  . = ALIGN(8);
  .data : {
	*(.data)
	CONSTRUCTORS
  }

  _gp = . + 0x200;
  __global = _gp;

  .sdata : {
    *(.sdata)
    *(.sdata.*)
  }
  _edata = .;			/* End of data section */

  _fbss = .;
  .sbss : {
    *(.sbss)
    *(.scommon)
  }
  .bss : {
    _bss_start = . ;
    *(.bss)
    *(COMMON)
  }
  _end = . ;
  end = . ;

  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }

}
