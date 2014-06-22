ENTRY(_boot)

MEMORY {
    RAM (wx): org = 0x00000, len = 512k
    ROM (rx): org = 0x80000, len = 256k
    IO   (w): org = 0xC0000, len = 256k
}

SECTIONS {
	.text (0x2000) : {
		reloc.o(.text)
	} > RAM
	
	.text (0x1000) : ALIGN(0x2) {
	    __payload_start = .;
		loader.o(.text)
	} > RAM
	
	.text : ALIGN(0x2) {
		*(.text)
	} > RAM

	.data : ALIGN(0x2) {
		*(.data)
		__payload_end = .;	
	} > RAM
}

__reloc_size = (__payload_end - __payload_start) / 4;
