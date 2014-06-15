#include <stdio.h>
#include <stdlib.h>


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
    
    printf("Bad hex char: %c (%d)\n",ch,ch);
    exit(1);
    
}

uint8_t readbyte() {
    int ch1 = getchar();
    int ch2 = getchar();
    
    if (ch1 == EOF || ch2 == EOF) {
        printf("EOF in readbyte\n");
        exit (1);
    }
    
   
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

int main (int argc, char ** argv) {
    
    char filename[] = "test.bin";
    
    FILE *out = fopen(filename,"rb+");
    if (!out) {
        printf("Failed to open %s\n",filename);
        return 1;
    }
    
    int ch;
    uint8_t byte;
    while(1) {
        ch = getchar();
        if (ch == EOF) break;
        if (ch == 'D') {
            uint32_t addr = read3bint();
            uint16_t sz = readshort();
            uint16_t mycrc = 0;
            
            fseek(out, addr, SEEK_SET);
            for (int i = 0; i < sz; i++) {
                byte = readbyte();
                fputc(byte,out);
                mycrc = crc16_update(mycrc,byte);
            }
            
            uint16_t crc= readshort();
            printf("addr %6X sz %4X crc %4X (ours %4X)",addr,sz,crc,mycrc);
            
            if (mycrc != crc) printf("  CRC MISSMATCH!");
                
            printf("\n");
        } else {
            printf("Bad command code %c\n",ch);
            return 1;
        }
        
        
    }
    
    fclose(out);
   
    return 0;
}