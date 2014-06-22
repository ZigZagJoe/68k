# 68008 makefile for bootloader

####### VARIABLES #######

# Get directory name, to use as output filename.
PRJ = bootloader
BIN = loader.s68

LINK_SCRIPT = loader.ld

# Program names

RM   = rm -f
CC   = m68k-elf-gcc
LD   = m68k-elf-ld
AS   = m68k-elf-as
OBJDUMP = m68k-elf-objdump
OBJCOPY = m68k-elf-objcopy

# Compiler and linker flags

INCPATHS=-I . -I ../include -I /Users/zigzagjoe/Documents/68008/deploy/lib/gcc/m68k-elf/4.2.4/include/

CFLAGS  = -Os -nostartfiles -nostdinc -nostdlib -m68000 -std=c99 -fno-builtin $(INCPATHS)

LDFLAGS = -nostartfiles -nostdlib -A m68000 -T $(LINK_SCRIPT) --oformat srec
ASFLAGS = -march=68000 -mcpu=68000

# List of source files

SRC_C=$(wildcard *.c)
SRC_S=$(wildcard *.s)

OBJS=$(SRC_C:.c=.o) $(SRC_S:.s=.o)
LSTS=$(SRC_C:.c=.lst)

# clear suffixes
.SUFFIXES:
.SUFFIXES: .s .o .c .lst

####### TARGETS ########

all:	$(BIN)

clean:
	$(RM) $(OBJS) $(BIN) $(PRJ).bin $(PRJ).lst
	
backup:
	cd ..; backup $(PRJ)

# Open source in default editor, on mac.
open:
	-open $(SRCS) 
	
run:	$(BIN)
	upload-strapper  $(PRJ).bin
	
$(BIN): $(OBJS) 
	$(LD)  $(OBJS) $(LDFLAGS) $(LIBS) -o $(BIN)
	$(OBJCOPY) -I srec $(BIN) -O binary $(PRJ).bin
	$(OBJDUMP) -D $(BIN) -m68000 > $(PRJ).lst
	@echo
	@echo -n 'Binary size: '
	@stat -f %z $(PRJ).bin | sed 's/$$/ bytes/'
	
listing: $(LSTS)

.s.o:
	$(AS) $(ASFLAGS) -o $@ $<
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
.c.lst:
	$(CC) $(CFLAGS) -c -S -o $@ $<

