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

#define LEFT_BUMPER bisset(GPDR, LEFT_BUMPER_BIT)
#define RIGHT_BUMPER bisset(GPDR, RIGHT_BUMPER_BIT)

#define LEFT_MOTOR TADR
#define RIGHT_MOTOR TBDR

// 1.5ms
#define MOTOR_STOP 110
// 1ms
#define MOTOR_FWD 148
// 2ms
#define MOTOR_BCK 74

// custom characters
#define CH_UPARR 0
#define CH_DNARR 1
#define CH_STOP 2

#define BCK_JOB 0x31BB

/* prototypes */

typedef struct {
    uint8_t ML;
    uint8_t MR;
    int32_t duration; // in ms
    uint32_t ID;     
} motor_state;

// functions
void lcd_load_ch();
void push_state(motor_state ms);

/* global state */
motor_state drive_stack[128];
uint8_t drive_stack_head = 127;

#define curr_state (drive_stack[drive_stack_head])

sem_t lcd_sem;
sem_t ds_sem;


// monitors bumper sensors
void task_check_bumper() {
    while(true) {
        if (LEFT_BUMPER || RIGHT_BUMPER) {
            sleep_for(10); // debounce
            
            if (LEFT_BUMPER || RIGHT_BUMPER) {
            
                // acquire the stack semaphore
                sem_acquire(&ds_sem);

                // if top of stack is reverse task, just refresh duration
                if (curr_state.ID == BCK_JOB) {
                    printf("Still touching something.\n");
                    curr_state.duration = 2000;
                } else {
                    printf("Hit something!\n");
                    
                    // stop *now*
                    LEFT_MOTOR = MOTOR_STOP;
                    RIGHT_MOTOR = MOTOR_STOP;
                    
                    // put the turn on the stack
                    if (LEFT_BUMPER) {
                        motor_state rot_right = { MOTOR_FWD, MOTOR_BCK, 600 + rand8(), 0x3144 };
                        push_state(rot_right);   
                    } else {
                        motor_state rot_left = { MOTOR_BCK, MOTOR_FWD, 600 + rand8(), 0x3111 };
                        push_state(rot_left);
                    }
                    
                    // put reverse on the stack
                    motor_state go_back = { MOTOR_BCK, MOTOR_BCK, 2000, BCK_JOB };
                    push_state(go_back);
                    
                }
              
                sem_release(&ds_sem);
                sleep_for(50);
            }
        }
        yield();
    }

}

// sets motor state to top of state stack, and consumes state when duration expired
void task_drive() {
    while(true) {
        sem_acquire(&ds_sem);
    
        motor_state *cs = &curr_state;
        
        LEFT_MOTOR = cs->ML;
        RIGHT_MOTOR = cs->MR;
        TIL311 = cs->ID & 0xFF;
        
        if (cs->duration != -1) {
            cs->duration -= 20;
            if (cs->duration < 1)
                drive_stack_head++; // pop state
        }
        
        sem_release(&ds_sem);
        sleep_for(20);
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
        
        // print heartbeat
        lcd_cursor(15,1);
        st++;
        lcd_data(chs[(st >> 2) & 1]);
           
        sem_release(&lcd_sem);
        sleep_for(100);
    }
}

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
	
 	TACR = 0x4;  // prescaler of 50
    TBCR = 0x4;
    
    RIGHT_MOTOR = MOTOR_STOP;
    LEFT_MOTOR = MOTOR_STOP;

    motor_state go_forward = { MOTOR_FWD, MOTOR_FWD, -1 /* never die */, 0xFF };
    push_state(go_forward); // this should never die
    
    enter_critical(); // ensure all tasks are created simultaneously 
     
    create_task(&task_drive, 0);	
	create_task(&task_check_bumper,0);
	create_task(&task_lcd_status,0);

  	leave_critical();

	return 0;
}

// put a motor state onto the stack
void push_state(motor_state ms) {
    drive_stack_head--;
    drive_stack[drive_stack_head] = ms;
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
