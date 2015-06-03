// software I2C library

#ifndef i2c_H
#define i2c_H

#include <stdint.h>

// library control functions
void i2c_init();
void i2c_set_slave(uint8_t addr);

// data functions
// these do not give a start/stop condition!
uint8_t i2c_write_byte(uint8_t byte);
uint8_t i2c_read_byte(uint8_t nack);

// bulk register actions
uint8_t i2c_reg_read(uint8_t *addr, uint8_t start, uint16_t count);
uint8_t i2c_reg_write(uint8_t *addr, uint8_t start, uint16_t count);

// single byte register reads/writes
uint8_t i2c_reg_readbyte(uint8_t reg);
uint8_t i2c_reg_writebyte(uint8_t reg, uint8_t value);

// bulk read/writes
uint8_t i2c_bulk_read(uint8_t *addr, uint16_t count);
uint8_t i2c_bulk_write(uint8_t *addr, uint16_t count) ;

uint8_t send_slave_address(uint8_t read);

#endif /* i2c_H */
