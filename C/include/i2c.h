// software I2C library, superfast ASM

#ifndef I2C_ASM_H
#define I2C_ASM_H

#include <stdint.h>

// library control functions
void i2c_init();
void i2c_set_slave(uint8_t addr);

// signal i2c bus states
void i2c_start_cond();
void i2c_stop_cond();

// basic r/w functions
// these do not emit a start/stop condition!
uint8_t i2c_write_byte(uint8_t byte);
uint8_t i2c_read_byte(uint8_t nack);

// bulk register r/w
uint8_t i2c_reg_read(uint8_t *addr, uint8_t start, uint16_t count);
uint8_t i2c_reg_write(uint8_t *addr, uint8_t start, uint16_t count);

// bulk r/w
uint8_t i2c_bulk_read(uint8_t *addr, uint16_t count);
uint8_t i2c_bulk_write(uint8_t *addr, uint16_t count) ;

// single byte register r/w
uint8_t i2c_reg_readbyte(uint8_t reg);
uint8_t i2c_reg_writebyte(uint8_t reg, uint8_t value);

#endif /* I2C_ASM_H */
