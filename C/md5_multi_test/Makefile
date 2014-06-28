# 68008 makefile for program loaded into ram

####### VARIABLES #######

# Get directory name, to use as output filename.
PRJ = $(notdir $(CURDIR))
BIN = $(PRJ).s68

CODE_LOC = rom
LDFLAGS = --defsym "__start_func=_kernel_start" --defsym "stack_size=4096"

# Program names

RM   = rm -f
CC   = m68k-elf-gcc
LD   = m68k-elf-ld
AS   = m68k-elf-as
OBJDUMP = m68k-elf-objdump
OBJCOPY = m68k-elf-objcopy

# Compiler and linker flags

INCPATHS=-I . -I ../include -I /Users/zigzagjoe/Documents/68008/deploy/lib/gcc/m68k-elf/4.2.4/include/
LIBPATHS=-L ../lib -L /Users/zigzagjoe/Documents/68008/deploy/lib/gcc/m68k-elf/4.2.4/m68000

CFLAGS  = -O3 -nostartfiles -nostdinc -nostdlib -m68000 -std=c99 -fno-builtin $(INCPATHS)

LDFLAGS += -nostartfiles -nostdlib -A m68000 -T $(LINK_SCRIPT) $(LIBPATHS) --oformat srec
ASFLAGS = -march=68000 -mcpu=68000

LIBS =  -lmd5 -lbeep  -lcore  -lhd44780 -lgcc
# -lbeep   -lprintf -lgcc -lhd44780  -lbeep  -lhd44780

# List of source files

LINK_SCRIPT = ../lksr_$(CODE_LOC).ld
STARTUP_SRC = ../startup_$(CODE_LOC).s

SRC_C=$(wildcard *.c)
SRC_S=$(wildcard *.s)

OBJS=$(SRC_C:.c=.o) $(SRC_S:.s=.o) startup.o
LSTS=$(SRC_C:.c=.lst)

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
	
startup.o:
	$(AS) $(ASFLAGS) $(STARTUP_SRC) -o startup.o
	
.s.o:
	$(AS) $(ASFLAGS) -o $@ $<
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
.c.lst:
	$(CC) $(CFLAGS) -c -S -o $@ $<
