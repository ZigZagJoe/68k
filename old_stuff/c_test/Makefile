### ZZJ's Fancypants Makefile for C++
# You should not need to modify this file unless on different OS.

####### VARIABLES #######

# Get directory name, to use as output filename.
PRJ = $(notdir $(CURDIR))
BIN = $(PRJ).S68

# Program names

RM   = rm -f
CC   = m68k-elf-gcc
LD   = m68k-elf-ld
AS   = m68k-elf-as
OBJDUMP = m68k-elf-objdump
OBJCOPY = m68k-elf-objcopy

# Compiler and linker flags

CFLAGS  = -nostartfiles -nostdinc -nostdlib -m68000 -std=c99 -fno-builtin -I . -I /Users/zigzagjoe/Documents/68008/deploy/lib/gcc/m68k-elf/4.2.4/include/
LDFLAGS = -nostartfiles -nostdlib -A m68000 -T link_scr.ld -L /Users/zigzagjoe/Documents/68008/deploy/lib/gcc/m68k-elf/4.2.4/m68000 -lgcc --oformat srec
ASFLAGS = 

LIBS =

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
	../upload $(PRJ).bin

listing: $(LSTS)
	
$(BIN): $(OBJS) 
	$(LD)  $(OBJS) $(LDFLAGS) $(LIBS) -o $(BIN)
	$(OBJCOPY) -I srec $(BIN) -O binary $(PRJ).bin
	$(OBJDUMP) -D $(BIN) -m68000 > $(PRJ).L68
	
startup.o: startup.s
	$(AS) $(ASFLAGS) startup.S -o startup.o
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
.c.S:
	$(CC) $(CFLAGS) -c -S -o $@ $<
