#ifndef BOOTLOADER
  #error Wrong srec_private - included header for loader-C    
#endif 

/*///// parse_srec header for loader-C /////*/

#include <io.h>
#include <flash.h>

#define dbgprintf ; //
// comment out the function call 
// have ; so that if (x) dbgprintf(...) does not break

void srec_progress();
// update progress % on TIL311 displays (in decimal!)

extern uint16_t perc_interv;
