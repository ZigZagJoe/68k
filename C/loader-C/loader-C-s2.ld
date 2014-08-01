ENTRY(_boot)

MEMORY {
    RAM (wx): org = 0x00000, len = 512k
    ROM (rx): org = 0x80000, len = 256k
    IO   (w): org = 0xC0000, len = 256k
}

SECTIONS {
	.stub (0x1000) : AT (0x80000) {
		 lzf_reloc.o(.text)
		 . = ALIGN (2);
		 payload = .;
		 bootloader-s1.o(*)
	} > RAM
}
