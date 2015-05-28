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

void partial_read(){
    int16_t gyro_y = 0;
    int y_dps;
    
     while(true) {

       bset_a(GPDR, 1);
        i2c_reg_read(&gyro_y, GYRO_ZOUT_H, 2); 
        bclr_a(GPDR, 1);
        
        
        y_dps = gyro_y / 131; 
     
       // putc(y_dps);
        printf("%5d\n", y_dps);
        serial_wait();
     }
}


void full_read() {
    int16_t all_regs[7]  ;  

    while(true) {
        bset_a(GPDR, 1);
        int code = i2c_reg_read(&all_regs, ACCEL_XOUT_H, 14);
        bclr_a(GPDR, 1);
        
        printf("AcX = %5d  AcY = %5d  AcZ = %5d TMP = %5d GyX = %5d  GyY = %5d  GyZ = %5d\n",all_regs[0],all_regs[1],all_regs[2],tmp_raw_to_F(all_regs[3]),all_regs[4],all_regs[5],all_regs[6]);
        serial_wait();
    }
}    

// 2.66ms
    
/*int16_t raw_to_250dps(int16_t x) {
   return (x >> 7) - (x >> 12) + (x >> 14); 
}

int16_t integrate7ms(int16_t x) {
    return  (x >> 7) - (x >> 10) + (x >> 13) + (x >> 15);
}*/

#define DEG_TO_RAW 131

void integrate() {
    int16_t gyro_y = 0;
    int32_t acc = 0;
    
    uint8_t i = 0;
    while(true) {
        bset_a(GPDR, 1);
        DELAY_MS(6);
        DELAY(20);
        bset_a(GPDR, 1);
        bclr_a(GPDR, 1);
        i2c_reg_read(&gyro_y, GYRO_ZOUT_H, 2); 
        
       // acc += integrate7ms(gyro_y);
        acc += gyro_y / 143;
    
        i++;
        if (i == 60) {
            i = 0;
            printf("%d\n",acc/131);
        }
        bclr_a(GPDR, 1);
    }

}
 
//void lzfx_decompress(int a,int b, int c, int d) {}
int main() {
   // TIL311 = 0x01;

    serial_start(SERIAL_SAFE);
    //millis_start();
    sei();

    bset(DDR,1);
    bclr_a(GPDR,1);    
    
    DELAY_MS(10);
    
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

   // partial_read();
    full_read();
    //integrate();
}

void leave_critical() {

}

void enter_critical() {

}
