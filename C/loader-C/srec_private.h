#ifndef BOOTLOADER
  #error Wrong srec_private - included header for loader-C    
#endif 

/*///// parse_srec header for loader-C /////*/

#include <io.h>
#include <flash.h>
#include "asm_funcs.h"

#define dbgprintf ; //
// comment out the debug printf 
// put ; so that if (x) dbgprintf(...) does not break

// update progress % on TIL311 displays (in decimal!)
void srec_progress();
