#include <stdarg.h>
#include <stdint.h>

void _printf(char *fmt, ... ){
        char buf[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(buf, 128, fmt, args);
        va_end (args);
        Serial.print(buf);
}

enum { 
  READY = 'A', // a
  OKAY,        // b
  UNKNOWN_CMD, // c
  BAD_INPUT,   // d
  CRC_ERROR,   // e
  WRITE_ERROR, // f
  TIMEOUT,     // g
  ERASE_FAIL   // h
};

uint8_t error = 0;

int getCh() {
  if (error) return 0;
  digitalWrite(6,1);
  uint32_t st = millis();
  while (!Serial.available())  {
     if (millis() - st > 1000) {
       error = TIMEOUT;
       digitalWrite(6,0);
       return 0;
     }
  }
  digitalWrite(6,0);
  return Serial.read();
}

uint16_t crc16_update(uint16_t crc, uint8_t a) {
    int i;
    
    crc ^= a;
    for (i = 0; i < 8; ++i)
    {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc = (crc >> 1);
    }
    
    return crc;
}

uint8_t hex_to_byte(uint8_t ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    
    ch = ch & 0xDF;
    
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    
   // _printf("Bad hex char: %c (%d)\n",ch,ch);
    if (error == 0)
      error = BAD_INPUT;
    return 0;
}

uint8_t readbyte() {
    int ch1 = getCh();
    int ch2 = getCh();

    ch1 = hex_to_byte(ch1);
    ch2 = hex_to_byte(ch2);
    
    return ch1 << 4 | ch2;
}

uint16_t readshort() {
    return (((uint16_t)readbyte()) << 8) | readbyte();
}

uint32_t read3bint() {
    return (((uint32_t)readbyte()) << 16) | (((uint32_t)readbyte()) << 8) | readbyte();
}

uint32_t readint() {
    return (((uint32_t)readshort()) << 8) | readshort();
}

void set_address(uint32_t addr) {
  PORTC = addr & 0xFF;
  PORTF = (addr >> 8) & 0xFF;
  PORTD = (addr >> 16) & 0xFF; 
}

#define CS 8
#define RD 9
#define WR 18

void do_write(uint32_t addr,byte b) {
  set_address(addr);
  DDRB = 0xFF;
  PORTB = b;
  asm("nop\nnop\n");
  digitalWrite(CS,0);
  digitalWrite(WR,0);
  asm("nop\nnop\n");
  digitalWrite(WR,1);
  digitalWrite(CS,1);
  DDRB = 0;
  PORTB = 0;
}

uint8_t do_read(uint32_t addr) {
  char data;
  set_address(addr);
  DDRB = 0;
  PORTB = 0;
  asm("nop\nnop\n");
  digitalWrite(CS,0);
  digitalWrite(RD,0);
  asm("nop\nnop\n");
  data = PINB;
  digitalWrite(RD,1);
  digitalWrite(CS,1);
  return data;
}

void write_byte(uint32_t addr, uint8_t b) {
  // data protection sequence
  do_write(0x5555,0xAA);
  do_write(0x2AAA,0x55);
  do_write(0x5555,0xA0);
  do_write(addr, b);
  delayMicroseconds(50); // should not take longer than this to write a byte
}

uint8_t buffer[1024];

uint8_t write_buffer(uint32_t addr, uint16_t sz) {
  byte bin, bout;
  
  for (uint16_t i = 0; i < sz; i++) {
    bin = buffer[i];
    if (bin == 0xFF) {  addr++;  continue; } // no need to write 0xFF when chip is erased
    
    write_byte(addr, bin);
   
    // try to write up to three times
    if (bin != do_read(addr)) { 
      write_byte(addr, bin);
      if (bin != do_read(addr)) { 
        write_byte(addr, bin);
        if (bin != do_read(addr)) { 
          write_byte(addr, bin);
          return 0;
        }
      }
    }
    
    addr++; 
  }
    
  return 1;
}

uint8_t chip_erase() {
  do_write(0x5555,0xAA);
  do_write(0x2AAA,0x55);
  do_write(0x5555,0x80);
  do_write(0x5555,0xAA);
  do_write(0x2AAA,0x55);
  do_write(0x5555,0x10);
  
  delay(100);
  return 1;
}

void setup() { 
  pinMode(6, OUTPUT);
  
  digitalWrite(CS,1);
  digitalWrite(RD,1);
  digitalWrite(WR,1);
  
  pinMode(CS, OUTPUT);
  pinMode(RD, OUTPUT);
  pinMode(WR, OUTPUT);
  
  DDRC = 0xFF;
  DDRF = 0xFF;
  DDRD = 0xFF;
}

void do_dump (uint32_t addr, uint32_t cnt) {
  int c = 0;
  char ascii[17];
  ascii[16] = 0;
  
  Serial.println();
  
  _printf("%6lX   ", addr);
  for (uint32_t i = 0; i < cnt; i++) {
    uint8_t b = do_read(addr);
    
    ascii[c] = (b > 31 & b < 127) ? b : '.';
    
    _printf("%02X ",b);
    
    addr++;
        
    if (++c == 16) {
      Serial.print("  ");
      Serial.print(ascii);
      Serial.println();
      if ((i+1) < cnt)
        _printf("%6lX   ", addr);
      c = 0;
    }
  }
  
  if (c < 15) Serial.println();
  
  Serial.write(4);
}

void loop() {
  Serial.write(READY);
  while (!Serial.available());
  int ch = Serial.read();
  
  if (ch == 'H') {
     Serial.write('T');
     Serial.write(OKAY);
  } else if (ch == 'E') {
     if (chip_erase()) {
       Serial.write(OKAY);
     } else
       Serial.write(ERASE_FAIL);
  } else if (ch == 'A') {
    Serial.write(OKAY);
    for (uint32_t addr = 0; addr < 0x80000; addr++) 
      Serial.write(do_read(addr)); 
    Serial.write(OKAY);
  } else if (ch == 'R') {
    error = 0;
    uint32_t addr = read3bint();
    uint16_t cnt = read3bint();
    
    if (error) {
      addr = 0x0;
      cnt = 512;
    }
    
    do_dump(addr,cnt);
    Serial.write(OKAY);
  } else if (ch == 'D') {
      error = 0;
      uint32_t addr = read3bint();
      uint16_t sz = readshort();

      uint16_t mycrc = 0;
       for (uint16_t i = 0; i < sz; i++) {
          buffer[i] =  readbyte();
          mycrc = crc16_update(mycrc,buffer[i]);
      }
      
      uint16_t crc = readshort();
      
      if (error) {
        Serial.write(error); 
        return;
      }
      
      /*Serial.print(addr, HEX);
      _printf(" sz %4X ",sz);
      _printf("crc %4X ",crc);
      _printf("(ours %4X)",mycrc);*/

      if (mycrc != crc) {
        Serial.write(CRC_ERROR); 
        return;
      }
      
      if (write_buffer(addr, sz)) {
        Serial.write(OKAY);
      } else
        Serial.write(WRITE_ERROR);
  } else {
      Serial.write(UNKNOWN_CMD);
  } 
}
