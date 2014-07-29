#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define UPLOADER
//#include "../C/loader-C/loader.h"

#define BAD_HEX_CHAR         1
#define INVALID_WRITE        2
#define FAILED_WRITE         4
#define FORMAT_ERROR         8
#define EARLY_EOF           16
#define INVALID_CHAR        32
#define CRC_ERROR           64
#define PROG_FAILURE       128

uint8_t *srec;
uint32_t pos;
uint32_t srec_sz;

uint32_t program_sz;
uint8_t errno;
uint8_t checksum;

FILE *out;

#define RETURN_IF_ERROR()    if (errno) return errno

uint8_t readch() {
    if (pos >= srec_sz) {
        errno |= EARLY_EOF;
        printf("EOF encountered in readch\n");
        return 0;
    }
    
    char ch = srec[pos++];
    
    if ((ch < '0' || ch > 'z') && ch !='\n' && ch != '\r') {
        printf("Invalid character encountered\n");
        errno |= INVALID_CHAR;
        return 0;
    }
    
    return ch;
}

uint8_t getn() {
    char ch = readch();
    
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    if (ch >= '0' && ch <= '9')
        return ch - '0';
        
    printf("Expected valid hex char, got %c.\n",ch);
    errno |= BAD_HEX_CHAR;
    return 0;        
}

uint8_t getb() {
    uint8_t b = (getn() << 4) | getn();
    fputc(b, out);
    checksum += b;
    return b;
}

uint32_t readAddr(uint8_t len) {
    uint32_t i = 0;
    while (len-- > 0)
        i = (i << 8) | getb();
    return i;
}

int main (int argc, char ** argv) {
    if (argc == 1) {
        printf("Usage: srec2bin file_to_convert [outname]\n");
        return 1;
    }
    
    char fn_buff[1024];
    char * outn;
    
    if (argc >= 3) {
        outn = argv[2];
    } else {
        strcpy(fn_buff, argv[1]);
        int len = strlen(fn_buff);
        strcat(fn_buff, ".srb");
        outn = fn_buff;
    }
    
    FILE *in = fopen(argv[1],"rb");
    if (!in) {
        printf("Failed to open %s\n",argv[1]);
        return 1;
    }
    
    fseek (in, 0, SEEK_END);   // non-portable
    srec_sz = ftell (in);
    rewind(in);
    
    fprintf(stderr, "File size: %d\n", srec_sz);
    srec = (uint8_t*)malloc(srec_sz);
    
    if (fread(srec, 1, srec_sz, in) != srec_sz) {
        printf("Did not read entire file\n");
        return 1;
    }
    
    fclose(in);
    
    out = fopen(outn,"w");
    
    if (!out) {
        printf("Failed to open %s\n", outn);
        return 1;
    } else
        printf("Writing output to %s\n", outn);
    
    errno = 0;
    pos = 0;
    errno = 0;
    program_sz = 0;
    
    char Sc; // char to hold 'S'
    uint8_t typ, len, file_crc, loc_crc;
    uint32_t address;
    uint32_t entry_point = 0;
    
    uint16_t rec_cnt = 0;
    uint16_t data_rec_cnt = 0;
   
    // number of address bytes for each record type
    const uint8_t addr_len[] = {2,2,3,4,0,2,3,4,3,2};
    
    while(1) {
        rec_cnt++;
        
        Sc = readch();  // if not a S, bad news
        
        if (Sc != 'S') {
            errno |= FORMAT_ERROR;
            printf("Record started with %c, not S.\n", Sc);
            break;
        }
        
        fputc('S',out);
        
        typ = readch();
        fputc(typ, out);
        
        typ -= '0'; // record type

        if (errno) break;
        
        if (typ > 9) {
            errno |= FORMAT_ERROR;
            printf("Record of invalid type\n");
            break;
        }  
        
        checksum = 0;              // reset checksum
        len = getb() - addr_len[typ] - 1;
       
        address = readAddr(addr_len[typ]);
        
        if (errno) break;
        
        switch (typ) {
            case 3: // data records (varying address size)
            case 2:
            case 1:
                data_rec_cnt++;
            case 0: // metadata record
                for (int i = 0; i < len && !errno; i++) {
                    uint8_t byt = getb();
                    if (typ != 0)
                        program_sz++;
                }
                break;
            case 5: // record count
            case 6:
                if (address != data_rec_cnt) {
                    errno |= FORMAT_ERROR;
                    printf("Data record count missmatch (read %d, actual %d)\n",address,data_rec_cnt);
                    return errno;
                }
                break;
            case 7: // entry point records (varying address size)
            case 8:
            case 9:
                entry_point = address;
                printf("Entry point of 0x%x\n",entry_point);
                break;
        }
        
        if (errno) break;
        
        if (len != 0 && (typ >= 4)) {
             errno |= FORMAT_ERROR;
             printf("Record has data in record that should not contain data!\n");
             return errno;
        }
        
        loc_crc = ~checksum;
        file_crc = getb();
        
        if (loc_crc != file_crc) {
            errno |= CRC_ERROR;
            printf("CRC error in record on line %d: got %d, calculated %d\n",rec_cnt, file_crc,loc_crc);
            return errno;
        }
        
        char ch;
        while ((ch = readch()) != '\n' && !errno) 
            if (ch != '\r') 
                printf("Warning: garbage at end of line %d\n",rec_cnt);
        
        fputc('\n',out);
        
        if (errno) break;
        
        if (srec[pos] == 0) 
            break;        
    }
    
    fclose(out);
    
    printf("Parsed %d records\n",rec_cnt);
    
    if (!errno) {
        printf("Program size: %d\n",program_sz);
        printf("\n%s converted successfully.\n",argv[1]);
    } else
        printf("Failed to convert %s\n", argv[1]);
    
}

