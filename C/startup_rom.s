.align   2
.text

.global _start
.global _exit

.extern __stack_start
.extern __data_start
.extern __data_length
.extern __bss_start
.extern __bss_length
.extern __data_file_start
.extern __start_func

.set IO_BASE, 0xC0000
.set TIL311, IO_BASE + 0x8000

| bootable magic
.long 0xc141c340

_start:
   move.b #0xCC, (TIL311)         | what up my glip glops!

   move.l #0xFFFFFFFF, (0x400)    | mark kernel not active
   
   move.l #__stack_start, %a7
   
   /* copy writeable data section into RAM */
   
   move.l #__data_size, %d0   

   cmp.l #0, %d0                    | check if empty data section
   beq clrbss

   move.l #__data_rom_start, %a0   | A0 = data start
   move.l #__data_ram_start, %a1   | A1 = data dest
   
   | due to use of dbra, we can copy a max of 256kb - more than enough
cpydat:
   move.l (%a0)+, (%a1)+           | *A1++ = *A0++
   dbra %d0, cpydat                | D0 != 0

clrbss:
   /* clear BSS section */
   
   move.l #__bss_size, %d0
   
   cmp.l #0, %d0                   | check for empty bss section
   beq run_main
   
   move.l #__bss_start, %a0        | A0 = _bss start
   
   | due to use of dbra, we can clear a max of 256kb of RAM (65536 * sizeof(long))
cbss:
   clr.l (%a0)+                    | clear [A0.l]
   dbra %d0, cbss                  | D0 != 0
   
   /* ready to run c code */
run_main:
   jsr __start_func                | jump to main
   
_exit:                             | loop forever
   bra _exit                       | could/should return to monitor if present
