/* C code for loader-C */

#include <loader.h>
#include <flash.h>

// C handler called by ASM
uint8_t handle_srec(uint8_t * start, uint32_t len, uint8_t fl) { 
    // do sanity check parse run
    uint8_t ret = parseSREC(start,len,fl,0);
    
    if (ret) 
        return ret;
    
    // if no errors, do programming
    // allow flash writes
    flash_arm(FLASH_ARM);
    ret = parseSREC(start,len,fl,1);
    flash_arm(0);
    
    if (ret) 
        return ret | PROG_FAILURE;
    
    return 0;
}
