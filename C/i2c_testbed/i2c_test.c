#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>
#include "i2c.h"

#include "MPU6050.h"


//void lzfx_decompress(int a,int b, int c, int d) {}
int main() {
   // TIL311 = 0x01;

    serial_start(SERIAL_SAFE);
    //millis_start();
    sei();
    
    bset(DDR,1);
    
    uint8_t success = 0; 

    printf("Init i2c\n");
    i2c_init(); //!< initialize twi interface
    i2c_set_slave(0x68);
    
     if (!i2c_reg_writebyte(PWR_MGMT_1, 0x80))   // reset gyro
        printf("Error: failed to reset MPU6050\n");
        
    DELAY_MS(100);  
  
    i2c_reg_writebyte(PWR_MGMT_1,0); // disable standby
   
    // get stable time source
    // Set clock source to be PLL with x-axis gyroscope reference, bits 2:0 = 001
    i2c_reg_writebyte(PWR_MGMT_1, 0x01);  
    i2c_reg_writebyte(PWR_MGMT_2, 0x00); 

    DELAY_MS(100);  
            
    i2c_reg_writebyte(ACCEL_CONFIG, 0xF0); // Enable self test on all three axes and set accelerometer range to +/- 8 g
    i2c_reg_writebyte(GYRO_CONFIG,  0xE0); // Enable self test on all three axes and set gyro range to +/- 250 degrees/s
   
    DELAY_MS(100);  
    
    float gyroBias[3], accelBias[3]; // Bias corrections for gyro and accelerometer
    calibrateMPU6050(gyroBias, accelBias);

    //DELAY_MS(200);  // Delay a while to let the device execute the self-test
    printf("Begin loop\n");
    
    int16_t gyro_y = 0;
    

    while(true) {
        bset(GPDR, 1);
        int code = i2c_reg_read(&gyro_y, GYRO_ZOUT_H, 2);
        bclr(GPDR, 1);
           
        int y_dps = (gyro_y >> 7) - (gyro_y >> 12) + (gyro_y >> 14); 
        
       // int y_dps = (int)((float)gyro_y * 0.00763F);
        

        /*if (!code)
            printf("Error\n");

       // mem_dump(&registers,14);
        
        //DELAY_MS(250);
        
        printf("AcX = %5d  AcY = %5d  AcZ = %5d TMP = %5d GyX = %5d  GyY = %5d  GyZ = %5d\n",reg16[0],reg16[1],reg16[2],reg16[3],reg16[4],reg16[5],reg16[6]);*/
        
    
        printf("%5d\n", y_dps);        
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
