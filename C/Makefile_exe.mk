# 68008 executable basic makefile

####### VARIABLES #######

ifndef PRJ
    PRJ = $(notdir $(CURDIR))
endif

ifndef BIN
    BIN = $(PRJ).s68
endif

ifndef OPTIMIZE
    OPTIMIZE = 3
endif

# RAM or ROM
ifndef CODE_LOC
    CODE_LOC = ram
else
    # convert to lower
    CODE_LOC := $(shell echo $(CODE_LOC) | tr A-Z a-z)
endif

ifeq ($(CODE_LOC),rom)
    ULFLAGS += -f
endif

ifdef USE_KERNEL
    LIBS+= -lkernel
    # Only required if using kernel
    LDFLAGS += --defsym "__start_func=_kernel_start"
endif

ifndef NO_CORELIB
    LIBS += -lcore -lgcc
endif

ifndef NO_STARTUP
    OBJS += startup_$(CODE_LOC).o
endif

ifndef LINK_SCRIPT
    LINK_SCRIPT = ../lksr_$(CODE_LOC).ld
endif

STARTUP_SRC = ../startup_$(CODE_LOC).s

# Program names

RM      = rm -f
CC      = m68k-elf-gcc
LD      = m68k-elf-ld
AS      = m68k-elf-as
OBJDUMP = m68k-elf-objdump
OBJCOPY = m68k-elf-objcopy

# Compiler and linker flags

INCPATHS = -I . -I ../include -I /Users/zigzagjoe/Documents/68008/deploy/lib/gcc/m68k-elf/4.2.4/include/
LIBPATHS = -L ../lib -L /Users/zigzagjoe/Documents/68008/deploy/lib/gcc/m68k-elf/4.2.4/m68000

CFLAGS  += -O$(OPTIMIZE) -nostartfiles -nostdinc -nostdlib -m68000 -std=c99 -fno-builtin $(INCPATHS)
LDFLAGS += -nostartfiles -nostdlib -A m68000 -T $(LINK_SCRIPT) $(LIBPATHS) --oformat srec
ASFLAGS += -march=68000 -mcpu=68000

# List of source files

SRC_C = $(wildcard *.c)
SRC_S = $(wildcard *.s)

OBJS += $(SRC_C:.c=.o) $(SRC_S:.s=.o)
LSTS  = $(SRC_C:.c=.lst)

# clear suffixes
.SUFFIXES:
.SUFFIXES: .c .o .lst .s

####### TARGETS ########

all:	$(BIN)

clean:
	$(RM) $(OBJS) $(BIN) $(LSTS) $(PRJ).bin $(PRJ).l68 $(PRJ).map
	
commit:
	git diff
	git add . 
	git commit -a

# Open source in default editor, on mac.
open:
	-edit $(SRC_C) $(SRC_S) Makefile
	
run:	$(BIN)
ifeq ($(CODE_LOC),rom)
	@echo ERROR: Can only upload a ROM project!
else
	upload-strapper $(PRJ).bin
endif
	
upload: $(BIN)
	srec2srb $(BIN) /tmp/_$(PRJ).srb
	upload-strapper -x -b -s $(ULFLAGS) /tmp/_$(PRJ).srb
    
listing: $(LSTS)
	
$(BIN): $(OBJS) 
	$(LD)  $(OBJS) $(LIBS) $(LDFLAGS) -o $(BIN) -Map $(PRJ).map
	$(OBJCOPY) -I srec $(BIN) -O binary $(PRJ).bin
	$(OBJDUMP) -D $(BIN) -m68000 > $(PRJ).l68
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
