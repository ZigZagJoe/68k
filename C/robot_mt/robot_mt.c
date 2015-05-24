#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <printf.h>
#include <string.h>

// IO devices for my specific machine
#include <io.h>
#include <interrupts.h>
#include <lcd.h>
#include <time.h>
#include <kernel.h>
#include <semaphore.h>

#include <binary.h>

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
const uint8_t trigger_dists[NUM_PAN_POSITIONS] = {0,0,57,45,40,45,57,0,0};
        
        
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
const uint8_t speed_to_timer[] = {MOTOR_T_BCK_FULL, MOTOR_T_BCK_23, MOTOR_T_BCK_13, MOTOR_T_NEUTRAL, MOTOR_T_FWD_13, MOTOR_T_FWD_23, MOTOR_T_FWD_FULL};

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
void push_state(motor_state ms);
void safe_push_state(motor_state ms); // uses the semaphore
void do_avoidance(uint8_t why);       // spawns avoidance task if neccisary
void sensor_move(uint32_t count, uint8_t duration);
void halt_and_exit(char *why, uint8_t code);
void pan_sensor(uint8_t p);

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

void task_handle_obstacle(uint8_t why) {
    // semaphore for synchronous motor actions
    sem_t move_sem;
    sem_init (&move_sem);
    
    motor_state stop = { MOTOR_STOP, MOTOR_STOP, -1, AVOID_MOTOR_STOP, 0 };
    
    // set base state to stopped
    safe_push_state(stop);
    
    estop = false;
    
    // reverse for breathing room
    motor_state go_back = { MOTOR_BCK_FULL, MOTOR_BCK_FULL, 300, 0x0ABB, &move_sem };
    do {
        safe_push_state(go_back);

        // wait for completion
        sem_wait(&move_sem);
    } while (BUMPER_DEPRESSED);
    
    // acquire semaphore for scan
    sem_acquire(&sp_sem);
    
    // could do a forward scan first, then 180 for rear scan...
    
    // turn around
    motor_state do_180 = { MOTOR_BCK_FULL, MOTOR_FWD_FULL, 750, 0x0A44, &move_sem };
    safe_push_state(do_180);    // begin the turn
    pan_sensor(sp_180_scan[1]); // move the sensor to the first position while turning
    sem_wait(&move_sem);        // wait for turn complete
    
    // set 180 scan pattern
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
    
    for (uint8_t i = 0; i < NUM_PAN_POSITIONS; i++) 
            printf("%3d  ", last_dist[i]);
            
    printf("\nmax = %d at %d\n",max, max_i);
    
    // turn approximately towards the farthest clear area
    // later use gyro for exact turn
    if (max_i != PAN_CENTER) {
        if (max_i > PAN_CENTER) {
            motor_state do_right = { MOTOR_FWD_FULL, MOTOR_BCK_FULL, 100 + 50 * (max_i-PAN_CENTER), 0x0A44, &move_sem };
            safe_push_state(do_right);        
        } else {
            motor_state do_left = { MOTOR_BCK_FULL, MOTOR_FWD_FULL, 100 + 50 * (PAN_CENTER-max_i), 0x0A11, &move_sem };
            safe_push_state(do_left);
        }
        sem_wait(&move_sem); 
    }
        
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
        halt_and_exit("FATAL: TOP TASK IN task_handle_obstacle IS NOT STOP TASK!", 0xEA);
     
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
       
    motor_state go_forward = { MOTOR_FWD_FULL, MOTOR_FWD_FULL, -1 /* never die */, 0xFF, 0 };
    push_state(go_forward); // this should never die
    
    while(true) {
        if (BUMPER_DEPRESSED)           // check the emergency bumpers
            do_avoidance(BUMPER_HIT);   // initiate emergency stop handling
    
        while(estop) // emergency stop; don't drive motors
            yield();
        
        sem_acquire(&ds_sem);
    
        motor_state *cs = &curr_state;
        
        if (cs->duration != 0) {
            LEFT_MOTOR =  speed_to_timer[-(cs->ML) + MOTOR_SPEEDS]; // convert from signed int to unsigned index
            RIGHT_MOTOR = speed_to_timer[ (cs->MR) + MOTOR_SPEEDS]; // convert from signed int to unsigned index

            TIL311 = cs->ID & 0xFF;
        
            if (cs->duration != -1) {
                cs->duration -= 15;
                if (cs->duration < 1) { 
                    if (cs->notify)
                        *(cs->notify) = 0;
                    drive_stack_head++; // pop state
                }
            }
        } else 
            drive_stack_head++; // ignore state as duration is 0
        
        sem_release(&ds_sem);
        sleep_for(15);
    }
}

// print the current hardware state to LCD, with a heartbeat
void task_lcd_status() {
    lcd_init();
 	
 	sem_init(&lcd_sem);
   
	lcd_load_ch();

	lcd_printf("68008 ROBOT LOL");
	lcd_cursor(0,1);
	
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
        lcd_cursor(6,1);
        lcd_data(BUMPER_DEPRESSED?'H':' ');
        lcd_data(task_active(avoidance_task) ? 'A':' ');
        lcd_data(estop?'E':' ');
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
            if (distance < trigger_dists[scan_i])
                do_avoidance(RANGE_WARNING);
            
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
    
    while(true) {
        sleep_for(250);
        
        if (task_active(avoidance_task)) 
            continue;
        
        int16_t min = 32767;
        uint8_t at = 0;
        for (uint8_t i = PAN_LEFT_45; i <= PAN_RIGHT_45; i++) {
            if ( last_dist[i] < min) {
                min = last_dist[i];
                at = i;
            }
        }
         
        if (min < 100) {   
            printf("Closest object is at %d, dist %d\n", at, min);
            sem_init(&sl_sem);
            if (at == PAN_CENTER || at == PAN_LEFT_22) {
                //motor_state slight_right = { MOTOR_FWD_FULL, correction, duration, 0xCCF4, &sl_sem };
                //safe_push_state(slight_right);
            
            }
           // if 
        }
        
    }
    /* } else if (distance < 120) {
        sem_acquire(&ds_sem);
        
        uint8_t correction = (distance > 80 && current_pan != PAN_RIGHT_45 && current_pan != PAN_LEFT_45) 
                                ? MOTOR_FWD_23 : MOTOR_FWD_13;
                                
        uint32_t duration = 10000 / distance;
        
        if ((curr_state.ID & 0xFF00) != 0xCC00) { 
            if (current_pan <= PAN_CENTER) {  
                motor_state slight_right = { MOTOR_FWD_FULL, correction, duration, 0xCCF4 };
                push_state(slight_right);
            } else {
                motor_state slight_left = { correction, MOTOR_FWD_FULL, duration, 0xCCF1 };
                push_state(slight_left);
            }
        } else {
            curr_state.duration = duration;
            if (curr_state.ID == 0xCCF4) {
                curr_state.MR = correction;
            } else 
                curr_state.ML = correction;
        }
    
        sem_release(&ds_sem);
    }*/
}

void task_print_dist() {
    while(true) {
        for (uint8_t i = 0; i < NUM_PAN_POSITIONS; i++) 
            printf("%3d  ", last_dist[i]);
        
        putc('\n');
        sleep_for(500);
    }
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
    
    create_task(&task_drive, 0);	
	create_task(&task_distance_sense,0);
    create_task(&task_lcd_status,0);
    create_task(&task_executive, 0);
    create_task(&task_print_dist,0);
  
  	leave_critical();

	return 0;
}

// put a motor state onto the stack
void push_state(motor_state ms) {
    if (ms.notify)
        *ms.notify = 1;
    drive_stack_head--;
    drive_stack[drive_stack_head] = ms;
    estop = false;
}

void safe_push_state(motor_state ms) {
    sem_acquire(&ds_sem);
    push_state(ms);
    sem_release(&ds_sem);
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


void halt_and_exit(char *why, uint8_t code) {
    enter_critical();
    estop_motors();
    puts(why);
    while(tx_busy()); // wait for message to be printed
    exit(code); // abort
}
