#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <printf.h>
#include <string.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <time.h>
#include <binary.h>

#include <i2c.h>
#include <lcd.h>
#include <kernel.h>
#include <semaphore.h>

#include "MPU6050.h"

// custom lcd characters
#define CH_UPARR 0
#define CH_UPARR_H 1
#define CH_DNARR 2
#define CH_DNARR_H 3
#define CH_STOP 4

// physical pin definitons
#define BUMPER_BIT 3
#define PAN_SERVO_BIT 4
#define DISTANCE_SENSOR_BIT 5

#define LEFT_MOTOR TBDR
#define RIGHT_MOTOR TCDR

#define BUMPER_DEPRESSED bisset(GPDR, BUMPER_BIT)

// high level positions
#define PAN_LEFT_90  0
#define PAN_LEFT_77  1
#define PAN_LEFT_45  2
#define PAN_LEFT_22  3
#define PAN_CENTER   4
#define PAN_RIGHT_22 5
#define PAN_RIGHT_45 6  
#define PAN_RIGHT_77 7  
#define PAN_RIGHT_90 8

// low level pulse lengths
#define PAN_LEFT_90_V  450
#define PAN_CENTER_V   263
#define PAN_RIGHT_90_V  85

// calculated angles
#define PAN_LEFT_45_V  ((PAN_LEFT_90_V + PAN_CENTER_V) / 2)
#define PAN_RIGHT_45_V ((PAN_RIGHT_90_V + PAN_CENTER_V) / 2)
#define PAN_LEFT_77_V  ((PAN_LEFT_90_V + PAN_LEFT_45_V) / 2)
#define PAN_RIGHT_77_V ((PAN_RIGHT_90_V + PAN_RIGHT_45_V) / 2)
#define PAN_LEFT_22_V  ((PAN_LEFT_45_V + PAN_CENTER_V) / 2)
#define PAN_RIGHT_22_V ((PAN_RIGHT_45_V + PAN_CENTER_V) / 2) 

#define NUM_PAN_POSITIONS 9

const int32_t pan_ind_to_angle[] = {90,77,45,22,0,-22,-45,-77,-90};

const uint32_t pan_to_pulse[] = {PAN_LEFT_90_V, PAN_LEFT_77_V, PAN_LEFT_45_V, PAN_LEFT_22_V, PAN_CENTER_V, PAN_RIGHT_22_V, PAN_RIGHT_45_V, PAN_RIGHT_77_V, PAN_RIGHT_90_V};

// scan patterns
// sweep 90 degrees: provides good compromise of speed and watched area
const uint8_t sp_90_swp[] = {8, PAN_CENTER, PAN_LEFT_22, PAN_LEFT_45, PAN_LEFT_22, PAN_CENTER, PAN_RIGHT_22, PAN_RIGHT_45, PAN_RIGHT_22};
// sweep from extreme to extreme, for more complete (but slower) intel
const uint8_t sp_180_swp[] = {16, PAN_CENTER, PAN_LEFT_22, PAN_LEFT_45, PAN_LEFT_77, PAN_LEFT_90, PAN_LEFT_77, PAN_LEFT_45, PAN_LEFT_22, PAN_CENTER, PAN_RIGHT_22, PAN_RIGHT_45, PAN_RIGHT_77,PAN_RIGHT_90,PAN_RIGHT_77,PAN_RIGHT_45, PAN_RIGHT_22};  

// scan from one extreme to another, for single scans only
const uint8_t sp_180_scan[] = {9, PAN_LEFT_90, PAN_LEFT_77, PAN_LEFT_45, PAN_LEFT_22, PAN_CENTER, PAN_RIGHT_22, PAN_RIGHT_45, PAN_RIGHT_77,PAN_RIGHT_90}; 

// only read forward
const uint8_t sp_center_only[] = {1, PAN_CENTER};
 
// avoidance trigger distances
const uint8_t trigger_dists[NUM_PAN_POSITIONS] = {0,0,50,43,40,43,50,0,0};
        
#define DIST_FAR 100
        
// low level motor drive speeds

// 1.5ms
#define MOTOR_T_NEUTRAL 111
// 1ms
#define MOTOR_T_FWD_FULL (MOTOR_T_NEUTRAL+38)
// 2ms
#define MOTOR_T_BCK_FULL (MOTOR_T_NEUTRAL-38)
#define MOTOR_T_FWD_23 (MOTOR_T_NEUTRAL+38*2/3)
#define MOTOR_T_BCK_23 (MOTOR_T_NEUTRAL-38*2/3)
#define MOTOR_T_FWD_13 (MOTOR_T_NEUTRAL+38*1/3)
#define MOTOR_T_BCK_13 (MOTOR_T_NEUTRAL-38*1/3)

// from motor speed value to timer value
uint8_t speed_to_timer[] = {MOTOR_T_BCK_FULL, MOTOR_T_BCK_23, MOTOR_T_BCK_13, MOTOR_T_NEUTRAL, MOTOR_T_FWD_13, MOTOR_T_FWD_23, MOTOR_T_FWD_FULL};

// speed to indicator char
const uint8_t speed_to_ch[] = { CH_DNARR, CH_DNARR_H, CH_DNARR_H, CH_STOP, CH_UPARR_H,CH_UPARR_H, CH_UPARR };
    
// number of directional speeds 
#define MOTOR_SPEEDS 3

// high level speeds
// negatives used so can negate for reversed drive motor

#define MOTOR_BCK_FULL -3
#define MOTOR_BCK_23 -2
#define MOTOR_BCK_13 -1
#define MOTOR_STOP 0
#define MOTOR_FWD_13 1
#define MOTOR_FWD_23 2
#define MOTOR_FWD_FULL 3

#define MOTOR_STATE_NEVER_DIE -1000

/* prototypes */

typedef struct {
    int8_t ML;
    int8_t MR;
    int32_t duration; // in ms
    uint16_t ID;
    sem_t *notify;     
} motor_state;

// functions
void estop_motors();
void lcd_load_ch();
uint8_t push_state(motor_state ms);
uint8_t safe_push_state(motor_state ms); // uses the semaphore
void do_avoidance(uint8_t why);       // spawns avoidance task if neccisary
void sensor_move(uint32_t count, uint8_t duration);
void halt_and_exit(char *why, uint8_t code);
void pan_sensor(uint8_t p);
void precise_turn(int16_t angle);

// tasks
void task_handle_obstacle(uint8_t why);
void task_drive();
void task_lcd_status();
void task_distance_sense();

/* global state */
motor_state drive_stack[32];
uint8_t drive_stack_head = 31;

#define curr_state (drive_stack[drive_stack_head])

sem_t lcd_sem;
sem_t ds_sem;
sem_t avoid_sem;
sem_t sp_sem;

volatile uint8_t *scan_pattern;
volatile uint8_t  scan_i;
volatile uint8_t  current_pan;             // current position of pan servo
volatile uint8_t  dist_raw;                // return from timera IRQ
volatile uint16_t last_dist[NUM_PAN_POSITIONS];            // current scan results
volatile uint8_t  estop;              // motor emergency stop: halts drive stack processing
volatile task_t   avoidance_task;     // used to monitor state of avoidance processing
volatile int16_t  gyro_y_dps;
    
uint8_t ser_debug = 0;


#define dprintf(...) if (ser_debug) printf(__VA_ARGS__)

ISR(measure_done) {
    dist_raw = TADR;
}

// motor task ids
#define BUMPER_HIT 1
#define RANGE_WARNING 2

// wait for a semaphore to become available, then immediately release it
void sem_wait(sem_t *sem) {
    sem_acquire(sem);
    sem_release(sem);
}

#define AVOID_MOTOR_STOP 0x0A55

void task_prep_sensor() {
    pan_sensor(sp_180_scan[1]); // move the sensor to the first position while turning
}

void task_handle_obstacle(uint8_t why) {
    // semaphore for synchronous motor actions
    sem_t move_sem;
    sem_init (&move_sem);
    
    motor_state stop = { MOTOR_STOP, MOTOR_STOP, MOTOR_STATE_NEVER_DIE, AVOID_MOTOR_STOP, 0 };
    
    // set base state to stopped
    safe_push_state(stop);
    
    estop = false;
    
    // acquire semaphore for scan
    sem_acquire(&sp_sem);
    task_t sensor_move = create_task(&task_prep_sensor,0);  
    
    // reverse for breathing room
    motor_state go_back = { MOTOR_BCK_FULL, MOTOR_BCK_FULL, 300, 0x0ABB, &move_sem };
    do {
        safe_push_state(go_back);

        // wait for completion
        sem_wait(&move_sem);
    } while (BUMPER_DEPRESSED);
    
    wait_for_exit(sensor_move);
    
    // could do a forward scan first, then 180 for rear scan...
    
    scan_pattern = &sp_180_scan;
    scan_i = 1;
    // zero distance array
    memset(last_dist, 0, sizeof(last_dist));
    sem_release(&sp_sem);
    
    // wait for the scan to be completed
    // also, find the farthest (apparent) clear distance
    uint8_t can_exit = 0;
    int16_t max = 0;
    int16_t max_i;
    
    while(!can_exit) {
        yield();
        can_exit = 1;
        for (int i = 0; i < NUM_PAN_POSITIONS; i++)
            if (!last_dist[i]) {
                can_exit = 0;
                break;
            } else if (last_dist[i] > max) {
                max = last_dist[i];
                max_i = i;
            }
    }
    
    dprintf("FWD 180: ");
    for (uint8_t i = 0; i < NUM_PAN_POSITIONS; i++) 
            dprintf("%3d  ", last_dist[i]);
       
    dprintf("\n");
         
    // turn and do another scan
    sem_acquire(&sp_sem);
    sensor_move = create_task(&task_prep_sensor,0);  
    precise_turn(180);          // begin the turn
  
    // set 180 scan pattern
    scan_pattern = &sp_180_scan;
    scan_i = 1;
    // zero distance array
    memset(last_dist, 0, sizeof(last_dist));
    sem_release(&sp_sem);
    
    // wait for the scan to be completed
    // also, find the farthest (apparent) clear distance
    can_exit = 0;
    max = 0;

    while(!can_exit) {
        yield();
        can_exit = 1;
        for (int i = 0; i < NUM_PAN_POSITIONS; i++)
            if (!last_dist[i]) {
                can_exit = 0;
                break;
            } else if (last_dist[i] > max) {
                max = last_dist[i];
                max_i = i;
            }
    }
    
    dprintf("BCK 180: ");
    for (uint8_t i = 0; i < NUM_PAN_POSITIONS; i++) 
            dprintf("%3d  ", last_dist[i]);
            
    dprintf("\nFarthest clear distance %d at %d\n",max, max_i);
    
    // turn approximately towards the farthest clear area
    // later use gyro for exact turn
    if (max_i != PAN_CENTER)
        precise_turn(pan_ind_to_angle[max_i]);
        
    // restore normal scan pattern
    sem_acquire(&sp_sem);
    scan_pattern = &sp_90_swp;
    scan_i = 1;
    sem_release(&sp_sem);
    
    // remove the stop task
    sem_acquire(&ds_sem);
    
    if (curr_state.ID == AVOID_MOTOR_STOP) {
        curr_state.duration = 0;
    } else 
        halt_and_exit("FATAL: TOP TASK IN task_handle_obstacle IS NOT STOP TASK!", 0xAA);
     
    sem_release(&ds_sem);
    
    // and off we go
}

// sets motor state to top of state stack, and consumes state when duration expired
void task_drive() {  
    sem_init(&ds_sem);
    
    memset(&drive_stack, 0, sizeof(drive_stack));
   
    // configure timers
	TACR = 0;    // timerA disabled
 	TBCR = 0x4;  // prescaler of 50
    TCDCR |= 0x4 << 4; // prescaler of 50
    
    estop_motors();


    precise_turn(90);
    
        return;
    
    while(true) {
      
        sleep_for(1000);
        return;
        //precise_turn(-90);
        //sleep_for(1000);
       /* precise_turn(90);
        sleep_for(500);
        precise_turn(90);
        sleep_for(500);   
        precise_turn(-90);
        sleep_for(500);
        precise_turn(-90);
        sleep_for(500);
        precise_turn(-90);
        sleep_for(500);
        precise_turn(-90);
        sleep_for(500);   
        precise_turn(180);
        sleep_for(500);   
        precise_turn(45);
        sleep_for(500); 
        precise_turn(45);
        sleep_for(500); 
        precise_turn(-45);
        sleep_for(500);   
        precise_turn(-45);
        sleep_for(500); 
        precise_turn(180);
        sleep_for(500); 
        precise_turn(-360);
        sleep_for(1500); */
    }
    
    
    motor_state go_forward = { MOTOR_FWD_FULL, MOTOR_FWD_FULL, MOTOR_STATE_NEVER_DIE /* never die */, 0xFFFF, 0 };
    push_state(go_forward); // this should never die
    
    uint8_t stable = 0;
    
    while(true) {
        if (BUMPER_DEPRESSED)           // check the emergency bumpers
            do_avoidance(BUMPER_HIT);   // initiate emergency stop handling
    
        while(estop) // emergency stop; don't drive motors
            yield();
        
        sem_acquire(&ds_sem);
    
        motor_state *cs = &curr_state;
        
        if (cs->duration != 0) {
            
            // if on base task (move forward), trim motors according to gryo
            if (cs->ID == 0xFFFF) {
                if (abs(gyro_y_dps) > 3 && stable) {
                    // negative values = turning right
                    if (gyro_y_dps < 0) {
                        // turning right means left is moving faster than right
                        // 6 FWD_FULL = forward speed for right, reverse speed for left
                        // 0 BCK_FULL = reverse speed for right, forward speed for left
                        if (speed_to_timer[6] != MOTOR_T_FWD_FULL) {
                            speed_to_timer[6]++; // increase speed of other motor first
                        } else if (speed_to_timer[0] < MOTOR_T_BCK_13)
                            speed_to_timer[0]++; // move towards neutral - reduce speed
                    } else {
                        if (speed_to_timer[0] != MOTOR_T_BCK_FULL) {
                            speed_to_timer[0]--; // increase speed of other motor first
                        } else if (speed_to_timer[6] > MOTOR_T_FWD_13)
                            speed_to_timer[6]--; // move towards neutral
                    }
                }
            } else {
                // untrim motors
                speed_to_timer[6] = MOTOR_T_FWD_FULL;
                speed_to_timer[0] = MOTOR_T_BCK_FULL;
            }
            
            LEFT_MOTOR  = speed_to_timer[-(cs->ML) + MOTOR_SPEEDS]; // convert from signed int to unsigned index
            RIGHT_MOTOR = speed_to_timer[ (cs->MR) + MOTOR_SPEEDS]; // convert from signed int to unsigned index

            TIL311 = cs->ID & 0xFF;
        
            if (cs->duration != MOTOR_STATE_NEVER_DIE) {
                cs->duration -= 15;
                if (cs->duration <= 0) { 
                    stable = 0;
                    if (cs->notify)
                        *(cs->notify) = 0;
                    drive_stack_head++; // pop state
                } else
                    stable = 1;
            } else
                stable = 1;
        } else {
            if (cs->notify)
                *(cs->notify) = 0;
            drive_stack_head++; // ignore state as duration is 0
        }
        
        sem_release(&ds_sem);
        sleep_for(15);
    }
}

char dist_to_char(uint8_t ind) {
    int16_t d = last_dist[ind];
    if (d == 0) return ' ';
    if (d > DIST_FAR) return '^';
    if (d > trigger_dists[ind]) return '-';
    return '_';
}

// print the current hardware state to LCD, with a heartbeat
void task_lcd_status() {
    lcd_init();
 	
 	sem_init(&lcd_sem);
   
	lcd_load_ch();

	lcd_cursor(0,0);
	
	lcd_printf("68008 ROBOT");

    uint8_t st = 0;
    
    uint8_t chs[] = {0xA5,' '};
    
    while(true) {
        sem_acquire(&lcd_sem);
        
        lcd_cursor(1,1);
            
        if (estop) {
            lcd_data('X');
            lcd_data(' ');
            lcd_data('X');
        } else {
            lcd_data(speed_to_ch[curr_state.ML + MOTOR_SPEEDS]);
            lcd_data(' ');
            lcd_data(speed_to_ch[curr_state.MR + MOTOR_SPEEDS]);
        }
        
        // sensor status
        lcd_data(' ');
        lcd_data(BUMPER_DEPRESSED?'H':' ');
        lcd_data(task_active(avoidance_task) ? 'A':' ');
        lcd_data(estop?'E':' ');
        lcd_data(' ');
        
        // distance graph
        lcd_data(dist_to_char(PAN_LEFT_45));
        lcd_data(dist_to_char(PAN_LEFT_22));
        lcd_data(dist_to_char(PAN_CENTER));
        lcd_data(dist_to_char(PAN_RIGHT_22));
        lcd_data(dist_to_char(PAN_RIGHT_45));
        lcd_cursor(12,0);
        lcd_printf("%4d",gyro_y_dps);
        
        //lcd_printf("%03d",last_dist[PAN_CENTER]);
        
        // print heartbeat
        lcd_cursor(15,1);
        st++;
        lcd_data(chs[(st >> 2) & 1]);
           
        sem_release(&lcd_sem);
        sleep_for(100);
    }
}

void task_distance_sense() {
    sem_init(&sp_sem);
    
    __vectors.user[MFP_INT + MFP_GPI4 - USER_ISR_START] = &measure_done;
    
    bset (DDR, DISTANCE_SENSOR_BIT);
	bset (AER, 4);  // set positive edge triggered for TAI
	bclr (IMRA, 5); // mask timer A overflow IRQ. just going to check it later, don't want actual interrupt
    bset (IERA, 5); // enable timera interrupt, so if it triggers the bit in IPRA will be set
    
    bset (IERB, 6); // enable gpip4 interrupt...
    bset (IMRB, 6); // &unmask it
         
    bset (DDR, PAN_SERVO_BIT); // pan servo
    
    uint16_t distance;

    current_pan = 0;
    scan_i = 1;
    scan_pattern = &sp_90_swp;
    
    memset(&last_dist, 0, sizeof(last_dist));
    
    // home sensor
    pan_sensor(scan_pattern[1]);
    pan_sensor(scan_pattern[1]);
    pan_sensor(scan_pattern[1]);
    
	while(true) {         
        enter_critical();
        
        TACR = 0;
        bclr_a(IPRA, 5);

        dist_raw = 0;
       
        // tell the sensor to send a ranging ping
        bset_a(GPDR, DISTANCE_SENSOR_BIT);
        __asm volatile ("nop\nnop\n");
        bclr_a(GPDR, DISTANCE_SENSOR_BIT);
        
        // set up Timer A to count while TAI is high
        TADR = 0;
	    TACR = 31;
	    
	    leave_critical();
	    
	    long start = millis();
	    
	    while (dist_raw == 0 && ((millis() - start) < 20)) 
	        yield();
        
        TACR = 0;
        
        if (dist_raw == 0) {
            distance = 255; // assume the worst case distance
        } else {
            distance = 255 - dist_raw;
            if (bisset(IPRA,5))  // timer overflow occurred
                distance += 255;
        }
        
       // printf("%d: %d\n",current_pan, distance);
            
        last_dist[current_pan] = distance;
       
        if (!task_active(avoidance_task)) 
            if (distance < trigger_dists[scan_i]) {
                dprintf("Do avoidance due to range of %d < %d at angle %d\n",distance,trigger_dists[scan_i],scan_i);
                do_avoidance(RANGE_WARNING);
            }
            
        // read the next pan position and move sensor to it
        sem_acquire(&sp_sem);
        scan_i++;
        
        if ((scan_i-1) >= scan_pattern[0]) 
            scan_i = 1;
        
        if (current_pan != scan_pattern[scan_i])
            pan_sensor(scan_pattern[scan_i]); 
            
        sem_release(&sp_sem);
    }
}

// monitor scan results and do slight avoidance while implementing the higher goal of this robot
void task_executive() { 
    sem_t sl_sem;
    sem_init(&sl_sem);
    uint8_t juke_ind = 0; 
    
    while(true) {
        sleep_for(200);
        
        if (task_active(avoidance_task)) {
            if (juke_ind && sl_sem) {
                drive_stack[juke_ind].duration = 0;
                juke_ind = 0;
            }
            continue;
        }
        
        int16_t min = 32767;
        uint8_t at = 0;
        
        for (uint8_t i = PAN_LEFT_45; i <= PAN_RIGHT_45; i++) {
            if (last_dist[i] && last_dist[i] < min) {
                min = last_dist[i];
                at = i;
            }
        }
    
        if (min < DIST_FAR && sl_sem == 0) {   
            dprintf("Closest object is at %d, dist %d\n", at, min);
            
            if (at <= PAN_CENTER) {
                motor_state juke_right = { MOTOR_FWD_FULL, (at == PAN_LEFT_45 || min < 60) ? MOTOR_FWD_13 : MOTOR_FWD_23, 300, 0xEEC4, &sl_sem };
                juke_ind = safe_push_state(juke_right);
            } else  {
                motor_state juke_left = { (at == PAN_RIGHT_45 || min < 60) ? MOTOR_FWD_13 : MOTOR_FWD_23, MOTOR_FWD_FULL, 300, 0xEEC1, &sl_sem };
                juke_ind = safe_push_state(juke_left);
            }
        }
        
    }
}

void task_print_dist() {
    while(true) {
        sleep_for(500);
        
        if (!ser_debug) {
            ser_debug = serial_available();
            continue;
        } 
           
        for (uint8_t i = 0; i < NUM_PAN_POSITIONS; i++) 
            dprintf("%3d  ", last_dist[i]);
        
        putc('\n');
    }
}

void task_read_accel() { 
    int16_t gyro_y_raw = 0;
   // turn_done = 0;
            
    //GYRO_250DEG_V_TO_DEG 0.00763F

    while(true) {
       /* if (!turn_done) {// don't sample while the precise turn code is resident, it's updating gyro_y too
            i2c_reg_read(&gyro_y_raw, GYRO_ZOUT_H, 2);
            gyro_y_dps = gyro_y_raw / RAW_TO_250DPS;
        }
        
        //float y_dps = gyro_y * GYRO_250DEG_V_TO_DEG;
        //dprintf("%5d\n", gyro_y_dps);
        
        sleep_for(21);*/
    }
}


void task_init_accel() {
    printf("Initializing MPU6050... ");
    cli();
    
    i2c_init(); //!< initialize twi interface
    i2c_set_slave(0x68);
 
    if (!i2c_reg_writebyte(PWR_MGMT_1, 0x80)) { // reset gyro
       DELAY_MS(20);
       if (!i2c_reg_writebyte(PWR_MGMT_1, 0x80))  // try again
            halt_and_exit("Failed to reset MPU6050",0x5E);
    }
         
    float gyroBias[3], accelBias[3]; // bias corrections for gyro and accelerometer
    calibrateMPU6050(gyroBias, accelBias);
    
    sei();
    printf ("Done\n");

    // create a normal level task
    create_task(&task_read_accel,0);
    
    // explicitly exit as this is a super-lever task
    // exiting in this manner is leaving clutter on the supervisor stack
    // don't use it often...
    exit_task(); 
}

//(1s/(3686400/200)) * 30 / ((74us)/(1in))/2 in in

// initialize the hardware and create tasks

int main() {
 	TIL311 = 0xC5;

 	srand();
 	serial_start(SERIAL_SAFE);

	sem_init(&avoid_sem);
    avoidance_task = NULL;

    enter_critical(); // ensure all tasks are created simultaneously 
    
    printf("68k robot firmware built on " __DATE__ " at " __TIME__ "\n");
    printf("Press any key to begin debug logging.\n");
    create_task(&task_drive, 0);	
	//create_task(&task_distance_sense,0);
    //create_task(&task_executive, 0);
    
    task_t su = create_task(&task_init_accel,0);
    task_struct_t *ptr = su >> 16;
    ptr->FLAGS |= (1<<13);         // promote this task to supervisor level by editing its flags 
    
    //create_task(&task_lcd_status,0);
    //create_task(&task_print_dist,0);

  	leave_critical();

	return 0;
}

// put a motor state onto the stack
uint8_t push_state(motor_state ms) {
    if (ms.notify)
        *ms.notify = 1;
    drive_stack_head--;
    drive_stack[drive_stack_head] = ms;
    estop = false;
    return drive_stack_head;
}

uint8_t safe_push_state(motor_state ms) {
    sem_acquire(&ds_sem);
    uint8_t p = push_state(ms);
    sem_release(&ds_sem);
    return p;
}

void estop_motors() {
    estop = true;
    LEFT_MOTOR = MOTOR_T_NEUTRAL;
    RIGHT_MOTOR = MOTOR_T_NEUTRAL;
}

// spawns avoidance process
void do_avoidance(uint8_t why) { 
    enter_critical();
    
    if (!sem_try(&avoid_sem)) { // already in progress
        leave_critical();
        return;
    }
        
    if (!avoidance_task || !task_active(avoidance_task)) {
        estop_motors();
        avoidance_task = create_task(&task_handle_obstacle, 1, why);
    }
    
    sem_release(&avoid_sem); 
    leave_critical();
}


void halt_and_exit(char *why, uint8_t code) {
    enter_critical();
    estop_motors();
    puts(why);
    lcd_clear();
    lcd_cursor(0,0);
    lcd_puts(why);
    while(tx_busy()); // wait for message to be printed
    exit(code); // abort
}

void pan_sensor(uint8_t p) {
    int8_t duration = max(5,(abs(current_pan - p)) * 6);
    current_pan = p;
    sensor_move(pan_to_pulse[p], duration);  
}

void sensor_move(uint32_t count, uint8_t duration) {
    for (uint8_t i = 0; i < duration; i++) {
        enter_critical();
        bset(GPDR,PAN_SERVO_BIT);
        __asm volatile("move.l %0,%%d0\n" \
                                "1: subi.l #1, %%d0\n" \
                                "bne 1b\n" \
                                ::"d"(count):"d0");
        bclr(GPDR, PAN_SERVO_BIT);
        leave_critical();
        sleep_for(10);
    }
}



#define EST_SHIFT_FACTOR 3
// factor to use as a shift to multiply current velocity at - begin stop if would exceed

int32_t gyro_acc;      // gyro accumulator
uint32_t target;       // point at which turn completed
int16_t gyro_last;

uint8_t pt_braking;    // 1 if slowing down

sem_t turn_done;

int8_t rot_motor_fwd;
int8_t rot_motor_brake;

// towards target

int8_t motor_dir;
// positive = forward
// negative = backward

int8_t motor_value;
// current throttle

// reduce the motor deadzone
int pwr_to_motor(int x) {
    if (x > 1)      return MOTOR_T_NEUTRAL + 10 + x;
    if (x < -1)    return MOTOR_T_NEUTRAL - 10 + x;
    return MOTOR_T_NEUTRAL;
}

// positive = forward
// negative = backward 


    
void tick_integrate_gyro() {
    int16_t gyro_y_raw, diff_raw, travel_tick;
    
    i2c_reg_read(&gyro_y_raw, GYRO_ZOUT_H, 2); 
    
    gyro_y_dps = gyro_y_raw / RAW_TO_250DPS;
     
    travel_tick = gyro_y_raw / 143; // 1000/7 - integrate the time spent
    gyro_acc += travel_tick;
    
    diff_raw = gyro_y_raw - gyro_last;
    gyro_last = gyro_y_raw;
     
     print_dec(gyro_y_raw);
     putc(',');
      print_dec(gyro_acc);
     putc(',');
     print_dec(diff_raw);
     putc(',');

    if (!turn_done) { // log only
        puts("\n");
        return;
    }
    
    int deadzone = 200;
    int est_dist = 60;
    int adj_factor = 1;
    
    int abs_gyro = abs(gyro_acc); 
    uint8_t undershoot = (abs_gyro + (travel_tick * est_dist)) < (target - deadzone);
    uint8_t overshoot = (abs_gyro + (travel_tick * est_dist)) > (target + deadzone);
    
    //motor_value range is +- 18, -1 to 1 is dead zone
  
    int err = abs(abs_gyro - ((int)target));
   // factor = min((factor * err)/10000,factor);
  
    int power_cap = 28;
    power_cap = min (28, 28 * err/2000);
    
    //printf("%d,", motor_value);
    
    if (undershoot) { 
        // increase power in the forward direction
        if (motor_value != rot_motor_fwd) {
            motor_value += (-motor_dir * adj_factor);
        
            puts("U\n");
        } else puts("\n");
    } else if (overshoot) {
        // increase power in backward direction
        if (motor_value != rot_motor_brake) {
            motor_value += (motor_dir * adj_factor);
            
            puts("D\n");
        } else puts("\n");
    } else puts("\n");
    
    motor_value = constrain(motor_value, -power_cap, power_cap);
    LEFT_MOTOR = pwr_to_motor(motor_value);
    RIGHT_MOTOR = pwr_to_motor(motor_value);
    
    
    if ((abs_gyro - target) < 1000 && abs(gyro_y_raw) < 1000) {
        puts("Done\n");
        estop_motors();
        //_ontick_event = 0;
        sem_release(&turn_done);
    }
    
   // puts("\n");
}

void precise_turn(int16_t angle) {
    printf("Turn %d deg\n",angle);
    sem_acquire(&ds_sem);
    
    enter_critical();
    
    pt_braking = 0;
    gyro_acc = 0;
 	
 	target = abs(angle * RAW_TO_250DPS);
 	
    if (angle > 0) {
        motor_dir = -1;
        motor_value = 28;
        RIGHT_MOTOR = MOTOR_T_FWD_FULL;
        LEFT_MOTOR = MOTOR_T_FWD_FULL;
    } else {
        motor_dir = 1;
        motor_value = -28;
        RIGHT_MOTOR = MOTOR_T_BCK_FULL;
        LEFT_MOTOR = MOTOR_T_BCK_FULL;
    }
          
    gyro_last = 0;  
    rot_motor_fwd = motor_value;
    rot_motor_brake = -motor_value;
           
    sem_init(&turn_done);
    sem_acquire(&turn_done);
    
    _ontick_event = &tick_integrate_gyro;
    
    // every tick, the current rotation velocity will be integrated
    // once it hits near the target angle, the motors will be braked and the turn is done
    leave_critical();
    
    sem_wait(&turn_done);

    sleep_for(500);
    _ontick_event = 0;
    
    estop = false;
    sem_release(&ds_sem);
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
    
    lcd_cgram(CH_UPARR_H); // short up arrow
    lcd_data(B00000000);
    lcd_data(B00000100);
    lcd_data(B00001110);
    lcd_data(B00010101);
    lcd_data(B00000100);
    lcd_data(B00000100);
    lcd_data(B00000000);
    lcd_data(B00000000);
    
    lcd_cgram(CH_DNARR); // down arrow
    lcd_data(B00000000);
    lcd_data(B00000100);
    lcd_data(B00000100);
    lcd_data(B00000100);
    lcd_data(B00000100);
    lcd_data(B00010101);
    lcd_data(B00001110);
    lcd_data(B00000100);
    
    lcd_cgram(CH_DNARR_H); // short down arrow
    lcd_data(B00000000);
    lcd_data(B00000000);
    lcd_data(B00000000);
    lcd_data(B00000100);
    lcd_data(B00000100);
    lcd_data(B00010101);
    lcd_data(B00001110);
    lcd_data(B00000100);
    
    lcd_cgram(CH_STOP); // stop
    lcd_data(B00000000);
    lcd_data(B00000000);
    lcd_data(B00001110);
    lcd_data(B00011011);
    lcd_data(B00010101);
    lcd_data(B00011011);
    lcd_data(B00001110);
    lcd_data(B00000000);
        
    lcd_cursor(0,0); // back to data entry mode
}

