// invoke a trap
#define __TRAP(x) __asm volatile("trap %0\n":: "i"(x));

// define an interrupt handler
#define ISR(x) void __attribute__ ((interrupt_handler)) x(void)

typedef void(*__vector)(void);

typedef struct __attribute__((packed)) {
    uint32_t boot_sp;           // boot stack pointer. never used once running
    uint32_t boot_pc;           // boot program counter. never used once running
    __vector bus_error;         // /BERR asserted
    __vector address_error;     // attempt to access instr at odd address
    __vector illegal_instr;     // invalid instruction or 'illegal' opcode used
    __vector divide_by_0;       // divide by zero attempted
    __vector chk;               // chk instruction
    __vector trapv;             // trapv instruction
    __vector priv_violation;    // attempt to access supervisor state
    __vector trace;             // occurs after every instruction in trace mode
    __vector line1010;          // coprocessor emulation
    __vector line1111;          // coprocessor emulation
    __vector __reserved_1[3];
    __vector uninitialized_isr; // device interrupt bit set but vector not set
    __vector __reserved_2[8];
    __vector int_spurious;      // spurious interrupt (no DTACK)
    __vector auto_level1;       // auto vectored interrupt 1
    __vector auto_level2;       // auto vectored interrupt 2
    __vector auto_level3;       // auto vectored interrupt 3
    __vector auto_level4;       // auto vectored interrupt 4
    __vector auto_level5;       // auto vectored interrupt 5
    __vector auto_level6;       // auto vectored interrupt 6
    __vector auto_level7;       // auto vectored interrupt 7
    __vector trap[16];          // trap routines
    __vector __reserved_3[16];
    __vector user[192];     // user isrs (vectored interrupts)
} __vector_table;

#define user_isr_start 64

#define sei() __asm volatile("andi.w #0xF8FF, %SR\n");
#define cli() __asm volatile("ori.w #0x700, %SR\n");

// vector table at base of RAM
extern __vector_table __vectors;
