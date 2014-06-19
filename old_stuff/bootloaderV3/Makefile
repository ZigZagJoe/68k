
all:	loader

loader:
	asm68k-bin -e -w -o bootloader.bin bootloader.x68

listing:
	asm68k -e -w -l bootloader.lst bootloader.x68

run: loader
	upload-strapper bootloader.bin
