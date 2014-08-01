# 68008 executable makefile stub

PRJ = bootloader
LINK_SCRIPT = loader-C.ld

NO_CORELIB = 1
NO_STARTUP = 1

OPTIMIZE = s
CFLAGS += -DBOOTLOADER

DATE := $(shell date)
OBJS += build_date.o

include ../Makefile_exe.mk

build_date.o: .FORCE
	echo ".data\nBUILD_DATE:.string \"## $(DATE) ##\"\n" > build_date.s_tmp	
	$(AS) -o build_date.o build_date.s_tmp
	$(RM) build_date.s_tmp
	
.FORCE:
