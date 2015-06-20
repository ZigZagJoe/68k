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
#define PAN_HALT 255

// low level pulse lengths
#define PAN_LEFT_90_V  538
#define PAN_CENTER_V   338
#define PAN_RIGHT_90_V  125

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
const uint8_t sp_180_scan[] = {10, PAN_LEFT_90, PAN_LEFT_77, PAN_LEFT_45, PAN_LEFT_22, PAN_CENTER, PAN_RIGHT_22, PAN_RIGHT_45, PAN_RIGHT_77,PAN_RIGHT_90, PAN_HALT}; 

// only read forward
const uint8_t sp_center_only[] = {1, PAN_CENTER};
 
// avoidance trigger distances
const uint8_t trigger_dists[NUM_PAN_POSITIONS] = {0,0,43,38,35,38,43,0,0};
        
#define DIST_FAR 110
        
// low level motor drive speeds

// 1.5ms
#define MOTOR_T_NEUTRAL 120
#define MOTOR_DIST 40
// 1ms
#define MOTOR_T_FWD_FULL (MOTOR_T_NEUTRAL+MOTOR_DIST)
// 2ms
#define MOTOR_T_BCK_FULL (MOTOR_T_NEUTRAL-MOTOR_DIST)
#define MOTOR_T_FWD_23 (MOTOR_T_NEUTRAL+MOTOR_DIST*2/3)
#define MOTOR_T_BCK_23 (MOTOR_T_NEUTRAL-MOTOR_DIST*2/3)
#define MOTOR_T_FWD_13 (MOTOR_T_NEUTRAL+MOTOR_DIST*1/3)
#define MOTOR_T_BCK_13 (MOTOR_T_NEUTRAL-MOTOR_DIST*1/3)


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

uint8_t ser_debug = 0;

volatile uint8_t *scan_pattern;
volatile uint8_t  scan_i;
volatile uint8_t  current_pan;             // current position of pan servo
volatile uint8_t  dist_raw;                // return from timera IRQ
volatile uint16_t last_dist[NUM_PAN_POSITIONS];            // current scan results
volatile uint8_t  estop;              // motor emergency stop: halts drive stack processing
volatile task_t   avoidance_task;     // used to monitor state of avoidance processing
volatile int16_t  gyro_y_dps;
    
// from motor speed value to timer value
uint8_t speed_to_timer[] = {MOTOR_T_BCK_FULL, MOTOR_T_BCK_23, MOTOR_T_BCK_13, MOTOR_T_NEUTRAL, MOTOR_T_FWD_13, MOTOR_T_FWD_23, MOTOR_T_FWD_FULL};

int32_t gyro_y_acc;      // gyro accumulator
int32_t y_target;        // point at which turn completed
int16_t gyro_y_last;

sem_t turn_done;

int8_t motor_dir;
int8_t motor_value;
// current throttle


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
    
    TIL311 = 0x5F;
    
    scan_pattern = &sp_180_scan;
    scan_i = 0; // will move to first index when sp_sem is released
    // zero distance array
    memset(last_dist, 0, sizeof(last_dist));
    sem_release(&sp_sem);
    
    // wait for the scan to be completed
      
    while(true) {
        sleep_for(50);
        sem_acquire(&sp_sem);
        if (scan_pattern[scan_i] == PAN_HALT)
            break;
        sem_release(&sp_sem);
    }
        
    
    int16_t front_scan[NUM_PAN_POSITIONS];
    memcpy(front_scan, last_dist, sizeof(last_dist));
    
    dprintf("FWD 180: ");
    for (uint8_t i = 0; i < NUM_PAN_POSITIONS; i++) 
            dprintf("%3d  ", front_scan[i]);
       
    dprintf("\n");
      
    TIL311 = 0x7A;   
    create_task(&task_prep_sensor,0);  // turn the sensor while turning
    precise_turn(180);                 // begin the turn
  
    TIL311 = 0x5B;
    // set 180 scan pattern
    scan_pattern = &sp_180_scan;
    scan_i = 0; // will move to first index when sp_sem is released
    // zero distance array
    memset(last_dist, 0, sizeof(last_dist));
    sem_release(&sp_sem);
    
     // wait for the scan to be completed
    
    while(true) {
        sleep_for(50);
        sem_acquire(&sp_sem);
        if (scan_pattern[scan_i] == PAN_HALT)
            break;
        sem_release(&sp_sem);
    }
        
    scan_pattern = &sp_90_swp;
    scan_i = 0; // restore normal scan pattern
    
    // find the farthest (apparent) clear distance
    uint16_t max = 0;
    uint8_t max_i = 0;

    for (int i = 0; i < NUM_PAN_POSITIONS; i++)
        if (last_dist[i] > max) {
            max = last_dist[i];
            max_i = i;
        }

    dprintf("BCK 180: ");
    for (uint8_t i = 0; i < NUM_PAN_POSITIONS; i++) 
            dprintf("%3d  ", last_dist[i]);
            
    dprintf("\nFarthest clear distance %d at %d\n",max, max_i);
    
    // turn approximately towards the farthest clear area
    // later use gyro for exact turn
    
    TIL311 = 0xCD;
    
    if (max_i != PAN_CENTER)
        precise_turn(pan_ind_to_angle[max_i]);
        
    // resume scanning
    sem_release(&sp_sem);
    
    // remove the stop task
    sem_acquire(&ds_sem);
    
    if (curr_state.ID == AVOID_MOTOR_STOP) {
        curr_state.duration = 0;
    } else 
        halt_and_exit("FATAL: TOP TASK IN task_handle_obstacle IS NOT STOP TASK!", 0xAA);
     
    TIL311 = 0xAD;
    sem_release(&ds_sem);
    
    // and off we go
}

//void do_turn_test();

// sets motor state to top of state stack, and consumes state when duration expired
void task_drive() {  
    sem_init(&ds_sem);
    
    memset(&drive_stack, 0, sizeof(drive_stack));
   
    // configure timers
	TACR = 0;    // timerA disabled
 	TBCR = 0x4;  // prescaler of 50
    TCDCR |= 0x4 << 4; // prescaler of 50
    
    estop_motors();
    
    //do_turn_test();
      
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
    bset (IERA, 5); // enable the interrupt, so on overflow the bit in IPRA will be set
    
    bset (IERB, 6); // enable gpip4 interrupt...
    bset (IMRB, 6); // &unmask it
         
    bset (DDR, PAN_SERVO_BIT); // pan servo
    
    uint16_t distance;

    current_pan = 0;
    scan_i = 0;
    scan_pattern = &sp_90_swp;
    
    memset(&last_dist, 0, sizeof(last_dist));
    
    // home sensor
    pan_sensor(scan_pattern[1]);
    pan_sensor(scan_pattern[1]);
    pan_sensor(scan_pattern[1]);
    
	while(true) {  
	     // read the next pan position and move sensor to it
        sem_acquire(&sp_sem);
        
        scan_i++;
        
        if ((scan_i-1) >= scan_pattern[0]) 
            scan_i = 1;
           
        if (scan_pattern[scan_i] == PAN_HALT) {
            puts("Scan complete, wait.\n"); 
            while (scan_pattern[scan_i] == PAN_HALT) {
                sem_release(&sp_sem);
                yield();
                sem_acquire(&sp_sem);
            }
            sem_release(&sp_sem);
            puts("Resume\n");
            continue;
        } 
      
        if (current_pan != scan_pattern[scan_i])
            pan_sensor(scan_pattern[scan_i]); 
            
        sem_release(&sp_sem);
               
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
            if (bisset(IPRA, 5))  // timer overflow occurred
                distance += 255;
        }
        
        printf("%d: %d\n",current_pan, distance);
            
        last_dist[current_pan] = distance;
       
      /* puts("Updated ");
       puthexN(current_pan);
       putc('\n');*/
       
        if (!task_active(avoidance_task)) 
            if (distance < trigger_dists[scan_i]) {
                dprintf("Do avoidance due to range of %d < %d at angle %d\n",distance,trigger_dists[scan_i],scan_i);
                do_avoidance(RANGE_WARNING);
            }
    }
}

// monitor scan results and do slight avoidance while implementing the higher goal of this robot (which is?)
void task_executive() { 
    sem_t sl_sem;
    sem_init(&sl_sem);
    uint8_t juke_ind = 0; 
    
    const uint16_t juke_duration = 300;
    
    while(true) {
        sleep_for(juke_duration);
        
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
                motor_state juke_right = { MOTOR_FWD_FULL, (at == PAN_LEFT_45 || min < 60) ? MOTOR_FWD_13 : MOTOR_FWD_23, juke_duration, 0xEEC4, &sl_sem };
                juke_ind = safe_push_state(juke_right);
            } else  {
                motor_state juke_left = { (at == PAN_RIGHT_45 || min < 60) ? MOTOR_FWD_13 : MOTOR_FWD_23, MOTOR_FWD_FULL, juke_duration, 0xEEC1, &sl_sem };
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
    turn_done = 0;
            
    //GYRO_250DEG_V_TO_DEG 0.00763F

    while(true) {
        if (!turn_done) {// don't sample while the precise turn code is running, it's updating gyro_y too
            i2c_reg_read(&gyro_y_raw, GYRO_ZOUT_H, 2);
            gyro_y_dps = gyro_y_raw / RAW_TO_250DPS;
        } else 
            dprintf("%5d\n", gyro_y_acc);
        
        //float y_dps = gyro_y * GYRO_250DEG_V_TO_DEG;
        //dprintf("%5d\n", gyro_y_dps);
        
        sleep_for(24);
    }
}


void task_init_accel() {
    printf("Initializing MPU6050... ");
    cli();
    
    i2c_init(); // initialize i2c interface
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
    // don't use do this often...
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
	create_task(&task_distance_sense,0);
    create_task(&task_executive, 0);
    
    task_t su = create_task(&task_init_accel,0);
    task_struct_t *ptr = su >> 16;
    ptr->FLAGS |= (1<<13);         // promote this task to supervisor level by editing its flags 
    
    create_task(&task_lcd_status,0);
    create_task(&task_print_dist,0);
  
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
    TIL311 = 0xAA;
        
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
    int8_t duration = max(5,(abs(current_pan - p)) * 5);
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

// reduce the motor deadzone
int pwr_to_motor(int x) {
    if (x > 1)      return MOTOR_T_NEUTRAL + 10 + x;
    if (x < -1)    return MOTOR_T_NEUTRAL - 10 + x;
    return MOTOR_T_NEUTRAL;
}

// positive = forward
// negative = backward 
 
#define INTEGRATE_TICK 167
// 1000 ms / tick freq

void tick_integrate_gyro() {
    int16_t gyro_y_raw, gyro_y_diff, travel_tick;
    
    i2c_reg_read(&gyro_y_raw, GYRO_ZOUT_H, 2); 
    
    gyro_y_diff = gyro_y_raw - gyro_y_last;
    gyro_y_dps = gyro_y_raw / RAW_TO_250DPS;
     
    travel_tick = (gyro_y_raw + (gyro_y_diff >> 1)) / INTEGRATE_TICK; // trapezoidal approximation of distance
    gyro_y_acc += travel_tick;
    
    gyro_y_last = gyro_y_raw;
     
    /* print_dec(travel_tick);
     putc(',');
     print_dec(gyro_y_acc / 131);
     putc(',');
     */

	int8_t turn_max_pwr = 28;
    int8_t turn_min_pwr = -28;

    int deadzone = 300;
    int lookahead = 13;
    int adj_factor = 10;
    
    int abs_y_acc = abs(gyro_y_acc); 

    int est_endpt = gyro_y_acc + travel_tick * lookahead;
    est_endpt += ((((int)gyro_y_diff) * lookahead * lookahead) / INTEGRATE_TICK) >> 1;   
    est_endpt = abs(est_endpt);
    
    uint8_t undershoot = est_endpt < (y_target - deadzone);
    uint8_t overshoot = est_endpt > (y_target + deadzone);
 
    int err = abs(abs_y_acc - y_target);
    
    if (undershoot) { 
        motor_value += adj_factor;
    } else if (overshoot) {
        motor_value -= adj_factor;
    } 
    
    motor_value = constrain(motor_value, turn_min_pwr, turn_max_pwr);
    LEFT_MOTOR = pwr_to_motor(motor_dir ? (motor_value) : (-motor_value));
    RIGHT_MOTOR = pwr_to_motor(motor_dir ? (motor_value) : (-motor_value));
    
   /* print_dec(motor_value);
    putc('\n');*/
    
    if (err < deadzone) {
        estop_motors();
        _ontick_event = 0;
        sem_release(&turn_done);
    }
}


void precise_turn(int16_t angle) {
    printf("Turn %d deg\n",angle);
    sem_acquire(&ds_sem);
    
    enter_critical();
    
    gyro_y_acc = 0;
 	gyro_y_last = 0;
 	
 	y_target = abs(angle * RAW_TO_250DPS);
 
    motor_dir = (angle > 0);

    sem_init(&turn_done);
    sem_acquire(&turn_done);
    
    _ontick_event = &tick_integrate_gyro;
    
    // every tick, the current rotation velocity will be integrated
    // once it hits near the target angle, the motors will be braked and the turn is done
    leave_critical();
    
    sem_wait(&turn_done);

   /* sleep_for(1000);
    _ontick_event = 0;*/
    
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

