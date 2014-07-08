/* C code for loader-C */

#include <loader.h>
#include <flash.h>

// used in percentage readout
uint8_t base_per;

// C handler called by ASM
uint16_t handle_srec(uint8_t * start, uint32_t len, uint8_t fl) { 
    base_per = 0;
    
    // do sanity check parse run
    uint8_t ret = parseSREC(start,len,fl,0);
    
    if (ret) 
        return ret;
        
    base_per = 50;
    
    // if no errors, do programming
    // allow flash writes
    flash_arm(FLASH_ARM);
    ret = parseSREC(start,len,fl,1);
    flash_arm(0);
    
    if (ret) 
        return ret | PROG_FAILURE;
    
    return 0;
}
