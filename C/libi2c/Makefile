# 68008 lib makefile

####### VARIABLES #######

# Get directory name, to use as output filename.
PRJ = $(notdir $(CURDIR))
LIB = ../lib/$(PRJ).a

# Program names

RM   = rm -f
CC   = m68k-elf-gcc
AR   = m68k-elf-ar
AS   = m68k-elf-as

# Compiler and linker flags

INCPATHS= -I . -I ../include -I /Users/zigzagjoe/Documents/68008/deploy/lib/gcc/m68k-elf/4.2.4/include

# -fdata-sections -ffunction-sections
CFLAGS  = -fno-pic -O3 -nostartfiles -nostdinc -nostdlib -m68000 -std=c99 -fno-builtin $(INCPATHS) 
ASFLAGS = -march=68000 -mcpu=68000
ARFLAGS = -ar rcs $(LIB)

# List of source files

SRC_C=$(wildcard *.c)
SRC_S=$(wildcard *.s)

OBJS=$(SRC_C:.c=.o) $(SRC_S:.s=.o)
LSTS=$(SRC_C:.c=.lst)

# clear suffixes
.SUFFIXES:
.SUFFIXES: .c .o .lst .s

####### TARGETS ########

all:	$(LIB)

clean:
	$(RM) $(OBJS) $(LSTS) $(LIB)
	
backup:
	cd ..; backup $(PRJ)

# Open source in default editor, on mac.
open:
	-open $(SRCS) 

listing: $(LSTS)
	
$(LIB): $(OBJS)    
	$(AR) $(ARFLAGS) $(OBJS)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
.c.lst:
	$(CC) $(CFLAGS) -c -S -o $@ $<
	
.s.o:
	$(AS) $(ASFLAGS) -o $@ $<
