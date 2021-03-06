# 68008 makefile for program loaded into ram

####### VARIABLES #######

# Get directory name, to use as output filename.
PRJ = $(notdir $(CURDIR))
BIN = $(PRJ).S68

LINK_SCRIPT = ../lksr_ram.ld

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

CFLAGS  = -O -nostartfiles -nostdinc -nostdlib -m68000 -std=c99 -fno-builtin $(INCPATHS)

LDFLAGS = -nostartfiles -nostdlib -A m68000 -T $(LINK_SCRIPT) $(LIBPATHS) --oformat srec
ASFLAGS = 

LIBS =   -lmd5 -lcore -lprintf -lgcc -lhd44780 -ltime

# List of source files

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o) startup.o
LSTS=$(SRCS:.c=.S)

# clear suffixes
.SUFFIXES:
.SUFFIXES: .c .o .S

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
	$(AS) $(ASFLAGS) ../startup_ram.S -o startup.o
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
.c.S:
	$(CC) $(CFLAGS) -c -S -o $@ $<
