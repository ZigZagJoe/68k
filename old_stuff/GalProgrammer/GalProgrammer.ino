#define SCLK 10
#define SDIN 11
#define STB 12
#define SDOUT 13
#define PV 14

void setup() {
  pinMode(PV, OUTPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(SDIN, OUTPUT);
  pinMode(STB, OUTPUT);
  pinMode(SDOUT, INPUT);
  
  digitalWrite(PV, 0);
  digitalWrite(SCLK, 0);
  digitalWrite(SDIN, 0);
  digitalWrite(STB, 1);
  
  DDRD = 255;
  PORTD = 0;
}


//http://www.armory.com/~rstevew/Public/Pgmrs/GAL/program.htm
/*
 ,"GAL22V10",
        5892, // fuses
        24, // pins
       
        ,5828 // ues fuse,
      
        ,61, // erase row
        60, // erase all row
      
       ,cfg22V10 // *cfg
        ,sizeof(cfg22V10)/sizeof(int) // cfgbits*/
        
uint16_t cfg22V10[]=
{
    5809,5808,
    5811,5810,
    5813,5812,
    5815,5814,
    5817,5816,
    5819,5818,
    5821,5820,
    5823,5822,
    5825,5824,
    5827,5826
};

#define rows 44, // rows
#define bitsperrow 132 // bits,
#define uesrow 44 // ues row
#define uesbits (8*8) // ues bytes
#define pesrow 58 // pes row
#define pesbits (10*8) // pes bytes
#define cfgrow 16 // cfg row
#define cfgbits  sizeof(cfg22V10)/sizeof(uint16_t)

static void GetPES() {
    int bitmask,byte;

    StrobeRow(pesrow);
    readbitrow(pesrow, pesbits);
}

void readbitrow(byte addr, byte count) {
  if (addr < 10) 
    Serial.print(' ');
    
  Serial.print(addr);
  Serial.print(' ');
  
  for (int i = 0; i < count; i++) {
    if ((i%8) == 0) Serial.print(' ');
    int i = ReceiveBit();
    Serial.print(i);
  }
  
  Serial.println();
}


static void SetRow(uint8_t row) {
    PORTD=row;
     delayMicroseconds(50);
}

void SetSDIN(boolean on) {
   digitalWrite(SDIN, on);
    delayMicroseconds(50);
}

void SetSTB(boolean on) {
    digitalWrite(STB, on);
    delayMicroseconds(50);
}

void SetPV(boolean on) {
    digitalWrite(PV, on);
    delayMicroseconds(50);
}

void SetSCLK(boolean on) {
    digitalWrite(SCLK, on);
    delayMicroseconds(50);
}

boolean GetSDOUT(void){
    return digitalRead(SDOUT);
}

boolean ReceiveBit(void) {
    boolean bit;

    bit=GetSDOUT();
    SetSCLK(1);
    SetSCLK(0);
    return bit;
}

void DiscardBits(int n) {
    while(n-->0) ReceiveBit();
}
 
void SendBit(int bit) {
    SetSDIN(bit);
    SetSCLK(1);
    SetSCLK(0);
}

void SendBits(int n,int bit) {
    while(n-->0) SendBit(bit);
}

void SendAddress(int n,uint16_t row) {
    while(n-->0)
    {
	SendBit(row&1);
	row>>=1;
    }
}

void Strobe(int msec)
{
    delay(2);
    SetSTB(0);
    delay(msec);
    SetSTB(1);
    delay(2);
}

void StrobeRow(uint8_t row) {
	SetRow(0);
	SendBits(bitsperrow,0);
	SendAddress(6,row);
    Strobe(2);
}

void loop() {
  GetPES();
  Serial.println();
  while(Serial.read() == -1); 
}
