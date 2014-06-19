#ifndef __IO_H__
#define __IO_H__

#include <stdint.h>

// read or write a byte anywhere in RAM (useful for IO)
// volatile so compiler knows to never cache the value
#define MEM(X) (*ADDR(X))

// address to pointer
#define ADDR(X) ((volatile uint8_t *)(X))

// IO base address
#define IO_BASE 0xC0000

// size of each chunk of IO space
#define IO_INC  0x8000

// address of each device
#define IO_DEV0 (IO_BASE + IO_INC * 0)
#define IO_DEV1 (IO_BASE + IO_INC * 1)
#define IO_DEV2 (IO_BASE + IO_INC * 2)
#define IO_DEV3 (IO_BASE + IO_INC * 3)
#define IO_DEV4 (IO_BASE + IO_INC * 4)
#define IO_DEV5 (IO_BASE + IO_INC * 5)
#define IO_DEV6 (IO_BASE + IO_INC * 6)
#define IO_DEV7 (IO_BASE + IO_INC * 7)

// MFP & MFP REGISTERS
#define MFP IO_DEV0

// GPIO data
#define GPDR  MEM(MFP+0x01)
// Active edge
#define AER   MEM(MFP+0x03) 
// Data direction
#define DDR   MEM(MFP+0x05)
// Interrupt enable A
#define IERA  MEM(MFP+0x07) 
// Interrupt enable B
#define IERB  MEM(MFP+0x09) 
// Interrupt pending A
#define IPRA  MEM(MFP+0x0B)
// Interrupt pending B
#define IPRB  MEM(MFP+0x0D) 
// Interrupt in-service A
#define ISRA  MEM(MFP+0x0F)
// Interrupt in-service B
#define ISRB  MEM(MFP+0x11) 
// Interrupt mask A
#define IMRA  MEM(MFP+0x13)
// Interrupt mask B
#define IMRB  MEM(MFP+0x15) 
// Vector
#define VR    MEM(MFP+0x17)
// Timer A control
#define TACR  MEM(MFP+0x19) 
// Timer B control
#define TBCR  MEM(MFP+0x1B)
// Timer C & D control
#define TCDCR MEM(MFP+0x1D) 
// Timer A data
#define TADR  MEM(MFP+0x1F) 
// Timer B data
#define TBDR  MEM(MFP+0x21) 
// Timer C data
#define TCOR  MEM(MFP+0x23) 
// Timer D data
#define TDDR  MEM(MFP+0x25) 
// Synchronous character
#define SCR   MEM(MFP+0x27)
// USART control
#define UCR   MEM(MFP+0x29) 
// Receiver status
#define RSR   MEM(MFP+0x2B) 
// Transmitter Status
#define TSR   MEM(MFP+0x2D) 
// USART Data
#define UDR   MEM(MFP+0x2F)

// MFP interrupt numbers
enum { MFP_GPI0 = 0, MFP_GPI1, MFP_GPI2, MFP_GPI3, MFP_TIMERD, MFP_TIMERC,MFP_GPI4,MFP_GPI5, MFP_TIMERB,MFP_XMIT_ERR, MFP_XMIT_EMPTY, MFP_REC_ERR, MFP_CHAR_RDY, MFP_TIMERA, MFP_GPI6, MFP_GPI7 };
// bit 7 - 4 copied from VR, bits 3 - 0 come from above

#define MFP_INT 0x80

#define GPIO(X) (GPDR & (1<<X))


// TIL311 displays
#define TIL311 MEM(IO_DEV1)

#endif
