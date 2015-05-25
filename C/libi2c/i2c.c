// software i2c library
// very slow, the 68k isn't good at this

#include "i2c.h"

// global state
uint8_t curr_slave;

// enable i2c bus 
void i2c_init() {
	bset_a(DDR, SCL);
    bset_a(DDR, SDA);

	write_sda(1);
    write_scl(1);
    curr_slave = 0;
} 

// disable master mode (release pins)
void i2c_disable() {
    bclr_a(DDR, SCL);
    bclr_a(DDR, SDA);
}

// send start condition
void i2c_start() {
    write_sda(0);
	write_scl(0);	
}

// send stop condition
void i2c_stop() {
    write_scl(1);
	write_sda(1);
}

// set SCL
void write_scl(uint8_t x) {
    if(x) {
        bclr_a(DDR, SCL); //tristate
    } else {
        bset_a(DDR, SCL);
        bclr_a(GPDR, SCL);
    }
}

// set SDA
void write_sda(uint8_t x) {
    if(x) {
        bclr_a(DDR,SDA); //tristate
    } else {
        bset_a(DDR,SDA); //output 
        bclr_a(GPDR,SDA);
    }
}

// set the address of the current slave
void i2c_set_slave(uint8_t addr) {
    curr_slave = addr << 1;
}

// send slave addr
uint8_t send_slave_address(uint8_t read) {
	return i2c_write_byte(curr_slave | read);
} 

/* 
// write a single byte out
uint8_t i2c_write_byte(uint8_t byte) {
    uint8_t bit;
    
	for (bit = 0; bit < 8; bit++) {
        write_sda((byte & 0x80) != 0);
        toggle_scl(); //goes high
        toggle_scl(); //goes low
        byte <<= 1;
    }
    
	//release SDA
	SDA_IN();
	toggle_scl(); //goes high for the 9th clock
	
	// Check for ack
	if(READ_SDA()) 
		return 0;			
	
	//Pull SCL low
	toggle_scl(); //end of byte with acknowledgment. 
	
	// zzj fix for bad stop condition
	write_sda(0);
	//take SDA
	SDA_OUT();
	
	return 1;
}	
*/
/*// returns byte. unable to check for failure. argument weither to send ack or nack
uint8_t i2c_read_byte(uint8_t nack) {
    uint8_t byte = 0;
	uint8_t bit = 0;
	
	//release SDA
	SDA_IN();
	
	for (bit = 0; bit < 8; bit++) {
        toggle_scl();//goes high
        
        if(READ_SDA())
            byte |= (1 << (7-bit));

        toggle_scl(); //goes low
    }

	//take SDA
	SDA_OUT();
	
	if(!nack) {
		write_sda(0);
		toggle_scl(); //goes high for the 9th clock
		toggle_scl();
		write_sda(1);
	} else { //send NACK (last byte)
		write_sda(1);
		toggle_scl();
		toggle_scl(); 
	}		
	 
	return byte;
		
}	*/

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

// read bulk data from device
uint8_t read_bytes(uint8_t* data, uint8_t bytes) {
	uint8_t index;
	i2c_start();
	
	if(!send_slave_address(I2C_READ))
		return 0;	
		
	for(index = 0; index < bytes; index++)
	    data[index] = i2c_read_byte(index == (bytes-1));
	
	i2c_stop();
	
	return 1;
}	


uint8_t i2c_reg_readbyte(uint8_t reg) {
    uint8_t ret;
    i2c_reg_read(&ret,reg,1);
    return ret;
}

uint8_t i2c_reg_writebyte(uint8_t reg, uint8_t value) {
    uint8_t data[] = {reg,value};
    return write_data(data, 2);// wake up the MPU6050 by setting 0 to PWR_MGMT_1
}

/*
uint8_t i2c_reg_read(uint8_t *addr, uint8_t startReg, uint8_t count) {
	i2c_start();
	
	if(!send_slave_address(I2C_WRITE))
		return 0;	
		
	if (!i2c_write_byte(startReg)) 
	    return 0;
	
	i2c_stop();
	i2c_start();
		
	if(!send_slave_address(I2C_READ))
		return 0;	
			
	for(uint8_t i = 0; i < count; i++)
		addr[i] = i2c_read_byte((count-1) == i);
	
	i2c_stop();
	
	return 1;
}*/


/*
#define READ_SDA()      (GPDR & (1 << SDA))
#define SDA_OUT()       bset_a(DDR,SDA)
#define SDA_IN()        bclr_a(DDR,SDA)
#define READ_SCL()      (GPDR & (1 << SCL))
 
// toggles SCL
void toggle_scl() {
    if (GPDR & (1<<SCL)) {
        bset_a(DDR,SCL); //output 
        bclr_a(GPDR,SCL);
    } else {
        bclr_a(DDR, SCL); //tristate it
    }		
}

// write a single byte out
uint8_t i2c_write_byte(uint8_t byte) {
    uint8_t bit;
    
	for (bit = 0; bit < 8; bit++) {
        write_sda((byte & 0x80) != 0);
        toggle_scl(); //goes high
        toggle_scl(); //goes low
        byte <<= 1;
    }
    
	//release SDA
	SDA_IN();
	toggle_scl(); //goes high for the 9th clock
	
	// Check for ack
	if(READ_SDA()) 
		return 0;			
	
	//Pull SCL low
	toggle_scl(); //end of byte with acknowledgment. 
	
	// zzj fix for bad stop condition
	write_sda(0);
	//take SDA
	SDA_OUT();
	
	return 1;
}	

// returns byte. unable to check for failure. argument weither to send ack or nack
uint8_t i2c_read_byte(uint8_t nack) {
    uint8_t byte = 0;
	uint8_t bit = 0;
	
	//release SDA
	SDA_IN();
	
	for (bit = 0; bit < 8; bit++) {
        toggle_scl();//goes high
        
        if(READ_SDA())
            byte |= (1 << (7-bit));

        toggle_scl(); //goes low
    }

	//take SDA
	SDA_OUT();
	
	if(!nack) {
		write_sda(0);
		toggle_scl(); //goes high for the 9th clock
		toggle_scl();
		write_sda(1);
	} else { //send NACK (last byte)
		write_sda(1);
		toggle_scl();
		toggle_scl(); 
	}		
	 
	return byte;
		
}	*/
