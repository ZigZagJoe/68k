ENTRY(_boot)

MEMORY {
    RAM (wx): org = 0x00000, len = 512k
    ROM (rx): org = 0x80000, len = 256k
    IO   (w): org = 0xC0000, len = 256k
}

SECTIONS {
	.text (0x1000) : AT (0x80000) {
		__begin = .;
        reloc.o(.text)
	} > RAM
	
	.body : ALIGN(0x2) {
	    build_date.o(.data)
	    loader.o(.text)
	    loader.o(.data)
		*(.text)
		*(.data)
		*(.rodata)
		*(.rodata.*)
		__end = .;
	} > RAM
}

/* size of loader, in number of longs to copy */
__size = (__end - __begin) / 4;
