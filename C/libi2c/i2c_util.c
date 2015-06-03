// software i2c library utilty functions

#include "i2c.h"

// TODO: this should be integrated into the function calls
uint8_t curr_slave;

// set the address of the current slave
void i2c_set_slave(uint8_t addr) {
    curr_slave = addr << 1; // shift it now so routines do not need to
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

