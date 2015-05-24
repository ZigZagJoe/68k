#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>
#include "TWI_master.h"


int read_regs(uint8_t *addr) {
    unsigned char index,success = 0;
	if(!twi_start_cond())
		return 0;
	if(!send_slave_address(WRITE))
		return 0;	
		
	success = i2c_write_byte(0x3B);
	if (!success) return 0;
	
		//put stop here
	write_scl(1);
	__delay_cycles(SCL_SDA_DELAY);
	write_sda(1);
	
	if(!twi_start_cond())
		return 0;
		
	if(!send_slave_address(READ))
		return 0;	
			
	for(index = 0; index < 14; index++)
	{
		success = i2c_read_byte(addr, 14, index);
		if(!success)
			break; 
	}
	
	//put stop here
	write_scl(1);
	__delay_cycles(SCL_SDA_DELAY);
	write_sda(1);
	return success;
}

//void lzfx_decompress(int a,int b, int c, int d) {}
int main() {
   // TIL311 = 0x01;

    serial_start(SERIAL_SAFE);
    //millis_start();
    sei();
    
    bset(DDR,1);
    
    uint8_t success = 0; 

    printf("Init i2c\n");
    twi_init(); //!< initialize twi interface

    uint8_t pwr_on[] = {0x6B,0};

    printf("send pwr_on\n");
    success = write_data(pwr_on, 2);// wake up the MPU6050 by setting 0 to PWR_MGMT_1
    if (!success) printf("Error sending pwr_on\n");
    
    uint8_t registers[14];

    printf("Begin loop\n");
    
    int16_t *reg16 = &registers;
    while(true) {
        memset(&registers,0, sizeof(registers));


        bset(GPDR, 1);
        int code = read_regs(&registers);
        bclr(GPDR, 1);
        
        if (!code)
            printf("Error\n");

       // mem_dump(&registers,14);
        
        DELAY_MS(250);
        
        printf("AcX = %5d  AcY = %5d  AcZ = %5d TMP = %5d GyX = %5d  GyY = %5d  GyZ = %5d\n",reg16[0],reg16[1],reg16[2],reg16[3],reg16[4],reg16[5],reg16[6]);
        
    }
    
    
  /*AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
  Serial.print("AcX = "); Serial.print(AcX);
  Serial.print(" | AcY = "); Serial.print(AcY);
  Serial.print(" | AcZ = "); Serial.print(AcZ);
  Serial.print(" | Tmp = "); Serial.print(Tmp/340.00+36.53);  //equation for temperature in degrees C from datasheet
  Serial.print(" | GyX = "); Serial.print(GyX);
  Serial.print(" | GyY = "); Serial.print(GyY);
  Serial.print(" | GyZ = "); Serial.println(GyZ);*/
}

void leave_critical() {

}

void enter_critical() {

}
