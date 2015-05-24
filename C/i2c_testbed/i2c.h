// software I2C library
// very slow, the 68k isn't good at this

#ifndef i2c_H
#define i2c_H

#include <io.h>
#include <stdlib.h>

// pin bits within GPDR
#define SCL 6
#define SDA 7

#define READ_SDA()      (GPDR & (1 << SDA))
#define SDA_OUT()       bset_a(DDR,SDA)
#define SDA_IN()        bclr_a(DDR,SDA)
#define READ_SCL()      (GPDR & (1 << SCL))

#define WRITE 0x0
#define READ 0x1

//#define NO_CLOCK_STRETCH 

// library control functions
void i2c_init();
void i2c_disable();

// public use data functions
uint8_t i2c_set_slave(uint8_t addr);
uint8_t i2c_write_byte(uint8_t byte);
uint8_t i2c_read_byte(uint8_t nack);

uint8_t write_data(uint8_t* data, uint8_t bytes);
uint8_t read_bytes(uint8_t* data, uint8_t bytes);

void i2c_start();
void i2c_stop();

// internal use low level functions
void toggle_scl();
void write_scl(uint8_t x);
void write_sda(uint8_t x);

uint8_t send_slave_address(uint8_t read);

#endif /* i2c_H */
