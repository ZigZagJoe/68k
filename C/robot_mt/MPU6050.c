#include <stdint.h>
#include <i2c.h>
#include "MPU6050.h"

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

void calibrateMPU6050(float * gryBiasDPS, float * accBiasMS) {  

    uint8_t data[12]; // data array to hold accelerometer and gyro x, y, z, data
    uint16_t ii, packet_count, fifo_count;
    int32_t gyro_bias[3] = {0, 0, 0}, accel_bias[3] = {0, 0, 0};

    // reset device, reset all registers, clear gyro and accelerometer bias registers
    i2c_reg_writebyte(PWR_MGMT_1, 0x80); // Write a one to bit 7 reset bit; toggle reset device
    DELAY_MS(100);  

    // get stable time source
    // Set clock source to be PLL with x-axis gyroscope reference, bits 2:0 = 001
    i2c_reg_writebyte(PWR_MGMT_1, 0x01);  
    i2c_reg_writebyte(PWR_MGMT_2, 0x00); 
    DELAY_MS(200);

    // Configure device for bias calculation
    i2c_reg_writebyte(INT_ENABLE, 0x00);   // Disable all interrupts
    i2c_reg_writebyte(FIFO_EN, 0x00);      // Disable FIFO
    i2c_reg_writebyte(PWR_MGMT_1, 0x00);   // Turn on internal clock source
    i2c_reg_writebyte(I2C_MST_CTRL, 0x00); // Disable I2C master
    i2c_reg_writebyte(USER_CTRL, 0x00);    // Disable FIFO and I2C master modes
    i2c_reg_writebyte(USER_CTRL, 0x0C);    // Reset FIFO and DMP
    DELAY_MS(15);

    // Configure MPU6050 gyro and accelerometer for bias calculation
    i2c_reg_writebyte(CONFIG, 0x01);      // Set low-pass filter to 188 Hz
    i2c_reg_writebyte(SMPLRT_DIV, 0x00);  // Set sample rate to 1 kHz
    i2c_reg_writebyte(GYRO_CONFIG, 0x00);  // Set gyro full-scale to 250 degrees per second, maximum sensitivity
    i2c_reg_writebyte(ACCEL_CONFIG, 0x00); // Set accelerometer full-scale to 2 g, maximum sensitivity

    uint16_t  gyrosensitivity  = 131;   // = 131 LSB/degrees/sec
    uint16_t  accelsensitivity = 16384;  // = 16384 LSB/g

    // Configure FIFO to capture accelerometer and gyro data for bias calculation
    i2c_reg_writebyte(USER_CTRL, 0x40);   // Enable FIFO  
    i2c_reg_writebyte(FIFO_EN, 0x78);     // Enable gyro and accelerometer sensors for FIFO  (max size 1024 bytes in MPU-6050)
    DELAY_MS(60); // accumulate 80 samples in 80 milliseconds = 960 bytes

    // At end of sample accumulation, turn off FIFO sensor read
    i2c_reg_writebyte(FIFO_EN, 0x00);        // Disable gyro and accelerometer sensors for FIFO

    i2c_reg_read(&data,FIFO_COUNTH,2);

    fifo_count = ((uint16_t)data[0] << 8) | data[1];
    packet_count = fifo_count/12;// How many sets of full gyro and accelerometer data for averaging

    for (ii = 0; ii < packet_count; ii++) {
        int16_t accel_temp[3] = {0, 0, 0}, gyro_temp[3] = {0, 0, 0};

        i2c_reg_read(&data,FIFO_R_W,12);

        accel_temp[0] = (int16_t) (((int16_t)data[0] << 8) | data[1]  ) ;  // Form signed 16-bit integer for each sample in FIFO
        accel_temp[1] = (int16_t) (((int16_t)data[2] << 8) | data[3]  ) ;
        accel_temp[2] = (int16_t) (((int16_t)data[4] << 8) | data[5]  ) ;    
        gyro_temp[0]  = (int16_t) (((int16_t)data[6] << 8) | data[7]  ) ;
        gyro_temp[1]  = (int16_t) (((int16_t)data[8] << 8) | data[9]  ) ;
        gyro_temp[2]  = (int16_t) (((int16_t)data[10] << 8) | data[11]) ;

        accel_bias[0] += (int32_t) accel_temp[0]; // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
        accel_bias[1] += (int32_t) accel_temp[1];
        accel_bias[2] += (int32_t) accel_temp[2];
        gyro_bias[0]  += (int32_t) gyro_temp[0];
        gyro_bias[1]  += (int32_t) gyro_temp[1];
        gyro_bias[2]  += (int32_t) gyro_temp[2];
    }
    
    accel_bias[0] /= (int32_t) packet_count; // Normalize sums to get average count biases
    accel_bias[1] /= (int32_t) packet_count;
    accel_bias[2] /= (int32_t) packet_count;
    gyro_bias[0]  /= (int32_t) packet_count;
    gyro_bias[1]  /= (int32_t) packet_count;
    gyro_bias[2]  /= (int32_t) packet_count;

    if(accel_bias[2] > 0L) {accel_bias[2] -= (int32_t) accelsensitivity;}  // Remove gravity from the z-axis accelerometer bias calculation
    else {accel_bias[2] += (int32_t) accelsensitivity;}

    // Construct the gyro biases for push to the hardware gyro bias registers, which are reset to zero upon device startup
    data[0] = (-gyro_bias[0]/4  >> 8) & 0xFF; // Divide by 4 to get 32.9 LSB per deg/s to conform to expected bias input format
    data[1] = (-gyro_bias[0]/4)       & 0xFF; // Biases are additive, so change sign on calculated average gyro biases
    data[2] = (-gyro_bias[1]/4  >> 8) & 0xFF;
    data[3] = (-gyro_bias[1]/4)       & 0xFF;
    data[4] = (-gyro_bias[2]/4  >> 8) & 0xFF;
    data[5] = (-gyro_bias[2]/4)       & 0xFF;

    // Push gyro biases to hardware registers; works well for gyro but not for accelerometer
    i2c_reg_writebyte(XG_OFFS_USRH, data[0]); 
    i2c_reg_writebyte(XG_OFFS_USRL, data[1]);
    i2c_reg_writebyte(YG_OFFS_USRH, data[2]);
    i2c_reg_writebyte(YG_OFFS_USRL, data[3]);
    i2c_reg_writebyte(ZG_OFFS_USRH, data[4]);
    i2c_reg_writebyte(ZG_OFFS_USRL, data[5]);

    gryBiasDPS[0] = (float) gyro_bias[0]/(float) gyrosensitivity; // construct gyro bias in deg/s for later manual subtraction
    gryBiasDPS[1] = (float) gyro_bias[1]/(float) gyrosensitivity;
    gryBiasDPS[2] = (float) gyro_bias[2]/(float) gyrosensitivity;

    // Construct the accelerometer biases for push to the hardware accelerometer bias registers. These registers contain
    // factory trim values which must be added to the calculated accelerometer biases; on boot up these registers will hold
    // non-zero values. In addition, bit 0 of the lower byte must be preserved since it is used for temperature
    // compensation calculations. Accelerometer bias registers expect bias input as 2048 LSB per g, so that
    // the accelerometer biases calculated above must be divided by 8.

    int32_t accel_bias_reg[3] = {0, 0, 0}; // A place to hold the factory accelerometer trim biases

    i2c_reg_read(&data,XA_OFFSET_H,2); // Read factory accelerometer trim values
    accel_bias_reg[0] = (int16_t) ((int16_t)data[0] << 8) | data[1];

    i2c_reg_read(&data,YA_OFFSET_H,2);
    accel_bias_reg[1] = (int16_t) ((int16_t)data[0] << 8) | data[1];

    i2c_reg_read(&data,ZA_OFFSET_H,2);
    accel_bias_reg[2] = (int16_t) ((int16_t)data[0] << 8) | data[1];

    uint32_t mask = 1uL; // Define mask for temperature compensation bit 0 of lower byte of accelerometer bias registers
    uint8_t mask_bit[3] = {0, 0, 0}; // Define array to hold mask bit for each accelerometer bias axis

    for(ii = 0; ii < 3; ii++) {
        if(accel_bias_reg[ii] & mask) mask_bit[ii] = 0x01; // If temperature compensation bit is set, record that fact in mask_bit
    }

    // Construct total accelerometer bias, including calculated average accelerometer bias from above
    accel_bias_reg[0] -= (accel_bias[0]/8); // Subtract calculated averaged accelerometer bias scaled to 2048 LSB/g (16 g full scale)
    accel_bias_reg[1] -= (accel_bias[1]/8);
    accel_bias_reg[2] -= (accel_bias[2]/8);

    data[0] = (accel_bias_reg[0] >> 8) & 0xFF;
    data[1] = (accel_bias_reg[0])      & 0xFF;
    data[1] = data[1] | mask_bit[0]; // preserve temperature compensation bit when writing back to accelerometer bias registers
    data[2] = (accel_bias_reg[1] >> 8) & 0xFF;
    data[3] = (accel_bias_reg[1])      & 0xFF;
    data[3] = data[3] | mask_bit[1]; // preserve temperature compensation bit when writing back to accelerometer bias registers
    data[4] = (accel_bias_reg[2] >> 8) & 0xFF;
    data[5] = (accel_bias_reg[2])      & 0xFF;
    data[5] = data[5] | mask_bit[2]; // preserve temperature compensation bit when writing back to accelerometer bias registers

    // Push accelerometer biases to hardware registers; doesn't work well for accelerometer
    // Are we handling the temperature compensation bit correctly?
    //  i2c_reg_writebyte(XA_OFFSET_H, data[0]);  
    //  i2c_reg_writebyte(XA_OFFSET_L_TC, data[1]);
    //  i2c_reg_writebyte(YA_OFFSET_H, data[2]);
    //  i2c_reg_writebyte(YA_OFFSET_L_TC, data[3]);  
    //  i2c_reg_writebyte(ZA_OFFSET_H, data[4]);
    //  i2c_reg_writebyte(ZA_OFFSET_L_TC, data[5]);

    // Output scaled accelerometer biases for manual subtraction in the main program
    accBiasMS[0] = (float)accel_bias[0]/(float)accelsensitivity; 
    accBiasMS[1] = (float)accel_bias[1]/(float)accelsensitivity;
    accBiasMS[2] = (float)accel_bias[2]/(float)accelsensitivity;
}
