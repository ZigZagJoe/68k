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
#include <binary.h>

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
    
    /* mem_dump(all_regs, 14);
        while(true);*/
}    

// 2.66ms
    
/*int16_t raw_to_250dps(int16_t x) {
   return (x >> 7) - (x >> 12) + (x >> 14); 
}

int16_t integrate7ms(int16_t x) {
    return  (x >> 7) - (x >> 10) + (x >> 13) + (x >> 15);
}*/

#define DEG_TO_RAW 131
#define INTR_TIMR_D (1 << 4)


int32_t gryo_acc_l = 0;
int32_t gryo_acc_m = 0;
int32_t gryo_acc_t = 0;
       
int16_t gyro_last;

ISR(integrate) {
    int16_t gyro_y = 0;
    
    bset_a(GPDR, 1);
    i2c_reg_read(&gyro_y, GYRO_ZOUT_H, 2); 
    
     int16_t gyro_diff = gyro_y - gyro_last;
    
   // acc += integrate7ms(gyro_y);
    gryo_acc_l += gyro_y / 143;
    gryo_acc_m += (((int32_t)gyro_y + gyro_last) >> 1) / 143;
    gryo_acc_t += (gyro_y + (gyro_diff >> 1)) / 143;

    gyro_last = gyro_y;
    bclr_a(GPDR, 1);
}

#define INTEGRATE_TICK 167
// 1000 ms / tick freq

void integrate_readout() {

    __vectors.user[MFP_INT + MFP_TIMERD - USER_ISR_START] = &integrate;
	
	VR = MFP_INT;
	TCDCR &= 0xF0; // disable timer D
	
	TDDR = 120;      // 166.6 hz
	TCDCR |= 0x7;    // set prescaler of 50
	
	IERB |= INTR_TIMR_D;
	IMRB |= INTR_TIMR_D;
	
	
    while(true) {
        print_dec(gryo_acc_l);
        putc('\t');
        print_dec(gryo_acc_m);
        putc('\t');
        print_dec(gryo_acc_t);
        putc('\n');
        DELAY_MS(100);
    
    }
}
 
#define MCP7940_ADDR 0x6F
#define MCPREG_TIME 0
#define MCPREG_CONTROL 0x8
#define MCPREG_OSCTRIM 0x9
#define MCPREG_ALARM0 0xA
#define MCPREG_ALARM1 0x11
#define MCPREG_PWRDNST 0x18
#define MCPREG_PWRUPST 0x1c
#define MCPREG_RAM 0x20

typedef struct {
    /* reg 0 - RTCSEC */
    bool OSCPWR : 1;
    uint8_t sec : 7;
    
    /* reg 1 - RTCMIN */
    bool : 1;
    uint8_t min : 7;
    
    /* reg 2 - RTCHOUR */
    bool : 1;
    bool h12 : 1;
    uint8_t hr : 6;
    
    /* reg 3 - RTCWKDAY */
    bool : 2;
    bool OSCRUN : 1;
    bool PWRFAIL : 1;
    bool VBATEN : 1;
    uint8_t wkday : 3;
    
    /* reg 4 - RTCDATE */
    bool : 2;
    uint8_t day : 6;
    
    /* reg 5 - RTCMTH */
    bool : 2;
    bool LPYR : 1;
    uint8_t mth : 5;
    
    /* reg 6 - RTCYEAR */
    uint8_t yr : 8;
} mcp7940_time_bcd;

//void lzfx_decompress(int a,int b, int c, int d) {}
int main() {
   // TIL311 = 0x01;

    serial_start(SERIAL_SAFE);
    sei();


    bset(DDR,1);
    bclr_a(GPDR,1);    
    
    DELAY_MS(10);
    
    uint8_t success = 0; 

    printf("Init i2c\n");
    i2c_init(); //!< initialize twi interface
    
    /*i2c_set_slave(0x68);
    
     if (!i2c_reg_writebyte(PWR_MGMT_1, 0))   // reset gyro
        printf("Error: failed to reset MPU6050\n");
        
    DELAY_MS(10);  
  
    float gyroBias[3], accelBias[3]; // Bias corrections for gyro and accelerometer
    calibrateMPU6050(gyroBias, accelBias);

    //DELAY_MS(200);  // Delay a while to let the device execute the self-test
    printf("Begin loop\n");
    
 
    //partial_read();
    full_read();
    integrate_readout();*/
    
   // adc_test();
    //clock_test();
    
    printf("i2c bus scan:\n");
    for (uint8_t i = 0; i < 128; i++) {
        i2c_set_slave(i);
        if (i2c_poll_addr()) 
            printf("Device found at addr %d\n", i);
    }
    
    i2c_set_slave(B1010000);

  
    while(true);
}

/* this is not reliable, requires more work */
void eeprom_test() {
    srand();
   
    uint8_t buff_w[34];
    uint16_t *addr = &buff_w;
    uint8_t *buff_r = &buff_w[2];
    
    memset(&buff_w,0xFE,34);
    
    *addr = 0;
    
    while(true) {
        for (int i = 0; i < 31; i++ )
            buff_r[i] = rand8();
            
        printf("To write:");
        mem_dump(buff_r, 32);
        
  
        if (!i2c_bulk_write(buff_w,34))
            printf("error writing page\n");
           
        uint16_t c = 0; 
        while(!i2c_poll_addr()) c++;
        printf("Write completed in %d\n",c);
           
        i2c_bulk_write(addr,2);
        
        if (!i2c_bulk_read(buff_r, 32)) 
            printf("error reading page\n");
        
        printf("Read from %d:\n", *addr);
        mem_dump(buff_r, 32);
        
        DELAY_MS(200);
        //i2c_bulk_write(buff_w, 34);
        
    }
}

void adc_test() {
 i2c_set_slave(B1001101);
    
    while(true) {
        uint16_t v;
        if (i2c_bulk_read(&v, 2))
            printf("%d\n",v);
    
    }
    
}

void clock_test() {
  i2c_set_slave(MCP7940_ADDR); /* rtc */
    
    mcp7940_time_bcd time;  
   /* i2c_reg_read(&time, 0, sizeof(time));
    
    time.OSCPWR = 1;
    time.min = 0x46;
    time.sec = 0x0;
    time.hr = 0x22;
    time.mth = 0x6;
    time.yr = 0x15;
    time.day = 0x21;
    
    i2c_reg_write(&time, 0, sizeof(time));*/
    
    while(true) {
        bset_a(GPDR, 1);
        
        int code = i2c_reg_read(&time, 0, sizeof(time));
        
        if (!code) {
            printf("Read error!\n");
            continue;
        }
        
        if (!time.OSCPWR) { // osc is not running
            if (!i2c_reg_writebyte(0, 128)) {
                printf("Error turning on RTC\n");
            } else
                printf("Turned on RTC osc\n");
        }
        
        bclr_a(GPDR, 1);
        
        for (int i = 0; i < 33; i++) putc(8);
         
        printf ("%2x:%02x:%02x %2x/%02x/%02x",time.hr,time.min,time.sec,time.mth, time.day, time.yr);
    }   
}


void leave_critical() {

}

void enter_critical() {

}
