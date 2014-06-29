# 68008 executable basic makefile

####### VARIABLES #######

ifndef PRJ
    PRJ = $(notdir $(CURDIR))
endif

ifndef BIN
    BIN = $(PRJ).s68
endif

ifndef OPTIMIZE
    OPTIMIZE = s
endif

# RAM or ROM
ifndef CODE_LOC
    CODE_LOC = ram
else
    CODE_LOC := $(shell echo $(CODE_LOC) | tr A-Z a-z)
endif

ifdef USE_KERNEL
    LIBS+= -lkernel
    # Only required if using kernel
    LDFLAGS += --defsym "__start_func=_kernel_start"
endif

ifndef NO_CORELIB
    LIBS += -lcore -lgcc
endif

#############################################################
# Should not need to edit past here

ifndef LINK_SCRIPT
    LINK_SCRIPT = ../lksr_$(CODE_LOC).ld
endif

STARTUP_SRC = ../startup_$(CODE_LOC).s

# Program names

RM   = rm -f
CC   = m68k-elf-gcc
LD   = m68k-elf-ld
AS   = m68k-elf-as
OBJDUMP = m68k-elf-objdump
OBJCOPY = m68k-elf-objcopy

# Compiler and linker flags

INCPATHS = -I . -I ../include -I /Users/zigzagjoe/Documents/68008/deploy/lib/gcc/m68k-elf/4.2.4/include/
LIBPATHS = -L ../lib -L /Users/zigzagjoe/Documents/68008/deploy/lib/gcc/m68k-elf/4.2.4/m68000

CFLAGS  += -O$(OPTIMIZE) -nostartfiles -nostdinc -nostdlib -m68000 -std=c99 -fno-builtin $(INCPATHS)
LDFLAGS += -nostartfiles -nostdlib -A m68000 -T $(LINK_SCRIPT) $(LIBPATHS) --oformat srec
ASFLAGS += -march=68000 -mcpu=68000

# List of source files

SRC_C=$(wildcard *.c)
SRC_S=$(wildcard *.s)

OBJS=$(SRC_C:.c=.o) $(SRC_S:.s=.o)
LSTS=$(SRC_C:.c=.lst)

ifndef NO_STARTUP
    OBJS += startup_$(CODE_LOC).o
endif

# clear suffixes
.SUFFIXES:
.SUFFIXES: .c .o .lst .s

####### TARGETS ########

all:	$(BIN)

clean:
	$(RM) $(OBJS) $(BIN) $(LSTS) $(PRJ).bin
	
zip:	clean all
	zip -vr $(PRJ).zip . -x $(PRJ).zip
	
backup:
	cd ..; backup $(PRJ)

# Open source in default editor, on mac.
open:
	-open $(SRCS) 
	
run:	$(BIN)
	upload-strapper $(PRJ).bin

listing: $(LSTS)
	
$(BIN): $(OBJS) 
	$(LD)  $(OBJS) $(LIBS) $(LDFLAGS) -o $(BIN)
	$(OBJCOPY) -I srec $(BIN) -O binary $(PRJ).bin
	$(OBJDUMP) -D $(BIN) -m68000 > $(PRJ).L68
	@echo
	@echo -n 'Binary size: '
	@stat -f %z $(PRJ).bin | sed 's/$$/ bytes/'
	
startup_$(CODE_LOC).o:
	$(AS) $(ASFLAGS) $(STARTUP_SRC) -o startup_$(CODE_LOC).o
	
.s.o:
	$(AS) $(ASFLAGS) -o $@ $<
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
.c.lst:
	$(CC) $(CFLAGS) -c -S -o $@ $<
