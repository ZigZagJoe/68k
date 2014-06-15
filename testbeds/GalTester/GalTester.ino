typedef struct {
	uint8_t pin;
	char *name;
} pininfo;

void emit(uint16_t pattern, uint8_t count, pininfo pins[]) ;

uint8_t pin_mapping[] = { 0x255, 1,2,3,4,5,6,7,8,9,10,11,0x255,45,44,43,42,41,40,39,38,18,19, 20,0x255 };

uint8_t inputs[] = {1,2,3,4,5,6,7,8,9,10,11,13,23 };

void setup() {
	for (uint8_t i = 1; i < 24; i++) {
		if (i == 12) continue;
		pinMode(pin_mapping[i],INPUT);
	}
	
		
	for (uint8_t i = 0; i < sizeof(inputs); i++) {
		Serial.print("Setting input: ");
		Serial.println(inputs[i]);
		pinMode(pin_mapping[inputs[i]],OUTPUT);
		digitalWrite(pin_mapping[inputs[i]],0);
	}
}

#define PIN(X) { X, #X }

#define DS 1
#define AS 2
#define RW 3
#define A19 4
#define A18 5
#define A2 6
#define FC0 7
#define FC1 8
#define FC2 9
#define BOOT_R 10
#define DTACKI 11
#define SLOW_T 13
#define RD 14
#define WR 15
#define ROMCS 16
#define RAMCS 17
#define IOCS 18
#define IACK 19
#define DTACK 20
#define BOOTED 21
#define DS_I 22
#define SLOWCS 23

void emit(uint16_t pattern, uint8_t count, pininfo pins[]) {
	int bit = count-1;
	for (int i = 0; i < count; i++) {
		int bv = (pattern >> bit) & 0x1;
		Serial.print("      ");
		//Serial.print(pins[i].pin);
		//Serial.print(" ");
		Serial.print(bv);
		digitalWrite(pin_mapping[pins[i].pin], bv);
		bit--;
	}

}

void loop() {
	while (!Serial.read());
	
	digitalWrite(pin_mapping[BOOT_R],1);
	digitalWrite(pin_mapping[A18],1);
	digitalWrite(pin_mapping[A19],1);
	
	pininfo inputs[] = { PIN(AS),  PIN(DS), PIN(A19), PIN(A18), PIN(DTACKI), PIN(SLOWCS), PIN(SLOW_T) };
	pininfo outputs[] = { PIN(IOCS), PIN(ROMCS), PIN(RAMCS), PIN(DTACK) };
	
	uint8_t inc = sizeof(inputs) / sizeof(pininfo);
	uint8_t ouc = sizeof(outputs) / sizeof(pininfo);	
	
	Serial.print("Incount: ");
	Serial.println(inc);
	Serial.print("Outcount: ");
	Serial.println(ouc);
	
	uint16_t maxval = 1 << inc;
	
	for (uint8_t i = 0; i < inc; i++) {
		int len = strlen(inputs[i].name);
		if (len < 7) for (int spc = 0; spc < 7-len;spc ++) Serial.print(' ');
		Serial.print(inputs[i].name);
	}
	Serial.print("     ");
	for (uint8_t i = 0; i < ouc; i++) {
		int len = strlen(outputs[i].name);
		if (len < 7) for (int spc = 0; spc < 7-len;spc ++) Serial.print(' ');
		Serial.print(outputs[i].name);
	}
	Serial.println();	
	
	for (int spc = 0; spc < 7*(inc+ouc)+8;spc ++) 
		Serial.print('-');
		
	Serial.println();
	
	for (uint16_t v = 0; v < maxval; v++) {
		emit(v,inc,inputs);
		Serial.print("    |");
		for (uint8_t rd = 0; rd < ouc; rd++) {
			Serial.print("      ");
			Serial.print(digitalRead(pin_mapping[outputs[rd].pin]));
		}
		Serial.println();
	}
	
	delay(1000);
}
