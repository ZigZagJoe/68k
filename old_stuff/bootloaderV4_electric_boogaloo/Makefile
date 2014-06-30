# 68008 makefile for bootloader

####### VARIABLES #######

# Get directory name, to use as output filename.
PRJ = bootloader
BIN = loader.s68

LINK_SCRIPT = loader.ld

# Program names

RM   = rm -f
LD   = m68k-elf-ld
AS   = m68k-elf-as
OBJDUMP = m68k-elf-objdump
OBJCOPY = m68k-elf-objcopy

# Compiler and linker flags

ULFLAGS = -n
LDFLAGS = -nostartfiles -nostdlib -A m68000 -T $(LINK_SCRIPT) --oformat srec
ASFLAGS = 

# List of source files

SRCS=$(wildcard *.s)
OBJS=$(SRCS:.s=.o)

# clear suffixes
.SUFFIXES:
.SUFFIXES: .s .o

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
	upload-strapper $(ULFLAGS) $(PRJ).bin
	
$(BIN): $(OBJS) 
	$(LD)  $(OBJS) $(LDFLAGS) $(LIBS) -o $(BIN)
	$(OBJCOPY) -I srec $(BIN) -O binary $(PRJ).bin
	$(OBJDUMP) -D $(BIN) -m68000 > $(PRJ).lst
	@echo
	@echo -n 'Binary size: '
	@stat -f %z $(PRJ).bin | sed 's/$$/ bytes/'
	
.s.o:
	$(AS) $(ASFLAGS) -o $@ $<
	
