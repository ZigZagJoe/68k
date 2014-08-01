# 68008 executable makefile stub

PRJ = bootloader
LINK_SCRIPT = loader-C.ld

OPTIMIZE = s
CFLAGS += -DBOOTLOADER

# 68008 executable basic makefile

####### VARIABLES #######

ifndef BIN
    BIN = $(PRJ).s68
endif

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
LDFLAGS_1 += -nostartfiles -nostdlib -A m68000 $(LIBPATHS) --oformat binary -T loader-C-s1.ld
LDFLAGS_2 += -nostartfiles -nostdlib -A m68000 $(LIBPATHS) --oformat srec -T loader-C-s2.ld
ASFLAGS += -march=68000 -mcpu=68000
ULFLAGS += -c

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
	$(RM) $(OBJS) $(BIN) $(LSTS) $(PRJ).bin $(PRJ).l68 $(PRJ).map $(PRJ)-s1.map $(PRJ)-s1.bin $(PRJ)-s1.o $(PRJ)-s1.lzf
	
commit:
	git diff
	git add . 
	git commit -a

# Open source in default editor, on mac.
open:
	-edit $(SRC_C) $(SRC_S) Makefile
	
run:	$(BIN)
	upload-strapper $(ULFLAGS) $(PRJ).bin
	
listing: $(LSTS)
	
$(BIN): $(OBJS) 
	$(LD)  $(OBJS) $(LIBS) $(LDFLAGS_1) -o $(PRJ)-s1.bin -Map $(PRJ)-s1.map 
	lzf_img_gen $(PRJ)-s1.bin $(PRJ)-s1.lzf
	$(OBJCOPY) -I binary -O elf32-m68k -B 68000 $(PRJ)-s1.lzf $(PRJ)-s1.o
	$(LD) lzf_reloc.o $(PRJ)-s1.o $(LIBS) $(LDFLAGS_2) -o $(BIN) -Map $(PRJ).map 
	$(OBJCOPY) -I srec $(BIN) -O binary $(PRJ).bin
	$(OBJDUMP) -D $(BIN) -m68000 > $(PRJ).l68
	@echo
	@echo -n 'Binary size: '
	@stat -f %z $(PRJ).bin | sed 's/$$/ bytes/'
	
.s.o:
	$(AS) $(ASFLAGS) -o $@ $<
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
.c.lst:
	$(CC) $(CFLAGS) -c -S -o $@ $<
