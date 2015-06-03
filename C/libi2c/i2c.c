// software i2c library
// very slow, the 68k isn't good at this

#include "i2c.h"

// global state
uint8_t curr_slave;

// set the address of the current slave
void i2c_set_slave(uint8_t addr) {
    curr_slave = addr << 1;
}

// send slave addr
uint8_t send_slave_address(uint8_t read) {
	return i2c_write_byte(curr_slave | read);
} 


// write bulk data to device
uint8_t write_data(uint8_t* indata, uint8_t bytes) {
	uint8_t index, ack = 0;
	
	i2c_start();
	
	if(!send_slave_address(I2C_WRITE))
		return 0;	
	
	for(index = 0; index < bytes; index++) {
		 ack = i2c_write_byte(indata[index]);
		 if(!ack)
			break;		
	}
	
	i2c_stop();
	
	return ack;	
}

uint8_t i2c_reg_readbyte(uint8_t reg) {
    uint8_t ret;
    i2c_reg_read(&ret,reg,1);
    return ret;
}

uint8_t i2c_reg_writebyte(uint8_t reg, uint8_t value) {
    uint8_t data[] = {reg,value};
    return write_data(data, 2); // wake up the MPU6050 by setting 0 to PWR_MGMT_1
}

