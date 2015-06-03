// software i2c library
// very slow, the 68k isn't good at this

#include "i2c.h"

#include <io.h>
#include <stdlib.h>

uint8_t curr_slave;

// set the address of the current slave
void i2c_set_slave(uint8_t addr) {
    curr_slave = addr << 1;
}

uint8_t i2c_reg_readbyte(uint8_t reg) {
    uint8_t ret;
    i2c_reg_read(&ret, reg, 1);
    return ret;
}

uint8_t i2c_reg_writebyte(uint8_t reg, uint8_t value) {
    uint8_t data[] = {reg, value};
    return i2c_bulk_write(data, 2);
}

