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
    
     if (!i2c_reg_writebyte(PWR_MGMT_1, 0))   // reset gyro
        printf("Error: failed to reset MPU6050\n");
        
    DELAY_MS(10);  
  
    float gyroBias[3], accelBias[3]; // Bias corrections for gyro and accelerometer
    calibrateMPU6050(gyroBias, accelBias);

    //DELAY_MS(200);  // Delay a while to let the device execute the self-test
    printf("Begin loop\n");
    
    int16_t gyro_y = 0;
    int16_t all_regs[7]  ;  

    while(true) {
       /** bset(GPDR, 1);
        int code = i2c_reg_read(&gyro_y, GYRO_ZOUT_H, 2); 
        bclr(GPDR, 1);
         
        int y_dps = (gyro_y >> 7) - (gyro_y >> 12) + (gyro_y >> 14); 
        printf("%5d\n", y_dps);*/
        
        bset(GPDR, 1);
        int code = i2c_reg_read(&all_regs, ACCEL_XOUT_H, 14);
        bclr(GPDR, 1);
        printf("AcX = %5d  AcY = %5d  AcZ = %5d TMP = %5d GyX = %5d  GyY = %5d  GyZ = %5d\n",all_regs[0],all_regs[1],all_regs[2],all_regs[3],all_regs[4],all_regs[5],all_regs[6]); 
    }
    
}

void leave_critical() {

}

void enter_critical() {

}
