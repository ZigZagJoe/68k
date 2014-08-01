ENTRY(loader_start)

MEMORY {
    RAM (wx): org = 0x00000, len = 512k
    ROM (rx): org = 0x80000, len = 256k
    IO   (w): org = 0xC0000, len = 256k
}

SECTIONS {
    /DISCARD/ : {
	    lzf_reloc.o(*)
	}
	
	.body (0x1000) : {
	    loader.o(.text)
	    *(.text)
		*(.data)
		*(.rodata)
		*(.rodata.*)
	} > RAM
}
