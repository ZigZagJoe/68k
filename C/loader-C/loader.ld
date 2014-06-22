ENTRY(_boot)

MEMORY {
    RAM (wx): org = 0x00000, len = 512k
    ROM (rx): org = 0x80000, len = 256k
    IO   (w): org = 0xC0000, len = 256k
}

SECTIONS {
	.text (0x1000) : {
		reloc.o(.text)
	} > RAM
	
	.body : ALIGN(0x10) {
	    __payload_start = .;
		loader.o(.text)
		*(.text)
		*(.data)
		*(.rodata.*)
		__payload_end = .;	
	} > RAM
}

__reloc_size = (__payload_end - __payload_start) / 4;
