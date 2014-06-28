#!/bin/bash
for n in  $(seq 0 255)
{   
 hex=`echo "obase=16; $n" | bc`
 echo ".long 0x${hex}080000; nop;nop;nop;" > test.s_
 m68k-elf-as -o test.o_ test.s_
 echo -n "${hex}: "
 m68k-elf-objdump -m68000 -D test.o_ | grep "  0\:"
}

rm test.o_
rm test.s_
