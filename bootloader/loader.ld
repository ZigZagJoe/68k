ENTRY(_boot)

MEMORY {
    RAM (wx): org = 0x00000, len = 512k
    ROM (rx): org = 0x80000, len = 256k
    IO   (w): org = 0xC0000, len = 256k
}

SECTIONS {
	.text (0x2000) : {
		loader.o(.text)
	} > RAM
	
	.text : ALIGN(0x2) {
		__text_start = .;
		*(.text)
		__text_end = .;
	} > RAM

	.data : ALIGN(0x2) {
		__data_start = .;
		*(.data)
		__data_end = .;
	} > RAM
}
