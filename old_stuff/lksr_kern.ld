stack_size = 4096;

ENTRY(_start)

MEMORY {
    RAM (wx): org = 0x00000, len = 512k
    ROM (rx): org = 0x80000, len = 256k
    IO   (w): org = 0xC0000, len = 256k
}

__stack_start = ORIGIN(RAM) + LENGTH(RAM);
__stack_end = __stack_start - stack_size;
__heap_end = 0x60000;

/* define the address of the vector table (ie. base of address space) */
__vectors = 0;

SECTIONS {
	/* startup code must be first in binary, given 0x2000 as fixed entry point */
	.text.start (0x2000) : {
		kernel.o(.text)
	} > RAM

	.text : ALIGN(0x10) {
		__text_start = .;
		*(.text)
		__text_end = .;
	} > RAM

	.data : ALIGN(0x10) {
		__data_start = .;
		*(.data)
		__data_end = .;
	} > RAM

	.bss : ALIGN(0x10) {
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	} > RAM
}

__heap_start = __bss_end;
__heap_size = __heap_end - __heap_start;
