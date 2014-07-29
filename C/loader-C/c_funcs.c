/* C code for loader-C */

#include <loader.h>
#include <flash.h>

// used in percentage readout
uint8_t base_per;
uint16_t perc_interv;

// C handler called by ASM
uint16_t handle_srec(uint8_t * start, uint32_t len, uint8_t fl) { 
    base_per = 0;
    
    // i hate gcc inline ASM so many
    // calculate number of bytes per percentage point
    // we need to make use of the fact the 68k divu 
    // instruction takes a 32 bit number to be divided
    // this allows a max file size of 6,553,599 bytes
    __asm ( 
            "move.l %1, %%d0\n"
            "divu #100, %%d0\n"
            "move.w %%d0, (%0)\n"      
         :"=m"(perc_interv)    // outputs
         :"d"(len)             // inputs
         :"%%d0"               // clobber list
    );        
    
    uint8_t ret;
    
    // do sanity check run - parse srec only
    ret = parseSREC(start,len,fl, 0 /* writes disarmed */);
    
    if (ret) // fail?
        return ret;
        
    base_per = 50;
    
    // if no errors, do programming
    // allow flash writes
    flash_arm(FLASH_ARM);
    ret = parseSREC(start,len,fl, 1 /* writes armed */);
    flash_arm(0);
    
    if (ret) 
        return ret | PROG_FAILURE;
    
    return 0;
}
