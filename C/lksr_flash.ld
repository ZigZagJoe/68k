stack_size = 32768;

ENTRY(_start)

MEMORY {
    RAM (wx): org = 0x00000, len = 512k
    ROM (rx): org = 0x80000, len = 256k
    IO   (w): org = 0xC0000, len = 256k
}

__stack_start = ORIGIN(RAM) + LENGTH(RAM);
__stack_end = __stack_start - stack_size;
__heap_end = __stack_end;

/* define the address of the vector table (ie. base of address space) */
__vectors = 0;

SECTIONS {
	/* startup code must be first in binary, given 0x2000 as fixed entry point */
	.startup (0x81000) : {
		startup.o(.text)
	} > ROM

	.text : ALIGN(0x4) {
		*(.text)
    } > ROM
    
    .rodata : ALIGN(0x4) {
        *(.rodata.*)
    } > ROM
    
    /* offset in ROM */
	__data_rom_start = .;
	
	.data (0x2000) : AT (__data_rom_start) ALIGN(0x4) {
	    /* offset relative to load address */
	    __data_start = .;
		*(.data)
		__data_end = .;
	} > RAM
	
	.bss (__data_end) : ALIGN(0x4) {
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	} > RAM
	
	/DISCARD/ : {
	    *(.comment)
	    *(.debug*)
	}
}

__heap_start = __bss_end;
__heap_size = __heap_end - __heap_start;
__bss_size  = (__bss_end - __bss_start) / 4;
__data_size = (__data_end - __data_start) / 4;
