
all:	strapper

upload: strapper flashtool
	./flashtool/flashtool -p flashimg_strapper_test.bin

verify: flashtool
	./flashtool/flashtool

flashtool: 
	cd flashtool; make
	
strapper:
	asm68k-bin -e -w -o strapper_test.bin strapper-r3.x68
	@echo -n 'Binary size: '
	@stat -f %z strapper_test.bin | sed 's/$$/ bytes/'
	#mkflashimg.sh strapper_test.bin

#tester:
#	./asm68k-bin -e -w -o tester.bin tester.x68
#	./mkdualflash.sh tester.bin strapper_test.bin

listing:
	./asm68k -e -w -l strapper.lst strapper.x68

backup:
	cd ..; backup 68008
