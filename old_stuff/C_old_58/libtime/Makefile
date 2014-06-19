# 68008 lib makefile

####### VARIABLES #######

# Get directory name, to use as output filename.
PRJ = $(notdir $(CURDIR))
LIB = ../lib/$(PRJ).a

# Program names

RM   = rm -f
CC   = m68k-elf-gcc
AR   = m68k-elf-ar

# Compiler and linker flags

INCPATHS= -I . -I ../include -I /Users/zigzagjoe/Documents/68008/deploy/lib/gcc/m68k-elf/4.2.4/include

CFLAGS  = -O3 -nostartfiles -nostdinc -nostdlib -m68000 -std=c99 -fno-builtin $(INCPATHS)
ARFLAGS = -ar rcs $(LIB)

# List of source files

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
LSTS=$(SRCS:.c=.S)

# clear suffixes
.SUFFIXES:
.SUFFIXES: .c .o .S

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
	
.c.S:
	$(CC) $(CFLAGS) -c -S -o $@ $<
