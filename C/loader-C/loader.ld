ENTRY(_boot)

MEMORY {
    RAM (wx): org = 0x00000, len = 512k
    ROM (rx): org = 0x80000, len = 256k
    IO   (w): org = 0xC0000, len = 256k
}

SECTIONS {
	.text (0x1000) : AT (0x80000) {
		reloc.o(.text)
	} > RAM
	
	.body : ALIGN(0x10) {
	    loader.o(.text)
		*(.text)
		*(.data)
		*(.rodata.*)
	} > RAM
}
