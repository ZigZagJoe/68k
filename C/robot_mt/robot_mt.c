#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <printf.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>
#include <kernel.h>
#include <semaphore.h>

#include <binary.h>


#define LEFT_BUMPER_BIT 4
#define RIGHT_BUMPER_BIT 3
#define DISTANCE_SENSOR_BIT 5

#define LEFT_BUMPER bisset(GPDR, LEFT_BUMPER_BIT)
#define RIGHT_BUMPER bisset(GPDR, RIGHT_BUMPER_BIT)

#define LEFT_MOTOR TBDR
#define RIGHT_MOTOR TCDR

// high level positions
#define PAN_LEFT_90  0
#define PAN_LEFT_45  1
#define PAN_LEFT_22  2
#define PAN_CENTER   3
#define PAN_RIGHT_22 4
#define PAN_RIGHT_45 5  
#define PAN_RIGHT_90 6

// low level pulse lengths
#define PAN_LEFT_90_V  440
#define PAN_LEFT_45_V  338
#define PAN_LEFT_22_V  294
#define PAN_CENTER_V   235
#define PAN_RIGHT_22_V 192    
#define PAN_RIGHT_45_V 150
#define PAN_RIGHT_90_V  91

const uint32_t pan_to_pulse[] = {PAN_LEFT_90_V,PAN_LEFT_45_V, PAN_LEFT_22_V, PAN_CENTER_V, PAN_RIGHT_22_V, PAN_RIGHT_45_V, PAN_RIGHT_90_V};
    
// low level speeds

// 1.5ms
#define MOTOR_T_NEUTRAL 111
// 1ms
#define MOTOR_T_FWD_FULL (MOTOR_T_NEUTRAL+38)
// 2ms
#define MOTOR_T_BCK_FULL (MOTOR_T_NEUTRAL-38)
// 1ms
#define MOTOR_T_FWD_HALF (MOTOR_T_NEUTRAL+38*2/3)
// 2ms
#define MOTOR_T_BCK_HALF (MOTOR_T_NEUTRAL-38*2/3)

const uint8_t speeds[] = {MOTOR_T_BCK_FULL, MOTOR_T_BCK_HALF, MOTOR_T_NEUTRAL, MOTOR_T_FWD_HALF, MOTOR_T_FWD_FULL};

// number of directional speeds 
#define MOTOR_SPEEDS 2

// high level speeds
#define MOTOR_STOP 0
#define MOTOR_FWD_FULL 2
#define MOTOR_FWD_HALF 1
#define MOTOR_BCK_FULL -2
#define MOTOR_BCK_HALF -1

// custom characters
#define CH_UPARR 0
#define CH_DNARR 1
#define CH_STOP 2

#define BCK_JOB 0x31BB

/* prototypes */

typedef struct {
    int8_t ML;
    int8_t MR;
    int32_t duration; // in ms
    uint32_t ID;     
} motor_state;

// functions
void estop_motors();
void lcd_load_ch();
void push_state(motor_state ms);
void do_avoidance(); // spawns avoidance task
void sensor_move(uint32_t count, uint8_t duration);

// tasks
void task_handle_obstacle();
void task_check_bumper();
void task_drive();
void task_lcd_status();
void task_distance_sense();

/* global state */
motor_state drive_stack[128];
uint8_t drive_stack_head = 127;

#define curr_state (drive_stack[drive_stack_head])

sem_t lcd_sem;
sem_t ds_sem;

volatile uint8_t  dist_raw = 0;            // return from timera IRQ
volatile uint8_t  last_dist[7];            // current scan results
volatile uint8_t  estop = false;           // motor emergency stop: halts drive stack processing
volatile task_t   avoidance_task = NULL;
 
ISR(measure_done) {
    dist_raw = TADR;
}

// monitors bumper sensors
void task_check_bumper() {
    while(true) {
        if (LEFT_BUMPER || RIGHT_BUMPER) {
           do_avoidance();
           sleep_for(30);
           
        }
        yield();
    }
}

void do_avoidance() {
    estop_motors();
    
    if (!avoidance_task || !task_active(avoidance_task)) {
        avoidance_task = create_task(&task_handle_obstacle, 0);
        return;
    }
}

void task_handle_obstacle() {

    // if top of stack is reverse task, just refresh duration
    if (curr_state.ID == BCK_JOB) {
        curr_state.duration = 1200;
    } else {

        // stop *now*
        estop_motors();
        
        sem_acquire(&ds_sem);
    
        // put the turn on the stack
        if (true) {
            motor_state rot_right = { MOTOR_FWD_FULL, MOTOR_BCK_FULL, 300 + rand8(), 0x3144 };
            push_state(rot_right);   
        } else {
            motor_state rot_left = { MOTOR_BCK_FULL, MOTOR_FWD_FULL, 300 + rand8(), 0x3111 };
            push_state(rot_left);
        }
    
        // put reverse on the stack
        motor_state go_back = { MOTOR_BCK_FULL, MOTOR_BCK_FULL, 1200, BCK_JOB };
        push_state(go_back);
        
        sem_release(&ds_sem);
    
    }

    estop = false;
}

// sets motor state to top of state stack, and consumes state when duration expired
void task_drive() {
    while(true) {
    
        while(estop) // emergency stop; don't drive motors
            yield();
        
        sem_acquire(&ds_sem);
    
        motor_state *cs = &curr_state;
        
        if (cs->duration != 0) {
            LEFT_MOTOR =  speeds[-(cs->ML) + MOTOR_SPEEDS]; // convert from signed int to unsigned index
            RIGHT_MOTOR = speeds[ (cs->MR) + MOTOR_SPEEDS]; // convert from signed int to unsigned index

            TIL311 = cs->ID & 0xFF;
        
            if (cs->duration != -1) {
                cs->duration -= 15;
                if (cs->duration < 1)
                    drive_stack_head++; // pop state
            }
        } else 
            drive_stack_head++; // ignore state as duration is 0
        
        sem_release(&ds_sem);
        sleep_for(15);
    }
}

// print the current hardware state to LCD, with a heartbeat
void task_lcd_status() {
    uint8_t st = 0;
    
    uint8_t chs[] = {0xA5,' '};
    
    while(true) {
        sem_acquire(&lcd_sem);
        
        lcd_cursor(1,1);
            
        // print motor statuses
        if (curr_state.MR == MOTOR_STOP) {
            lcd_data(CH_STOP);
        } else 
            lcd_data(curr_state.MR > MOTOR_STOP ? CH_UPARR : CH_DNARR);
                        
        lcd_data(' ');
            
        if (curr_state.ML == MOTOR_STOP) {
            lcd_data(CH_STOP);
        } else 
            lcd_data(curr_state.ML > MOTOR_STOP ? CH_UPARR : CH_DNARR);
        
        // sensor status
        lcd_cursor(6,1);
        lcd_data(LEFT_BUMPER?'L':' ');
        lcd_data(' ');
        lcd_data(RIGHT_BUMPER?'R':' ');
        lcd_data(' ');
        lcd_printf("%03d",last_dist[PAN_CENTER]);
        
        // print heartbeat
        lcd_cursor(15,1);
        st++;
        lcd_data(chs[(st >> 2) & 1]);
           
        sem_release(&lcd_sem);
        sleep_for(100);
    }
}

void pan_sensor(uint8_t p) {
    uint32_t count = pan_to_pulse[p];
    
    sensor_move(count, 5);  
}

void sensor_move(uint32_t count, uint8_t duration) {
    for (uint8_t i = 0; i < duration; i++) {
        enter_critical();
        bset(GPDR,6);
        __asm volatile("move.l %0,%%d0\n" \
                                "1: subi.l #1, %%d0\n" \
                                "bne 1b\n" \
                                ::"d"(count):"d0");
        bclr(GPDR, 6);
        leave_critical();
        sleep_for(10);
    }
}


void task_distance_sense() {
    __vectors.user[MFP_INT + MFP_GPI4 - USER_ISR_START] = &measure_done;
    
    bset (DDR, DISTANCE_SENSOR_BIT);
	bset (AER, 4);  // set positive edge triggered
	bset (IERA, 5); // enable timera interrupt...
    bclr (IMRA, 5); // but mask it
    
    bset (IERB, 6); // enable gpip4 interrupt...
    bset (IMRB, 6); // unmask it
         
    bset (DDR, 6); // pan servo
    
    int16_t distance;
    
    const uint8_t scan_pattern[] = {PAN_CENTER, PAN_LEFT_22, PAN_LEFT_45, PAN_LEFT_22, PAN_CENTER, PAN_RIGHT_22, PAN_RIGHT_45, PAN_RIGHT_22};
    
    uint8_t scan_i = 0;
    
	while(true) {         
        enter_critical();
        
        TACR = 0;
	    bclr(IPRA, 5);

        dist_raw = 0;
       
        // tell the sensor to send a ranging ping
        bset(GPDR, DISTANCE_SENSOR_BIT);
        __asm volatile ("nop\nnop\n");
        bclr(GPDR, DISTANCE_SENSOR_BIT);
        
        // set up Timer A to count while TAI is high
        TADR = 0;
	    TACR = 31;
	    
	    leave_critical();
	    
	    long start = millis();
	    
	    while (dist_raw == 0 && ((millis() - start) < 20)) 
	        yield();
        
        TACR = 0;
        
        if (dist_raw == 0) {
            distance = 255; // assume the worst
        } else {
            distance = 255-dist_raw;
            if (bisset(IPRA,5))  // timer overflow occurred
                distance += 255;
        }
        
      //  printf("%d: %d\n",scan_i, distance);
            
        last_dist[scan_pattern[scan_i]] = distance;
       
        if (distance < 40)
            do_avoidance();

        if (++scan_i == 8) {
            scan_i = 0;
        }
        
        pan_sensor(scan_pattern[scan_i]);
        yield();
    }
}

//(1s/(3686400/200)) * 30 / ((74us)/(1in))/2 in in

// initialize the hardware and create tasks
int main() {
 	TIL311 = 0xC5;
 	
 	srand();
 	serial_start(SERIAL_SAFE);
	lcd_init();
	
 	sem_init(&lcd_sem);
    sem_init(&ds_sem);

	lcd_load_ch();

	lcd_printf("68008 ROBOT LOL");
	lcd_cursor(0,1);
	
 	TBCR = 0x4;  // prescaler of 50
    TCDCR |= 0x4 << 4;
    
    estop_motors();
   
    motor_state go_forward = { MOTOR_FWD_FULL, MOTOR_FWD_FULL, -1 /* never die */, 0xFF };
    push_state(go_forward); // this should never die
    
    enter_critical(); // ensure all tasks are created simultaneously 
     
    create_task(&task_drive, 0);	
	create_task(&task_check_bumper,0);
	create_task(&task_lcd_status,0);
	create_task(&task_distance_sense,0);

  	leave_critical();

	return 0;
}

// put a motor state onto the stack
void push_state(motor_state ms) {
    drive_stack_head--;
    drive_stack[drive_stack_head] = ms;
    estop = false;
}

void estop_motors() {
    estop = true;
    LEFT_MOTOR = MOTOR_T_NEUTRAL;
    RIGHT_MOTOR = MOTOR_T_NEUTRAL;
}

// load the custom characters into the LCD
void lcd_load_ch() {
    lcd_cgram(CH_UPARR); // up arrow
    lcd_data(B00000000);
    lcd_data(B00000100);
    lcd_data(B00001110);
    lcd_data(B00010101);
    lcd_data(B00000100);
    lcd_data(B00000100);
    lcd_data(B00000100);
    lcd_data(B00000100);
    
    lcd_cgram(CH_DNARR); // down arrow
    lcd_data(B00000000);
    lcd_data(B00000100);
    lcd_data(B00000100);
    lcd_data(B00000100);
    lcd_data(B00000100);
    lcd_data(B00010101);
    lcd_data(B00001110);
    lcd_data(B00000100);
    
    lcd_cgram(CH_STOP); // stop
    lcd_data(B00000000);
    lcd_data(B00001110);
    lcd_data(B00010001);
    lcd_data(B00011011);
    lcd_data(B00010101);
    lcd_data(B00011011);
    lcd_data(B00010001);
    lcd_data(B00001110);
        
    lcd_cursor(0,0); // back to data entry mode
}
